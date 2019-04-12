/***********************************************************************************
 * @author Brian Ibeling and Joshua Malburg
 * Brian.Ibeling@colorado.edu, joshua.malburg@colorad.edu
 * Advanced Embedded Software Development
 * ECEN5013 - Rick Heidebrecht
 * @date April 12, 2019
 * CCS  Version: 8.3.0.00009
 ************************************************************************************
 *
 * @file observerThread.c
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
#include "packet.h"

/* TivaWare includes */
#include "driverlib/sysctl.h"   /* for clk */
#include "driverlib/debug.h"
#include "driverlib/gpio.h"
#include "driverlib/pin_map.h"
#include "inc/hw_memmap.h"

/* FreeRTOS includes */
#include "FreeRTOS.h"
#include "queue.h"
#include "task.h"
#include "semphr.h"

#define ALARM_GPIO_PIN      (GPIO_PIN_0)

void observerTask(void *pvParameters)
{
    uint8_t count = 0, errCount = 0, alarm = 0, prev_alarm = 0;
    TaskStatusPacket statusMsg;

    /* init LED gpio */
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPION);
    while(!SysCtlPeripheralReady(SYSCTL_PERIPH_GPION));
    GPIOPinTypeGPIOOutput(GPIO_PORTN_BASE, ALARM_GPIO_PIN);

    /* set portion of statusMsg that does not change */
    memset(&statusMsg, 0,sizeof(TaskStatusPacket));
    statusMsg.processId = PID_OBSERVER;

    /* get status queue handle, etc */
    SensorThreadInfo info = *((SensorThreadInfo *)pvParameters);

    for (;;) {
        /* update statusMsg */
        statusMsg.header = count++;
        statusMsg.timestamp = (xTaskGetTickCount() - info.xStartTime) * portTICK_PERIOD_MS;

        /* send status msg */
        if(xQueueSend(info.statusFd, ( void *)&statusMsg, (TickType_t)10) != pdPASS) {
            ++errCount;
        }

        /* try to get semaphore */
        if( xSemaphoreTake( info.shmemMutex, THREAD_MUTEX_DELAY ) == pdTRUE )
        {
            /* check if moisture is low */
            if((info.pShmem->moistData.moistureLevel < info.pShmem->moistData.lowThreshold)) {

                /* if low, turn on alarm */
                alarm = 1;

                /* verify water value is on */
                if(info.pShmem->solenoidData.state == 1) {
                    /* if not turn on */
                    info.pShmem->solenoidData.cmd = 1;
                }
            }
            else if (info.pShmem->moistData.moistureLevel > info.pShmem->moistData.highThreshold) {
                alarm = 0;
            }

            /* release mutex */
            xSemaphoreGive(info.shmemMutex);
        }

        /* TODO - check connection status and set alarm if LOST */

        /* set alarm pin */
        if(alarm != prev_alarm) {
            GPIOPinWrite(GPIO_PORTN_BASE, ALARM_GPIO_PIN, alarm != 0 ? ALARM_GPIO_PIN : 0);
        }
        prev_alarm = alarm;

        /* sleep */
        vTaskDelay(REMOTE_TASK_DELAY_SEC * configTICK_RATE_HZ);
    }
}
