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
#include "my_debug.h"
#include "logger.h"
#include "packet.h"
#include "cmn_timer.h"
#include "platform.h"
#include "healthMonitor.h"

static uint8_t aliveFlag = 1;

/*---------------------------------------------------------------------------------*/
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
    uint8_t ind, statusMsgCount, logMsgCount;
	sigset_t mask;

    /* instantiate temp msg variable for dequeuing */
    logItem_t logItem, prevLogItem;
    uint8_t filename[64];
    uint8_t payload[128];

    memset(&logItem, 0, sizeof(logItem_t));
    logItem.pFilename = filename;
    logItem.pPayload = payload;
    logItem.logMsgId = LOG_MSG_INFO;
    prevLogItem = logItem;

    /* used to stop dequeue/write loop */
    uint8_t exitFlag = 1;
    uint8_t noMsgRecvd;
    uint8_t ret;

    /* add start msg to log msg queue */
    LOG_LOG_EVENT(LOG_EVENT_STARTED);
    MUTED_PRINT("log tid: %d\n",(pid_t)syscall(SYS_gettid));

    /* main status msg queue */
    hbMsgQueue = mq_open(sensorInfo.heartbeatMsgQueueName, O_RDWR | O_NONBLOCK, 0666, NULL);
    if(hbMsgQueue == -1) {
        printf("ERROR: loggingThread Failed to Open heartbeat MessageQueue - exiting.\n");
        LOG_LOG_EVENT(LOG_EVENT_SHMEM_ERROR);
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
    logFd = open(((LogThreadInfo *)threadInfo)->logFileName, O_CREAT | O_WRONLY | O_NONBLOCK | O_SYNC | O_APPEND, 0644);
    if(logFd < 0)
    {
        ERRNO_PRINT("loggingThread failed to open log file");
        SEND_STATUS_MSG(hbMsgQueue, PID_LOGGING, STATUS_ERROR, ERROR_CODE_USER_TERMALL0);

        /* add log event msg to queue; probably only
         * useful if log thread is restarted and sucesfully 
         * opens file and starts reading from queue */
        LOG_LOG_EVENT(LOG_EVENT_OPEN_LOGFILE_ERROR);
        return NULL;
    }
    /* add log event msg to queue */
    LOG_LOG_EVENT(LOG_EVENT_FILE_OPEN);
    LOG_LOG_EVENT(LOG_EVENT_BIST_COMPLETE);

    /* Clear memory objects */
    memset(&set, 0, sizeof(sigset_t));
    memset(&timerid, 0, sizeof(timer_t));

    timer_interval.tv_nsec = LOG_LOOP_TIME_NSEC;
    timer_interval.tv_sec = LOG_LOOP_TIME_SEC;
    setupTimer(&set, &timerid, signum, &timer_interval);

    MUTED_PRINT("loggingThread started successfully, pid: %d, SIGRTMIN+PID_e: %d\n",(pid_t)syscall(SYS_gettid), SIGRTMIN + PID_LOGGING);

    /* keep dequeuing and writing msgs until self decides to exit */
    while(exitFlag)
    {
        MUTED_PRINT("logging is alive\n");

        /* read all messages until none */
        noMsgRecvd = 0;
        logMsgCount = 0;
        do {
            statusMsgCount = 0;

            /* if signaled to exit, shove log exit command
            * (2nd highest priority) in buffer and set exit flag */
            if(aliveFlag == 0)
            {
                LOG_LOG_EVENT(LOG_EVENT_EXITING);
                INFO_PRINT("logger exit triggered\n");
                exitFlag = 0;
            }

            /* dequeue a msg */
            ret = LOG_DEQUEUE_ITEM(&logItem);

            /* check if queue is empty */
            if(ret == LOG_STATUS_TIMEOUT) {
                MUTED_PRINT("timeout\n");
                noMsgRecvd = 1;
            }
            /* check for other queue read errors */
            else if(ret != LOG_STATUS_OK)
            {
                ERROR_PRINT("log_dequeue_item error\n");
                SEND_STATUS_MSG(hbMsgQueue, PID_LOGGING, STATUS_ERROR, ERROR_CODE_USER_NOTIFY0);
                ++statusMsgCount;
            }

            else 
            {
                MUTED_PRINT("Logger successfully dequeued msg, count: %d\n", logMsgCount);

                /* verify msg is unique */
                if ((prevLogItem.logMsgId != logItem.logMsgId) || (prevLogItem.time != logItem.time) || 
                    (prevLogItem.checksum != logItem.checksum))
                {
                    ++logMsgCount;

                    MUTED_PRINT("NEW Msg Recvd, MsgId: %d, from sourceId: %d at %d usec\n\r", 
                    logItem.logMsgId, logItem.sourceId, logItem.time);
                
                    /* if read from queue successful, right to file */
                    MUTED_PRINT("writing log msg to file\n");
                    if(LOG_WRITE_ITEM(&logItem, logFd) != LOG_STATUS_OK)
                    {
                        ERROR_PRINT("log_dequeue_item error\n");
                        SEND_STATUS_MSG(hbMsgQueue, PID_LOGGING, STATUS_ERROR, ERROR_CODE_USER_NOTIFY0);
                        LOG_LOG_EVENT(LOG_EVENT_WRITE_LOGFILE_ERROR);
                        ++statusMsgCount;
                    }
                    else {
                        MUTED_PRINT("successfully wrote log msg to file\n");
                    }
                    prevLogItem = logItem;
                } 
            }
            
            /* calculate delta time (since last status msg TX) */
            clock_gettime(CLOCK_REALTIME, &currentTime);
            deltaTime = (currentTime.tv_sec - lastStatusMsgTime.tv_sec) + ((currentTime.tv_nsec - lastStatusMsgTime.tv_nsec) * 1e-9);

            /* only send OK status at rate of other threads 
            * and if we didn't send error status yet */
            if((deltaTime > otherThreadTime) && (statusMsgCount == 0)) {
                SEND_STATUS_MSG(hbMsgQueue, PID_LOGGING, STATUS_OK, ERROR_CODE_USER_NONE0);
                ++statusMsgCount;
                clock_gettime(CLOCK_REALTIME, &lastStatusMsgTime);
            }

            /* if processed too many, exit*/
            if(logMsgCount > (NUM_THREADS + NUM_REMOTE_REPORTING_THREADS) * 20) {
                INFO_PRINT("max log processing limit reached\n");
                noMsgRecvd = 1;
            }
        } while((noMsgRecvd == 0) && (exitFlag));

        /* wait on signal timer */
        sigwait(&set, &signum);
    }

    /* clean up */
    timer_delete(timerid);
    close(logFd);
    mq_close(hbMsgQueue);
    ERROR_PRINT("logger thread exiting\n");
    return NULL;
}
/*---------------------------------------------------------------------------------*/
void loggingSigHandler(int signo, siginfo_t *info, void *extra)
{
    if((info != NULL) && (extra != NULL))
    {
	    INFO_PRINT("loggingSigHandler, signum: %d\n",info->si_signo);
    }
    aliveFlag = 0;
}

/*---------------------------------------------------------------------------------*/
void logGetAliveFlag(uint8_t *pAlive)
{
  if(pAlive != NULL)
    *pAlive = aliveFlag;
}