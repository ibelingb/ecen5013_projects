/***********************************************************************************
 * @author Brian Ibeling
 * brian.ibeling@colorado.edu
 * Advanced Embedded Software Development
 * ECEN5013 - Rick Heidebrecht
 * @date March 14, 2019
 * arm-linux-gnueabi (Buildroot)
 * gcc (Ubuntu)
 ************************************************************************************
 *
 * @file lightSensor.c
 * @brief Light Thread Library
 *
 ************************************************************************************
 */

#include <stdint.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <time.h>
#include <mqueue.h>
#include <sys/shm.h>
#include <sys/mman.h>

#include "lightThread.h"
#include "lightSensor.h"
#include "lu_iic.h"
#include "logger.h"
#include "debug.h"
#include "cmn_timer.h"
#include "packet.h"
#include "platform.h"
#include "healthMonitor.h"

#define DEFAULT_POWER_STATE         (APDS9301_CTRL_POWERUP)
#define DEFAULT_TIMING_GAIN         (APDS9301_TIMING_GAIN_LOW)
#define DEFAULT_TIMING_INTEGRATION  (APDS9301_TIMING_INT_101)
#define DEFAULT_INT_SELECT          (APDS9301_INT_SELECT_LEVEL_DISABLE)
#define DEFAULT_INT_PERSIST         (APDS9301_INT_PERSIST_OUTSIDE_CYCLE)
#define DEFAULT_LOW_INT_THRESHOLD   (100)
#define DEFAULT_HIGH_INT_THRESHOLD  (9000)

/* Prototypes for private/helper functions */
int8_t getLightSensorData(int sensorFd, LightDataStruct *lightData);
int8_t initLightSensor(int sensorFd);
int8_t verifyLightSensorComm(int sensorFd);

/* Define static and global variables */
static uint8_t aliveFlag = 1;

/*---------------------------------------------------------------------------------*/
void* lightSensorThreadHandler(void* threadInfo)
{
  SensorThreadInfo sensorInfo = *(SensorThreadInfo *)threadInfo;
  LightDataStruct lightSensorData = {0};
  LightState_e lightState = LUX_STATE_DARK;
  int sensorFd;
  void* sharedMemPtr = NULL;
  int sharedMemFd;
  mqd_t hbMsgQueue;  /* main status MessageQueue */
  int8_t status;

  /* timer variables */
  timer_t timerid;
  sigset_t set;
  struct timespec timer_interval;
  int signum = SIGALRM;
  uint8_t ind;
  sigset_t mask;

  LOG_LIGHT_SENSOR_EVENT(LIGHT_EVENT_STARTED);

  /* block SIGRTs signals */
  sigemptyset(&mask);
  for(ind = 0; ind < NUM_THREADS; ++ind)
  {
    if(ind != (uint8_t)PID_LIGHT)
      sigaddset(&mask, SIGRTMIN + ind);
  }
  pthread_sigmask(SIG_BLOCK, &mask, NULL);

  /* Setup Timer */
  memset(&set, 0, sizeof(sigset_t));
  memset(&timerid, 0, sizeof(timer_t));
  timer_interval.tv_nsec = LIGHT_LOOP_TIME_NSEC;
  timer_interval.tv_sec = LIGHT_LOOP_TIME_SEC;
  setupTimer(&set, &timerid, signum, &timer_interval);

  /* main status msg queue */
  hbMsgQueue = mq_open(sensorInfo.heartbeatMsgQueueName, O_RDWR, 0666, NULL);
  if(hbMsgQueue == -1) {
    LOG_LIGHT_SENSOR_EVENT(LIGHT_EVENT_STATUS_QUEUE_ERROR);
    ERROR_PRINT("lightThread Failed to Open heartbeat MessageQueue - exiting.\n");
    return NULL;
  }

  /* Setup Shared memory for thread */
  sharedMemFd = shm_open(sensorInfo.sensorSharedMemoryName, O_RDWR, 0666);
  if(sharedMemFd == -1) {
    SEND_STATUS_MSG(hbMsgQueue, PID_LIGHT, STATUS_ERROR, ERROR_CODE_USER_NOTIFY0);
    LOG_LIGHT_SENSOR_EVENT(LIGHT_EVENT_SHMEM_ERROR);    
    ERROR_PRINT("lightSensorThread Failed to Open heartbeat MessageQueue - exiting.\n");
    return NULL;
  }

  /* Memory map the shared memory */
  sharedMemPtr = mmap(0, sensorInfo.sharedMemSize, PROT_READ | PROT_WRITE, MAP_SHARED, sharedMemFd, 0);
  if(*(int *)sharedMemPtr == -1) {
    SEND_STATUS_MSG(hbMsgQueue, PID_LIGHT, STATUS_ERROR, ERROR_CODE_USER_NOTIFY0);
    LOG_LIGHT_SENSOR_EVENT(LIGHT_EVENT_SHMEM_ERROR);    
    ERROR_PRINT("lightSensorThread Failed to complete memory mapping of shared memory - exiting\n");
    return NULL;
  }

  /* Initialize I2C bus and set static mutex variable */
  sensorFd = initIic("/dev/i2c-2");
  if(sensorFd < 0) {
    SEND_STATUS_MSG(hbMsgQueue, PID_LIGHT, STATUS_ERROR, ERROR_CODE_USER_NOTIFY0);
    LOG_LIGHT_SENSOR_EVENT(LIGHT_EVENT_I2C_ERROR);    
    ERROR_PRINT("lightSensorThread failed to initialize i2c Bus.\n");
  }

  /* Set initial states for LightSensor */
  if(initLightSensor(sensorFd) == EXIT_FAILURE) {
    SEND_STATUS_MSG(hbMsgQueue, PID_LIGHT, STATUS_ERROR, ERROR_CODE_USER_NOTIFY0);
    LOG_LIGHT_SENSOR_EVENT(LIGHT_EVENT_SENSOR_INIT_ERROR);
    ERROR_PRINT("lightThread initialization failed.\n");
  }

  /* Log SensorThread successfully created */
  LOG_INFO("Successfully created lightSensorThread.\n");
  MUTED_PRINT("lightThread started successfully, pid: %d, SIGRTMIN+PID_e: %d\n",(pid_t)syscall(SYS_gettid), SIGRTMIN + PID_LIGHT);

  /* Setup timer to periodically sample from Light Sensor */
  while(aliveFlag) {
    /* Capture light sensor data from device */
    pthread_mutex_lock(sensorInfo.i2cBusMutex);
    status = getLightSensorData(sensorFd, &lightSensorData);
    pthread_mutex_unlock(sensorInfo.i2cBusMutex);

    if(status == EXIT_SUCCESS)
    {
      /* Write to shared memory */
      pthread_mutex_lock(sensorInfo.sharedMemMutex);
      memcpy(sharedMemPtr+(sensorInfo.lightDataOffset), &lightSensorData, sizeof(LightDataStruct));
      pthread_mutex_unlock(sensorInfo.sharedMemMutex);

      /* Set LightState Enum based on Lux data returned */
      if(lightSensorData.apds9301_luxData > LIGHT_DARK_THRESHOLD) {
        /* Check for dark->light transition; log event */
        if(lightState == LUX_STATE_DARK) {
          LOG_LIGHT_SENSOR_EVENT(LIGHT_EVENT_DAY);
          printf("Light Sensor - Dark->Light Transition detected\n");
        }

        lightState = LUX_STATE_LIGHT;
      } else {
        /* Check for light->dark transition; log event */
        if(lightState == LUX_STATE_LIGHT) {
          LOG_LIGHT_SENSOR_EVENT(LIGHT_EVENT_NIGHT);
          printf("Light Sensor - Light->Dark Transition detected\n");
        }

        lightState = LUX_STATE_DARK;
      }

      /* TODO - derive method to set status sent to main */
      SEND_STATUS_MSG(hbMsgQueue, PID_LIGHT, STATUS_OK, ERROR_CODE_USER_NONE0);
    }
    else 
    {
      ERROR_PRINT("LightThread failed to communicate with lightSensor properly.\n");
      SEND_STATUS_MSG(hbMsgQueue, PID_LIGHT, STATUS_ERROR, ERROR_CODE_USER_NOTIFY0);
      LOG_LIGHT_SENSOR_EVENT(LIGHT_EVENT_SENSOR_READ_ERROR);
    }

    /* Wait on signal timer */
    sigwait(&set, &signum);
  }

  /* Thread Cleanup */
  LOG_LIGHT_SENSOR_EVENT(LIGHT_EVENT_EXITING);
  ERROR_PRINT("Light thread exiting\n");
  timer_delete(timerid);
  mq_close(hbMsgQueue);
  close(sharedMemFd);

  return NULL;
}

/*---------------------------------------------------------------------------------*/
/* HELPER METHODS */
/*---------------------------------------------------------------------------------*/
int8_t getLightSensorData(int sensorFd, LightDataStruct *lightData) 
{
  if(EXIT_FAILURE == verifyLightSensorComm(sensorFd))
    return EXIT_FAILURE;

  /* Collect data Light Sensor */
  apds9301_getLuxData(sensorFd, &lightData->apds9301_luxData);
  apds9301_getDeviceId(sensorFd, &lightData->apds9301_devicePartNo, &lightData->apds9301_deviceRevNo);
  apds9301_getControl(sensorFd, &lightData->apds9301_powerControl);
  apds9301_getTimingGain(sensorFd, &lightData->apds9301_timingGain);
  apds9301_getTimingIntegration(sensorFd, &lightData->apds9301_timingIntegration);
  apds9301_getInterruptControl(sensorFd, &lightData->apds9301_intSelect, &lightData->apds9301_intPersist);
  apds9301_getLowIntThreshold(sensorFd, &lightData->apds9301_intThresLow);
  apds9301_getHighIntThreshold(sensorFd, &lightData->apds9301_intThresHigh);

  return EXIT_SUCCESS;
}

/*---------------------------------------------------------------------------------*/
int8_t initLightSensor(int sensorFd)
{
  int failureCount = 0;
  Apds9301_PowerCtrl_e controlRegRead;
  Apds9301_TimingGain_e timingGainRead;
  Apds9301_TimingInt_e timingIntRead;
  Apds9301_IntSelect_e intSelect;
  Apds9301_IntPersist_e intPersist;
  uint16_t lowThresholdRead = 0;
  uint16_t highThresholdRead = 0;

  /* Verify able to communicate with LightSensor */
  if(EXIT_FAILURE == verifyLightSensorComm(sensorFd))
    return EXIT_FAILURE;

  /* Initialize Light Sensor with default settings */
  apds9301_clearInterrupt(sensorFd);
  apds9301_setControl(sensorFd, DEFAULT_POWER_STATE);
  apds9301_setTimingGain(sensorFd, DEFAULT_TIMING_GAIN);
  apds9301_setTimingIntegration(sensorFd, DEFAULT_TIMING_INTEGRATION);
  apds9301_setInterruptControl(sensorFd, DEFAULT_INT_SELECT, DEFAULT_INT_PERSIST);
  apds9301_setLowIntThreshold(sensorFd, DEFAULT_LOW_INT_THRESHOLD);
  apds9301_setHighIntThreshold(sensorFd, DEFAULT_HIGH_INT_THRESHOLD);

  /* BIST Test */
  /* Verify initial conditions set were properly loaded */
  apds9301_getControl(sensorFd, &controlRegRead);
  apds9301_getTimingGain(sensorFd, &timingGainRead);
  apds9301_getTimingIntegration(sensorFd, &timingIntRead);
  apds9301_getInterruptControl(sensorFd, &intSelect, &intPersist);
  apds9301_getLowIntThreshold(sensorFd, &lowThresholdRead);
  apds9301_getHighIntThreshold(sensorFd, &highThresholdRead);

  if(controlRegRead != DEFAULT_POWER_STATE)
  {
    ERROR_PRINT("Failed to set APDS9301 control reg power state to 0x%x\n", DEFAULT_POWER_STATE);
    failureCount++;
  }
  if(timingGainRead != DEFAULT_TIMING_GAIN)
  {
    ERROR_PRINT("Failed to set APDS9301 timing reg gain to 0x%x\n", DEFAULT_TIMING_GAIN);
    failureCount++;
  }
  if(timingIntRead != DEFAULT_TIMING_INTEGRATION)
  {
    ERROR_PRINT("Failed to set APDS9301 timing reg integration to 0x%x\n", DEFAULT_TIMING_INTEGRATION);
    failureCount++;
  }
  if(intSelect != DEFAULT_INT_SELECT)
  {
    ERROR_PRINT("Failed to set APDS9301 Interrupt Control reg Select to 0x%x\n", DEFAULT_INT_SELECT);
    failureCount++;
  }
  if(intPersist != DEFAULT_INT_PERSIST)
  {
    ERROR_PRINT("Failed to set APDS9301 Interrupt Control reg Persist to 0x%x\n", DEFAULT_INT_PERSIST);
    failureCount++;
  }
  if(lowThresholdRead != DEFAULT_LOW_INT_THRESHOLD)
  {
    ERROR_PRINT("Failed to set APDS9301 Interrupt Low Threshold reg to 0x%x\n", DEFAULT_LOW_INT_THRESHOLD);
    failureCount++;
  }
  if(highThresholdRead != DEFAULT_HIGH_INT_THRESHOLD)
  {
    ERROR_PRINT("Failed to set APDS9301 Interrupt High Threshold reg to 0x%x\n", DEFAULT_HIGH_INT_THRESHOLD);
    failureCount++;
  }

  if(failureCount == 7)
  {
    ERROR_PRINT("Failed to set all default settings for APDS9301 - returning failure.\n");
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}

/*---------------------------------------------------------------------------------*/
int8_t verifyLightSensorComm(int sensorFd) 
{
  uint8_t devicePartNo, deviceRevNo = 0;

  /* Verify able to communicate with device - check able to read device Part Number */
  apds9301_getDeviceId(sensorFd, &devicePartNo, &deviceRevNo);
  if(devicePartNo != APDS9301_PARTNO)
  {
    ERROR_PRINT("lightThread failed to read APDS9301 Part Number properly | lightThread -> lightSensor comm failed.\n");
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}

/*---------------------------------------------------------------------------------*/
void lightSigHandler(int signo, siginfo_t *info, void *extra)
{
  INFO_PRINT("lightSigHandler, signum: %d\n",info->si_signo);
  aliveFlag = 0;
}

/*---------------------------------------------------------------------------------*/
void lightGetAliveFlag(uint8_t *pAlive)
{
  if(pAlive != NULL)
    *pAlive = aliveFlag;
}

