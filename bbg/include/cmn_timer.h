/***********************************************************************************
 * @author Josh Malburg
 * joshua.malburg@colorado.edu
 * Advanced Embedded Software Development
 * ECEN5013 - Rick Heidebrecht
 * @date March 17, 2019
 * arm-linux-gnueabi (Buildroot)
 * gcc (Ubuntu)
 ************************************************************************************
 *
 * @file cmn_timer.h
 *
 ************************************************************************************
 */

#ifndef CMN_TIMER_H
#define CMN_TIMER_H

#include <stdint.h>
#include <signal.h>

#define TEMP_LOOP_TIME_SEC      (0)
#define TEMP_LOOP_TIME_NSEC     (499e-3 * 1e9)
#define LIGHT_LOOP_TIME_SEC     (TEMP_LOOP_TIME_SEC)
#define LIGHT_LOOP_TIME_NSEC    (TEMP_LOOP_TIME_NSEC)
#define REMOTE_LOOP_TIME_SEC    (TEMP_LOOP_TIME_SEC)
#define REMOTE_LOOP_TIME_NSEC   (TEMP_LOOP_TIME_NSEC)
#define REMOTE_CLIENT_TIMEOUT_USEC (TEMP_LOOP_TIME_NSEC/1000)
#define MAIN_LOOP_TIME_SEC      (TEMP_LOOP_TIME_SEC * 2)
#define MAIN_LOOP_TIME_NSEC     (TEMP_LOOP_TIME_NSEC * 2)
#define LOG_LOOP_TIME_SEC       (0)
#define LOG_LOOP_TIME_NSEC      (5e-3 * 1e9)

#ifndef __linux__
#include "FreeRTOS.h"
#define SOLENOID_TASK_DELAY_SEC         (0.5)
#define LIGHT_TASK_DELAY_SEC            (0.5)
#define REMOTE_TASK_DELAY_SEC           (0.5)
#define OBSERVER_TASK_DELAY_SEC         (0.5)
#define MOISTURE_TASK_DELAY_SEC         (0.5)
#define THREAD_MUTEX_DELAY              (( TickType_t ) 10)
#endif

/**
 * @brief generic setup of posix timer
 * 
 * @param pSet pointer to set struct
 * @param pTimer pointer to timer
 * @param signum signal number
 * @param pRate timing of timer
 * @return int8_t  success of operation
 */
int8_t setupTimer(sigset_t *pSet, timer_t *pTimer, int signum, const struct timespec *pRate);

#endif /* CMN_TIMER_H */
