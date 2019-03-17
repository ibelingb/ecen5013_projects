/* Remote Thread Library */

#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <string.h>
#include <mqueue.h>
#include <sys/shm.h>
#include <sys/mman.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include "remoteThread.h"
#include "packet.h"

#define MAX_CLIENTS (5)

/* Prototypes for private/helper functions */
static void txHeartbeatMsg();
static void getCmdResponse(RemoteCmdPacket* packet);

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

  /* Log RemoteThread successfully created */
  printf("Created remoteThread to listen on port %d.\n", PORT);
  // TODO - send msg to log

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

  /* Log RemoteThread successfully Connected to client */
  printf("Connected remoteThread to external Client on port %d.\n", PORT);
  // TODO - send msg to log

  while(threadAlive) {
    /* Check for incoming commands from remote clients on socket port */
    recv(sockfd2, &cmdPacket, sizeof(struct RemoteCmdPacket), 0);

    /* Read from Shared Memory and pass requested data back to client */
    pthread_mutex_lock(sensorInfo.sharedMemMutex);
    memcpy(&tempData, (sharedMemPtr+sensorInfo.tempDataOffset), sizeof(struct TempDataStruct));
    memcpy(&lightData, (sharedMemPtr+sensorInfo.lightDataOffset), sizeof(struct LightDataStruct));
    pthread_mutex_unlock(sensorInfo.sharedMemMutex);

    /* Populate Cmd Response data based on received cmd */
    getCmdResponse(&cmdPacket);

    /* Transmit data packet back to remote client requesting data */
    send(sockfd2, &cmdPacket, sizeof(struct RemoteCmdPacket), 0);

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

static void getCmdResponse(RemoteCmdPacket* packet){

  /* Log cmd received */
  // TODO - send msg to log about external cmd received

  /* Based on received command, populate response to provide back to client */
  switch(packet->cmd) {
    case TEMPCMD_GETTEMP :
      packet->data0.status_float = tempData.tmp102_temp;
      printf("TEMPCMD_GETTEMP cmd received | Transmitting data: {%.3f}\n", packet->data0.status_float);
      break;
    case TEMPCMD_GETLOWTHRES :
      packet->data0.status_float = tempData.tmp102_lowThreshold;
      printf("TEMPCMD_GETLOWTHRES cmd received | Transmitting data: {%.3f}\n", packet->data0.status_float);
      break;
    case TEMPCMD_GETHIGHTHRES :
      packet->data0.status_float = tempData.tmp102_highThreshold;
      printf("TEMPCMD_GETHIGHTHRES cmd received | Transmitting data: {%.3f}\n", packet->data0.status_float);
      break;
    case TEMPCMD_GETCONFIG :
      packet->data0.status_float = tempData.tmp102_config;
      printf("TEMPCMD_GETCONFIG cmd received | Transmitting data: {%.3f}\n", packet->data0.status_float);
      break;
    case TEMPCMD_GETRESOLUTION :
      packet->data0.status_float = tempData.tmp102_tempResolution;
      printf("TEMPCMD_GETRESOLUTION cmd received | Transmitting data: {%.3f}\n", packet->data0.status_float);
      break;
    case TEMPCMD_GETFAULT :
      packet->data0.status_uint32 = tempData.tmp102_fault;
      printf("TEMPCMD_GETFAULT case cmd received | Transmitting data: {%d}\n", packet->data0.status_uint32);
      break;
    case TEMPCMD_GETEXTMODE :
      packet->data0.status_uint32 = tempData.tmp102_extendedMode;
      printf("TEMPCMD_GETEXTMODE cmd received | Transmitting data: {%d}\n", packet->data0.status_uint32);
      break;
    case TEMPCMD_GETSHUTDOWNMODE :
      packet->data0.status_uint32 = tempData.tmp102_shutdownMode;
      printf("TEMPCMD_GETSHUTDOWNMODE cmd received | Transmitting data: {%d}\n", packet->data0.status_uint32);
      break;
    case TEMPCMD_GETALERT :
      packet->data0.status_uint32 = tempData.tmp102_alert;
      printf("TEMPCMD_GETALERT cmd received | Transmitting data: {%d}\n", packet->data0.status_uint32);
      break;
    case TEMPCMD_GETCONVRATE :
      packet->data0.status_uint32 = tempData.tmp102_convRate;
      printf("TEMPCMD_GETCONVRATE cmd received | Transmitting data: {%d}\n", packet->data0.status_uint32);
      break;
    case LIGHTCMD_GETLUXLOW :
      packet->data0.status_uint32 = lightData.apds9301_luxData0Low;
      packet->data1.status_uint32 = lightData.apds9301_luxData1Low;
      printf("LIGHTCMD_GETLUXLOW cmd received | Transmitting data: {%d}{%d}\n", 
              packet->data0.status_uint32, packet->data1.status_uint32);
      break;
    case LIGHTCMD_GETLUXHIGH :
      packet->data0.status_uint32 = lightData.apds9301_luxData0High;
      packet->data1.status_uint32 = lightData.apds9301_luxData1High;
      printf("LIGHTCMD_GETLUXHIGH cmd received | Transmitting data: {%d}{%d}\n",
              packet->data0.status_uint32, packet->data1.status_uint32);
      break;
    case LIGHTCMD_GETDEVID :
      packet->data0.status_uint32 = lightData.apds9301_deviceId;
      printf("LIGHTCMD_GETDEVID cmd received | Transmitting data: {%d}\n", packet->data0.status_uint32);
      break;
    case LIGHTCMD_GETCTRL :
      packet->data0.status_uint32 = lightData.apds9301_controlReg;
      printf("LIGHTCMD_GETCTRL cmd received | Transmitting data: {%d}\n", packet->data0.status_uint32);
      break;
    case LIGHTCMD_GETTIMING :
      packet->data0.status_uint32 = lightData.apds9301_timingReg;
      printf("LIGHTCMD_GETTIMING cmd received | Transmitting data: {%d}\n", packet->data0.status_uint32);
      break;
    case LIGHTCMD_GETLOWTHRES :
      packet->data0.status_uint32 = lightData.apds9301_intThresLowLow;
      packet->data1.status_uint32 = lightData.apds9301_intThresLowHigh;
      printf("LIGHTCMD_GETLOWTHRES cmd received | Transmitting data: {%d}{%d}\n",
              packet->data0.status_uint32, packet->data1.status_uint32);
      break;
    case LIGHTCMD_GETHIGHTHRES :
      packet->data0.status_uint32 = lightData.apds9301_intThresHighLow;
      packet->data1.status_uint32 = lightData.apds9301_intThresHighHigh;
      printf("LIGHTCMD_GETHIGHTHRES cmd received | Transmitting data: {%d}{%d}\n",
              packet->data0.status_uint32, packet->data1.status_uint32);
      break;
    default:
      printf("cmd received not recognized - cmd ignored\n");
      break;
  }

  return;
}
/*---------------------------------------------------------------------------------*/
