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
 * @brief TODO
 *
 * Resources Utilized:
 *   - https://www.freertos.org/FreeRTOS-Plus/FreeRTOS_Plus_TCP/TCP_Networking_Tutorial_TCP_Client_and_Server.html
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
#include "FreeRTOS_Sockets.h"
#include "queue.h"
#include "task.h"
#include "semphr.h"

/*---------------------------------------------------------------------------------*/
static uint8_t keepAlive;   /* global to kill thread */

/*---------------------------------------------------------------------------------*/
void init(void);

/*---------------------------------------------------------------------------------*/
void remoteTask(void *pvParameters)
{
    uint8_t count = 0;
    float luxData, moisture;
    const TickType_t xDelay = LOG_QUEUE_RECV_WAIT_DELAY / portTICK_PERIOD_MS;
    TaskStatusPacket statusMsg;
    LogMsgPacket logMsg;
    keepAlive = 1;

    LOG_REMOTE_CLIENT_EVENT(REMOTE_EVENT_STARTED);
    MUTED_PRINT("Remote Task #: %d\n\r", getTaskNum());

    /* initialize sockets for sensor, status, and logging data channels */
    Socket_t xSensorSocket, xStatusSocket, xLoggingSocket;
    socklen_t xSize = sizeof( struct freertos_sockaddr );
    static const TickType_t xTimeOut = pdMS_TO_TICKS( 2000 );

    /* set portion of statusMsg that does not change */
    memset(&logMsg, 0,sizeof(LogMsgPacket));

    /* get status queue handle, etc */
    SensorThreadInfo info = *((SensorThreadInfo *)pvParameters);

    /* TODO - set BIST error in logMsg if necessary */
    LOG_REMOTE_CLIENT_EVENT(REMOTE_BIST_COMPLETE);
    if(0) {
        LOG_REMOTE_CLIENT_EVENT(REMOTE_INIT_SUCCESS);
    }
    else {
        LOG_REMOTE_CLIENT_EVENT(REMOTE_INIT_ERROR);
    }

    /* send data, log and status msgs to Control Node */
    while(keepAlive)
    {
        ++count;

        /* try to read sensor data from shmem */
        /* TODO - need to restrict how offet this is sent */
        if( xSemaphoreTake( info.shmemMutex, THREAD_MUTEX_DELAY ) == pdTRUE )
        {
            /* read data from shmem */
            /* TODO - read shmem method */
            luxData = info.pShmem->lightData.apds9301_luxData;
            moisture = info.pShmem->moistData.moistureLevel;
            luxData = luxData *= 1.0f;
            moisture *= 1.0f;

            /* release mutex */
            xSemaphoreGive(info.shmemMutex);
        }
        else {
            /* mutex timeout occurred */
            LOG_REMOTE_CLIENT_EVENT(REMOTE_SHMEM_ERROR);
        }
        /* TODO - send shmem data to Control Node */



        /* get thread status msgs */
        if(xQueueReceive(info.statusFd, (void *)&statusMsg, xDelay) != pdFALSE)
        {
            /* for development (verify queue send/recv) */
            //PRINT_STATUS_MSG_HEADER(&statusMsg);

            /* TODO - send status msgs to Control Node */
        }
        else {
            //LOG_REMOTE_CLIENT_EVENT(REMOTE_LOG_QUEUE_ERROR);
        }

        /* get log msgs */
        if(xQueueReceive(info.logFd, (void *)&logMsg, xDelay) != pdFALSE)
        {
            /* for development (verify queue send/recv) */
            PRINT_LOG_MSG_HEADER(&logMsg);

            /* TODO - send log msgs to Control Node */
        }
        else {
            //LOG_REMOTE_CLIENT_EVENT(REMOTE_LOG_QUEUE_ERROR);
        }
    }
    LOG_REMOTE_CLIENT_EVENT(REMOTE_EVENT_EXITING);
}

/*---------------------------------------------------------------------------------*/
void killRemoteTask(void)
{
    keepAlive = 0;
}

/*---------------------------------------------------------------------------------*/
