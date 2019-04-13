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
#include "cmn_timer.h"
#include "uartstdio.h"
#include "packet.h"
#include "my_debug.h"

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
    /* TODO - timer too long? */
    const TickType_t xDelay = OBSERVER_TASK_DELAY_SEC / portTICK_PERIOD_MS;
    TaskStatusPacket statusMsg;
    LogMsgPacket logMsg;

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

    /* send data and status msgs to Control Node */
    for (;;)
    {
        ++count;

        /* try to get semaphore */
        if( xSemaphoreTake( info.shmemMutex, THREAD_MUTEX_DELAY ) == pdTRUE )
        {
            /* read data from shmem */
            /* TODO - read shmem method */
            temp = info.pShmem->lightData.apds9301_luxData;
            moisture = info.pShmem->moistData.moistureLevel;

            /* release mutex */
            xSemaphoreGive(info.shmemMutex);
        }
        /* send read data */
        /* TODO - send via network method */

        /* get thread status msgs */
        if(xQueueReceive(info.statusFd, (void *)&statusMsg, xDelay) != pdFALSE)
        {
            switch (statusMsg.processId) {
            case PID_LIGHT:
//                INFO_PRINT("Temperature %d.%d degC\r\n ",
//                           (uint16_t)temp, ((uint16_t)(temp * 1000)) - (((uint16_t)temp) * 1000));

                INFO_PRINT("LuxData %d.%d \r\n ",
                           (uint16_t)temp, ((uint16_t)(temp * 1000)) - (((uint16_t)temp) * 1000));
            case PID_SOLENOID:
            case PID_OBSERVER:
            case PID_MOISTURE:
                INFO_PRINT("Moisture %d percent\r\n ", (int)moisture);

                /* send read data */
                /* TODO - send via network method */

                /* UART can go away, no requirements */
                INFO_PRINT("Got %d count at %d ms from %d\r\n ", statusMsg.header, statusMsg.timestamp, statusMsg.processId);
                break;
            default:
                break;
            }
        }

        /* get log msgs */
        if(xQueueReceive(info.logFd, (void *)&logMsg, xDelay) != pdFALSE)
        {
            /* send read log msg */
            /* TODO - send via network method */
        }
    }
}
