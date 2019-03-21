/* Light Thread Library */

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

extern int gExitSig;

/* Prototypes for private/helper functions */
void getLightSensorData(int sensorFd, LightDataStruct *lightData);

/* Define static and global variables */

/*---------------------------------------------------------------------------------*/
void* lightSensorThreadHandler(void* threadInfo)
{
  SensorThreadInfo sensorInfo = *(SensorThreadInfo *)threadInfo;
  LightDataStruct lightSensorData = {0};
  int sensorFd;
  void* sharedMemPtr = NULL;
  int sharedMemFd;

  /* timer variables */
  timer_t timerid;
  sigset_t set;
  struct timespec timer_interval;
  int signum = SIGALRM;

  /* Register Signal Handler */
  // TODO?

  /* Setup Timer */
  memset(&set, 0, sizeof(sigset_t));
  memset(&timerid, 0, sizeof(timer_t));
  timer_interval.tv_nsec = LIGHT_LOOP_TIME_NSEC;
  timer_interval.tv_sec = LIGHT_LOOP_TIME_SEC;
  setupTimer(&set, &timerid, signum, &timer_interval);

  /* Setup Shared memory for thread */
  sharedMemFd = shm_open(sensorInfo.sensorSharedMemoryName, O_RDWR, 0666);
  if(sharedMemFd == -1) {
    ERROR_PRINT("ERROR: lightSensorThread Failed to Open heartbeat MessageQueue - exiting.\n");
    return NULL;
  }

  /* Memory map the shared memory */
  sharedMemPtr = mmap(0, sensorInfo.sharedMemSize, PROT_READ | PROT_WRITE, MAP_SHARED, sharedMemFd, 0);
  if(*(int *)sharedMemPtr == -1) {
    ERROR_PRINT("ERROR: lightSensorThread Failed to complete memory mapping of shared memory - exiting\n");
    return NULL;
  }

  /* Initialize I2C bus and set static mutex variable */
  sensorFd = initIic("/dev/i2c-2");
  if(sensorFd < 0) {
    ERROR_PRINT("ERROR: lightSensorThread failed to initialize i2c Bus.\n");
  }

  /* Log SensorThread successfully created */
  LOG_INFO("Successfully created lightSensorThread.\n");
  LOG_LIGHT_SENSOR_EVENT(LIGHT_EVENT_STARTED);

  /* Setup timer to periodically sample from Light Sensor */
  while(gExitSig) {
    /* Capture light sensor data from device */
    pthread_mutex_lock(sensorInfo.i2cBusMutex);
    getLightSensorData(sensorFd, &lightSensorData);
    pthread_mutex_unlock(sensorInfo.i2cBusMutex);

    /* Write to shared memory */
    pthread_mutex_lock(sensorInfo.sharedMemMutex);
    memcpy(sharedMemPtr+(sensorInfo.lightDataOffset), &lightSensorData, sizeof(LightDataStruct));
    pthread_mutex_unlock(sensorInfo.sharedMemMutex);

    /* Transmit heartbeat message to parent process */
    LOG_HEARTBEAT();

    /* Wait on signal timer */
    sigwait(&set, &signum);
  }

  /* Thread Cleanup */
  LOG_LIGHT_SENSOR_EVENT(LIGHT_EVENT_EXITING);
  INFO_PRINT("Light thread exiting\n");
  close(sharedMemFd);

  return NULL;
}

/*---------------------------------------------------------------------------------*/
/* HELPER METHODS */
void getLightSensorData(int sensorFd, LightDataStruct *lightData) {
  // TODO - improve error handling

  /* Verify able to communicate with device - check device ID */
  apds9301_getDeviceId(sensorFd, &lightData->apds9301_devicePartNo, &lightData->apds9301_deviceRevNo);
  if(lightData->apds9301_devicePartNo != APDS9301_PARTNO){
    /* Unable to read device ID successfully - transmit error message to Logger */
    // TODO
    return;
  }

  /* Collect data from all available Light Sensor registers */
  apds9301_getLuxData(sensorFd, &lightData->apds9301_luxData);
  apds9301_getControl(sensorFd, &lightData->apds9301_powerControl);
  apds9301_getTimingGain(sensorFd, &lightData->apds9301_timingGain);
  apds9301_getTimingIntegration(sensorFd, &lightData->apds9301_timingIntegration);
  apds9301_getInterruptControl(sensorFd, &lightData->apds9301_intSelect, &lightData->apds9301_intPersist);
  apds9301_getLowIntThreshold(sensorFd, &lightData->apds9301_intThresLow);
  apds9301_getHighIntThreshold(sensorFd, &lightData->apds9301_intThresHigh);
}

