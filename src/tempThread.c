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
#include <stdint.h>
#include <string.h>     // for strerror()
#include <unistd.h>
#include <time.h>
#include <errno.h>
#include <signal.h>

#include "tempThread.h"
#include "debug.h"
#include "logger.h"
#include "cmn_timer.h"
#include "tempSensor.h"
#include "lu_iic.h"
#include "packet.h"

#define TEMP_ERR_COUNT_LIMIT  (8)

extern int gExitSig;

/* private helper methods */
uint8_t getData(int fd, TempDataStruct *pData);


/*---------------------------------------------------------------------------------*/
void* tempSensorThreadHandler(void* threadInfo)
{
  /* timer variables */
  timer_t timerid;
  sigset_t set;
  struct timespec timer_interval;
  int signum = SIGALRM;
  TempDataStruct data;
  uint8_t errCount = 0;
  uint8_t overTempState = 0;

  LOG_TEMP_SENSOR_EVENT(TEMP_EVENT_STARTED);

  /* init handle for i2c bus */
	int fd = initIic("/dev/i2c-2");
	if(fd < 0)
	  ERROR_PRINT("ERROR: tempSensorThreadHandler() couldn't init i2c handle, err#%d (%s)\n\r", errno, strerror(errno));

  /* initialize sensor */
  //if(tmp102_initialize(fd) == EXIT_FAILURE)
  //{ ERROR_PRINT("tmp102_initialize failed\n"); errCount++; }
  
  /* report BIST status to main? */

  /** set up timer **/
  /* Clear memory objects */
  memset(&set, 0, sizeof(sigset_t));
  memset(&timerid, 0, sizeof(timer_t));
  memset(&data, 0, sizeof(TempDataStruct));

  timer_interval.tv_nsec = TEMP_LOOP_TIME_NSEC;
  timer_interval.tv_sec = TEMP_LOOP_TIME_SEC;
  setupTimer(&set, &timerid, signum, &timer_interval);

  while(gExitSig)
  {
    LOG_HEARTBEAT();
    INFO_PRINT("temp alive\n");

    /* get data */
    errCount += getData(fd, &data);

    if(errCount > TEMP_ERR_COUNT_LIMIT)
    {
      LOG_TEMP_SENSOR_EVENT(TEMP_EVENT_ERROR);
      errCount = 0;
    }

    /* send log msg on alert edge */
    if((overTempState == 0) && (data.tmp102_alert == TMP102_ALERT_ACTIVE))
    {
      overTempState = 1;
      LOG_TEMP_SENSOR_EVENT(TEMP_EVENT_OVERTEMP);
    }
    if((overTempState == 1) && (data.tmp102_alert == TMP102_ALERT_OFF))
    {
      overTempState = 0;
      LOG_TEMP_SENSOR_EVENT(TEMP_EVENT_OVERTEMP);
    }

    /* write to shared memory */

    /* send status to main */
      
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

uint8_t getData(int fd, TempDataStruct *pData)
{
  uint8_t errCount = 0;

  /* get temp */
  if(tmp102_getTempC(fd, &pData->tmp102_temp) < 0)
  { ERROR_PRINT("tmp102_getTempC failed\n"); errCount++; }
  else { INFO_PRINT("got temp value: %f degC\n", pData->tmp102_temp); }

  /*read threshold value */
  if(EXIT_FAILURE == tmp102_getLowThreshold(fd, &pData->tmp102_lowThreshold))
  { ERROR_PRINT("tmp102_getLowThreshold write failed\n"); errCount++; }
  else { INFO_PRINT("got Tlow value: %f\n", pData->tmp102_lowThreshold); }

  /*read threshold value */
	if(EXIT_FAILURE == tmp102_getHighThreshold(fd, &pData->tmp102_highThreshold))
	{ ERROR_PRINT("tmp102_getHighThreshold write failed\n"); errCount++; }
	else { INFO_PRINT("got Thigh value: %f\n", pData->tmp102_highThreshold); }

  /*read value */
	if(EXIT_FAILURE == tmp102_getFaultQueueSize(fd, &pData->tmp102_fault))
	{ ERROR_PRINT("tmp102_getFaultQueueSize read failed\n"); errCount++; }
	else { INFO_PRINT("got startCount value: %d\n", pData->tmp102_fault); }

  /*read value */
	if(EXIT_FAILURE == tmp102_getExtendedMode(fd, &pData->tmp102_extendedMode))
	{ ERROR_PRINT("tmp102_getExtendedMode read failed\n"); errCount++; }
	else { INFO_PRINT("got startMode value: %d\n", pData->tmp102_extendedMode); }

  /*read value */
	if(EXIT_FAILURE == tmp102_getShutdownState(fd, &pData->tmp102_shutdownMode))
	{ ERROR_PRINT("tmp102_getShutdownState read failed\n"); errCount++; }
	else { INFO_PRINT("got startState value: %d\n", pData->tmp102_shutdownMode); }

  /*read value */
	if(EXIT_FAILURE == tmp102_getConvRate(fd, &pData->tmp102_convRate))
	{ ERROR_PRINT("tmp102_getConvRate read failed\n"); errCount++; }
	else { INFO_PRINT("got startRate value: %d\n", pData->tmp102_convRate); }

  /*read start alert value */
	if(EXIT_FAILURE == tmp102_getAlert(fd, &pData->tmp102_alert))
	{ ERROR_PRINT("tmp102_getAlert read failed\n"); errCount++; }
	else { INFO_PRINT("got startAlert value: %d\n", pData->tmp102_alert); }

  return errCount;
}
/*---------------------------------------------------------------------------------*/
