/***********************************************************************************
 * @author Brian Ibeling and Joshua Malburg
 * Brian.Ibeling@colorado.edu, joshua.malburg@colorad.edu
 * Advanced Embedded Software Development
 * ECEN5013 - Rick Heidebrecht
 * @date April 12, 2019
 * CCS  Version: 8.3.0.00009
 ************************************************************************************
 *
 * @file remoteThread.c
 * @brief
 *
 ************************************************************************************
 */

/* stdlib includes */
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

/* app specific includes */
#include "remoteThread.h"
#include "cmn_timer.h"
#include "uartstdio.h"
#include "packet.h"
#include "my_debug.h"
#include "healthMonitor.h"
#include "logger.h"

/* TivaWare includes */
#include "driverlib/sysctl.h"
#include "driverlib/debug.h"
#include "driverlib/gpio.h"
#include "driverlib/pin_map.h"
#include "inc/hw_memmap.h"

/* FreeRTOS includes */
#include "FreeRTOS.h"
#include "queue.h"
#include "task.h"
#include "semphr.h"

void init(void);


void remoteTask(void *pvParameters)
{
    uint8_t count = 0, errCount = 0;
    float temp, moisture;
    const TickType_t xDelay = LOG_QUEUE_RECV_WAIT_DELAY / portTICK_PERIOD_MS;

    TaskStatusPacket statusMsg;
    LogMsgPacket logMsg;

    LOG_REMOTE_CLIENT_EVENT(REMOTE_EVENT_STARTED);

    /* initialize socket */
    /* TODO - socket stuff */

    /* set portion of statusMsg that does not change */
    memset(&logMsg, 0,sizeof(LogMsgPacket));

    /* get status queue handle, etc */
    SensorThreadInfo info = *((SensorThreadInfo *)pvParameters);

    /* TODO - set BIST error in logMsg if necessary */
    if(xQueueSend(info.logFd, ( void *)&logMsg, THREAD_MUTEX_DELAY) != pdPASS) {
        ++errCount;
    }

    /* send data, log and status msgs to Control Node */
    for (;;)
    {
        ++count;

        /* try to read sensor data from shmem */
        if( xSemaphoreTake( info.shmemMutex, THREAD_MUTEX_DELAY ) == pdTRUE )
        {
            /* read data from shmem */
            /* TODO - read shmem method */
            temp = info.pShmem->lightData.apds9301_luxData;
            moisture = info.pShmem->moistData.moistureLevel;
            temp = temp *= 1.0f;
            moisture *= 1.0f;

            /* release mutex */
            xSemaphoreGive(info.shmemMutex);
        }
        /* TODO - send shmem data to Control Node */

        /* get thread status msgs */
        if(xQueueReceive(info.statusFd, (void *)&statusMsg, xDelay) != pdFALSE)
        {
            /* for development (verify queue send/recv) */
            PRINT_STATUS_MSG_HEADER(&statusMsg);

            /* TODO - send status msgs to Control Node */
        }

        /* get log msgs */
        if(xQueueReceive(info.logFd, (void *)&logMsg, xDelay) != pdFALSE)
        {
            /* for development (verify queue send/recv) */
            PRINT_LOG_MSG_HEADER(&logMsg);

            /* TODO - send log msgs to Control Node */
        }
    }
}
