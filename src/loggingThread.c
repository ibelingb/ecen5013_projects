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

#include "loggingThread.h"
#include "debug.h"
#include "logger.h"
#include "packet.h"

extern int gExitLog;

/*---------------------------------------------------------------------------------*/
void* logThreadHandler(void* threadInfo)
{
    int logFd;  /* log file descriptor */

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

    /* try to open logfile */
    logFd = open(((LogThreadInfo *)threadInfo)->logFileName, O_CREAT | O_WRONLY | O_NONBLOCK | O_SYNC, 0644);
    if(logFd < 0)
    {
        ERROR_PRINT("failed to open log file, err#%d (%s)\n\r", errno, strerror(errno));
        return NULL;
    }

    #if (SHORT_CIRCUIT_FOR_DEBUG != 0)
        /* send log msgs */

        LOG_LOG_EVENT(LOG_EVENT_FILE_OPEN);
        #if (DEBUG_TEST_ALL_MSG_TYPES != 0)
            LOG_LOG_EVENT(LOG_EVENT_WRITE_ERROR);
            LOG_LOG_EVENT(LOG_EVENT_OPEN_ERROR);
        #endif
    #endif

    /* keep dequeuing and writing msgs until self decides to exit */
    while(exitFlag)
    {
        
        /* if signaled to exit, shove log exit command
         * (2nd highest priority) in buffer and set exit flag */
        if(gExitLog == 0)
        {
            #if (SHORT_CIRCUIT_FOR_DEBUG != 0)
                LOG_LOG_EVENT(LOG_EVENT_EXITING);
            #endif
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
        /* TODO - replace with timer */
        usleep(1000);

        /* TODO - send status to main status msg queue */
    }
    INFO_PRINT("logger thread exiting\n");
    return NULL;
}

/*---------------------------------------------------------------------------------*/
/* HELPER METHODS */


/*---------------------------------------------------------------------------------*/
