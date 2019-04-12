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

/* FreeRTOS includes */
#include "FreeRTOS.h"
#include <queue.h>
#include <task.h>

void observerTask(void *pvParameters)
{
    uint8_t count = 0, errCount = 0;
    LogPacket_t logMsg;


    /* set portion of logMsg that does not change */
    memset(&logMsg, 0,sizeof(LogPacket_t));
    memcpy(logMsg.name, pcTaskGetName(NULL), sizeof(pcTaskGetName(NULL)));
    logMsg.msgId = PID_OBSERVER;
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

        /* sleep */
        vTaskDelay(REMOTE_TASK_DELAY_SEC * configTICK_RATE_HZ);
    }
}
