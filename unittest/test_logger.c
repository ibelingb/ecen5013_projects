/***********************************************************************************
 * @author Josh Malburg
 * 
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

#include "debug.h"
#include "logger.h"
#include "tempThread.h"
#include "lightThread.h"
#include "loggingThread.h"
#include "remoteThread.h"

#define NUM_THREADS (3)

/* global for causing threads to exit */
static uint8_t gExitSig = 1;

static void *sensorThread(void *log);
static void *remoteThread(void *log);
static void *logThread(void *log);


int main(void)
{
    pthread_t pThread[NUM_THREADS];
    uint8_t loopCount;

    /* initialize the logger and other resources */
    LOG_INIT();
    LOG_LOGGER_INITIALIZED();
    LOG_GPIO_INITIALIZED();

    /* spaw child threads */
    pthread_create(&pThread[0], NULL, sensorThread, NULL);
    pthread_create(&pThread[1], NULL, remoteThread, NULL);
    pthread_create(&pThread[2], NULL, logThread, NULL);

    /* system is now fully functional */
    gExitSig = 0;
    LOG_SYSTEM_INITIALIZED();

    /* do stuff, wait for exit signal 
    (e.g. in this case, when count to expire) */
    while(loopCount++ < 8)
    {
        LOG_HEARTBEAT();
        sleep(1);
    }

    /* add exit notification msg to log */
    LOG_SYSTEM_HALTED();

    for(uint8_t ind; ind < NUM_THREADS; ++ind)
 	    pthread_join(pThread[ind], NULL);

    return EXIT_SUCCESS;
}

static void *sensorThread(void *log)
{
    uint8_t count = 0;
    /* send status (started (i.e. running)) to log */

    /* init sensor, run BIST */

    /* log BIST sensor results */
    LOG_TEMP_SENSOR_EVENT(TEMP_EVENT_STARTED);
    LOG_LIGHT_SENSOR_EVENT(LIGHT_EVENT_STARTED);

    while(gExitSig)
    {
        /* log event */
        if((count++ % 4) == 0)
        {
            LOG_TEMP_SENSOR_EVENT(TEMP_EVENT_OVERTEMP);
            LOG_LIGHT_SENSOR_EVENT(LIGHT_EVENT_ERROR);
        }

        sleep(1);
    }

    LOG_TEMP_SENSOR_EVENT(TEMP_EVENT_EXITING);
    LOG_LIGHT_SENSOR_EVENT(LIGHT_EVENT_EXITING);
    return NULL;
}

static void *remoteThread(void *log)
{
    /* send status (started (i.e. running)) to log */

    /* init sensor, run BIST */
    /* log BIST server results */
    LOG_REMOTE_HANDLING_EVENT(REMOTE_EVENT_STARTED);

    while(gExitSig)
    {
        /* log event */
        LOG_REMOTE_HANDLING_EVENT(REMOTE_EVENT_CNCT_ACCEPTED);
        sleep(1);
    }

    LOG_REMOTE_HANDLING_EVENT(REMOTE_EVENT_EXITING);
    return NULL;
}

static void *logThread(void *log)
{
    /* send status (started (i.e. running)) to log */
    LOG_LOG_EVENT(LOG_EVENT_STARTED);

    /* init file / queue (run BIST) */

    /* log BIST results */
    LOG_LOG_EVENT(LOG_EVENT_FILE_OPEN);

    while(gExitSig)
    {
        /* read from queue */

        /* and write to file */
        sleep(1);
    }
    LOG_LOG_EVENT(LOG_EVENT_EXITING);
    return NULL;
}
