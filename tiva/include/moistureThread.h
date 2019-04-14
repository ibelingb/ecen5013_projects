/***********************************************************************************
 * @author Joshua Malburg
 * joshua.malburg@colorado.edu
 * Advanced Embedded Software Development
 * ECEN5013 - Rick Heidebrecht
 * @date April 12, 2019
 * CCS  Version: 8.3.0.00009
 ************************************************************************************
 *
 * @file moistureThread.h
 * @brief Moisture Thread
 *
 ************************************************************************************
 */

#ifndef MOISTURETHREAD_H_
#define MOISTURETHREAD_H_

typedef enum {
    MOIST_EVENT_STARTED = 0,
    MOIST_EVENT_BIST_SUCCESS,
    MOIST_EVENT_BIST_FAILED,
    MOIST_EVENT_SENSOR_GPIO_ERROR,
    MOIST_EVENT_STATUS_QUEUE_ERROR,
    MOIST_EVENT_LOG_QUEUE_ERROR,
    MOIST_EVENT_SHMEM_ERROR,
    MOIST_EVENT_EXITING,
    MOIST_EVENT_END
} MoistureEvent_e;

void moistureTask(void *pvParameters);

#endif /* MOISTURETHREAD_H_ */
