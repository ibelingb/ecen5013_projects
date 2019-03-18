/***********************************************************************************
 * @author Joshua Malburg
 * joshua.malburg@colorado.edu
 * Advanced Embedded Software Development
 * ECEN5013 - Rick Heidebrecht
 * @date March 17, 2019
 * arm-linux-gnueabi (Buildroot)
 * gcc (Ubuntu)
 ************************************************************************************
 *
 * @file main.c
 * @brief temp thread
 *
 ************************************************************************************
 */
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>     // for strerror()
#include <stddef.h>
#include <signal.h>
#include <unistd.h>
#include <errno.h>
#include <sys/syscall.h>
#include <pthread.h>

#include "tempThread.h"
#include "debug.h"
#include "logger.h"
#include "cmn_timer.h"

extern int gExitSig;


/*---------------------------------------------------------------------------------*/
void* tempSensorThreadHandler(void* threadInfo)
{
  /* timer variables */
  timer_t timerid;
  sigset_t set;
  struct timespec timer_interval;
  int signum = SIGALRM;

  LOG_TEMP_SENSOR_EVENT(TEMP_EVENT_STARTED);

  /* Clear memory objects */
  memset(&set, 0, sizeof(sigset_t));
  memset(&timerid, 0, sizeof(timer_t));

  timer_interval.tv_nsec = TEMP_LOOP_TIME_NSEC;
  timer_interval.tv_sec = TEMP_LOOP_TIME_SEC;
  setupTimer(&set, &timerid, signum, &timer_interval);

  while(gExitSig)
  {
    LOG_HEARTBEAT();
    INFO_PRINT("temp alive\n");
      
    /* wait on signal timer */
    sigwait(&set, &signum);
  }

  LOG_TEMP_SENSOR_EVENT(TEMP_EVENT_EXITING);
  INFO_PRINT("temp thread exiting\n");
  timer_delete(timerid);
  return NULL;
}

/*---------------------------------------------------------------------------------*/
/* HELPER METHODS */

/*---------------------------------------------------------------------------------*/
