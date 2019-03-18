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
#define MAIN_LOOP_TIME_SEC      (1)
#define MAIN_LOOP_TIME_NSEC     (0)
#define LOG_LOOP_TIME_SEC       (0)
#define LOG_LOOP_TIME_NSEC      (49e-3 * 1e9)

int8_t setupTimer(sigset_t *pSet, timer_t *pTimer, int signum, const struct timespec *pRate);

#endif /* CMN_TIMER_H */