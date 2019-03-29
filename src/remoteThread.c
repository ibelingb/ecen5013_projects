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
 * @file remoteThread.c
 * @brief Remote Thread Library
 *
 ************************************************************************************
 */

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
#include <signal.h>
#include <errno.h>

#include "remoteThread.h"
#include "packet.h"
#include "logger.h"
#include "debug.h"
#include "cmn_timer.h"
#include "platform.h"
#include "healthMonitor.h"

#define MAX_CLIENTS (5)

/* Prototypes for private/helper functions */
static void getCmdResponse(RemoteCmdPacket* packet);

/* Define static and global variables */
static SensorThreadInfo sensorInfo;
static LightDataStruct lightData; /* Used to read data from Shared Memory */
static TempDataStruct tempData;  /* Used to read data from Shared Memory */
static uint8_t aliveFlag = 1;

/*---------------------------------------------------------------------------------*/
void remoteSigHandler(int signo, siginfo_t *info, void *extra)
{
	INFO_PRINT("remoteSigHandler, signum: %d\n",info->si_signo);
  aliveFlag = 0;
}

void remoteGetAliveFlag(uint8_t *pAlive)
{
  if(pAlive != NULL)
    *pAlive = aliveFlag;
}

void* remoteThreadHandler(void* threadInfo)
{
  sensorInfo = *(SensorThreadInfo *)threadInfo;
  RemoteCmdPacket cmdPacket = {0};
  mqd_t logMsgQueue; /* logger MessageQueue */
  mqd_t hbMsgQueue;  /* main heartbeat MessageQueue */
  struct mq_attr mqAttr;
  void* sharedMemPtr = NULL;
  int sharedMemFd;
  int sockfdServer, sockfdClient, socketFlags;
  struct sockaddr_in servAddr, cliAddr;
  unsigned int cliLen = sizeof(cliAddr);
  ssize_t clientResponse = 0; /* Used to determine if client has disconnected from server */
  uint8_t ind;
	sigset_t mask;

  /* timer variables */
  timer_t timerid;
  sigset_t set;
  struct timespec timer_interval;
  int signum = SIGALRM;

  /* Setup Timer */
  memset(&set, 0, sizeof(sigset_t));
  memset(&timerid, 0, sizeof(timer_t));
  timer_interval.tv_nsec = LIGHT_LOOP_TIME_NSEC;
  timer_interval.tv_sec = LIGHT_LOOP_TIME_SEC;
  setupTimer(&set, &timerid, signum, &timer_interval);

  /* block SIGRTs signals */
  sigemptyset(&mask);

  for(ind = 0; ind < NUM_THREADS; ++ind)
  {
    if(ind != (uint8_t)PID_REMOTE)
      sigaddset(&mask, SIGRTMIN + ind);
  }
  pthread_sigmask(SIG_BLOCK, &mask, NULL);

  /* Open FDs for Main and Logging Message queues */
  logMsgQueue = mq_open(sensorInfo.logMsgQueueName, O_RDWR, 0666, mqAttr);
  hbMsgQueue = mq_open(sensorInfo.heartbeatMsgQueueName, O_RDWR, 0666, mqAttr);
  if(logMsgQueue == -1){
    printf("ERROR: remoteThread Failed to Open Logging MessageQueue - exiting.\n");
    LOG_REMOTE_HANDLING_EVENT(REMOTE_EVENT_ERROR);
    return NULL;
  }
  if(hbMsgQueue == -1) {
    printf("ERROR: remoteThread Failed to Open heartbeat MessageQueue - exiting.\n");
    LOG_REMOTE_HANDLING_EVENT(REMOTE_EVENT_ERROR);
    return NULL;
  }

  /* Setup Shared memory for thread */
  sharedMemFd = shm_open(sensorInfo.sensorSharedMemoryName, O_RDWR, 0666);
  if(sharedMemFd == -1) {
    printf("ERROR: remoteThread Failed to Open heartbeat MessageQueue - exiting.\n");
    LOG_REMOTE_HANDLING_EVENT(REMOTE_EVENT_ERROR);
    return NULL;
  }

  /* Memory map the shared memory */
  sharedMemPtr = mmap(0, sensorInfo.sharedMemSize, PROT_READ | PROT_WRITE, MAP_SHARED, sharedMemFd, 0);
  if(*(int *)sharedMemPtr == -1) {
    printf("ERROR: remoteThread Failed to complete memory mapping of shared memory - exiting\n");
    LOG_REMOTE_HANDLING_EVENT(REMOTE_EVENT_ERROR);
    return NULL;
  }

  /** Establish connection on remote socket **/
  /* Create Socket */
  sockfdServer = socket(AF_INET, SOCK_STREAM, 0);
  if(sockfdServer == -1){
    printf("ERROR: remoteThread failed to create socket - exiting.\n");
    LOG_REMOTE_HANDLING_EVENT(REMOTE_EVENT_ERROR);
    return NULL;
  }

  /* Update Socket Server connections to be non-blocking */
  socketFlags = fcntl(sockfdServer, F_GETFL);
  fcntl(sockfdServer, F_SETFL, socketFlags | O_NONBLOCK);
  /* Update Socket Client connections to be non-blocking */
  struct timeval timeout;
  timeout.tv_usec = LIGHT_LOOP_TIME_USEC;
  timeout.tv_sec = LIGHT_LOOP_TIME_SEC;

  /* Set properties and bind socket */
  servAddr.sin_family = AF_INET;
  servAddr.sin_addr.s_addr = INADDR_ANY;
  servAddr.sin_port = htons((int)PORT);
  if(bind(sockfdServer, (struct sockaddr*)&servAddr, sizeof(servAddr)) == -1) {
    printf("ERROR: remoteThread failed to bind socket - exiting.\n");
    LOG_REMOTE_HANDLING_EVENT(REMOTE_EVENT_ERROR);
    return NULL;
  }

  /* Listen for Client Connection */
  if(listen(sockfdServer, MAX_CLIENTS) == -1) {
    printf("ERROR: remoteThread failed to successfully listen for client connection - exiting.\n");
    LOG_REMOTE_HANDLING_EVENT(REMOTE_EVENT_ERROR);
    return NULL;
  }

  /* Log RemoteThread successfully created */
  printf("Created remoteThread to listen on port %d.\n", PORT);
  LOG_REMOTE_HANDLING_EVENT(REMOTE_EVENT_STARTED);

  while(aliveFlag) {
    /* Accept Client Connection */
    if(clientResponse == 0){
      sockfdClient = accept(sockfdServer, (struct sockaddr*)&cliAddr, &cliLen);
      if(sockfdClient == -1){
  
        /* Add non-blocking logic to allow remoteThread to report status while waiting for client conn */
        if(errno == EWOULDBLOCK) {
          SEND_STATUS_MSG(hbMsgQueue, PID_REMOTE, STATUS_ERROR, ERROR_CODE_USER_NONE0);
          sigwait(&set, &signum);
          continue;
        }

        /* Report error if client fails to connect to server */
        printf("ERROR: remoteThread failed to accept client connection for socket - exiting.\n");
        LOG_REMOTE_HANDLING_EVENT(REMOTE_EVENT_ERROR);
        continue;
      } else if(sockfdClient > 0) {
        /* Log RemoteThread successfully Connected to client */
        printf("Connected remoteThread to external Client on port %d.\n", PORT);
        LOG_REMOTE_HANDLING_EVENT(REMOTE_EVENT_CNCT_ACCEPTED);

        /* Update Socket Client connections to be non-blocking */
        setsockopt(sockfdClient, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof(struct timeval));
      }
    }

    /* Check for incoming commands from remote clients on socket port */
    clientResponse = recv(sockfdClient, &cmdPacket, sizeof(struct RemoteCmdPacket), 0);
    if (clientResponse == -1) { 

      /* Non-blocking logic to allow remoteThread to report status while waiting for client cmd */
      if(errno == EWOULDBLOCK) {
        SEND_STATUS_MSG(hbMsgQueue, PID_REMOTE, STATUS_ERROR, ERROR_CODE_USER_NONE0);
        sigwait(&set, &signum);
        continue;
      }

      /* Handle error with receiving data from client socket */
      printf("ERROR: remoteThread failed to handle incoming command from remote client.\n");
      LOG_REMOTE_HANDLING_EVENT(REMOTE_EVENT_ERROR);
      continue;
    } else if(clientResponse == 0) { 
      /* Handle disconnect from client socket */
      printf("WARNING: remoteThread connection lost with client on port %d.\n", PORT);
      LOG_REMOTE_HANDLING_EVENT(REMOTE_EVENT_CNCT_LOST);
      continue;
    }

    /* Read from Shared Memory and pass requested data back to client */
    pthread_mutex_lock(sensorInfo.sharedMemMutex);
    memcpy(&tempData, (sharedMemPtr+sensorInfo.tempDataOffset), sizeof(struct TempDataStruct));
    memcpy(&lightData, (sharedMemPtr+sensorInfo.lightDataOffset), sizeof(struct LightDataStruct));
    pthread_mutex_unlock(sensorInfo.sharedMemMutex);

    /* Populate Cmd Response data based on received cmd */
    getCmdResponse(&cmdPacket);

    /* Transmit data packet back to remote client requesting data */
    if(send(sockfdClient, &cmdPacket, sizeof(struct RemoteCmdPacket), 0) == -1) {
      printf("ERROR: remoteThread failed to handle outgoing requested data to remote client.\n");
      LOG_REMOTE_HANDLING_EVENT(REMOTE_EVENT_ERROR);
      continue;
    }

    /* TODO - derive method to set status sent to main */
    SEND_STATUS_MSG(hbMsgQueue, PID_REMOTE, STATUS_ERROR, ERROR_CODE_USER_NONE0);
  }

  /* Thread Cleanup */
  printf("remoteThread Cleanup\n");
  mq_close(logMsgQueue);
  mq_close(hbMsgQueue);
  close(sharedMemFd);
  close(sockfdClient);
  close(sockfdServer);

  return NULL;
}

/*---------------------------------------------------------------------------------*/
/* HELPER METHODS */
static void getCmdResponse(RemoteCmdPacket* packet){
  bool validCmdRecv = true;

  /* Based on received command, populate response to provide back to client */
  switch(packet->cmd) {
    case TEMPCMD_GETTEMP :
      packet->data.status_float = tempData.tmp102_temp;
      printf("TEMPCMD_GETTEMP cmd received | Transmitting data: {%.3f}\n", packet->data.status_float);
      break;
    case TEMPCMD_GETLOWTHRES :
      packet->data.status_float = tempData.tmp102_lowThreshold;
      printf("TEMPCMD_GETLOWTHRES cmd received | Transmitting data: {%.3f}\n", packet->data.status_float);
      break;
    case TEMPCMD_GETHIGHTHRES :
      packet->data.status_float = tempData.tmp102_highThreshold;
      printf("TEMPCMD_GETHIGHTHRES cmd received | Transmitting data: {%.3f}\n", packet->data.status_float);
      break;
    case TEMPCMD_GETCONFIG :
      packet->data.status_float = tempData.tmp102_config;
      printf("TEMPCMD_GETCONFIG cmd received | Transmitting data: {%.3f}\n", packet->data.status_float);
      break;
    case TEMPCMD_GETRESOLUTION :
      packet->data.status_float = tempData.tmp102_tempResolution;
      printf("TEMPCMD_GETRESOLUTION cmd received | Transmitting data: {%.3f}\n", packet->data.status_float);
      break;
    case TEMPCMD_GETFAULT :
      packet->data.status_uint32 = tempData.tmp102_fault;
      printf("TEMPCMD_GETFAULT case cmd received | Transmitting data: {%d}\n", packet->data.status_uint32);
      break;
    case TEMPCMD_GETEXTMODE :
      packet->data.status_uint32 = tempData.tmp102_extendedMode;
      printf("TEMPCMD_GETEXTMODE cmd received | Transmitting data: {%d}\n", packet->data.status_uint32);
      break;
    case TEMPCMD_GETSHUTDOWNMODE :
      packet->data.status_uint32 = tempData.tmp102_shutdownMode;
      printf("TEMPCMD_GETSHUTDOWNMODE cmd received | Transmitting data: {%d}\n", packet->data.status_uint32);
      break;
    case TEMPCMD_GETALERT :
      packet->data.status_uint32 = tempData.tmp102_alert;
      printf("TEMPCMD_GETALERT cmd received | Transmitting data: {%d}\n", packet->data.status_uint32);
      break;
    case TEMPCMD_GETCONVRATE :
      packet->data.status_uint32 = tempData.tmp102_convRate;
      printf("TEMPCMD_GETCONVRATE cmd received | Transmitting data: {%d}\n", packet->data.status_uint32);
      break;
    case LIGHTCMD_GETLUXDATA :
      packet->data.status_float = lightData.apds9301_luxData;
      printf("LIGHTCMD_GETLUXDATA cmd received | Transmitting data: {%f}\n", packet->data.status_float);
      break;
    case LIGHTCMD_GETDEVPARTNO :
      packet->data.status_uint32 = lightData.apds9301_devicePartNo;
      printf("LIGHTCMD_GETDEVPARTNO cmd received | Transmitting data: {0x%x}\n", packet->data.status_uint32);
      break;
    case LIGHTCMD_GETDEVREVNO :
      packet->data.status_uint32 = lightData.apds9301_deviceRevNo;
      printf("LIGHTCMD_GETDEVREVNO cmd received | Transmitting data: {0x%x}\n", packet->data.status_uint32);
      break;
    case LIGHTCMD_GETPOWERCTRL :
      packet->data.status_uint32 = lightData.apds9301_powerControl;
      printf("LIGHTCMD_GETPOWERCTRL cmd received | Transmitting data: {0x%x}\n", packet->data.status_uint32);
      break;
    case LIGHTCMD_GETTIMINGGAIN :
      packet->data.status_uint32 = lightData.apds9301_timingGain;
      printf("LIGHTCMD_GETTIMINGGAIN cmd received | Transmitting data: {0x%x}\n", packet->data.status_uint32);
      break;
    case LIGHTCMD_GETTIMINGINTEGRATION :
      packet->data.status_uint32 = lightData.apds9301_timingIntegration;
      printf("LIGHTCMD_GETTIMINGINTEGRATION cmd received | Transmitting data: {0x%x}\n", packet->data.status_uint32);
      break;
    case LIGHTCMD_GETINTSELECT :
      packet->data.status_uint32 = lightData.apds9301_intSelect;
      printf("LIGHTCMD_GETINTSELECT cmd received | Transmitting data: {0x%x}\n", packet->data.status_uint32);
      break;
    case LIGHTCMD_GETINTPERSIST :
      packet->data.status_uint32 = lightData.apds9301_intPersist;
      printf("LIGHTCMD_GETINTPERSIST cmd received | Transmitting data: {0x%x}\n", packet->data.status_uint32);
      break;
    case LIGHTCMD_GETLOWTHRES :
      packet->data.status_uint32 = lightData.apds9301_intThresLow;
      printf("LIGHTCMD_GETLOWTHRES cmd received | Transmitting data: {0x%x}\n", packet->data.status_uint32);
      break;
    case LIGHTCMD_GETHIGHTHRES :
      packet->data.status_uint32 = lightData.apds9301_intThresHigh;
      printf("LIGHTCMD_GETHIGHTHRES cmd received | Transmitting data: {0x%x}\n", packet->data.status_uint32);
      break;
    default:
      printf("cmd received not recognized - cmd ignored\n");
      LOG_REMOTE_HANDLING_EVENT(REMOTE_EVENT_INVALID_RECV);
      validCmdRecv = false;
      break;
  }

  /* TODO: Below log causing 3-4 sec delay if too many commands are received - need to resolve */
  if(validCmdRecv)
    //LOG_REMOTE_HANDLING_EVENT(REMOTE_EVENT_CMD_RECV);

  return;
}
/*---------------------------------------------------------------------------------*/
