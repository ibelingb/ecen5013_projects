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
 * @file cmn_timer.c
 *
 ************************************************************************************
 */

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>     // for memset
#include <sys/syscall.h>
#include <time.h>
#include <signal.h>
#include <unistd.h>

int8_t setupTimer(sigset_t *pSet, timer_t *pTimer, int signum, const struct timespec *pRate)
{
  struct sigevent timer_sigevent;
  struct itimerspec trig;

  /* clear mem objects */
  memset(&timer_sigevent, 0, sizeof(struct sigevent));
  memset(&trig, 0, sizeof(struct itimerspec));

  /* create and initiate timer */
  /* Set the notification method as SIGEV_THREAD_ID:
    * Upon timer expiration, only this thread gets notified */
  timer_sigevent.sigev_notify = SIGEV_THREAD_ID;
  timer_sigevent._sigev_un._tid = (pid_t)syscall(SYS_gettid);
  timer_sigevent.sigev_signo = SIGALRM;

  sigemptyset(pSet);
  sigaddset(pSet, signum);
  sigprocmask(SIG_BLOCK, pSet, NULL);

  /* Create timer */
  timer_create(CLOCK_REALTIME, &timer_sigevent, pTimer);

  /* Set expiration and interval time */
  trig.it_value.tv_nsec = pRate->tv_nsec;
  trig.it_value.tv_sec = pRate->tv_sec;
  trig.it_interval.tv_nsec = pRate->tv_nsec;
  trig.it_interval.tv_sec = pRate->tv_sec;

  /* Arm / start the timer */
  timer_settime(*pTimer, 0, &trig, NULL);

  return EXIT_SUCCESS;
}