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

/*---------------------------------------------------------------------------------*/
void* logThreadHandler(void* threadInfo)
{
    int logFd;  /* log file descriptor */

    /* timer variables */
    static timer_t timerid;
    sigset_t set;
    int signum = SIGALRM;
    struct timespec timer_interval;

    /* instantiate temp msg variable for dequeuing */
    logItem_t logItem;
    uint8_t filename[64];
    uint8_t payload[128];
    logItem.pFilename = filename;
    logItem.pPayload = payload;
    logItem.logMsgId = LOG_MSG_INFO;

    /* used to stop dequeue/write loop */
    uint8_t exitFlag = 1;

    /* add start msg to log msg queue */
    LOG_LOG_EVENT(LOG_EVENT_STARTED);
    INFO_PRINT("log tid: %d\n",(pid_t)syscall(SYS_gettid));

    /* try to open logfile */
    logFd = open(((LogThreadInfo *)threadInfo)->logFileName, O_CREAT | O_WRONLY | O_NONBLOCK | O_SYNC | O_TRUNC, 0644);
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

        /* if signaled to exit, shove log exit command
         * (2nd highest priority) in buffer and set exit flag */
        if(gExitLog == 0)
        {
            LOG_LOG_EVENT(LOG_EVENT_EXITING);
            INFO_PRINT("logger exit triggered\n");
            exitFlag = 0;
        }
        
        /* dequeue a msg */
        if(LOG_DEQUEUE_ITEM(&logItem) != LOG_STATUS_OK)
        {
            /* TODO - set error in status to main */
            ERROR_PRINT("log_dequeue_item error\n");
        }
        else
        {
            if(logItem.logMsgId == LOG_MSG_CORE_DUMP)
                INFO_PRINT("core dump not implemented\n");

            /* if read from queue successful, right to file */
            if(LOG_WRITE_ITEM(&logItem, logFd) != LOG_STATUS_OK)
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
