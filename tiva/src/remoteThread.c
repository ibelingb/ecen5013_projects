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
#include "packet.h"
#include "uartstdio.h"

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

void init(void);


void remoteTask(void *pvParameters)
{
    /* TODO - timer too long? */
    const TickType_t xDelay = OBSERVER_TASK_DELAY_SEC / portTICK_PERIOD_MS;
    LogPacket_t logMsg;

    /* init UART IO */
    init();

    /* initialize socket */
    /* TODO - socket stuff */

    /* get log queue handle */
    ThreadInfo_t info = *((ThreadInfo_t *)pvParameters);

    /* send data and status msgs to Control Node */
    for (;;)
    {
        /* read shared memory data */
        /* TODO - read shmem method */

        /* send read data */
        /* TODO - send via network method */

        /* get thread status msgs */
        if(xQueueReceive(info.logFd, (void *)&logMsg, xDelay) != pdFALSE)
        {
            switch (logMsg.msgId) {
            case PID_LIGHT:
            case PID_SOLENOID:
            case PID_OBSERVER:
            case PID_MOISTURE:
                /* send read data */
                /* TODO - send via network method */

                /* UART can go away, no requirements */
                UARTprintf("\r\n Got %d count at %d ms from %s", logMsg.count, logMsg.time, logMsg.name);
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
