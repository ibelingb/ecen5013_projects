/* Light Thread Library */

#include "lightThread.h"
#include "packet.h"
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <stdio.h>
#include <mqueue.h>
#include <sys/shm.h>
#include <sys/mman.h>

/* Prototypes for private/helper functions */
void getLightSensorData(LightDataStruct* data);

/*---------------------------------------------------------------------------------*/
void* lightSensorThreadHandler(void* threadInfo)
{
  SensorThreadInfo sensorInfo = *(SensorThreadInfo *)threadInfo;
  LightDataStruct lightSensorData;
  mqd_t logMsgQueue;
  mqd_t hbMsgQueue; /* main heartbeat MessageQueue */
  struct mq_attr mqAttr;
  void* sharedMemPtr = NULL;
  int sharedMemFd;

  /* Open FDs for Main and Logging Message queues */
  logMsgQueue = mq_open(sensorInfo.logMsgQueueName, O_RDWR, 0666, mqAttr);
  hbMsgQueue = mq_open(sensorInfo.heartbeatMsgQueueName, O_RDWR, 0666, mqAttr);
  if(logMsgQueue == -1){
    printf("ERROR: lightSensorThread Failed to Open Logging MessageQueue - exiting.\n");
    return NULL;
  }
  if(hbMsgQueue == -1){
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
  if(*(int *)sharedMemPtr == -1){
    printf("ERROR: lightSensorThread Failed to complete memory mapping of shared memory - exiting\n");
    return NULL;
  }

  /* Setup timer to periodically sample from Light Sensor */
  // TODO

  /* Log SensorThread successfully created */
  // TODO

  /* 0-fill light sensor struct before getting data */
  memset(&lightSensorData, 0, sizeof(LightDataStruct));

  /* Capture light sensor data, write to shared memory */
  getLightSensorData(&lightSensorData);
  // TODO - write data to shared memory



  // TODO - update
  return NULL;
}

/*---------------------------------------------------------------------------------*/
/* HELPER METHODS */
void getLightSensorData(LightDataStruct* data) {


  /* Collect data from all available Light Sensor registers */
  // TODO

  return; 
}

/*---------------------------------------------------------------------------------*/
