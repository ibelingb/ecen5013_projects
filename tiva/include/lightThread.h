/***********************************************************************************
 * @author Brian Ibeling and Joshua Malburg
 * Brian.Ibeling@colorado.edu, joshua.malburg@colorad.edu
 * Advanced Embedded Software Development
 * ECEN5013 - Rick Heidebrecht
 * @date April 12, 2019
 * CCS  Version: 8.3.0.00009
 ************************************************************************************
 *
 * @file lightThread.h
 * @brief
 *
 ************************************************************************************
 */

#ifndef LIGHT_THREAD_H_
#define LIGHT_THREAD_H_

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

void lightTask(void *pvParameters);

/*---------------------------------------------------------------------------------*/
#endif /* LIGHT_THREAD_H_ */


