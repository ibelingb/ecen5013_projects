/***********************************************************************************
 * @author Joshua Malburg
 * joshua.malburg@colorado.edu
 * Advanced Embedded Software Development
 * ECEN5013 - Rick Heidebrecht
 * @date March 8, 2019
 * arm-linux-gnueabi (Buildroot)
 * gcc (Ubuntu)
 ************************************************************************************
 *
 * @file tempThread.h
 * @brief Temp sensor thread
 *
 ************************************************************************************
 */

#ifndef TEMP_THREAD_H_
#define TEMP_THREAD_H_

#include <stdint.h>
#include "tempSensor.h"

typedef enum {
    TEMP_EVENT_STARTED = 0,
    TEMP_EVENT_OVERTEMP,
    TEMP_EVENT_OVERTEMP_RELEQUISHED,
    TEMP_EVENT_SENSOR_INIT_ERROR,
    TEMP_EVENT_SENSOR_READ_ERROR,
    TEMP_EVENT_STATUS_QUEUE_ERROR,
    TEMP_EVENT_LOG_QUEUE_ERROR,
    TEMP_EVENT_SHMEM_ERROR,
    TEMP_EVENT_I2C_ERROR,
    TEMP_EVENT_EXITING,
    TEMP_EVENT_END
} TempEvent_e;

/*---------------------------------------------------------------------------------*/
void* tempSensorThreadHandler(void* threadInfo);
void tempSigHandler(int signo, siginfo_t *info, void *extra);
void tempGetAliveFlag(uint8_t *pAlive);

/*---------------------------------------------------------------------------------*/
#endif /* TEMP_THREAD_H_ */
