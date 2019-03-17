/* Remote Thread Library */

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

#include "remoteThread.h"
#include "packet.h"

#define PORT (5001)
#define MAX_CLIENTS (5)

/* Prototypes for private/helper functions */
static void txHeartbeatMsg();
static void setCmdResponse(RemoteCmd_e cmd, char* response);

/* Define static and global variables */
static SensorThreadInfo sensorInfo;
static bool threadAlive;
static LightDataStruct lightData; /* Used to read data from Shared Memory */
static TempDataStruct tempData;  /* Used to read data from Shared Memory */

/*---------------------------------------------------------------------------------*/
void* remoteThreadHandler(void* threadInfo)
{
  sensorInfo = *(SensorThreadInfo *)threadInfo;
  RemoteCmdPacket cmdPacket = {0};
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
    /* Check for incoming commands from remote client(s) (non-blocking) */
    // TODO

    /* Verify received cmd and packet are valid */
    // TODO

    /* Read from Shared Memory and pass requested data back to client */
    pthread_mutex_lock(sensorInfo.sharedMemMutex);
    memcpy(&tempData, (sharedMemPtr+sensorInfo.tempDataOffset), sizeof(struct TempDataStruct));
    memcpy(&lightData, (sharedMemPtr+sensorInfo.lightDataOffset), sizeof(struct LightDataStruct));
    pthread_mutex_unlock(sensorInfo.sharedMemMutex);

    setCmdResponse(cmdPacket.cmd, cmdPacket.payload);

    sleep(5);

    // TODO Move to timer
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
static void txHeartbeatMsg(){
  // TODO

  return;
}

static void setCmdResponse(RemoteCmd_e cmd, char* response){
  /* Based on received command, populate response to provide back to client */
  switch(cmd) {
    case TEMPCMD_GETTEMP :
      break;
    case TEMPCMD_GETTHRES :
      break;
    case TEMPCMD_GETCONFIG :
      break;
    case TEMPCMD_GETRESOLUTION :
      break;
    case TEMPCMD_GETFAULT :
      break;
    case TEMPCMD_GETEXTMODE :
      break;
    case TEMPCMD_GETSHUTDOWNMODE :
      break;
    case TEMPCMD_GETALERT :
      break;
    case TEMPCMD_GETCONVRATE :
      break;
    case LIGHTCMD_GETLUX :
      break;
    case LIGHTCMD_GETDEVID :
      break;
    case LIGHTCMD_GETCTRL :
      break;
    case LIGHTCMD_GETTIMING :
      break;
    case LIGHTCMD_GETLOWTHRES :
      break;
    case LIGHTCMD_GETHIGHTHRES :
      break;
    default:
      break;
  }

  return;
}
/*---------------------------------------------------------------------------------*/
