/***********************************************************************************
 * @author Joshua Malburg (joma0364)
 * joshua.malburg@colorado.edu
 * Advanced Embedded Software Development
 * ECEN5013 - Rick Feidebrecht
 * @date April 10, 2019
 * CCS  Version: 8.3.0.00009
 ************************************************************************************
 *
 * @file main.c
 * @brief
 *
 * References:
 *
 * from: http://www.ti.com/lit/an/spma073/spma073.pdf
 * https://jspicer.net/2018/07/27/solution-for-i2c-busy-status-latency/
 * https://github.com/jspicer-ltu/Tiva-C-Embedded/tree/master/Experiment17-I2C
 * https://github.com/akobyl/TM4C129_FreeRTOS_Demo
 *
 ************************************************************************************
 */

/* stdlib includes */
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

/* app specific includes */
#include "main.h"
#include "uartstdio.h"
#include "packet.h"

#include "lightThread.h"
#include "remoteThread.h"
#include "moistureThread.h"
#include "solenoidThread.h"

/* TivaWare includes */
#include "driverlib/sysctl.h"   /* for clk */
#include "driverlib/debug.h"
#include "driverlib/gpio.h"
#include "driverlib/pin_map.h"
#include "inc/hw_memmap.h"

/* FreeRTOS includes */
#include "FreeRTOSConfig.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

#define LOG_QUEUE_LENGTH        (8)
#define LOGGER_DELAY_MS         (1000)

/* private functions */
void loggerTask(void *pvParameters);


int main(void)
{
    uint32_t sysClock;
    static ThreadInfo_t info;

    /* init system clock */
    sysClock = SysCtlClockFreqSet((SYSCTL_OSC_MAIN | SYSCTL_USE_PLL |
                            SYSCTL_XTAL_25MHZ |SYSCTL_CFG_VCO_480), SYSTEM_CLOCK);

    /* create log queue */
    QueueHandle_t logQueue = xQueueCreate(LOG_QUEUE_LENGTH, sizeof(LogPacket_t));
    if(logQueue == 0) {
        return EXIT_FAILURE;
    }

    /* init thread info struct */
    memset(&info, 0,sizeof(ThreadInfo_t));
    info.logFd = logQueue;
    info.sysClock = sysClock;
    info.xStartTime = xTaskGetTickCount();

    /* create tasks */
    xTaskCreate(loggerTask, (const portCHAR *)"Logger",
                configMINIMAL_STACK_SIZE, (void *)&info, 1, NULL);

    xTaskCreate(ledTask, (const portCHAR *)"LEDs",
                configMINIMAL_STACK_SIZE, (void *)&info, 1, NULL);

    xTaskCreate(tempTask, (const portCHAR *)"Temp",
                configMINIMAL_STACK_SIZE, (void *)&info, 1, NULL);

    vTaskStartScheduler();
    return EXIT_SUCCESS;
}


/**
 * @brief logger task
 */
void loggerTask(void *pvParameters)
{
    const TickType_t xDelay = LOGGER_DELAY_MS / portTICK_PERIOD_MS;
    LogPacket_t logMsg;

    /* init UART IO */
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);
    GPIOPinConfigure(GPIO_PA0_U0RX);
    GPIOPinConfigure(GPIO_PA1_U0TX);
    GPIOPinTypeUART(GPIO_PORTA_BASE, GPIO_PIN_0 | GPIO_PIN_1);

    /* setup UART */
    UARTStdioConfig(0, 57600, SYSTEM_CLOCK);

    /* get log queue handle */
    ThreadInfo_t info = *((ThreadInfo_t *)pvParameters);

    for (;;) {
        /* get / wait for msg */
        if(xQueueReceive(info.logFd, (void *)&logMsg, xDelay) != pdFALSE)
        {
            /* print msg based on ID */
            if(logMsg.msgId == MSG_ID_TEMP) {
                UARTprintf("\r\n Temperature %d.%d degC at %d ms from %s",
                           (uint16_t)logMsg.temp, ((uint16_t)(logMsg.temp * 1000)) - (((uint16_t)logMsg.temp) * 1000),
                           logMsg.time, logMsg.name);
            }
            else if (logMsg.msgId == MSG_ID_LED) {
                UARTprintf("\r\n Toggle %d count at %d ms from %s", logMsg.count, logMsg.time, logMsg.name);
            }
        }
    }
}

/*  ASSERT() Error function
 *
 *  failed ASSERTS() from driverlib/debug.h are executed in this function
 */
void __error__(char *pcFilename, uint32_t ui32Line)
{
    // Place a breakpoint here to capture errors until logging routine is finished
    while (1)
    {
    }
}
