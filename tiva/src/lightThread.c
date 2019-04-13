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
#include "queue.h"
#include "task.h"
#include "semphr.h"

#include "packet.h"
#include "my_debug.h"

/*---------------------------------------------------------------------------------*/
#define TMP102_ADDR             (0x48ul)                    /* I2C address of sensor */
#define TMP102_TEMP_REG         (0x00ul)

#define TMP102_TEMP_SCALEFACTOR (0.0625f)                   /* LSB weight of temp and threshold registers */
#define TMP102_TEMP_START_BIT   (4)                         /* offset for temperature data in register */
#define TMP102_BITS_TO_TEMPC(BITS, SHIFT) (((float)((0xFFFF & (BITS)) >> SHIFT)) * TMP102_TEMP_SCALEFACTOR)

#define DEFAULT_POWER_STATE         (APDS9301_CTRL_POWERUP)
#define DEFAULT_TIMING_GAIN         (APDS9301_TIMING_GAIN_LOW)
#define DEFAULT_TIMING_INTEGRATION  (APDS9301_TIMING_INT_101)
#define DEFAULT_INT_SELECT          (APDS9301_INT_SELECT_LEVEL_DISABLE)
#define DEFAULT_INT_PERSIST         (APDS9301_INT_PERSIST_OUTSIDE_CYCLE)
#define DEFAULT_LOW_INT_THRESHOLD   (100)
#define DEFAULT_HIGH_INT_THRESHOLD  (9000)

/*---------------------------------------------------------------------------------*/
int8_t initLightSensor();
int8_t verifyLightSensorComm();
int8_t getTempC(float *pTemp);
/*---------------------------------------------------------------------------------*/
void lightTask(void *pvParameters)
{
    uint8_t errCount = 0, count = 0;
    TaskStatusPacket statusMsg;
    float temperature;
    float luxData = 0.0;

    memset(&statusMsg, 0, sizeof(TaskStatusPacket));
    statusMsg.processId = PID_LIGHT;

    /* get status queue handle */
    SensorThreadInfo info = *((SensorThreadInfo *)pvParameters);

    /* init i2c bus */
    initI2c(info.sysClock);

    /* init sensor */
    if(initLightSensor() == EXIT_FAILURE) {
        // TODO: LOG BIST FAILURE
        ERROR_PRINT("lightThread initialization failed.\n");
    }
    // TODO: LOG BIST SUCCESS

    for (;;) {
        /* update statusMsg */
        statusMsg.header = count++;
        statusMsg.timestamp = (xTaskGetTickCount() - info.xStartTime) * portTICK_PERIOD_MS;

        /* send msg */
        if(xQueueSend(info.statusFd, ( void *)&statusMsg, (TickType_t)10) != pdPASS) {
            ++errCount;
        }

        /* get sensor data; Write data to shared memory */
        if(apds9301_getLuxData(0, &luxData) == EXIT_SUCCESS) {
            /* try to get semaphore */
            if( xSemaphoreTake( info.shmemMutex, THREAD_MUTEX_DELAY ) == pdTRUE )
            {
                /* write data to shmem */
                info.pShmem->lightData.apds9301_luxData = luxData;

                /* release mutex */
                xSemaphoreGive(info.shmemMutex);
            }
        }

        /* sleep */
        vTaskDelay(LIGHT_TASK_DELAY_SEC * configTICK_RATE_HZ);
    }
}


/*---------------------------------------------------------------------------------*/
/**
 * @brief - Initialization method for APDS-9301 Light Sensor.
 *
 * @param sensorFd - Unused; Used by Linux-based API
 * @return EXIT_SUCCESS or EXIT_FAILURE for successful setting device registers.
 */
int8_t initLightSensor()
{
  int failureCount = 0;
  Apds9301_PowerCtrl_e controlRegRead;
  Apds9301_TimingGain_e timingGainRead;
  Apds9301_TimingInt_e timingIntRead;
  Apds9301_IntSelect_e intSelect;
  Apds9301_IntPersist_e intPersist;
  uint16_t lowThresholdRead = 0;
  uint16_t highThresholdRead = 0;

  /* Verify able to communicate with LightSensor */
  if(EXIT_FAILURE == verifyLightSensorComm())
    return EXIT_FAILURE;

  /* Initialize Light Sensor with default settings */
  apds9301_clearInterrupt(0);
  apds9301_setControl(0, DEFAULT_POWER_STATE);
  apds9301_setTimingGain(0, DEFAULT_TIMING_GAIN);
  apds9301_setTimingIntegration(0, DEFAULT_TIMING_INTEGRATION);
  apds9301_setInterruptControl(0, DEFAULT_INT_SELECT, DEFAULT_INT_PERSIST);
  apds9301_setLowIntThreshold(0, DEFAULT_LOW_INT_THRESHOLD);
  apds9301_setHighIntThreshold(0, DEFAULT_HIGH_INT_THRESHOLD);

  /* BIST Test */
  /* Verify initial conditions set were properly loaded */
  apds9301_getControl(0, &controlRegRead);
  apds9301_getTimingGain(0, &timingGainRead);
  apds9301_getTimingIntegration(0, &timingIntRead);
  apds9301_getInterruptControl(0, &intSelect, &intPersist);
  apds9301_getLowIntThreshold(0, &lowThresholdRead);
  apds9301_getHighIntThreshold(0, &highThresholdRead);

  if(controlRegRead != DEFAULT_POWER_STATE)
  {
    ERROR_PRINT("Failed to set APDS9301 control reg power state to 0x%x\n", DEFAULT_POWER_STATE);
    failureCount++;
  }
  if(timingGainRead != DEFAULT_TIMING_GAIN)
  {
    ERROR_PRINT("Failed to set APDS9301 timing reg gain to 0x%x\n", DEFAULT_TIMING_GAIN);
    failureCount++;
  }
  if(timingIntRead != DEFAULT_TIMING_INTEGRATION)
  {
    ERROR_PRINT("Failed to set APDS9301 timing reg integration to 0x%x\n", DEFAULT_TIMING_INTEGRATION);
    failureCount++;
  }
  if(intSelect != DEFAULT_INT_SELECT)
  {
    ERROR_PRINT("Failed to set APDS9301 Interrupt Control reg Select to 0x%x\n", DEFAULT_INT_SELECT);
    failureCount++;
  }
  if(intPersist != DEFAULT_INT_PERSIST)
  {
    ERROR_PRINT("Failed to set APDS9301 Interrupt Control reg Persist to 0x%x\n", DEFAULT_INT_PERSIST);
    failureCount++;
  }
  if(lowThresholdRead != DEFAULT_LOW_INT_THRESHOLD)
  {
    ERROR_PRINT("Failed to set APDS9301 Interrupt Low Threshold reg to 0x%x\n", DEFAULT_LOW_INT_THRESHOLD);
    failureCount++;
  }
  if(highThresholdRead != DEFAULT_HIGH_INT_THRESHOLD)
  {
    ERROR_PRINT("Failed to set APDS9301 Interrupt High Threshold reg to 0x%x\n", DEFAULT_HIGH_INT_THRESHOLD);
    failureCount++;
  }

  if(failureCount == 7)
  {
    ERROR_PRINT("Failed to set all default settings for APDS9301 - returning failure.\n");
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}

/*---------------------------------------------------------------------------------*/
/**
 * @brief - Method to check that communication with ADPS-9301 device is successful
 *          over I2C by reading Device Part Number.
 *
 * @param sensorFd - FD for APDS-9301 sensor device.
 * @return EXIT_SUCCESS or EXIT_FAILURE for successful comm with device.
 */
int8_t verifyLightSensorComm()
{
  uint8_t devicePartNo, deviceRevNo = 0;

  /* Verify able to communicate with device - check able to read device Part Number */
  apds9301_getDeviceId(0, &devicePartNo, &deviceRevNo);
  if(devicePartNo != APDS9301_PARTNO)
  {
    ERROR_PRINT("lightThread failed to read APDS9301 Part Number properly | lightThread -> lightSensor comm failed.\n");
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}

/*---------------------------------------------------------------------------------*/
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