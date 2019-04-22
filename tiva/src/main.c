/***********************************************************************************
 * @author Joshua Malburg
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
#include "lightThread.h"
#include "remoteThread.h"
#include "moistureThread.h"
#include "solenoidThread.h"
#include "observerThread.h"
#include "packet.h"
#include "logger.h"
#include "my_debug.h"

/* TivaWare includes */
#include "driverlib/sysctl.h"       /* for clk */
#include "driverlib/debug.h"
#include "driverlib/gpio.h"
#include "driverlib/pin_map.h"
#include "inc/hw_memmap.h"

/* FreeRTOS includes */
#include "FreeRTOS.h"
#include "FreeRTOS_IP.h"
#include "FreeRTOS_Sockets.h"
#include "FreeRTOSConfig.h"
#include "FreeRTOSIPConfig.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"

/*---------------------------------------------------------------------------------*/
#define STATUS_QUEUE_LENGTH         (8)
#define LOG_QUEUE_LENGTH            (8)

/* Note: FreeRTOS doesn't have an efficient way of
 * getting the thread / task number, so main has to call
 * its private setTaskNum method after creating the threads
 */
typedef struct {
    TaskHandle_t hdl;   /* FreeRTOS handle */
    int16_t taskNum;    /* FreeRTOS task number, used in logger */
} TaskStruct_t;

static TaskStruct_t tasks[PID_END];
uint32_t g_sysClk;

/*---------------------------------------------------------------------------------*/
void initUART(void);
int8_t setTaskNum(TaskHandle_t xHandle, ProcessId_e process);


/*---------------------------------------------------------------------------------*/
int main(void)
{
    uint32_t sysClock;
    static SensorThreadInfo info;

    /* Handles for the tasks create by main() */
    static TaskHandle_t lightTaskHandle = NULL;
    static TaskHandle_t remoteTaskHandle = NULL;
    static TaskHandle_t moistureTaskHandle = NULL;
    static TaskHandle_t observerTaskHandle = NULL;
    static TaskHandle_t solenoidTaskHandle = NULL;

    /* init system clock */
    sysClock = SysCtlClockFreqSet((SYSCTL_OSC_MAIN | SYSCTL_USE_PLL |
                            SYSCTL_XTAL_25MHZ |SYSCTL_CFG_VCO_480), SYSTEM_CLOCK);
    g_sysClk = sysClock;
    initUART();
    //InitIP();

    /* create status queue */
    QueueHandle_t statusQueue = xQueueCreate(STATUS_QUEUE_LENGTH, sizeof(struct TaskStatusPacket));
    if(statusQueue == 0) {
        return EXIT_FAILURE;
        /* TODO - don't want to return, what to do?? */
    }

    /* create log queue */
    QueueHandle_t logQueue = xQueueCreate(STATUS_QUEUE_LENGTH, sizeof(struct LogMsgPacket));
    if(logQueue == 0) {
        return EXIT_FAILURE;
        /* TODO - don't want to return, what to do?? */
    }

    /* create shared memory block */
    static Shmem_t shmem;
    memset(&shmem, 0,sizeof(LightDataStruct) +
                       sizeof(MoistureDataStruct) +
                       sizeof(SolenoidDataStruct));

    /* init thread info struct */
    memset(&info, 0,sizeof(SensorThreadInfo));
    info.statusFd = statusQueue;
    info.logFd = logQueue;
    info.sysClock = sysClock;
    info.xStartTime = xTaskGetTickCount();
    info.shmemMutex = xSemaphoreCreateMutex();
    info.pShmem = &shmem;

    /* initialize the logger (i.e. queue & writeability)
      * main should probably do this before creating
      * other threads to ensure logger is working before
      * main or other threads start send msgs to mog queue */
    if(LOG_INIT(&info) != LOG_STATUS_OK)
    {
      ERROR_PRINT("failed to init logger\n");
      return EXIT_FAILURE;
    }

    /* logger (i.e. queue writer) init complete */
    LOG_LOGGER_INITIALIZED();

    /* create threads */
    xTaskCreate(observerTask, (const portCHAR *)"Observer", configMINIMAL_STACK_SIZE, (void *)&info, 1, &observerTaskHandle);
    setTaskNum(observerTaskHandle, PID_OBSERVER);

    xTaskCreate(solenoidTask, (const portCHAR *)"Solenoid", configMINIMAL_STACK_SIZE, (void *)&info, 1, &solenoidTaskHandle);
    setTaskNum(solenoidTaskHandle, PID_SOLENOID);

    xTaskCreate(lightTask, (const portCHAR *)"Light", configMINIMAL_STACK_SIZE, (void *)&info, 1, &lightTaskHandle);
    setTaskNum(lightTaskHandle, PID_LIGHT);

    xTaskCreate(remoteTask, (const portCHAR *)"Remote", configMINIMAL_STACK_SIZE, (void *)&info, 1, &remoteTaskHandle);
    setTaskNum(remoteTaskHandle, PID_REMOTE_CLIENT);

    xTaskCreate(moistureTask, (const portCHAR *)"Moist", configMINIMAL_STACK_SIZE, (void *)&info, 1, &moistureTaskHandle);
    setTaskNum(moistureTaskHandle, PID_MOISTURE);

    vTaskStartScheduler();

    vQueueDelete(logQueue);
    vQueueDelete(statusQueue);
    vSemaphoreDelete(info.shmemMutex);
    return EXIT_SUCCESS;
}

/*---------------------------------------------------------------------------------*/
void initUART(void)
{
    /* init UART IO */
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);
    GPIOPinConfigure(GPIO_PA0_U0RX);
    GPIOPinConfigure(GPIO_PA1_U0TX);
    GPIOPinTypeUART(GPIO_PORTA_BASE, GPIO_PIN_0 | GPIO_PIN_1);

    /* setup UART */
    UARTStdioConfig(0, 57600, SYSTEM_CLOCK);
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

/*---------------------------------------------------------------------------------*/

int16_t getTaskNum(void)
{
    int8_t ind;
    TaskHandle_t hdl = xTaskGetCurrentTaskHandle();

    /* loop through struct to find task */
    for(ind = 0; ind < PID_END; ++ind)
    {
        if(hdl == tasks[ind].hdl) {
            return tasks[ind].taskNum;
        }
    }
    return 0;
}
/*----------------------------------------------------------------------------------*/

int8_t setTaskNum(TaskHandle_t xHandle, ProcessId_e process)
{
    TaskStatus_t xTaskDetails;

    /* got get struct w/ num in it */
    vTaskGetInfo(xHandle, &xTaskDetails, pdTRUE, eInvalid );

    /* update number in our structure */
    tasks[process].taskNum = xTaskDetails.xTaskNumber;
    tasks[process].hdl = xHandle;
    return EXIT_SUCCESS;
}

/*-----------------------------------------------------------------------------------*/

/* Called by FreeRTOS+TCP when the network connects or disconnects.  Disconnect
events are only received if implemented in the MAC driver. */
void vApplicationIPNetworkEventHook( eIPCallbackEvent_t eNetworkEvent )
{
    uint32_t ulIPAddress, ulNetMask, ulGatewayAddress, ulDNSServerAddress;
    char cBuffer[16];

    /* If the network has just come up...*/
    if( eNetworkEvent == eNetworkUp )
    {
        /* Print out the network configuration, which may have come from a DHCP
        server. */
        FreeRTOS_GetAddressConfiguration( &ulIPAddress, &ulNetMask, &ulGatewayAddress, &ulDNSServerAddress );
        FreeRTOS_inet_ntoa( ulIPAddress, cBuffer );
        INFO_PRINT( "\r\n\r\nIP Address: %s\r\n", cBuffer);

        FreeRTOS_inet_ntoa( ulNetMask, cBuffer );
        INFO_PRINT("Subnet Mask: %s\r\n", cBuffer);

        FreeRTOS_inet_ntoa( ulGatewayAddress, cBuffer );
        INFO_PRINT("Gateway Address: %s\r\n", cBuffer);

        FreeRTOS_inet_ntoa( ulDNSServerAddress, cBuffer );
        INFO_PRINT("DNS Server Address: %s\r\n\r\n\r\n", cBuffer);
    }
}
/*-----------------------------------------------------------------------------------*/

uint32_t ulApplicationGetNextSequenceNumber( uint32_t ulSourceAddress,
                                                    uint16_t usSourcePort,
                                                    uint32_t ulDestinationAddress,
                                                    uint16_t usDestinationPort )
{
     ( void ) ulSourceAddress;
     ( void ) usSourcePort;
     ( void ) ulDestinationAddress;
     ( void ) usDestinationPort;

     return uxRand();
}

UBaseType_t uxRand()
{
    //TODO: add random number generator
    return 1;
}


void vLoggingPrintf(const char *pcFormatString, ...)
{
    va_list vaArgP;

    //
    // Start the varargs processing.
    //
    va_start(vaArgP, pcFormatString);
    UARTprintf(pcFormatString, vaArgP);

    //
    // We're finished with the varargs now.
    //
    va_end(vaArgP);
}

