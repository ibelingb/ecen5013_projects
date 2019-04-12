/***********************************************************************************
 * @author Brian Ibeling
 * brian.ibeling@colorado.edu
 * Advanced Embedded Software Development
 * ECEN5013 - Rick Heidebrecht
 * @date April 12, 2019
 * CCS  Version: 8.3.0.00009
 ************************************************************************************
 *
 * @file lightThread.c
 * @brief Light Thread
 *
 ************************************************************************************
 */

/* stdlib includes */
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

/* app specific includes */
#include "tiva_i2c.h"
#include "cmn_timer.h"


/* FreeRTOS includes */
#include "FreeRTOS.h"
#include <queue.h>
#include <task.h>
#include "semphr.h"

#include "packet.h"

#define TMP102_ADDR             (0x48ul)                    /* I2C address of sensor */
#define TMP102_TEMP_REG         (0x00ul)

#define TMP102_TEMP_SCALEFACTOR (0.0625f)                   /* LSB weight of temp and threshold registers */
#define TMP102_TEMP_START_BIT   (4)                         /* offset for temperature data in register */
#define TMP102_BITS_TO_TEMPC(BITS, SHIFT) (((float)((0xFFFF & (BITS)) >> SHIFT)) * TMP102_TEMP_SCALEFACTOR)


int8_t getTempC(float *pTemp);

void lightTask(void *pvParameters)
{
    uint8_t errCount = 0, count = 0;
    TaskStatusPacket statusMsg;
    float temperature;


    memset(&statusMsg, 0, sizeof(TaskStatusPacket));
    statusMsg.processId = PID_LIGHT;


    /* get status queue handle */
    SensorThreadInfo info = *((SensorThreadInfo *)pvParameters);

    /* init i2c bus */
    initI2c(info.sysClock);

    /* init sensor */
    /* TODO */

    for (;;) {
        /* update statusMsg */
        statusMsg.header = count++;
        statusMsg.timestamp = (xTaskGetTickCount() - info.xStartTime) * portTICK_PERIOD_MS;

        /* send msg */
        if(xQueueSend(info.statusFd, ( void *)&statusMsg, (TickType_t)10) != pdPASS) {
            ++errCount;
        }

        /* get sensor data */
        if(getTempC(&temperature) != EXIT_SUCCESS) {
            temperature = -100.0f;
        }

        /* try to get semaphore */
        if( xSemaphoreTake( info.shmemMutex, THREAD_MUTEX_DELAY ) == pdTRUE )
        {
            /* write data to shmem */
            info.pShmem->lightData.apds9301_luxData = temperature;

            /* release mutex */
            xSemaphoreGive(info.shmemMutex);
        }

        /* sleep */
        vTaskDelay(LIGHT_TASK_DELAY_SEC * configTICK_RATE_HZ);
    }
}

int8_t getTempC(float *pTemp)
{
    uint8_t data[2];
    uint16_t tmp;

    /* validate inputs */
    if(pTemp == NULL)
        return EXIT_FAILURE;

    /* get register value */
    recvIic2Bytes(TMP102_ADDR, TMP102_TEMP_REG, &data[0]);
    tmp = data[1] + (data[0] << 8);

    /* if extended address mode shift one less bit */
    uint8_t shiftValue = TMP102_TEMP_START_BIT;
    float maxTemp = 128.0f;

    /* test for negative value */
    float tmpFloat;
    if((tmp >> 15) != 0) {
        /* convert bits to Degrees C */
        tmpFloat = (maxTemp * 2.0f) - TMP102_BITS_TO_TEMPC(tmp, shiftValue);
    }
    else {
        /* convert bits to Degrees C */
        tmpFloat = TMP102_BITS_TO_TEMPC(tmp, shiftValue);
    }
    *pTemp = tmpFloat;
    return EXIT_SUCCESS;
}
