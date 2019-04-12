/***********************************************************************************
 * @author Brian Ibeling and Joshua Malburg
 * Brian.Ibeling@colorado.edu, joshua.malburg@colorad.edu
 * Advanced Embedded Software Development
 * ECEN5013 - Rick Heidebrecht
 * @date April 12, 2019
 * CCS  Version: 8.3.0.00009
 ************************************************************************************
 *
 * @file solenoidThread.c
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
#include "tiva_i2c.h"
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
#include <queue.h>
#include <task.h>
#include "semphr.h"


void solenoidTask(void *pvParameters)
{
    uint8_t enable = 0, count = 0, errCount = 0;
    TaskStatusPacket statusMsg;

    /* init LED gpio */
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPION);
    while(!SysCtlPeripheralReady(SYSCTL_PERIPH_GPION));
    GPIOPinTypeGPIOOutput(GPIO_PORTN_BASE, GPIO_PIN_0);

    /* set portion of statusMsg that does not change */
    memset(&statusMsg, 0,sizeof(TaskStatusPacket));
    statusMsg.processId = PID_SOLENOID;

    /* get status queue handle */
    SensorThreadInfo info = *((SensorThreadInfo *)pvParameters);

    for (;;) {

        /* update statusMsg */
        statusMsg.header = count++;
        statusMsg.timestamp = (xTaskGetTickCount() - info.xStartTime) * portTICK_PERIOD_MS;

        /* send status msg */
        if(xQueueSend(info.statusFd, ( void *)&statusMsg, (TickType_t)10) != pdPASS) {
            ++errCount;
        }

        /* Turn on the LED */
        enable = enable == 0 ? 0xFF : 0x00;
        GPIOPinWrite(GPIO_PORTN_BASE, GPIO_PIN_0, GPIO_PIN_0 & enable);
        GPIOPinWrite(GPIO_PORTN_BASE, GPIO_PIN_1, GPIO_PIN_1 & enable);

        /* try to get semaphore */
        if( xSemaphoreTake( info.shmemMutex, THREAD_MUTEX_DELAY ) == pdTRUE )
        {
            /* write data to shmem */

            /* release mutex */
            xSemaphoreGive(info.shmemMutex);
        }

        /* sleep */
        vTaskDelay(SOLENOID_TASK_DELAY_SEC * configTICK_RATE_HZ);
    }
}
