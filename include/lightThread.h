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
  LIGHT_EVENT_EXITING,
  LIGHT_EVENT_END
} LightEvent_e;

/*---------------------------------------------------------------------------------*/

/**
 * @brief 
 *
 * @param threadInfo
 *
 * @return 
 */
void* lightSensorThreadHandler(void* threadInfo);

/**
 * @brief 
 *
 * @param signo
 * @param info
 * @param extra
 */
void lightSigHandler(int signo, siginfo_t *info, void *extra);

/**
 * @brief 
 *
 * @param pAlive
 */
void lightGetAliveFlag(uint8_t *pAlive);

/*---------------------------------------------------------------------------------*/
#endif /* LIGHT_THREAD_H_ */
