/***********************************************************************************
 * @author Josh Malburg
 * joshua.malburg@colorado.edu
 * Advanced Embedded Software Development
 * ECEN5013 - Rick Heidebrecht
 * @date March 15, 2019
 * arm-linux-gnueabi (Buildroot)
 * gcc (Ubuntu)
 ************************************************************************************
 *
 * @file test_logger.c
 * @brief verify logger msgs & writes to queue
 *
 ************************************************************************************
 */

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <stdint.h>
#include <pthread.h>
#include <mqueue.h>
#include <errno.h>
#include <string.h>     // for strerror()

#include "debug.h"
#include "logger.h"
#include "tempThread.h"
#include "lightThread.h"
#include "loggingThread.h"
#include "remoteThread.h"

#define NUM_THREADS (4)


/* global for causing threads to exit */
int gExitSig = 1;
int gExitLog = 1;

#if (SHORT_CIRCUIT_FOR_DEBUG != 0)
    static void *tempThread(void *pArg);
    static void *lightThread(void *pArg);
    static void *remoteThread(void *pArg);
#endif

int main(void)
{
    pthread_t pThread[NUM_THREADS];
    mqd_t logQueue;
    char *logMsgQueueName = "/mq";
    char *logFile = "scripts/log.bin";
    LogThreadInfo logThreadInfo;
    struct mq_attr mqAttr;

    /* Define MQ attributes */
    mqAttr.mq_flags   = 0;
    mqAttr.mq_maxmsg  = MSG_QUEUE_DEPTH / 2;
    mqAttr.mq_msgsize = sizeof(LogMsgPacket);
    mqAttr.mq_curmsgs = 0;

    /* set thread arg struct */
    strcpy(logThreadInfo.logMsgQueueName, logMsgQueueName);
    strcpy(logThreadInfo.logFileName, logFile);

    /* Ensure MQs and Shared Memory properly cleaned up before starting */
    mq_unlink(logMsgQueueName);

    if(remove(logMsgQueueName) == -1 && errno != ENOENT)
	    ERROR_PRINT("couldn't delete path, err#%d (%s)\n\r", errno, strerror(errno));

    logQueue = mq_open(logMsgQueueName, O_CREAT, 0666, &mqAttr);
    if(logQueue < 0)
    {
        ERROR_PRINT("failed to open log msg queue, err#%d (%s)\n\r", errno, strerror(errno));
        return EXIT_FAILURE;
    }

    /* initialize the logger (i.e. queue & writeability) 
     * main should probably do this before creating
     * other threads to ensure logger is working before
     * main or other threads start send msgs to mog queue */
    if(LOG_INIT(&logThreadInfo) != LOG_STATUS_OK)
    {
        ERROR_PRINT("failed to init logger\n");
        return EXIT_FAILURE;
    }
    LOG_LOGGER_INITIALIZED();
    INFO_PRINT("main tid: %d\n",(pid_t)syscall(SYS_gettid));

    /* spaw child threads */
    pthread_create(&pThread[3], NULL, logThreadHandler, &logThreadInfo);

    #if (SHORT_CIRCUIT_FOR_DEBUG != 0)
        pthread_create(&pThread[0], NULL, lightThread, NULL);
        pthread_create(&pThread[1], NULL, tempThread, NULL);
        pthread_create(&pThread[2], NULL, remoteThread, NULL);
        

        /* events logged by main */
        LOG_MAIN_EVENT(MAIN_EVENT_STARTED_THREADS);
        LOG_MAIN_EVENT(MAIN_EVENT_THREAD_UNRESPONSIVE);
        LOG_MAIN_EVENT(MAIN_EVENT_RESTART_THREAD);
        LOG_MAIN_EVENT(MAIN_EVENT_LOG_QUEUE_FULL);
        LOG_MAIN_EVENT(MAIN_EVENT_ISSUING_EXIT_CMD);

        /* initialize other resources */
        LOG_GPIO_INITIALIZED();

        /* system is now fully functional */
        LOG_SYSTEM_INITIALIZED();

        /* do stuff, wait for exit signal 
        (e.g. in this case, when count to expire) */
        uint8_t loopCount;
        while(loopCount++ < 8)
        {
            LOG_HEARTBEAT();
            sleep(1);
        }

        /* add exit notification msg to log */
        gExitSig = 0;
        LOG_SYSTEM_HALTED();

        /* wait to kill log so exit msgs get logged */
        sleep(2);
        gExitLog = 0;

        for(uint8_t ind; ind < NUM_THREADS; ++ind)
            pthread_join(pThread[ind], NULL);
    #else
            gExitSig = 0;
            pthread_join(pThread[3], NULL);
    #endif

    mq_close(logQueue);

    INFO_PRINT("main exiting\n");
    return EXIT_SUCCESS;
}
#if (SHORT_CIRCUIT_FOR_DEBUG != 0)
    static void *lightThread(void *log)
    {
        /* send log msgs */
        LOG_LIGHT_SENSOR_EVENT(LIGHT_EVENT_STARTED);
        LOG_LIGHT_SENSOR_EVENT(LIGHT_EVENT_DAY);
        LOG_LIGHT_SENSOR_EVENT(LIGHT_EVENT_NIGHT);
        LOG_LIGHT_SENSOR_EVENT(LIGHT_EVENT_ERROR);
        INFO_PRINT("light tid: %d\n",(pid_t)syscall(SYS_gettid));

        while(gExitSig)
        {
            sleep(1);
        }

        LOG_LIGHT_SENSOR_EVENT(LIGHT_EVENT_EXITING);
        INFO_PRINT("light thread exiting\n");
        return NULL;
    }

    static void *tempThread(void *log)
    {
        /* send log msgs */
        LOG_TEMP_SENSOR_EVENT(TEMP_EVENT_STARTED);
        LOG_TEMP_SENSOR_EVENT(TEMP_EVENT_OVERTEMP);
        LOG_TEMP_SENSOR_EVENT(TEMP_EVENT_OVERTEMP_RELEQUISHED);
        LOG_TEMP_SENSOR_EVENT(TEMP_EVENT_ERROR);
        INFO_PRINT("temp tid: %d\n",(pid_t)syscall(SYS_gettid));
        
        while(gExitSig)
        {
            sleep(1);
        }

        LOG_TEMP_SENSOR_EVENT(TEMP_EVENT_EXITING);
        INFO_PRINT("temp thread exiting\n");
        return NULL;
    }

    static void *remoteThread(void *log)
    {
        /* send log msgs */
        LOG_REMOTE_HANDLING_EVENT(REMOTE_EVENT_STARTED);
        LOG_REMOTE_HANDLING_EVENT(REMOTE_EVENT_CNCT_ACCEPTED);
        LOG_REMOTE_HANDLING_EVENT(REMOTE_EVENT_CNCT_LOST);
        LOG_REMOTE_HANDLING_EVENT(REMOTE_EVENT_CMD_RECV);
        LOG_REMOTE_HANDLING_EVENT(REMOTE_EVENT_INVALID_RECV);
        LOG_REMOTE_HANDLING_EVENT(REMOTE_EVENT_ERROR);
        LOG_REMOTE_HANDLING_EVENT(REMOTE_EVENT_CNCT_ACCEPTED);
        INFO_PRINT("remote tid: %d\n",(pid_t)syscall(SYS_gettid));

        while(gExitSig)
        {
            sleep(1);
        }

        LOG_REMOTE_HANDLING_EVENT(REMOTE_EVENT_EXITING);
        INFO_PRINT("remote thread exiting\n");
        return NULL;
    }
#endif

