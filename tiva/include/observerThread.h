/***********************************************************************************
 * @author Brian Ibeling and Joshua Malburg
 * Brian.Ibeling@colorado.edu, joshua.malburg@colorad.edu
 * Advanced Embedded Software Development
 * ECEN5013 - Rick Heidebrecht
 * @date April 12, 2019
 * CCS  Version: 8.3.0.00009
 ************************************************************************************
 *
 * @file observerThread.h
 * @brief
 *
 ************************************************************************************
 */

#ifndef OBSERVERTHREAD_H_
#define OBSERVERTHREAD_H_

typedef enum {
    OBSERVE_EVENT_STARTED = 0,
    OBSERVE_EVENT_BIST_SUCCESS,
    OBSERVE_EVENT_BIST_FAILED,
    OBSERVE_EVENT_GPIO_ERROR,
    OBSERVE_EVENT_STATUS_QUEUE_ERROR,
    OBSERVE_EVENT_LOG_QUEUE_ERROR,
    OBSERVE_EVENT_SHMEM_ERROR,
    OBSERVE_EVENT_CMD_OVERRIDE_ASSERTED,
    OBSERVE_EVENT_EXITING,
    OBSERVE_EVENT_END
} ObserverEvent_e;

void observerTask(void *pvParameters);
void killObserverTask(void);

#endif /* OBSERVERTHREAD_H_ */
