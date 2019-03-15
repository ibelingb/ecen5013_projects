/* Remote Thread Library */

#include "remoteThread.h"
#include "packet.h"
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <stdbool.h>
#include <unistd.h> // TODO: remove after sleep() replaced with timer
#include <string.h>
#include <mqueue.h>
#include <sys/shm.h>
#include <sys/mman.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define PORT (5001)
#define MAX_CLIENTS (5)

/* Prototypes for private/helper functions */
static void txHeartbeatMsg();
static void remoteTimerHandler();

/* Define static and global variables */
static SensorThreadInfo sensorInfo;
static bool threadAlive;

/*---------------------------------------------------------------------------------*/
void* remoteThreadHandler(void* threadInfo)
{
  sensorInfo = *(SensorThreadInfo *)threadInfo;
  struct LightDataStruct lightData = {0}; /* Used to read data from Shared Memory */
  struct TempDataStruct tempData = {0};  /* Used to read data from Shared Memory */
  mqd_t logMsgQueue; /* logger MessageQueue */
  mqd_t hbMsgQueue;  /* main heartbeat MessageQueue */
  struct mq_attr mqAttr;
  void* sharedMemPtr = NULL;
  int sharedMemFd;
  int sockfd, sockfd2;
  struct sockaddr_in servAddr, cliAddr;
  unsigned int cliLen;
  threadAlive = true;

  /* Register Signal Handler */
  // TODO

  /* Open FDs for Main and Logging Message queues */
  logMsgQueue = mq_open(sensorInfo.logMsgQueueName, O_RDWR, 0666, mqAttr);
  hbMsgQueue = mq_open(sensorInfo.heartbeatMsgQueueName, O_RDWR, 0666, mqAttr);
  if(logMsgQueue == -1){
    printf("ERROR: remoteThread Failed to Open Logging MessageQueue - exiting.\n");
    return NULL;
  }
  if(hbMsgQueue == -1) {
    printf("ERROR: remoteThread Failed to Open heartbeat MessageQueue - exiting.\n");
    return NULL;
  }

  /* Setup Shared memory for thread */
  sharedMemFd = shm_open(sensorInfo.sensorSharedMemoryName, O_RDWR, 0666);
  if(sharedMemFd == -1) {
    printf("ERROR: remoteThread Failed to Open heartbeat MessageQueue - exiting.\n");
    return NULL;
  }

  /* Memory map the shared memory */
  sharedMemPtr = mmap(0, sensorInfo.sharedMemSize, PROT_READ | PROT_WRITE, MAP_SHARED, sharedMemFd, 0);
  if(*(int *)sharedMemPtr == -1) {
    printf("ERROR: remoteThread Failed to complete memory mapping of shared memory - exiting\n");
    return NULL;
  }

  /** Establish connection on remote socket **/
  /* Create Socket */
  sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if(sockfd == -1){
    printf("ERROR: remoteThread failed to create socket - exiting.\n");
    return NULL;
  }

  /* Set properties and bind socket */
  servAddr.sin_family = AF_INET;
  servAddr.sin_addr.s_addr = INADDR_ANY;
  servAddr.sin_port = htons((int)PORT);
  if(bind(sockfd, (struct sockaddr*)&servAddr, sizeof(servAddr)) == -1) {
    printf("ERROR: remoteThread failed to bind socket - exiting.\n");
    return NULL;
  }

  /* Listen for Client Connection */
  if(listen(sockfd, MAX_CLIENTS) == -1) {
    printf("ERROR: remoteThread failed to successfully listen for client connection - exiting.\n");
    return NULL;
  }

  /* Accept Client Connection */
  cliLen = sizeof(cliAddr);
  sockfd2 = accept(sockfd, (struct sockaddr*)&cliAddr, &cliLen);
  if(sockfd2 == -1){
    printf("ERROR: remoteThread failed to accept client connection for socket - exiting.\n");
    return NULL;
  }

  /* Log SensorThread successfully created */
  printf("Successfully created remoteThread to listen on port %d.\n", PORT);
  // TODO - send msg to log

  while(threadAlive) {
    /* block for incoming commands from remote client(s) */
    // TODO

    /* Based on cmd received, read from Shared Memory and pass data based on client socket */
    //sharedMem
    pthread_mutex_lock(sensorInfo.sharedMemMutex);
    memcpy(&tempData, (sharedMemPtr+sensorInfo.tempDataOffset), sizeof(struct TempDataStruct));
    memcpy(&lightData, (sharedMemPtr+sensorInfo.lightDataOffset), sizeof(struct LightDataStruct));
    pthread_mutex_unlock(sensorInfo.sharedMemMutex);

    sleep(5);
    
    txHeartbeatMsg();
  }

  /* Setup timer to periodically send heartbeat to parent thread */
  // TODO

  /* Thread Cleanup */
  mq_close(logMsgQueue);
  mq_close(hbMsgQueue);
  close(sharedMemFd);

  return NULL;
}

/*---------------------------------------------------------------------------------*/
/* HELPER METHODS */
static void remoteTimerHandler(){
  // TODO

  return;
}


static void txHeartbeatMsg(){
  // TODO

  return;
}

/*---------------------------------------------------------------------------------*/
