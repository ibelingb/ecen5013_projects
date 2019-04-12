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

/* TivaWare includes */
#include "driverlib/sysctl.h"
#include "driverlib/debug.h"
#include "driverlib/gpio.h"
#include "driverlib/pin_map.h"
#include "inc/hw_memmap.h"

/* FreeRTOS includes */
#include "FreeRTOS.h"
#include <queue.h>
#include <task.h>
#include "semphr.h"

void init(void);


void remoteTask(void *pvParameters)
{
    float temp;
    /* TODO - timer too long? */
    const TickType_t xDelay = OBSERVER_TASK_DELAY_SEC / portTICK_PERIOD_MS;
    TaskStatusPacket statusMsg;

    /* init UART IO */
    init();

    /* initialize socket */
    /* TODO - socket stuff */

    /* get status queue handle */
    SensorThreadInfo info = *((SensorThreadInfo *)pvParameters);

    /* send data and status msgs to Control Node */
    for (;;)
    {
        /* try to get semaphore */
        if( xSemaphoreTake( info.shmemMutex, THREAD_MUTEX_DELAY ) == pdTRUE )
        {
            /* read data from shmem */
            /* TODO - read shmem method */
            temp = info.pShmem->lightData.apds9301_luxData;

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
                UARTprintf("\r\n Temperature %d.%d degC",
                           (uint16_t)temp, ((uint16_t)(temp * 1000)) - (((uint16_t)temp) * 1000));
                break;
            case PID_SOLENOID:
            case PID_OBSERVER:
            case PID_MOISTURE:
                /* send read data */
                /* TODO - send via network method */

                /* UART can go away, no requirements */
                UARTprintf("\r\n Got %d count at %d ms from %d", statusMsg.header, statusMsg.timestamp, statusMsg.processId);
                break;
            default:
                break;
            }
        }
    }
}

/* initialize IO for UART */
void init(void)
{
    /* init UART IO */
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);
    GPIOPinConfigure(GPIO_PA0_U0RX);
    GPIOPinConfigure(GPIO_PA1_U0TX);
    GPIOPinTypeUART(GPIO_PORTA_BASE, GPIO_PIN_0 | GPIO_PIN_1);

    /* setup UART */
    UARTStdioConfig(0, 57600, SYSTEM_CLOCK);
}
