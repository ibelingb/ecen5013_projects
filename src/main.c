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
#include <string.h>
#include <mqueue.h>
#include <signal.h>

#include "tempThread.h"
#include "lightThread.h"
#include "remoteThread.h"
#include "loggingThread.h"
#include "packet.h"

/* Define static and global variables */


int main(int argc, char *argv[]){
  TaskStatusPacket recvThreadStatus;
  SensorThreadInfo sensorThreadInfo;
  RemoteThreadInfo remoteThreadInfo;
  logThreadInfo logThreadInfo;
  LogMsgPacket logPacket;
  char *taskStatusMsgQueueName  = "/heartbeat_mq";
  char *logMsgQueueName        = "/logging_mq";
  char *sensorSharedMemoryName = "/sensor_sm";
  struct mq_attr = mqAttr;
  mqd_t logMsgQueue;
  mqd_t taskStatusMsgQueue;
  int sharedMemSize = (sizeof(struct TempDataStruct) + sizeof(struct LightDataStruct));
  int sharedMemFd = 0;
  int status = 0;

  /* Check inputs */
  // TODO

  /* Register Signal Handler */
  // TODO

  /* Initialize created structs and packets to be 0-filled */
  memset(&recvThreadStatus, 0, sizeof(struct TaskStatusPacket));
  memset(&sensorThreadInfo, 0, sizeof(struct SensorThreadInfo));
  memset(&remoteThreadInfo, 0, sizeof(struct RemoteThreadInfo));
  memset(&logThreadInfo,    0, sizeof(struct LogThreadInfo));
  memset(&logPacket,        0, sizeof(struct LogMsgPacket));
  memset(&mqAttr,           0, sizeof(struct mq_attr));

  /* Ensure MQs and Shared Memory properly cleaned up before starting */
  mq_unlink(taskStatusMsgQueueName);
  mq_unlink(logMsgQueueName);
  shm_unlink(sensorSharedMemoryName);
  /* Define MQ attributes */
  mqAttr.mq_flags   = 0;
  mqAttr.mq_magmsg  = MSG_QUEUE_DEPTH;
  mqAttr.mq_msgsize = MSG_QUEUE_MSG_SIZE;
  mqAttr.mq_curmsgs = 0;

  /*** Create all IPC mechanisms used by children threads ***/
  /* Create MessageQueue for logger to receive log messages */
  logMsgQueue = mq_open(logMsgQueueName, O_CREAT, 0666, &mqAttr);
  if(logMsgQueue == -1)
  {
    printf("ERROR: main() failed to create MessageQueue for Logger - exiting.\n");
    return -1; // TODO - update
  }

  /* Create MessageQueue to receive thread status from all children */
  taskStatusMsgQueue = mq_open(logMsgQueueName, O_CREAT | O_RDRW, 0666, &mqAttr);
  if(taskStatusMsgQueue == -1)
  {
    printf("ERROR: main() failed to create MessageQueue for Main TaskStatus reception - exiting.\n");
    return -1;
  }

  /* Create Shared Memory for data sharing between SensorThreads and RemoteThread */
  sharedMemFd = shm_open(sensorSharedMemoryName, O_CREAT, 0666);
  if(sharedMemFd == -1)
  {
    printf("ERROR: main() failed to create shared memory for sensor and remote threads - exiting.\n");
    return -1;
  }
  /* Configure size of SensorSharedMemory */
  if(ftruncate(sharedMemFd, sharedMemSize) == -1)
  {
    printf("ERROR: main() failed to configure size of shared memory - exiting.\n");
    return -1;
  }
  /* MemoryMap shared memory */
  if(*(int*)mmap(0, sharedMemSize, PROT_READ | PROT_WRITE, MAP_SHARED, sharedMemFd, 0) == -1)
  {
    printf("ERROR: main() failed to complete memory mapping for shared memory - exiting.\n");
    return -1;
  }

  /* Populate ThreadInfo objects to pass names for created IPC pieces to threads */
  strcpy(sensorThreadInfo.heartbeatMsgQueueName, taskStatusMsgQueueName);
  strcpy(sensorThreadInfo.logMsgQueueName, logMsgQueueName);
  strcpy(sensorThreadInfo.sensorSharedMemoryName, sensorSharedMemoryName);
  sensorThreadInfo.sharedMemSize = sharedMemSize;

  // TODO: if remoteThreadInfo not needed, reuse sensorThreadInfo struct for remoteThread pthread_create()
  strcpy(remoteThreadInfo.heartbeatMsgQueueName, taskStatusMsgQueueName);
  strcpy(remoteThreadInfo.logMsgQueueName, logMsgQueueName);
  strcpy(remoteThreadInfo.sensorSharedMemoryName, sensorSharedMemoryName);
  remoteThreadInfo.sharedMemSize = sharedMemSize;

  strcpy(logThreadInfo.heartbeatMsgQueueName, taskStatusMsgQueueName);
  strcpy(logThreadInfo.logMsgQueueName, logMsgQueueName);

  /* Create threads */
  //TODO

  /* Join threads */
  //TODO

  /* Verify all threads have started successfully */
  // TODO - Add checks for task->state

  /* Parent thread Asymmetrical - running concurrently with children threads */
  /* Periodically get thread status, send to logging thread */
  // Setup timer

  /* Cleanup */
  mq_unlink(taskStatusMsgQueueName);
  mq_unlink(logMsgQueueName);
  shm_unlink(sensorSharedMemoryName);
  mq_close(taskStatusMsgQueue);
  mq_close(logMsgQueue);
  close(sharedMemFd);
}

