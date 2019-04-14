/***********************************************************************************
 * @author Brian Ibeling and Joshua Malburg
 * Brian.Ibeling@colorado.edu, joshua.malburg@colorad.edu
 * Advanced Embedded Software Development
 * ECEN5013 - Rick Heidebrecht
 * @date April 12, 2019
 * CCS  Version: 8.3.0.00009
 ************************************************************************************
 *
 * @file solenoidThread.h
 * @brief
 *
 ************************************************************************************
 */

#ifndef SOLENOIDTHREAD_H_
#define SOLENOIDTHREAD_H_

typedef enum {
    SOLE_EVENT_STARTED = 0,
    SOLE_EVENT_BIST_SUCCESS,
    SOLE_EVENT_BIST_FAILED,
    SOLE_EVENT_SENSOR_GPIO_ERROR,
    SOLE_EVENT_STATUS_QUEUE_ERROR,
    SOLE_EVENT_LOG_QUEUE_ERROR,
    SOLE_EVENT_SHMEM_ERROR,
    SOLE_EVENT_EXITING,
    SOLE_EVENT_END
} SolenoidEvent_e;

void solenoidTask(void *pvParameters);
void killSolenoidTask(void);

#endif /* SOLENOIDTHREAD_H_ */
