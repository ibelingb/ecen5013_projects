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
#include "packet.h"
#include "tiva_i2c.h"
#include "cmn_timer.h"

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


void ledTask(void *pvParameters)
{
    uint8_t enable = 0, count = 0, errCount = 0;
    LogPacket_t logMsg;

    /* init LED gpio */
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPION);
    while(!SysCtlPeripheralReady(SYSCTL_PERIPH_GPION));
    GPIOPinTypeGPIOOutput(GPIO_PORTN_BASE, GPIO_PIN_0);

    /* set portion of logMsg that does not change */
    memset(&logMsg, 0,sizeof(LogPacket_t));
    memcpy(logMsg.name, pcTaskGetName(NULL), sizeof(pcTaskGetName(NULL)));
    logMsg.msgId = MSG_ID_LED;
    logMsg.temp = 0;

    /* get log queue handle */
    ThreadInfo_t info = *((ThreadInfo_t *)pvParameters);

    for (;;) {

        /* update logMsg */
        logMsg.count = count++;
        logMsg.time = (xTaskGetTickCount() - info.xStartTime) * portTICK_PERIOD_MS;

        /* send msg */
        if(xQueueSend(info.logFd, ( void *)&logMsg, (TickType_t)10) != pdPASS) {
            ++errCount;
        }

        /* Turn on the LED */
        enable = enable == 0 ? 0xFF : 0x00;
        GPIOPinWrite(GPIO_PORTN_BASE, GPIO_PIN_0, GPIO_PIN_0 & enable);
        GPIOPinWrite(GPIO_PORTN_BASE, GPIO_PIN_1, GPIO_PIN_1 & enable);

        /* sleep */
        vTaskDelay(LED_TASK_DELAY_SEC * configTICK_RATE_HZ);
    }
}
