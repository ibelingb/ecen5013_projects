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
#include <errno.h>

#include "tempThread.h"
#include "lightThread.h"
#include "remoteThread.h"
#include "loggingThread.h"
#include "packet.h"
#include "bbgLeds.h"
#include "debug.h"
#include "logger.h"

#define NUM_THREADS (4)

/* Define static and global variables */
pthread_t gThreads[NUM_THREADS];
pthread_mutex_t gSharedMemMutex; // TODO: Could have separate mutexes for each sensor writing to SHM
pthread_mutex_t gI2cBusMutex; 

/* temp globals for causing threads to exit */
int gExitSig = 1;
int gExitLog = 1;

int main(int argc, char *argv[]){
  char *heartbeatMsgQueueName = "/heartbeat_mq";
  char *logMsgQueueName        = "/logging_mq";
  char *sensorSharedMemoryName = "/sensor_sm";
  char *logFile = "scripts/log.bin";
  TaskStatusPacket recvThreadStatus;
  SensorThreadInfo sensorThreadInfo;
  LogThreadInfo logThreadInfo;
  LogMsgPacket logPacket;
  struct mq_attr mqAttr;
  mqd_t logMsgQueue;
  mqd_t heartbeatMsgQueue;
  int sharedMemSize = (sizeof(struct TempDataStruct) + sizeof(struct LightDataStruct));
  int sharedMemFd = 0;
  char ind;

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
  mq_unlink(logMsgQueueName);
  mq_unlink(heartbeatMsgQueueName);
  shm_unlink(sensorSharedMemoryName);

  if(remove(logMsgQueueName) == -1 && errno != ENOENT)
	  ERROR_PRINT("ERROR: main() couldn't delete log queue path, err#%d (%s)\n\r", errno, strerror(errno));

  /* Define MQ attributes */
  mqAttr.mq_flags   = 0;
  mqAttr.mq_maxmsg  = MSG_QUEUE_DEPTH;
  mqAttr.mq_msgsize = MSG_QUEUE_MSG_SIZE;
  mqAttr.mq_curmsgs = 0;

  /*** get logger up and running before initializing the rest of the system ***/
  /* Create MessageQueue for logger to receive log messages */
  logMsgQueue = mq_open(logMsgQueueName, O_CREAT, 0666, &mqAttr);
  if(logMsgQueue == -1)
  {
    printf("ERROR: main() failed to create MessageQueue for Logger - exiting.\n");
    return EXIT_FAILURE;
  }

  /* Create MessageQueue to receive thread status from all children */
  heartbeatMsgQueue = mq_open(heartbeatMsgQueueName, O_CREAT | O_RDWR, 0666, &mqAttr);
  if(heartbeatMsgQueue == -1)
  {
    printf("ERROR: main() failed to create MessageQueue for Main TaskStatus reception - exiting.\n");
    return EXIT_FAILURE;
  }

  /* initialize thread args for logging thread */
  strcpy(logThreadInfo.heartbeatMsgQueueName, heartbeatMsgQueueName);
  strcpy(logThreadInfo.logMsgQueueName, logMsgQueueName);
  strcpy(logThreadInfo.logFileName, logFile);

  /* initialize the logger (i.e. queue & writeability) 
    * main should probably do this before creating
    * other threads to ensure logger is working before
    * main or other threads start send msgs to mog queue */
  if(LOG_INIT(&logThreadInfo) != LOG_STATUS_OK)
  {
    ERROR_PRINT("failed to init logger\n");
    return EXIT_FAILURE;
  }

  /* logger (i.e. queue writer) init complete */
  LOG_LOGGER_INITIALIZED();

  /* create logger thread */
  if(pthread_create(&gThreads[0], NULL, logThreadHandler, (void*)&logThreadInfo) != 0)
  {
    printf("ERROR: Failed to create Logging Thread - exiting main().\n");
    return EXIT_FAILURE;
  }

  /*** initialize rest of IPC resources ***/
  /* Create Shared Memory for data sharing between SensorThreads and RemoteThread */
  sharedMemFd = shm_open(sensorSharedMemoryName, O_CREAT | O_RDWR, 0666);
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
  strcpy(sensorThreadInfo.heartbeatMsgQueueName, heartbeatMsgQueueName);
  strcpy(sensorThreadInfo.logMsgQueueName, logMsgQueueName);
  strcpy(sensorThreadInfo.sensorSharedMemoryName, sensorSharedMemoryName);
  sensorThreadInfo.sharedMemSize = sharedMemSize;
  sensorThreadInfo.lightDataOffset = sizeof(TempDataStruct);
  sensorThreadInfo.sharedMemMutex = &gSharedMemMutex;
  sensorThreadInfo.i2cBusMutex = &gI2cBusMutex;

  pthread_mutex_init(&gSharedMemMutex, NULL);
  pthread_mutex_init(&gI2cBusMutex, NULL);

  // /* Create other threads */
  // if(pthread_create(&gThreads[1], NULL, remoteThreadHandler, (void*)&sensorThreadInfo))
  // {
  //   printf("ERROR: Failed to create Remote Thread - exiting main().\n");
  //   return EXIT_FAILURE;
  // }
  // if(pthread_create(&gThreads[2], NULL, tempSensorThreadHandler, (void*)&sensorThreadInfo))
  // {
  //   printf("ERROR: Failed to create TempSensor Thread - exiting main().\n");
  //   return EXIT_FAILURE;
  // }
  // if(pthread_create(&gThreads[3], NULL, lightSensorThreadHandler, (void*)&sensorThreadInfo))
  // {
  //   printf("ERROR: Failed to create LightSensor Thread - exiting main().\n");
  //   return EXIT_FAILURE;
  // }
  LOG_MAIN_EVENT(MAIN_EVENT_STARTED_THREADS);

  /* Log main thread started succesfully */
  printf("The Main() thread has successfully started with all child threads created.\n");

  /* system is now fully functional */
  LOG_SYSTEM_INITIALIZED();

  /* Parent thread Asymmetrical - running concurrently with children threads */
  /* Periodically get thread status, send to logging thread */
  // TODO: Setup timer

  ind = 0;
  while(ind++ < 8)
  {
    // TODO
    LOG_HEARTBEAT();
    sleep(1);
  }
  LOG_SYSTEM_HALTED();

  /* wait to kill log so exit msgs get logged */
  usleep(10 * 1000);
  gExitLog = 0;

  /* join to clean up children */
  for(ind = 0; ind < NUM_THREADS; ++ind)
    pthread_join(gThreads[(uint8_t)ind], NULL);
  
  /* Cleanup */
  printf("main() Cleanup.\n");
  mq_unlink(heartbeatMsgQueueName);
  mq_unlink(logMsgQueueName);
  shm_unlink(sensorSharedMemoryName);
  mq_close(heartbeatMsgQueue);
  mq_close(logMsgQueue);
  pthread_mutex_destroy(&gSharedMemMutex);
  pthread_mutex_destroy(&gI2cBusMutex);
  close(sharedMemFd);
}

