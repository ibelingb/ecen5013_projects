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
#include <sys/syscall.h>

#include "my_debug.h"
#include "logger.h"
#include "tempThread.h"
#include "lightThread.h"
#include "loggingThread.h"
#include "remoteThread.h"

#define NUM_THREADS (4)

/* global for causing threads to exit */
int gExitSig = 1;
int gExitLog = 1;

static void *DUMMY_tempThread(void *pArg);
static void *DUMMY_lightThread(void *pArg);
static void *DUMMY_remoteThread(void *pArg);

int main(void)
{
    pthread_t pThread[NUM_THREADS];
    mqd_t logQueue, heartbeatMsgQueue;
    char *heartbeatMsgQueueName = "/heartbeat_mq";
    char *logMsgQueueName        = "/logging_mq";
    char *logFile = "/usr/bin/log.bin";
    LogThreadInfo logThreadInfo;
    struct mq_attr mqAttr;

    /* Define MQ attributes */
    mqAttr.mq_flags   = 0;
    mqAttr.mq_maxmsg  = MSG_QUEUE_DEPTH;
    mqAttr.mq_msgsize = MSG_QUEUE_MSG_SIZE;
    mqAttr.mq_curmsgs = 0;

    /* set thread arg struct */
    strcpy(logThreadInfo.logMsgQueueName, logMsgQueueName);
    strcpy(logThreadInfo.heartbeatMsgQueueName, heartbeatMsgQueueName);
    strcpy(logThreadInfo.logFileName, logFile);

    /* Ensure MQs and Shared Memory properly cleaned up before starting */
    mq_unlink(logMsgQueueName);

    if(remove(logMsgQueueName) == -1 && errno != ENOENT)
	    ERRNO_PRINT("couldn't delete path");

    logQueue = mq_open(logMsgQueueName, O_CREAT, 0666, &mqAttr);
    if(logQueue < 0)
    {
        ERRNO_PRINT("failed to open log msg queue");
        return EXIT_FAILURE;
    }

    /* Ensure MQs and Shared Memory properly cleaned up before starting */
    mq_unlink(heartbeatMsgQueueName);

    if(remove(heartbeatMsgQueueName) == -1 && errno != ENOENT)
    { ERRNO_PRINT("couldn't delete log queue path"); return EXIT_FAILURE; }

    /* Create MessageQueue to receive thread status from all children */
    heartbeatMsgQueue = mq_open(heartbeatMsgQueueName, O_CREAT | O_RDWR | O_NONBLOCK, 0666, &mqAttr);
    if(heartbeatMsgQueue == -1)
    {
        ERROR_PRINT("ERROR: failed to create MessageQueue for Main TaskStatus reception - exiting.\n");
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

    /* and create logging thread before writing first log msg */
    pthread_create(&pThread[3], NULL, logThreadHandler, &logThreadInfo);

    usleep(20 * 1000);
    LOG_LOGGER_INITIALIZED();
    INFO_PRINT("main tid: %d\n",(pid_t)syscall(SYS_gettid));

    /* create other threads */
    pthread_create(&pThread[0], NULL, DUMMY_lightThread, NULL);
    pthread_create(&pThread[1], NULL, DUMMY_tempThread, NULL);
    pthread_create(&pThread[2], NULL, DUMMY_remoteThread, NULL);
    
    /* events logged by main */
    LOG_MAIN_EVENT(MAIN_EVENT_STARTED_THREADS);
    LOG_MAIN_EVENT(MAIN_EVENT_LIGHT_THREAD_UNRESPONSIVE);
    LOG_MAIN_EVENT(MAIN_EVENT_TEMP_THREAD_UNRESPONSIVE);
    LOG_MAIN_EVENT(MAIN_EVENT_REMOTE_THREAD_UNRESPONSIVE);
    LOG_MAIN_EVENT(MAIN_EVENT_LOGGING_THREAD_UNRESPONSIVE);
    LOG_MAIN_EVENT(MAIN_EVENT_RESTART_THREAD);
    LOG_MAIN_EVENT(MAIN_EVENT_LOG_QUEUE_FULL);
    LOG_MAIN_EVENT(MAIN_EVENT_ISSUING_EXIT_CMD);

    /* initialize other resources */
    LOG_GPIO_INITIALIZED();

    /* system is now fully functional */
    LOG_SYSTEM_INITIALIZED();

    /* do stuff, wait for exit signal 
    (e.g. in this case, when count to expire) */
    uint8_t loopCount = 0;
    while(loopCount++ < 8)
    {
        LOG_HEARTBEAT();
        sleep(1);
    }

    /* add exit notification msg to log */
    gExitSig = 0;
    LOG_SYSTEM_HALTED();

    /* wait to kill log so exit msgs get logged */
    usleep(10*1000);
    gExitLog = 0;

    for(uint8_t ind; ind < NUM_THREADS; ++ind)
        pthread_join(pThread[ind], NULL);

    mq_close(logQueue);

    printf("main exiting\n");
    return EXIT_SUCCESS;
}
static void *DUMMY_lightThread(void *log)
{
    /* send log msgs */
    LOG_LIGHT_SENSOR_EVENT(LIGHT_EVENT_STARTED);
    usleep(10000);
    LOG_LIGHT_SENSOR_EVENT(LIGHT_EVENT_DAY);
    usleep(10000);
    LOG_LIGHT_SENSOR_EVENT(LIGHT_EVENT_NIGHT);
    INFO_PRINT("light tid: %d\n",(pid_t)syscall(SYS_gettid));

    while(gExitSig)
    {
        sleep(1);
    }

    LOG_LIGHT_SENSOR_EVENT(LIGHT_EVENT_EXITING);
    printf("light thread exiting\n");
    return NULL;
}

static void *DUMMY_tempThread(void *log)
{
    /* send log msgs */
    LOG_TEMP_SENSOR_EVENT(TEMP_EVENT_STARTED);
    usleep(10000);
    LOG_TEMP_SENSOR_EVENT(TEMP_EVENT_OVERTEMP);
    usleep(10000);
    LOG_TEMP_SENSOR_EVENT(TEMP_EVENT_OVERTEMP_RELEQUISHED);
    usleep(10000);
    LOG_TEMP_SENSOR_EVENT(TEMP_EVENT_SENSOR_INIT_ERROR);
    usleep(10000);
    LOG_TEMP_SENSOR_EVENT(TEMP_EVENT_SENSOR_READ_ERROR);
    usleep(10000);
    LOG_TEMP_SENSOR_EVENT(TEMP_EVENT_STATUS_QUEUE_ERROR);
    usleep(10000);
    LOG_TEMP_SENSOR_EVENT(TEMP_EVENT_SHMEM_ERROR);
    usleep(10000);
    LOG_TEMP_SENSOR_EVENT(TEMP_EVENT_I2C_ERROR);
    INFO_PRINT("temp tid: %d\n",(pid_t)syscall(SYS_gettid));
    
    while(gExitSig)
    {
        sleep(1);
    }

    LOG_TEMP_SENSOR_EVENT(TEMP_EVENT_EXITING);
    printf("temp thread exiting\n");
    return NULL;
}

static void *DUMMY_remoteThread(void *log)
{
    /* send log msgs */
    LOG_REMOTE_HANDLING_EVENT(REMOTE_EVENT_STARTED);
    usleep(10000);
    LOG_REMOTE_HANDLING_EVENT(REMOTE_EVENT_CNCT_ACCEPTED);
    usleep(10000);
    LOG_REMOTE_HANDLING_EVENT(REMOTE_EVENT_CNCT_LOST);
    usleep(10000);
    LOG_REMOTE_HANDLING_EVENT(REMOTE_EVENT_CMD_RECV);
    usleep(10000);
    LOG_REMOTE_HANDLING_EVENT(REMOTE_EVENT_INVALID_RECV);
    usleep(10000);
    LOG_REMOTE_HANDLING_EVENT(REMOTE_EVENT_CNCT_ACCEPTED);
    INFO_PRINT("remote tid: %d\n",(pid_t)syscall(SYS_gettid));

    while(gExitSig)
    {
        sleep(1);
    }

    LOG_REMOTE_HANDLING_EVENT(REMOTE_EVENT_EXITING);
    printf("remote thread exiting\n");
    return NULL;
}

