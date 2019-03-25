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
#include <sys/shm.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <fcntl.h>    //for O_RDWR

#include "tempThread.h"
#include "debug.h"
#include "logger.h"
#include "cmn_timer.h"
#include "tempSensor.h"
#include "lu_iic.h"
#include "packet.h"
#include "platform.h"
#include "healthMonitor.h"

#define TEMP_ERR_COUNT_LIMIT  (8)
#define INIT_TEMP_AVG_COUNT   (8)
#define INIT_THRESHOLD_PAD    (2)

static uint8_t aliveFlag = 1;

/* private helper methods */
uint8_t getData(int fd, TempDataStruct *pData);
int8_t initSensor(int fd);

/*---------------------------------------------------------------------------------*/
void tempSigHandler(int signo, siginfo_t *info, void *extra)
{
	INFO_PRINT("tempSigHandler, signum: %d\n",info->si_signo);
    aliveFlag = 0;
}

void tempGetAliveFlag(uint8_t *pAlive)
{
  if(pAlive != NULL)
    *pAlive = aliveFlag;
}

void* tempSensorThreadHandler(void* threadInfo)
{
  SensorThreadInfo sensorInfo = *(SensorThreadInfo *)threadInfo;
  void* sharedMemPtr = NULL;
  int sharedMemFd;
  mqd_t hbMsgQueue;  /* main status MessageQueue */

  /* timer variables */
  timer_t timerid;
  sigset_t set;
  struct timespec timer_interval;
  int signum = SIGALRM;
  TempDataStruct data;
  uint8_t errCount = 0;
  uint8_t overTempState = 0;
  uint8_t ind;
	sigset_t mask;

  LOG_TEMP_SENSOR_EVENT(TEMP_EVENT_STARTED);

  /* block SIGRTs signals */
	sigemptyset(&mask);

    for(ind = 0; ind < NUM_THREADS; ++ind)
    {
        if(ind != (uint8_t)PID_TEMP)
            sigaddset(&mask, SIGRTMIN + ind);
    }
    pthread_sigmask(SIG_BLOCK, &mask, NULL);


  /* init handle for i2c bus */
	int fd = initIic("/dev/i2c-2");
	if(fd < 0)
	  ERROR_PRINT("ERROR: tempSensorThreadHandler() couldn't init i2c handle, err#%d (%s)\n\r", errno, strerror(errno));
  
  /* Setup Shared memory for thread */
  sharedMemFd = shm_open(sensorInfo.sensorSharedMemoryName, O_RDWR, 0666);
  if(sharedMemFd == -1) {
    printf("ERROR: lightSensorThread Failed to Open heartbeat MessageQueue - exiting.\n");
    return NULL;
  }

  /* Memory map the shared memory */
  sharedMemPtr = mmap(0, sensorInfo.sharedMemSize, PROT_READ | PROT_WRITE, MAP_SHARED, sharedMemFd, 0);
  if(*(int *)sharedMemPtr == -1) {
    printf("ERROR: lightSensorThread Failed to complete memory mapping of shared memory - exiting\n");
    return NULL;
  }

  /* main status msg queue */
  hbMsgQueue = mq_open(sensorInfo.heartbeatMsgQueueName, O_RDWR, 0666, NULL);
  if(hbMsgQueue == -1) {
    printf("ERROR: remoteThread Failed to Open heartbeat MessageQueue - exiting.\n");
    LOG_TEMP_SENSOR_EVENT(TEMP_EVENT_ERROR);
    return NULL;
  }
  
  /* initialize sensor */
  if(initSensor(fd) == EXIT_FAILURE)
  { ERROR_PRINT("temp initSensor failed\n"); errCount++; }

  /** set up timer **/
  /* Clear memory objects */
  memset(&set, 0, sizeof(sigset_t));
  memset(&timerid, 0, sizeof(timer_t));
  memset(&data, 0, sizeof(TempDataStruct));

  timer_interval.tv_nsec = TEMP_LOOP_TIME_NSEC;
  timer_interval.tv_sec = TEMP_LOOP_TIME_SEC;
  setupTimer(&set, &timerid, signum, &timer_interval);

  while(aliveFlag) 
  {
    LOG_HEARTBEAT();
    MUTED_PRINT("temp alive\n");

    /* get data */
    pthread_mutex_lock(sensorInfo.i2cBusMutex);
    errCount += getData(fd, &data);
    pthread_mutex_unlock(sensorInfo.i2cBusMutex);

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
    pthread_mutex_lock(sensorInfo.sharedMemMutex);
    memcpy(sharedMemPtr+(sensorInfo.tempDataOffset), &data, sizeof(TempDataStruct));
    pthread_mutex_unlock(sensorInfo.sharedMemMutex);

    /* TODO - derive method to set status sent to main */
    SEND_STATUS_MSG(hbMsgQueue, PID_TEMP, STATUS_ERROR, ERROR_CODE_USER_NONE0);
      
    /* wait on signal timer */
    sigwait(&set, &signum);
  }

  LOG_TEMP_SENSOR_EVENT(TEMP_EVENT_EXITING);
  INFO_PRINT("temp thread exiting\n");
  timer_delete(timerid);
  close(sharedMemFd);
  return NULL;
}

/*---------------------------------------------------------------------------------*/
/* HELPER METHODS */

int8_t initSensor(int fd)
{
   TempDataStruct tmp, initValue;
   float tempAccum = 0;
   uint8_t accumCount;

   initValue.tmp102_shutdownMode  = TMP102_DEVICE_IN_NORMAL;
   initValue.tmp102_extendedMode  = TMP102_ADDR_MODE_NORMAL;
   initValue.tmp102_fault         = TMP102_REQ_FOUR_FAULT;
   initValue.tmp102_convRate      = TMP102_CONV_RATE_4HZ;

  /*** address mode ***/
  /* set address mode to normal */
	if(EXIT_FAILURE == tmp102_setExtendedMode(fd, initValue.tmp102_extendedMode))
	{ ERROR_PRINT("init (address mode) failed\n"); return EXIT_FAILURE; }

	/* read it back */
	if(EXIT_FAILURE == tmp102_getExtendedMode(fd, &tmp.tmp102_extendedMode))
	{ ERROR_PRINT("init (address mode) failed\n"); return EXIT_FAILURE; }

	/* verify read back value match write value */
	if(tmp.tmp102_extendedMode != initValue.tmp102_extendedMode)
	{ ERROR_PRINT("init (address mode) failed\n"); return EXIT_FAILURE; }

  /*** fault queue size ***/
	/* set queue depth */
	if(EXIT_FAILURE == tmp102_setFaultQueueSize(fd, initValue.tmp102_fault))
	{ ERROR_PRINT("init (fault queue) failed\n"); return EXIT_FAILURE; }

	/* read it back */
	if(EXIT_FAILURE == tmp102_getFaultQueueSize(fd, &tmp.tmp102_fault))
	{ ERROR_PRINT("init (fault queue) failed\n"); return EXIT_FAILURE; }

	/* verify read back value match write value */
	if(tmp.tmp102_fault != initValue.tmp102_fault)
	{ ERROR_PRINT("init (fault queue) failed\n"); return EXIT_FAILURE; }

  /*** conversion rate ***/
  	/* write new value */
	if(EXIT_FAILURE == tmp102_setConvRate(fd, initValue.tmp102_convRate))
	{ ERROR_PRINT("init (conv rate) failed\n"); return EXIT_FAILURE; }

	/* read it back */
	if(EXIT_FAILURE == tmp102_getConvRate(fd, &tmp.tmp102_convRate))
	{ ERROR_PRINT("init (conv rate) failed\n"); return EXIT_FAILURE; }

	/* verify read back value match write value */
	if(tmp.tmp102_convRate != initValue.tmp102_convRate)
	{ ERROR_PRINT("init (conv rate) failed\n"); return EXIT_FAILURE; }

  /*** shutdown state ***/
	/* take out of shutdown mode */
	if(EXIT_FAILURE == tmp102_setShutdownState(fd, initValue.tmp102_shutdownMode))
	{ ERROR_PRINT("init (shutdown state) failed\n"); return EXIT_FAILURE; }

	/* read it back */
	if(EXIT_FAILURE == tmp102_getShutdownState(fd, &tmp.tmp102_shutdownMode))
	{ ERROR_PRINT("init (shutdown state) failed\n"); return EXIT_FAILURE; }

  /* verify it matches set value */
  if(tmp.tmp102_shutdownMode != initValue.tmp102_shutdownMode)
  { ERROR_PRINT("init (shutdown state) failed\n"); return EXIT_FAILURE; }

  MUTED_PRINT("temp sensor init delay...");
  sleep(1);
  MUTED_PRINT("done\n");

  /* find average temp */
  for(accumCount = 0; accumCount < INIT_TEMP_AVG_COUNT; ++accumCount)
  {
    if(tmp102_getTempC(fd, &tmp.tmp102_temp) < 0)
		{ ERROR_PRINT("tmp102_getTempC failed\n"); return EXIT_FAILURE; }
    tempAccum += tmp.tmp102_temp;
    MUTED_PRINT("tempC: %f\n",tmp.tmp102_temp);
  }

  /* calc average and set init threshold values */
  tempAccum /= accumCount;
  MUTED_PRINT("average tempC: %f\n", tempAccum);

  initValue.tmp102_highThreshold = tempAccum + INIT_THRESHOLD_PAD;
  initValue.tmp102_lowThreshold = tempAccum + (INIT_THRESHOLD_PAD / 2);

  /*** set low threshold ***/
  /* set value */
  if(EXIT_FAILURE == tmp102_setLowThreshold(fd, initValue.tmp102_lowThreshold))
	{ ERROR_PRINT("test_LowThreshold read failed\n"); return EXIT_FAILURE; }

	/* read it back */
	if(EXIT_FAILURE == tmp102_getLowThreshold(fd, &tmp.tmp102_lowThreshold))
	{ ERROR_PRINT("test_LowThreshold write failed\n"); return EXIT_FAILURE; }

  /* verify it matches set value */
	if(tmp.tmp102_lowThreshold != initValue.tmp102_lowThreshold)
	{ ERROR_PRINT("test_LowThreshold read doesn't match written\n"); return EXIT_FAILURE; }
  
  /*** set high threshold ***/
  /* set value */
	if(EXIT_FAILURE == tmp102_setHighThreshold(fd, initValue.tmp102_highThreshold))
	{ ERROR_PRINT("test_HighThreshold read failed\n"); return EXIT_FAILURE; }

	/* read it back */
	if(EXIT_FAILURE == tmp102_getHighThreshold(fd, &tmp.tmp102_highThreshold))
	{ ERROR_PRINT("test_HighThreshold write failed\n"); return EXIT_FAILURE; }
 
  /* verify it matches set value */
	if(tmp.tmp102_highThreshold != initValue.tmp102_highThreshold)
	{ ERROR_PRINT("test_HighThreshold read doesn't match written\n"); return EXIT_FAILURE; }
 
  return EXIT_SUCCESS;
}

uint8_t getData(int fd, TempDataStruct *pData)
{
  uint8_t errCount = 0;
  TempDataStruct tmp;
  /* get temp */
  if(tmp102_getTempC(fd, &tmp.tmp102_temp) < 0)
  { ERROR_PRINT("tmp102_getTempC failed\n"); errCount++; }
  else { MUTED_PRINT("got temp value: %f degC\n", tmp.tmp102_temp); }

  /*read threshold value */
  if(EXIT_FAILURE == tmp102_getLowThreshold(fd, &tmp.tmp102_lowThreshold))
  { ERROR_PRINT("tmp102_getLowThreshold write failed\n"); errCount++; }
  else { MUTED_PRINT("got Tlow value: %f\n", tmp.tmp102_lowThreshold); }

  /*read threshold value */
	if(EXIT_FAILURE == tmp102_getHighThreshold(fd, &tmp.tmp102_highThreshold))
	{ ERROR_PRINT("tmp102_getHighThreshold write failed\n"); errCount++; }
	else { MUTED_PRINT("got Thigh value: %f\n", tmp.tmp102_highThreshold); }

  /*read value */
	if(EXIT_FAILURE == tmp102_getFaultQueueSize(fd, &tmp.tmp102_fault))
	{ ERROR_PRINT("tmp102_getFaultQueueSize read failed\n"); errCount++; }
	else { MUTED_PRINT("got startCount value: %d\n", tmp.tmp102_fault); }

  /*read value */
	if(EXIT_FAILURE == tmp102_getExtendedMode(fd, &tmp.tmp102_extendedMode))
	{ ERROR_PRINT("tmp102_getExtendedMode read failed\n"); errCount++; }
	else { MUTED_PRINT("got startMode value: %d\n", tmp.tmp102_extendedMode); }

  /*read value */
	if(EXIT_FAILURE == tmp102_getShutdownState(fd, &tmp.tmp102_shutdownMode))
	{ ERROR_PRINT("tmp102_getShutdownState read failed\n"); errCount++; }
	else { MUTED_PRINT("got startState value: %d\n", tmp.tmp102_shutdownMode); }

  /*read value */
	if(EXIT_FAILURE == tmp102_getConvRate(fd, &tmp.tmp102_convRate))
	{ ERROR_PRINT("tmp102_getConvRate read failed\n"); errCount++; }
	else { MUTED_PRINT("got startRate value: %d\n", tmp.tmp102_convRate); }

  /*read start alert value */
	if(EXIT_FAILURE == tmp102_getAlert(fd, &tmp.tmp102_alert))
	{ ERROR_PRINT("tmp102_getAlert read failed\n"); errCount++; }
	else { MUTED_PRINT("got startAlert value: %d\n", tmp.tmp102_alert); }

  *pData = tmp;
  return errCount;
}
/*---------------------------------------------------------------------------------*/
