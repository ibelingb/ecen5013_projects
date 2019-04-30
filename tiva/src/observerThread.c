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
 * @brief Watchdog / Monitor task to determine if Remote Node has entered non-nominal
 *        state; perform error handling.
 *
 * Watchdog / monitor task to determine Remote Node state and in the event comm with
 * the Control Node is lost, the Observer task will then control watering of the plant
 * by checking the soil moisture readings against the last LOW THRES value set and
 * water the plant accordingly.
 *
 ************************************************************************************
 */
/* stdlib includes */
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

/* app specific includes */
#include "observerThread.h"
#include "cmn_timer.h"
#include "packet.h"
#include "my_debug.h"
#include "healthMonitor.h"
#include "logger.h"
#include "remoteThread.h"

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

/*---------------------------------------------------------------------------------*/
#define ALARM_GPIO_PIN                  (GPIO_PIN_0)

/*---------------------------------------------------------------------------------*/
static uint8_t keepAlive;   /* global to kill thread */

/*---------------------------------------------------------------------------------*/
void observerTask(void *pvParameters)
{
    uint8_t count = 0, alarm = 0, prev_alarm = 0;
    TaskStatusPacket statusMsg;
    LogMsgPacket logMsg;
    uint8_t statusMsgCount;
    SystemState_e systemState = DEGRADED; /* Default state on startup */
    keepAlive = 1;


    LOG_OBSERVER_EVENT(OBSERVE_EVENT_STARTED);
    MUTED_PRINT("Observer Task #: %d\n\r", getTaskNum());

    /* init alarm gpio */
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPION);
    while(!SysCtlPeripheralReady(SYSCTL_PERIPH_GPION));
    GPIOPinTypeGPIOOutput(GPIO_PORTN_BASE, ALARM_GPIO_PIN);

    /* set portion of statusMsg that does not change */
    memset(&statusMsg, 0,sizeof(TaskStatusPacket));
    statusMsg.processId = PID_OBSERVER;

    /* set portion of statusMsg that does not change */
    memset(&logMsg, 0,sizeof(LogMsgPacket));

    /* get status queue handle, etc */
    SensorThreadInfo info = *((SensorThreadInfo *)pvParameters);

    info.pShmem->moistData.highThreshold = SOIL_SATURATION_HIGH_THRES;
    info.pShmem->moistData.lowThreshold = SOIL_SATURATION_LOW_THRES;

    /* send BIST results to logger */
    if(0) {
        LOG_OBSERVER_EVENT(OBSERVE_EVENT_BIST_FAILED);
    }
    else {
        LOG_OBSERVER_EVENT(OBSERVE_EVENT_BIST_SUCCESS);
    }

    while(keepAlive)
    {
        statusMsgCount = 0;

        /* try to get semaphore */
        if( xSemaphoreTake( info.shmemMutex, THREAD_MUTEX_DELAY ) == pdTRUE )
        {
            /* Determine if light task has become unresponsive */
            if(info.pShmem->lightSensorUpdateTs + 3000 < (xTaskGetTickCount() - info.xStartTime) * portTICK_PERIOD_MS) {
                info.pShmem->lightSensorState = DEGRADED;
            }

            /* If unable to receive cmds from Control Node or unable to provide feedback data to Control Node, enter Fault state (water plant autonomously) */
            if((g_dataSocketLost != 0) || (g_cmdSocketLost != 0)) {
                systemState = FAULT;
                INFO_PRINT("RemoteNode in FAULT mode - autonomously water plant based on soil moisture\n");
            } else if((g_dataSocketLost == 0) && (g_cmdSocketLost == 0) &&
                     ((g_logSocketLost != 0) || (g_statusSocketLost != 0) || (info.pShmem->lightSensorState < NOMINAL))) {
                INFO_PRINT("RemoteNode in DEGRADED mode - Non-critical system component missing. Log Socket Lost: %d | Status Socket Lost: %d | LightSensor State: %d\n",
                           g_logSocketLost, g_statusSocketLost, info.pShmem->lightSensorState);
                systemState = DEGRADED;
            } else {
                systemState = NOMINAL;
            }

            /* check if moisture is low */
            if((info.pShmem->moistData.moistureLevel < info.pShmem->moistData.lowThreshold)) {

                /* if low, turn on alarm LED */
                alarm = 1;

                /* Autonomously water plant if in FAULT mode (comm with ControlNode failed) */
                if(systemState == FAULT) {
                    /* verify water value is on */
                    if(info.pShmem->solenoidData.state == 0) {
                        /* if not turn on */
                        LOG_OBSERVER_EVENT(OBSERVE_EVENT_CMD_OVERRIDE_ASSERTED);
                        info.pShmem->solenoidData.cmd = 1;
                    }
                }
            }
            else if (info.pShmem->moistData.moistureLevel > info.pShmem->moistData.highThreshold) {
                alarm = 0;
            }

            /* release mutex */
            xSemaphoreGive(info.shmemMutex);
        }
        else {
            LOG_OBSERVER_EVENT(OBSERVE_EVENT_SHMEM_ERROR);
        }

        /* check connection status and set alarm if LOST */
        if((g_cmdSocketLost == 1) || (g_statusSocketLost == 1) ||
                (g_logSocketLost == 1) || (g_dataSocketLost == 1)) {
            alarm = 1;
        }

        /* set alarm pin */
        if(alarm != prev_alarm) {
            GPIOPinWrite(GPIO_PORTN_BASE, ALARM_GPIO_PIN, alarm != 0 ? ALARM_GPIO_PIN : 0);
        }
        prev_alarm = alarm;

        /* only send OK status if we didn't send error status yet */
        if(statusMsgCount == 0) {
            /* update statusMsg */
            statusMsg.header = count++;
            statusMsg.timestamp = (xTaskGetTickCount() - info.xStartTime) * portTICK_PERIOD_MS;
            SEND_STATUS_MSG(info.statusFd, PID_OBSERVER, STATUS_OK, ERROR_CODE_USER_NONE0);
            ++statusMsgCount;
        }

        /* sleep */
        vTaskDelay(REMOTE_TASK_DELAY_SEC * configTICK_RATE_HZ);
    }
    LOG_OBSERVER_EVENT(OBSERVE_EVENT_EXITING);
    vTaskDelete(NULL);
}

/*---------------------------------------------------------------------------------*/
void killObserverTask(void)
{
    keepAlive = 0;
}

/*---------------------------------------------------------------------------------*/
