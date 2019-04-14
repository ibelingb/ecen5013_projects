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
#include "solenoidThread.h"
#include "cmn_timer.h"
#include "packet.h"
#include "my_debug.h"
#include "healthMonitor.h"
#include "logger.h"


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
#define SOLENOID_STATE_OFF          (0)
#define SOLENOID_STATE_ON           (1)
#define SOLENOID_ON_TIME_DURATION   (5000)  /* MSEC */

/*---------------------------------------------------------------------------------*/
static uint8_t keepAlive;   /* global to kill thread */

/*---------------------------------------------------------------------------------*/
int8_t updateSolenoidData(SensorThreadInfo *pInfo);

/*---------------------------------------------------------------------------------*/
void solenoidTask(void *pvParameters)
{
    uint8_t count = 0;
    TaskStatusPacket statusMsg;
    LogMsgPacket logMsg;
    uint8_t statusMsgCount;
    keepAlive = 1;

    LOG_SOLENOID_EVENT(SOLE_EVENT_STARTED);
    MUTED_PRINT("Solenoid Task #: %d\n\r", getTaskNum());

    /* init LED gpio */
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPION);
    while(!SysCtlPeripheralReady(SYSCTL_PERIPH_GPION));
    GPIOPinTypeGPIOOutput(GPIO_PORTN_BASE, GPIO_PIN_1);

    /* set portion of statusMsg that does not change */
    memset(&statusMsg, 0,sizeof(TaskStatusPacket));
    statusMsg.processId = PID_SOLENOID;

    /* set portion of statusMsg that does not change */
    memset(&logMsg, 0,sizeof(LogMsgPacket));

    /* get status queue handle, etc */
    SensorThreadInfo info = *((SensorThreadInfo *)pvParameters);

    /* TODO - set BIST error in logMsg if necessary */


    while(keepAlive)
    {
        statusMsgCount = 0;

        /* try to get semaphore */
        if( xSemaphoreTake( info.shmemMutex, THREAD_MUTEX_DELAY ) == pdTRUE )
        {
            /* read and update solenoid data */
            updateSolenoidData(&info);

            /* release mutex */
            xSemaphoreGive(info.shmemMutex);
        }

        /* only send OK status if we didn't send error status yet */
        if(statusMsgCount == 0) {
            /* update statusMsg */
            statusMsg.header = count++;
            statusMsg.timestamp = (xTaskGetTickCount() - info.xStartTime) * portTICK_PERIOD_MS;
            SEND_STATUS_MSG(info.statusFd, PID_SOLENOID, STATUS_OK, ERROR_CODE_USER_NONE0);
            ++statusMsgCount;
        }

        /* sleep */
        vTaskDelay(SOLENOID_TASK_DELAY_SEC * configTICK_RATE_HZ);
    }
    LOG_SOLENOID_EVENT(SOLE_EVENT_EXITING);
}

/*---------------------------------------------------------------------------------*/
int8_t updateSolenoidData(SensorThreadInfo *pInfo)
{
    uint8_t solenoidState;
    static uint8_t next_solenoidState;
    static uint8_t prev_solenoidState;
    static uint32_t solenoidStartTime;
    int32_t solenoidOnTime;

    /* OFF State */
    solenoidState = next_solenoidState;
    if(solenoidState == SOLENOID_STATE_OFF) {
        if(pInfo->pShmem->solenoidData.cmd == SOLENOID_STATE_ON) {
            next_solenoidState = SOLENOID_STATE_ON;
        }
    }
    /* ON State */
    else {
        /* upon entry, mark solenoid start time,
         * and set remainingOnTime */
        if((prev_solenoidState != solenoidState) && (solenoidState == SOLENOID_STATE_ON)) {
            solenoidStartTime = (xTaskGetTickCount() - pInfo->xStartTime) * portTICK_PERIOD_MS;
            pInfo->pShmem->solenoidData.remainingOnTime = SOLENOID_ON_TIME_DURATION;
            pInfo->pShmem->solenoidData.cmd = 0;
        }
        else {
            /* calculate on time duration */
            solenoidOnTime = (xTaskGetTickCount() - solenoidStartTime) * portTICK_PERIOD_MS;

            if(solenoidOnTime < 0) {
                /* integer rollover occurred, calc real diff */
            }

            if(solenoidOnTime > SOLENOID_ON_TIME_DURATION) {
                pInfo->pShmem->solenoidData.remainingOnTime = 0;
                next_solenoidState = SOLENOID_STATE_OFF;
            }
            else {
                pInfo->pShmem->solenoidData.remainingOnTime = SOLENOID_ON_TIME_DURATION - solenoidOnTime;
            }
        }
    }

    /* if state change occurred, set new solenoid value */
    if(prev_solenoidState != solenoidState) {

        GPIOPinWrite(GPIO_PORTN_BASE, GPIO_PIN_1, GPIO_PIN_1 & (solenoidState != 0 ? 0xFF : 0x00));
    }
    prev_solenoidState = solenoidState;
    pInfo->pShmem->solenoidData.state = solenoidState;

    return EXIT_SUCCESS;
}

/*---------------------------------------------------------------------------------*/
void killSolenoidTask(void)
{
    keepAlive = 0;
}
/*---------------------------------------------------------------------------------*/
