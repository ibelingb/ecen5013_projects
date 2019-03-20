/* Light Thread Library */

#include "lightThread.h"
#include "lightSensor.h"
#include "packet.h"
#include "lu_iic.h"
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h> // TODO: remove after sleep() replaced with timer
#include <mqueue.h>
#include <sys/shm.h>
#include <sys/mman.h>

/* Prototypes for private/helper functions */
static void txHeartbeatMsg();
static void lightTimerHandler();

/* Define static and global variables */
static SensorThreadInfo sensorInfo;
static pthread_mutex_t* i2cBusMutex;
static void* sharedMemPtr;
static int sensorFd;
static bool threadAlive;

/*---------------------------------------------------------------------------------*/
void* lightSensorThreadHandler(void* threadInfo)
{
  sensorInfo = *(SensorThreadInfo *)threadInfo;
  mqd_t logMsgQueue; /* logger MessageQueue */
  mqd_t hbMsgQueue;  /* main heartbeat MessageQueue */
  struct mq_attr mqAttr;
  void* sharedMemPtr = NULL;
  int sharedMemFd;
  threadAlive = true;

  /* Register Signal Handler */
  // TODO

  /* Open FDs for Main and Logging Message queues */
  logMsgQueue = mq_open(sensorInfo.logMsgQueueName, O_RDWR, 0666, mqAttr);
  hbMsgQueue = mq_open(sensorInfo.heartbeatMsgQueueName, O_RDWR, 0666, mqAttr);
  if(logMsgQueue == -1){
    printf("ERROR: lightSensorThread Failed to Open Logging MessageQueue - exiting.\n");
    return NULL;
  }
  if(hbMsgQueue == -1) {
    printf("ERROR: lightSensorThread Failed to Open heartbeat MessageQueue - exiting.\n");
    return NULL;
  }

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

  /* Initialize FD for I2C bus and set static mutex variable */
  sensorFd = initIic("/dev/i2c-2");
  if(sensorFd < 0) {
    printf("ERROR: lightSensorThread failed to initialize i2c Bus.\n");
  }
  i2cBusMutex = sensorInfo.i2cBusMutex;

  /* Log SensorThread successfully created */
  printf("Successfully created lightSensorThread.\n");
  // TODO

  /* Setup timer to periodically sample from Light Sensor */
  // TODO
  while(threadAlive) {
    lightTimerHandler();
    sleep(5);
  }

  /* Thread Cleanup */
  mq_close(logMsgQueue);
  mq_close(hbMsgQueue);
  close(sharedMemFd);

  return NULL;
}

/*---------------------------------------------------------------------------------*/
/* HELPER METHODS */
void getLightSensorData(LightDataStruct *lightData) {

  /* Verify able to communicate with device - check device ID */
  pthread_mutex_lock(i2cBusMutex);
  apds9301_getDeviceId(sensorFd, &lightData->apds9301_devicePartNo, &lightData->apds9301_deviceRevNo);
  pthread_mutex_unlock(i2cBusMutex);
  if(lightData->apds9301_devicePartNo != APDS9301_PARTNO){
    /* Unable to read device ID successfully - transmit error message to Logger and */
    // TODO
    //threadAlive = false;
    return;
  }

  /* Collect data from all available Light Sensor registers */
  pthread_mutex_lock(i2cBusMutex);
  apds9301_getLuxData(sensorFd, &lightData->apds9301_luxData);
  apds9301_getControl(sensorFd, &lightData->apds9301_powerControl);
  apds9301_getTimingGain(sensorFd, &lightData->apds9301_timingGain);
  apds9301_getTimingIntegration(sensorFd, &lightData->apds9301_timingIntegration);
  apds9301_getInterruptControl(sensorFd, &lightData->apds9301_intSelect, &lightData->apds9301_intPersist);
  apds9301_getLowIntThreshold(sensorFd, &lightData->apds9301_intThresLow);
  apds9301_getHighIntThreshold(sensorFd, &lightData->apds9301_intThresHigh);
  pthread_mutex_unlock(i2cBusMutex);
}

static void txHeartbeatMsg() {
  //LogMsgPacket logPacket = {0};
  // TODO
}

static void lightTimerHandler() {
  /* Capture light sensor data from device */
  LightDataStruct lightSensorData;
  getLightSensorData(&lightSensorData);

  /* Write to shared memory */
  pthread_mutex_lock(sensorInfo.sharedMemMutex);
  memcpy(sharedMemPtr+(sensorInfo.lightDataOffset), &lightSensorData, sizeof(LightDataStruct));
  pthread_mutex_unlock(sensorInfo.sharedMemMutex);

  /* Transmit heartbeat message to parent process */
  txHeartbeatMsg();
}

/*---------------------------------------------------------------------------------*/
