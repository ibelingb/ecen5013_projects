/***********************************************************************************
 * @author Josh Malburg
 * joshua.malburg@colorado.edu
 * Advanced Embedded Software Development
 * ECEN5013 - Rick Heidebrecht
 * @date March 16, 2019
 * arm-linux-gnueabi (Buildroot)
 * gcc (Ubuntu)
 ************************************************************************************
 *
 * @file test_loggingThread.c
 * @brief thread dequeues and writes msgs to file
 *
 ************************************************************************************
 */

#include <stdint.h>
#include <stddef.h>
#include <unistd.h>
#include <errno.h>
#include <string.h> /* for strerror() */
#include <signal.h>
#include <time.h>
#include <sys/syscall.h>
#include <signal.h>
#include <fcntl.h>

#include "loggingThread.h"
#include "debug.h"
#include "logger.h"
#include "packet.h"
#include "cmn_timer.h"
#include "platform.h"
#include "healthMonitor.h"

static uint8_t aliveFlag = 1;

/*---------------------------------------------------------------------------------*/
void loggingSigHandler(int signo, siginfo_t *info, void *extra)
{
	INFO_PRINT("loggingSigHandler, signum: %d\n",info->si_signo);
    aliveFlag = 0;
}

void logGetAliveFlag(uint8_t *pAlive)
{
  if(pAlive != NULL)
    *pAlive = aliveFlag;
}

void* logThreadHandler(void* threadInfo)
{
    SensorThreadInfo sensorInfo = *(SensorThreadInfo *)threadInfo;
    int logFd;                                          /* log file descriptor */
    mqd_t hbMsgQueue = -1;                              /* main status MessageQueue */
    struct timespec currentTime, lastStatusMsgTime;     /* to calc delta time */
    float deltaTime;                                    /* delta time since last sent status msg */

    /* how often other thread sends msg */
    const float otherThreadTime = TEMP_LOOP_TIME_SEC + (TEMP_LOOP_TIME_NSEC * 1e-9);

    /* timer variables */
    static timer_t timerid;
    sigset_t set;
    int signum = SIGALRM;
    struct timespec timer_interval;
    uint8_t ind, statusMsgCount;
	sigset_t mask;

    /* instantiate temp msg variable for dequeuing */
    logItem_t logItem, prevLogItem;
    uint8_t filename[64];
    uint8_t payload[128];
    logItem.pFilename = filename;
    logItem.pPayload = payload;
    logItem.logMsgId = LOG_MSG_INFO;
    prevLogItem = logItem;

    /* used to stop dequeue/write loop */
    uint8_t exitFlag = 1;

    /* add start msg to log msg queue */
    LOG_TEMP_SENSOR_EVENT(LOG_EVENT_STARTED);
    MUTED_PRINT("log tid: %d\n",(pid_t)syscall(SYS_gettid));

    /* main status msg queue */
    hbMsgQueue = mq_open(sensorInfo.heartbeatMsgQueueName, O_RDWR, 0666, NULL);
    if(hbMsgQueue == -1) {
        printf("ERROR: remoteThread Failed to Open heartbeat MessageQueue - exiting.\n");
        LOG_TEMP_SENSOR_EVENT(LOG_EVENT_OPEN_ERROR);
        return NULL;
    }

    /* block SIGRTs signals */
	sigemptyset(&mask);
    for(ind = 0; ind < NUM_THREADS; ++ind)
    {
        if(ind != (uint8_t)PID_LOGGING)
            sigaddset(&mask, SIGRTMIN + ind);
    }
    pthread_sigmask(SIG_BLOCK, &mask, NULL);

    /* try to open logfile */
    logFd = open(((LogThreadInfo *)threadInfo)->logFileName, O_CREAT | O_WRONLY | O_NONBLOCK | O_SYNC | O_TRUNC, 0644);
    if(logFd < 0)
    {
        ERROR_PRINT("failed to open log file, err#%d (%s)\n\r", errno, strerror(errno));
        SEND_STATUS_MSG(hbMsgQueue, PID_LOGGING, STATUS_ERROR, ERROR_CODE_USER_NOTIFY0);

        /* add log event msg to queue; probably only
         * useful if log thread is restarted and sucesfully 
         * opens file and starts reading from queue */
        LOG_LOG_EVENT(LOG_EVENT_OPEN_ERROR);
        return NULL;
    }
    /* add log event msg to queue */
    LOG_LOG_EVENT(LOG_EVENT_FILE_OPEN);

        
    /* send all log msgs for testing log parser, etc */
    #if (DEBUG_TEST_ALL_MSG_TYPES != 0)
        LOG_LOG_EVENT(LOG_EVENT_WRITE_ERROR);
        LOG_LOG_EVENT(LOG_EVENT_OPEN_ERROR);
    #endif

    /* Clear memory objects */
    memset(&set, 0, sizeof(sigset_t));
    memset(&timerid, 0, sizeof(timer_t));

    timer_interval.tv_nsec = LOG_LOOP_TIME_NSEC;
    timer_interval.tv_sec = LOG_LOOP_TIME_SEC;
    setupTimer(&set, &timerid, signum, &timer_interval);

    /* keep dequeuing and writing msgs until self decides to exit */
    while(exitFlag)
    {
        /* wait on signal timer */
        sigwait(&set, &signum);
        statusMsgCount = 0;
        MUTED_PRINT("logging is alive\n");

        /* if signaled to exit, shove log exit command
         * (2nd highest priority) in buffer and set exit flag */
        if(aliveFlag == 0)
        {
            LOG_LOG_EVENT(LOG_EVENT_EXITING);
            INFO_PRINT("logger exit triggered\n");
            exitFlag = 0;
        }

        /* dequeue a msg */
        if(LOG_DEQUEUE_ITEM(&logItem) != LOG_STATUS_OK)
        {
            ERROR_PRINT("log_dequeue_item error\n");
            SEND_STATUS_MSG(hbMsgQueue, PID_LOGGING, STATUS_ERROR, ERROR_CODE_USER_NOTIFY0);
            ++statusMsgCount;
        }
        else if ((prevLogItem.logMsgId != logItem.logMsgId) || (prevLogItem.time != logItem.time)
     || (prevLogItem.checksum != logItem.checksum))
        {
            if(logItem.logMsgId == LOG_MSG_CORE_DUMP)
                INFO_PRINT("core dump not implemented\n");

            /* if read from queue successful, right to file */
            if(LOG_WRITE_ITEM(&logItem, logFd) != LOG_STATUS_OK)
            {
                ERROR_PRINT("log_dequeue_item error\n");
                SEND_STATUS_MSG(hbMsgQueue, PID_LOGGING, STATUS_ERROR, ERROR_CODE_USER_NOTIFY0);
                ++statusMsgCount;
            }
            prevLogItem = logItem;
        }
        clock_gettime(CLOCK_REALTIME, &currentTime);
        deltaTime = (currentTime.tv_sec - lastStatusMsgTime.tv_sec) + ((currentTime.tv_nsec - lastStatusMsgTime.tv_nsec) * 1e-9);

        /* only send OK status at rate of other threads 
         * and if we didn't send error status yet */
        if((deltaTime > otherThreadTime) && (statusMsgCount == 0)) {
            SEND_STATUS_MSG(hbMsgQueue, PID_LOGGING, STATUS_OK, ERROR_CODE_USER_NONE0);
            ++statusMsgCount;
            clock_gettime(CLOCK_REALTIME, &lastStatusMsgTime);
        }
    }

    /* clean up */
    timer_delete(timerid);
    mq_close(hbMsgQueue);
    close(logFd);
    INFO_PRINT("logger thread exiting\n");
    return NULL;
}
/*---------------------------------------------------------------------------------*/

