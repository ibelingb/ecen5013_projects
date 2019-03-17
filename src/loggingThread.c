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

#include "loggingThread.h"
#include "debug.h"
#include "logger.h"
#include "packet.h"

// TODO - Remove? Defined in test_logger.c
int gExitLog;

/*---------------------------------------------------------------------------------*/
void* logThreadHandler(void* threadInfo)
{
    int logFd;  /* log file descriptor */

    //TODO - Remove? Defined in test_logger.c
    gExitLog = 1;

    /* timer variables */
    struct sigevent timer_sigevent;
    static timer_t timerid;
    struct itimerspec trig;
    sigset_t set;
    int signum = SIGALRM;

    /* instantiate temp msg variable for dequeuing */
    logItem_t logItem;
    uint8_t filename[64];
    uint8_t payload[128];
    logItem.pFilename = filename;
    logItem.pPayload = payload;

    /* used to stop dequeue/write loop */
    uint8_t exitFlag = 1;

    /* add start msg to log msg queue */
    LOG_LOG_EVENT(LOG_EVENT_STARTED);
    INFO_PRINT("log tid: %d\n",(pid_t)syscall(SYS_gettid));

    /* try to open logfile */
    logFd = open(((LogThreadInfo *)threadInfo)->logFileName, O_CREAT | O_WRONLY | O_NONBLOCK | O_SYNC, 0644);
    if(logFd < 0)
    {
        ERROR_PRINT("failed to open log file, err#%d (%s)\n\r", errno, strerror(errno));

        /* add log event msg to queue; probably only
         * useful if log thread is restarted and sucesfully 
         * opens file and starts reading from queue */
        LOG_LOG_EVENT(LOG_EVENT_OPEN_ERROR);
        return NULL;
    }
    /* add log event msg to queue */
    LOG_LOG_EVENT(LOG_EVENT_FILE_OPEN);

    #if (SHORT_CIRCUIT_FOR_DEBUG != 0)
        
        /* send all log msgs for testing log parser, etc */
        #if (DEBUG_TEST_ALL_MSG_TYPES != 0)
            LOG_LOG_EVENT(LOG_EVENT_WRITE_ERROR);
            LOG_LOG_EVENT(LOG_EVENT_OPEN_ERROR);
        #endif
    #endif

    /* Clear memory objects */
    memset(&timer_sigevent, 0, sizeof(struct sigevent));
    memset(&trig, 0, sizeof(struct itimerspec));

    /* create and initiate timer */
    /* Set the notification method as SIGEV_THREAD_ID:
     * Upon timer expiration, only this thread gets notified */
    timer_sigevent.sigev_notify = SIGEV_THREAD_ID;
    timer_sigevent._sigev_un._tid = (pid_t)syscall(SYS_gettid);
    timer_sigevent.sigev_signo = SIGALRM;

    sigemptyset(&set);
    sigaddset(&set, signum);
    sigprocmask(SIG_BLOCK, &set, NULL);

    /* Create timer */
    timer_create(CLOCK_REALTIME, &timer_sigevent, &timerid);

    /* Set expiration and interval time */
    trig.it_value.tv_nsec = TIMER_INTERVAL * 1e9;
    trig.it_interval.tv_nsec = TIMER_INTERVAL * 1e9;

    /* Arm / start the timer */
    timer_settime(timerid, 0, &trig, NULL);

    /* keep dequeuing and writing msgs until self decides to exit */
    while(exitFlag)
    {
        /* wait on signal timer */
        sigwait(&set, &signum);

        /* if signaled to exit, shove log exit command
         * (2nd highest priority) in buffer and set exit flag */
        if(gExitLog == 0)
        {
            #if (SHORT_CIRCUIT_FOR_DEBUG != 0)
                LOG_LOG_EVENT(LOG_EVENT_EXITING);
            #endif
            INFO_PRINT("logger exit triggered\n");
            exitFlag = 0;
        }
        
        /* dequeue a msg */
        if(log_dequeue_item(&logItem) != LOG_STATUS_OK)
        {
            /* TODO - set error in status to main */
            ERROR_PRINT("log_dequeue_item error\n");
        }
        else
        {
            /* if read from queue successful, right to file */
            if(log_write_item(&logItem, logFd) != LOG_STATUS_OK)
            {
                /* TODO - set error in status to main */
                ERROR_PRINT("log_dequeue_item error\n");
            }
        }
        /* TODO - send status to main status msg queue */
    }

    /* clean up */
    timer_delete(timerid);
	  close(logFd);
    INFO_PRINT("logger thread exiting\n");
    return NULL;
}

/*---------------------------------------------------------------------------------*/
/* HELPER METHODS */


/*---------------------------------------------------------------------------------*/
