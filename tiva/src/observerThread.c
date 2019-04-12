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
    TaskStatusPacket statusMsg;

    /* set portion of statusMsg that does not change */
    memset(&statusMsg, 0,sizeof(TaskStatusPacket));
    statusMsg.processId = PID_OBSERVER;

    /* get log queue handle */
    SensorThreadInfo info = *((SensorThreadInfo *)pvParameters);

    for (;;) {
        /* update statusMsg */
        statusMsg.header = count++;
        statusMsg.timestamp = (xTaskGetTickCount() - info.xStartTime) * portTICK_PERIOD_MS;

        /* send msg */
        if(xQueueSend(info.statusFd, ( void *)&statusMsg, (TickType_t)10) != pdPASS) {
            ++errCount;
        }

        /* sleep */
        vTaskDelay(REMOTE_TASK_DELAY_SEC * configTICK_RATE_HZ);
    }
}
