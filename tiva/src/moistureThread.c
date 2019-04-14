/***********************************************************************************
 * @author Brian Ibeling and Joshua Malburg
 * Brian.Ibeling@colorado.edu, joshua.malburg@colorad.edu
 * Advanced Embedded Software Development
 * ECEN5013 - Rick Heidebrecht
 * @date April 12, 2019
 * CCS  Version: 8.3.0.00009
 ************************************************************************************
 *
 * @file moistureThread.c
 * @brief
 *
 * reference: TivaWare single_ended.c
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
#include "my_debug.h"
#include "healthMonitor.h"
#include "logger.h"
#include "moistureThread.h"

/* TivaWare includes */
#include "driverlib/sysctl.h"   /* for clk */
#include "driverlib/debug.h"
#include "driverlib/pin_map.h"
#include "driverlib/gpio.h"
#include "inc/hw_memmap.h"
#include "driverlib/adc.h"

/* FreeRTOS includes */
#include "FreeRTOS.h"
#include "queue.h"
#include "task.h"
#include "semphr.h"

#define SOIL_SAMPLE_COUNT   (10)
#define SOIL_ADC_SCALE      (100.0f / 4096.0f)

/* global to kill thread */
static uint8_t keepAlive;


int8_t ConfigureADC0(void);
int8_t getMoisture(float *pMoist);
int8_t getADC0Value(uint32_t *pValue);

void moistureTask(void *pvParameters)
{
    uint8_t count = 0;
    TaskStatusPacket statusMsg;
    LogMsgPacket logMsg;
    float moisture;
    uint8_t statusMsgCount;
    keepAlive = 1;

    LOG_MOISTURE_EVENT(MOIST_EVENT_STARTED);

    /* set portion of statusMsg that does not change */
    memset(&statusMsg, 0,sizeof(TaskStatusPacket));
    statusMsg.processId = PID_MOISTURE;

    /* set portion of statusMsg that does not change */
    memset(&logMsg, 0,sizeof(LogMsgPacket));

    /* get status queue handle, etc */
    SensorThreadInfo info = *((SensorThreadInfo *)pvParameters);

    /* init peripheral and send BIST event to log queue */
    if(ConfigureADC0() != EXIT_SUCCESS) {
        LOG_MOISTURE_EVENT(MOIST_EVENT_BIST_FAILED);
    }
    else {
        LOG_MOISTURE_EVENT(MOIST_EVENT_BIST_SUCCESS);
    }

    while(keepAlive)
    {
        statusMsgCount = 0;

        /* get sensor data */
        if(getMoisture(&moisture) != EXIT_SUCCESS) {
            SEND_STATUS_MSG(info.statusFd, PID_MOISTURE, STATUS_ERROR, ERROR_CODE_USER_NOTIFY0);
            ++statusMsgCount;
            LOG_MOISTURE_EVENT(MOIST_EVENT_ADC_ERROR);
            ERROR_PRINT("getMoisture fault\r\n");
        }
        else {
            MUTED_PRINT("moisture = %d\r\n ", (int)moisture);
        }

        /* try to get semaphore */
        if( xSemaphoreTake( info.shmemMutex, THREAD_MUTEX_DELAY ) == pdTRUE )
        {
            /* write data to shmem */
            info.pShmem->moistData.moistureLevel = moisture;

            /* release mutex */
            xSemaphoreGive(info.shmemMutex);
        }
        else {
            LOG_MOISTURE_EVENT(MOIST_EVENT_SHMEM_ERROR);
        }

        /* only send OK status if we didn't send error status yet */
        if(statusMsgCount == 0) {
            /* update statusMsg */
            statusMsg.header = count++;
            statusMsg.timestamp = (xTaskGetTickCount() - info.xStartTime) * portTICK_PERIOD_MS;
            SEND_STATUS_MSG(info.statusFd, PID_MOISTURE, STATUS_OK, ERROR_CODE_USER_NONE0);
            ++statusMsgCount;
        }

        /* sleep */
        vTaskDelay(MOISTURE_TASK_DELAY_SEC * configTICK_RATE_HZ);
    }
    LOG_MOISTURE_EVENT(MOIST_EVENT_EXITING);
}

/*
 * @brief init ADC0
 */
int8_t ConfigureADC0(void)
{
    /* enable ADC0 peripheral */
    SysCtlPeripheralEnable(SYSCTL_PERIPH_ADC0);

    /* For this example ADC0 is used with AIN0 on port E7.
     * The actual port and pins used may be different on your part, consult
     * the data sheet for more information.  GPIO port E needs to be enabled
     * so these pins can be used. */
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOE);

    /* Select the analog ADC function for these pins.
     * Consult the data sheet to see which functions are allocated per pin  */
    GPIOPinTypeADC(GPIO_PORTE_BASE, GPIO_PIN_3);

    /* Configure ADC0 Sample Sequencer 3 for processor trigger operation */
    ADCSequenceConfigure(ADC0_BASE, 3, ADC_TRIGGER_PROCESSOR, 0);

    /* Configure step 0 on sequence 3.  Sample channel 0 (ADC_CTL_CH0) in
     * single-ended mode (default) and configure the interrupt flag
     * (ADC_CTL_IE) to be set when the sample is done.  Tell the ADC logic
     * that this is the last conversion on sequence 3 (ADC_CTL_END).  Sequence
     * 3 has only one programmable step.  Sequence 1 and 2 have 4 steps, and
     * sequence 0 has 8 programmable steps.  Since we are only doing a single
     * conversion using sequence 3 we will only configure step 0.  For more
     * information on the ADC sequences and steps, reference the datasheet. */
    ADCSequenceStepConfigure(ADC0_BASE, 3, 0, ADC_CTL_CH0 | ADC_CTL_IE |
                             ADC_CTL_END);

    /* Enable the sequencer */
    ADCSequenceEnable(ADC0_BASE, 3);

    /* Clear the interrupt bit for sequencer 3 to make sure it is not set
     * before the first sample is taken */
    ADCIntClear(ADC0_BASE, 3);

    return EXIT_SUCCESS;
}

/**
 * @brief calculate soil moisture
 */
int8_t getMoisture(float *pMoist)
{
    float adcAccum = 0;
    uint32_t tmp = 0;
    uint8_t sample;

    /* get samples to calc average */
    for(sample = 0; sample < SOIL_SAMPLE_COUNT;)
    {
        if(getADC0Value(&tmp) != EXIT_SUCCESS) {
            return EXIT_FAILURE;
        }
        else {
            ++sample;
            adcAccum += ((float)tmp) * SOIL_ADC_SCALE;
        }
    }
    /* calc average */
    *pMoist = adcAccum / ((float)sample);

    return EXIT_SUCCESS;
}

/*
 * @brief go read an ADC sample
 */
int8_t getADC0Value(uint32_t *pValue)
{
    uint32_t pui32ADC0Value = 0;

    /* Take a reading with the ADC */
    ADCProcessorTrigger(ADC0_BASE, 3);

    /* Wait for the ADC to finish taking the sample */
    while(!ADCIntStatus(ADC0_BASE, 3, false))
    {
    }

    /* Clear the interrupt */
    ADCIntClear(ADC0_BASE, 3);

    /* Read the analog voltage measurement */
    ADCSequenceDataGet(ADC0_BASE, 3, &pui32ADC0Value);

    *pValue = pui32ADC0Value;

    return EXIT_SUCCESS;
}

void killMoistureTask(void)
{
    keepAlive = 0;
}
