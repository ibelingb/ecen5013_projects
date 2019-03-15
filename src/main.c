/***********************************************************************************
 * @author Brian Ibeling and Joshua Malburg (joma0364)
 * brian.ibeling@colorado.edu
 * joshua.malburg@colorado.edu
 * Advanced Embedded Software Development
 * ECEN5013 - Rick Heidebrecht
 * @date March 8, 2019
 * arm-linux-gnueabi (Buildroot)
 * gcc (Ubuntu)
 ************************************************************************************
 *
 * @file main.c
 * @brief main for project 1 - Multithreaded sensor sampling application
 *
 * NOTE: Main process will create and destory all IPC mechanisms. Children threads will
 *       open()/close() resources as needed.
 *
 ************************************************************************************
 */

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <mqueue.h>
#include <signal.h>
#include <pthread.h>
#include <sys/shm.h>
#include <sys/mman.h>

#include "tempThread.h"
#include "lightThread.h"
#include "remoteThread.h"
#include "loggingThread.h"
#include "packet.h"
#include "bbgLeds.h"

#define NUM_THREADS (4)

/* Define static and global variables */
pthread_t gThreads[NUM_THREADS];
pthread_mutex_t gSharedMemMutex; // TODO: Could have separate mutexes for each sensor writing to SHM
pthread_mutex_t gI2cBusMutex; 

int main(int argc, char *argv[]){
  char *taskStatusMsgQueueName = "/heartbeat_mq";
  char *logMsgQueueName        = "/logging_mq";
  char *sensorSharedMemoryName = "/sensor_sm";
  TaskStatusPacket recvThreadStatus;
  SensorThreadInfo sensorThreadInfo;
  LogThreadInfo logThreadInfo;
  LogMsgPacket logPacket;
  struct mq_attr mqAttr;
  mqd_t logMsgQueue;
  mqd_t taskStatusMsgQueue;
  int sharedMemSize = (sizeof(struct TempDataStruct) + sizeof(struct LightDataStruct));
  int sharedMemFd = 0;

  /* Check inputs */
  // TODO

  /* Register Signal Handler */
  // TODO

  /* Initialize created structs and packets to be 0-filled */
  memset(&recvThreadStatus, 0, sizeof(struct TaskStatusPacket));
  memset(&sensorThreadInfo, 0, sizeof(struct SensorThreadInfo));
  memset(&logThreadInfo,    0, sizeof(struct LogThreadInfo));
  memset(&logPacket,        0, sizeof(struct LogMsgPacket));
  memset(&mqAttr,           0, sizeof(struct mq_attr));

  /* Ensure MQs and Shared Memory properly cleaned up before starting */
  mq_unlink(taskStatusMsgQueueName);
  mq_unlink(logMsgQueueName);
  shm_unlink(sensorSharedMemoryName);
  /* Define MQ attributes */
  mqAttr.mq_flags   = 0;
  mqAttr.mq_maxmsg  = MSG_QUEUE_DEPTH;
  mqAttr.mq_msgsize = MSG_QUEUE_MSG_SIZE;
  mqAttr.mq_curmsgs = 0;

  /*** Create all IPC mechanisms used by children threads ***/
  /* Create MessageQueue for logger to receive log messages */
  logMsgQueue = mq_open(logMsgQueueName, O_CREAT, 0666, &mqAttr);
  if(logMsgQueue == -1)
  {
    printf("ERROR: main() failed to create MessageQueue for Logger - exiting.\n");
    return EXIT_FAILURE;
  }

  /* Create MessageQueue to receive thread status from all children */
  taskStatusMsgQueue = mq_open(logMsgQueueName, O_CREAT | O_RDWR, 0666, &mqAttr);
  if(taskStatusMsgQueue == -1)
  {
    printf("ERROR: main() failed to create MessageQueue for Main TaskStatus reception - exiting.\n");
    return EXIT_FAILURE;
  }

  /* Create Shared Memory for data sharing between SensorThreads and RemoteThread */
  sharedMemFd = shm_open(sensorSharedMemoryName, O_CREAT, 0666);
  if(sharedMemFd == -1)
  {
    printf("ERROR: main() failed to create shared memory for sensor and remote threads - exiting.\n");
    return EXIT_FAILURE;
  }
  /* Configure size of SensorSharedMemory */
  if(ftruncate(sharedMemFd, sharedMemSize) == -1)
  {
    printf("ERROR: main() failed to configure size of shared memory - exiting.\n");
    return EXIT_FAILURE;
  }
  /* MemoryMap shared memory */
  if(*(int*)mmap(0, sharedMemSize, PROT_READ | PROT_WRITE, MAP_SHARED, sharedMemFd, 0) == -1)
  {
    printf("ERROR: main() failed to complete memory mapping for shared memory - exiting.\n");
    return EXIT_FAILURE;
  }

  /* Populate ThreadInfo objects to pass names for created IPC pieces to threads */
  strcpy(sensorThreadInfo.heartbeatMsgQueueName, taskStatusMsgQueueName);
  strcpy(sensorThreadInfo.logMsgQueueName, logMsgQueueName);
  strcpy(sensorThreadInfo.sensorSharedMemoryName, sensorSharedMemoryName);
  sensorThreadInfo.sharedMemSize = sharedMemSize;
  sensorThreadInfo.lightDataOffset = sizeof(TempDataStruct);
  sensorThreadInfo.sharedMemMutex = &gSharedMemMutex;
  sensorThreadInfo.i2cBusMutex = &gI2cBusMutex;

  strcpy(logThreadInfo.heartbeatMsgQueueName, taskStatusMsgQueueName);
  strcpy(logThreadInfo.logMsgQueueName, logMsgQueueName);

  pthread_mutex_init(&gSharedMemMutex, NULL);
  pthread_mutex_init(&gI2cBusMutex, NULL);

  /* Create threads */
  if(pthread_create(&gThreads[0], NULL, logThreadHandler, (void*)&logThreadInfo) != 0)
  {
    printf("ERROR: Failed to create Logging Thread - exiting main().\n");
    return EXIT_FAILURE;
  }
  if(pthread_create(&gThreads[1], NULL, remoteThreadHandler, (void*)&sensorThreadInfo))
  {
    printf("ERROR: Failed to create Remote Thread - exiting main().\n");
    return EXIT_FAILURE;
  }
  if(pthread_create(&gThreads[2], NULL, tempSensorThreadHandler, (void*)&sensorThreadInfo))
  {
    printf("ERROR: Failed to create TempSensor Thread - exiting main().\n");
    return EXIT_FAILURE;
  }
  if(pthread_create(&gThreads[3], NULL, lightSensorThreadHandler, (void*)&sensorThreadInfo))
  {
    printf("ERROR: Failed to create LightSensor Thread - exiting main().\n");
    return EXIT_FAILURE;
  }

  /* Log main thread started succesfully */
  printf("The Main() thread has successfully started with all child threads created.\n");
  // TODO - add msg to log

  /* Parent thread Asymmetrical - running concurrently with children threads */
  /* Periodically get thread status, send to logging thread */
  // TODO: Setup timer
  while(1){
    // TODO

    sleep(10);
  }


  // TODO: Since not joining threads, is there a cleanup method for the created pthreads? pthread_exit()?
  /* Cleanup */
  mq_unlink(taskStatusMsgQueueName);
  mq_unlink(logMsgQueueName);
  shm_unlink(sensorSharedMemoryName);
  mq_close(taskStatusMsgQueue);
  mq_close(logMsgQueue);
  pthread_mutex_destroy(&gSharedMemMutex);
  pthread_mutex_destroy(&gI2cBusMutex);
  close(sharedMemFd);
}

