/***********************************************************************************
 * @author Brian Ibeling
 * Brian.Ibeling@colorado.edu
 * Advanced Embedded Software Development
 * ECEN5013 - Rick Heidebrecht
 * @date March 8, 2019
 * arm-linux-gnueabi (Buildroot)
 * gcc (Ubuntu)
 ************************************************************************************
 *
 * @file lightThread.h
 * @brief Light sensor thread for APDS-9301 device
 *
 * https://www.sparkfun.com/products/14350
 *
 ************************************************************************************
 */

#ifndef LIGHT_THREAD_H_
#define LIGHT_THREAD_H_

#include <stdint.h>
#include <signal.h>
#include "lightSensor.h"
#include "packet.h"

typedef enum {
  LIGHT_EVENT_STARTED = 0,
  LIGHT_EVENT_DAY,
  LIGHT_EVENT_NIGHT,
  LIGHT_EVENT_SENSOR_INIT_ERROR,
  LIGHT_EVENT_SENSOR_READ_ERROR,
  LIGHT_EVENT_STATUS_QUEUE_ERROR,
  LIGHT_EVENT_LOG_QUEUE_ERROR,
  LIGHT_EVENT_SHMEM_ERROR,
  LIGHT_EVENT_I2C_ERROR,
  LIGHT_EVENT_BIST_COMPLETE,
  LIGHT_EVENT_SENSOR_INIT_SUCCESS,
  LIGHT_EVENT_EXITING,
  LIGHT_EVENT_END
} LightEvent_e;

/*---------------------------------------------------------------------------------*/

/**
 * @brief - APDS-9301 Light Sensor Thread used to periodically read sensor data and write
 *          to Sensor Shared Memory.
 *
 * @param threadInfo - Struct containing resources allocated and filenames established by main()
 *
 * @return void*
 */
void* lightSensorThreadHandler(void* threadInfo);

/**
 * @brief - Method for handling signals sent to lightSensor Child Thread
 *
 * @param signo - Signal number
 * @param info - Struct used to pass data through to Signal Handler.
 * @param extra - Additional params if needed by sigAction struct when setting sa_sigaction
 * @return void
 */
void lightSigHandler(int signo, siginfo_t *info, void *extra);

/**
 * @brief Get method to receive lightThread's alive status.
 *
 * @param pAlive - Pointer to return alive status back to caller function to.
 * @return void
 */
void lightGetAliveFlag(uint8_t *pAlive);

/*---------------------------------------------------------------------------------*/
#endif /* LIGHT_THREAD_H_ */
