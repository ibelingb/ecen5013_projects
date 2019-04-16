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
#include "my_debug.h"
#include "cmn_timer.h"
#include "platform.h"
#include "healthMonitor.h"

#define MAX_CLIENTS (5)

/* Prototypes for private/helper functions */
static void getCmdResponse(RemoteCmdPacket* packet);

/* Define static and global variables */
static SensorThreadInfo sensorInfo;
static LightDataStruct lightData; /* Used to read data from Shared Memory */
//static TempDataStruct tempData;  /* Used to read data from Shared Memory */
static uint8_t aliveFlag = 1;

/*---------------------------------------------------------------------------------*/
void* remoteThreadHandler(void* threadInfo)
{
  /* Validate input */
  if(threadInfo == NULL){
    ERROR_PRINT("remoteThread Failed to initialize; NULL pointer for provided threadInfo parameter - exiting.\n");
    return NULL;
  }

  sensorInfo = *(SensorThreadInfo *)threadInfo;
  RemoteCmdPacket cmdPacket = {0};
  mqd_t logMsgQueue; /* logger MessageQueue */
  mqd_t hbMsgQueue;  /* main heartbeat MessageQueue */
  struct mq_attr mqAttr;
  //void* sharedMemPtr = NULL;
  //int sharedMemFd;
  int sockfdSensorServer, sockfdSensorClient, socketSensorFlags;
  struct sockaddr_in servAddr, cliAddr;
  unsigned int cliLen = sizeof(cliAddr);
  size_t cmdPacketSize = sizeof(struct RemoteCmdPacket);
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

  LOG_REMOTE_HANDLING_EVENT(REMOTE_EVENT_STARTED);

  /* Open FDs for Main and Logging Message queues */
  logMsgQueue = mq_open(sensorInfo.logMsgQueueName, O_RDWR, 0666, mqAttr);
  hbMsgQueue = mq_open(sensorInfo.heartbeatMsgQueueName, O_RDWR, 0666, mqAttr);
  if(logMsgQueue == -1){
    ERROR_PRINT("remoteThread Failed to Open Logging MessageQueue - exiting.\n");
    LOG_REMOTE_HANDLING_EVENT(REMOTE_LOG_QUEUE_ERROR);
    return NULL;
  }
  if(hbMsgQueue == -1) {
    ERROR_PRINT("remoteThread Failed to Open heartbeat MessageQueue - exiting.\n");
    LOG_REMOTE_HANDLING_EVENT(REMOTE_STATUS_QUEUE_ERROR);
    return NULL;
  }

  /* Setup Shared memory for thread */
  /*
  sharedMemFd = shm_open(sensorInfo.sensorSharedMemoryName, O_RDWR, 0666);
  if(sharedMemFd == -1) {
    ERROR_PRINT("remoteThread Failed to Open heartbeat MessageQueue - exiting.\n");
    LOG_REMOTE_HANDLING_EVENT(REMOTE_SHMEM_ERROR);
    return NULL;
  }
  */

  /* Memory map the shared memory */
  /*
  sharedMemPtr = mmap(0, sensorInfo.sharedMemSize, PROT_READ | PROT_WRITE, MAP_SHARED, sharedMemFd, 0);
  if(*(int *)sharedMemPtr == -1) {
    ERROR_PRINT("remoteThread Failed to complete memory mapping of shared memory - exiting\n");
    LOG_REMOTE_HANDLING_EVENT(REMOTE_SHMEM_ERROR);
    return NULL;
  }
  */

  /** Establish connection on remote socket **/
  /* Create Server Socket Interfaces */
  sockfdSensorServer = socket(AF_INET, SOCK_STREAM, 0);
  if(sockfdSensorServer == -1){
    ERROR_PRINT("remoteThread failed to create Data Socket - exiting.\n");
    LOG_REMOTE_HANDLING_EVENT(REMOTE_SERVER_SOCKET_ERROR);
    return NULL;
  }

  /* Update Socket Server connections to be non-blocking */
  socketSensorFlags = fcntl(sockfdSensorServer, F_GETFL);
  fcntl(sockfdSensorServer, F_SETFL, socketSensorFlags | O_NONBLOCK);

  /* Update Socket Client connections to be non-blocking */
  struct timeval timeout;
  timeout.tv_usec = REMOTE_CLIENT_TIMEOUT_USEC;
  timeout.tv_sec = 0;

  /* Set properties and bind socket */
  servAddr.sin_family = AF_INET;
  servAddr.sin_addr.s_addr = INADDR_ANY;
  servAddr.sin_port = htons((int)SENSOR_PORT);
  if(bind(sockfdSensorServer, (struct sockaddr*)&servAddr, sizeof(servAddr)) == -1) {
    ERROR_PRINT("remoteThread failed to bind Data Socket - exiting.\n");
    LOG_REMOTE_HANDLING_EVENT(REMOTE_SERVER_SOCKET_ERROR);
    return NULL;
  }

  /* Listen for Client Connection */
  if(listen(sockfdSensorServer, MAX_CLIENTS) == -1) {
    ERROR_PRINT("remoteThread failed to successfully listen for Sensor Client connection - exiting.\n");
    LOG_REMOTE_HANDLING_EVENT(REMOTE_SERVER_SOCKET_ERROR);
    return NULL;
  }

  /* Log RemoteThread successfully created */
  INFO_PRINT("Created remoteThread to listen on the following ports | Sensor:{%d} | Cmd:{%d} | Log:{%d} |\n", SENSOR_PORT, CMD_PORT, LOG_PORT);
  MUTED_PRINT("remoteThread started successfully, pid: %d, SIGRTMIN+PID_e: %d\n",(pid_t)syscall(SYS_gettid), SIGRTMIN + PID_REMOTE);

  /* BIST/Power-on Test 
   *    - Waiting for client connection at this point;
   *    - Verify able to lock mutex and communicate with Shared Memory.
   */
  /* Write invalid/inaccurate values to tempData and lightData - verify overwritten by reading SHMEM */
  /*
  tempData.tmp102_temp = 5;
  tempData.overTempState = 5;
  lightData.apds9301_luxData = 5;
  lightData.lightState = 5;
  pthread_mutex_lock(sensorInfo.sharedMemMutex);
  memcpy(&tempData, (sharedMemPtr+sensorInfo.tempDataOffset), sizeof(struct TempDataStruct));
  memcpy(&lightData, (sharedMemPtr+sensorInfo.lightDataOffset), sizeof(struct LightDataStruct));
  pthread_mutex_unlock(sensorInfo.sharedMemMutex);
  if((tempData.tmp102_temp != 5) &&
     (tempData.overTempState != 5) &&
     (lightData.apds9301_luxData != 5) &&
     (lightData.lightState != 5))
  {
    LOG_REMOTE_HANDLING_EVENT(REMOTE_BIST_COMPLETE);
    LOG_REMOTE_HANDLING_EVENT(REMOTE_INIT_SUCCESS);
  } else {
  */
    /* Failed to read SHMEM and overwrite local objects */
  /*
    LOG_REMOTE_HANDLING_EVENT(REMOTE_BIST_COMPLETE);
    LOG_REMOTE_HANDLING_EVENT(REMOTE_INIT_ERROR);
    ERROR_PRINT("remoteThread initialization failed.\n");
    return NULL;
  }
  */
  /* Reset values to 0 */
  //tempData.tmp102_temp = 0;
  //tempData.overTempState = 0;
  lightData.apds9301_luxData = 0;
  lightData.lightState = 0;
  /* BIST/Power-on Test Complete */

  while(aliveFlag) {
    SEND_STATUS_MSG(hbMsgQueue, PID_REMOTE, STATUS_OK, ERROR_CODE_USER_NONE0);
    sigwait(&set, &signum);

    /* Accept Client Connection for Sensor data */
    if(clientResponse == 0)
    {
      sockfdSensorClient = accept(sockfdSensorServer, (struct sockaddr*)&cliAddr, &cliLen);
      if(sockfdSensorClient == -1){
        /* Add non-blocking logic to allow remoteThread to report status while waiting for client conn */
        if(errno == EWOULDBLOCK) {
          continue;
        }

        /* Report error if client fails to connect to server */
        ERROR_PRINT("remoteThread failed to accept client connection for socket - exiting.\n");
        LOG_REMOTE_HANDLING_EVENT(REMOTE_CLIENT_SOCKET_ERROR);
        continue;
      } else if(sockfdSensorClient > 0) {
        /* Log RemoteThread successfully Connected to client */
        printf("Connected remoteThread to external Client on port %d.\n", SENSOR_PORT);
        LOG_REMOTE_HANDLING_EVENT(REMOTE_EVENT_CNCT_ACCEPTED);

        /* Update Socket Client connections to be non-blocking */
        setsockopt(sockfdSensorClient, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof(struct timeval));
      }
    }

    /* Check for incoming commands from remote clients on socket port */
    clientResponse = recv(sockfdSensorClient, &cmdPacket, cmdPacketSize, 0);
    if (clientResponse == -1) 
    {
      /* Non-blocking logic to allow remoteThread to report status while waiting for client cmd */
      if(errno == EWOULDBLOCK) {
        continue;
      }

      /* Handle error with receiving data from client socket */
      ERROR_PRINT("remoteThread failed to handle incoming command from remote client.\n");
      LOG_REMOTE_HANDLING_EVENT(REMOTE_CLIENT_SOCKET_ERROR);
      continue;
    } else if(clientResponse == 0) { 
      /* Handle disconnect from client socket */
      printf("remoteThread connection lost with client on port %d.\n", SENSOR_PORT);
      LOG_REMOTE_HANDLING_EVENT(REMOTE_EVENT_CNCT_LOST);
      continue;
    }

    /* Verify bytes received is the expected size for a cmdPacket */
    if(clientResponse != cmdPacketSize){
      ERROR_PRINT("remoteThread received cmd of invalid length from remote client.\n"
             "Expected {%d} | Received {%d}", cmdPacketSize, clientResponse);
      LOG_REMOTE_HANDLING_EVENT(REMOTE_EVENT_INVALID_RECV);
      continue;
    }

    /* Read from Shared Memory and pass requested data back to client */
    /*
    pthread_mutex_lock(sensorInfo.sharedMemMutex);
    memcpy(&tempData, (sharedMemPtr+sensorInfo.tempDataOffset), sizeof(struct TempDataStruct));
    memcpy(&lightData, (sharedMemPtr+sensorInfo.lightDataOffset), sizeof(struct LightDataStruct));
    pthread_mutex_unlock(sensorInfo.sharedMemMutex);
    */

    /* Populate Cmd Response data based on received cmd */
    getCmdResponse(&cmdPacket);

    /* Transmit data packet back to remote client requesting data */
    if(send(sockfdSensorClient, &cmdPacket, sizeof(struct RemoteCmdPacket), 0) == -1) {
      ERROR_PRINT("remoteThread failed to handle outgoing requested data to remote client.\n");
      LOG_REMOTE_HANDLING_EVENT(REMOTE_CLIENT_SOCKET_ERROR);
      continue;
    }
  }

  /* Thread Cleanup */
  LOG_REMOTE_HANDLING_EVENT(REMOTE_EVENT_EXITING);
  ERROR_PRINT("Remote thread exiting\n");
  timer_delete(timerid);
  mq_close(logMsgQueue);
  mq_close(hbMsgQueue);
  //close(sharedMemFd);
  close(sockfdSensorClient);
  close(sockfdSensorServer);

  return NULL;
}

/*---------------------------------------------------------------------------------*/
void remoteSigHandler(int signo, siginfo_t *info, void *extra)
{
  if((info != NULL) && (extra != NULL))
  {
    INFO_PRINT("remoteSigHandler, signum: %d\n",info->si_signo);
    aliveFlag = 0;
  }
}

/*---------------------------------------------------------------------------------*/
void remoteGetAliveFlag(uint8_t *pAlive)
{
  if(pAlive != NULL)
    *pAlive = aliveFlag;
}

/*---------------------------------------------------------------------------------*/
/* HELPER METHODS */
/*---------------------------------------------------------------------------------*/
/**
 * @brief - Populate provided packet with requested command data from data read from SharedMemory.
 *
 * @param packet - Pointer to packet to populate and return to caller.
 * @return void
 */
static void getCmdResponse(RemoteCmdPacket* packet){
  if(packet == NULL){
    ERROR_PRINT("getCmdResponse() received NULL pointer for packet parameter - Process Cmd Response failed.\n");
    return;
  }

  /* Log cmd received */
  LOG_REMOTE_HANDLING_EVENT(REMOTE_EVENT_CMD_RECV);

  /* Based on received command, populate response to provide back to client */
  switch(packet->cmd) {
    /*
    case TEMPCMD_GETTEMP :
      packet->data.status_float = tempData.tmp102_temp;
      INFO_PRINT("TEMPCMD_GETTEMP cmd received | Transmitting data: {%.3f}\n", packet->data.status_float);
      break;
    case TEMPCMD_GETLOWTHRES :
      packet->data.status_float = tempData.tmp102_lowThreshold;
      INFO_PRINT("TEMPCMD_GETLOWTHRES cmd received | Transmitting data: {%.3f}\n", packet->data.status_float);
      break;
    case TEMPCMD_GETHIGHTHRES :
      packet->data.status_float = tempData.tmp102_highThreshold;
      INFO_PRINT("TEMPCMD_GETHIGHTHRES cmd received | Transmitting data: {%.3f}\n", packet->data.status_float);
      break;
    case TEMPCMD_GETCONFIG :
      packet->data.status_float = tempData.tmp102_config;
      INFO_PRINT("TEMPCMD_GETCONFIG cmd received | Transmitting data: {%.3f}\n", packet->data.status_float);
      break;
    case TEMPCMD_GETRESOLUTION :
      packet->data.status_float = tempData.tmp102_tempResolution;
      INFO_PRINT("TEMPCMD_GETRESOLUTION cmd received | Transmitting data: {%.3f}\n", packet->data.status_float);
      break;
    case TEMPCMD_GETFAULT :
      packet->data.status_uint32 = tempData.tmp102_fault;
      INFO_PRINT("TEMPCMD_GETFAULT case cmd received | Transmitting data: {%d}\n", packet->data.status_uint32);
      break;
    case TEMPCMD_GETEXTMODE :
      packet->data.status_uint32 = tempData.tmp102_extendedMode;
      INFO_PRINT("TEMPCMD_GETEXTMODE cmd received | Transmitting data: {%d}\n", packet->data.status_uint32);
      break;
    case TEMPCMD_GETSHUTDOWNMODE :
      packet->data.status_uint32 = tempData.tmp102_shutdownMode;
      INFO_PRINT("TEMPCMD_GETSHUTDOWNMODE cmd received | Transmitting data: {%d}\n", packet->data.status_uint32);
      break;
    case TEMPCMD_GETALERT :
      packet->data.status_uint32 = tempData.tmp102_alert;
      INFO_PRINT("TEMPCMD_GETALERT cmd received | Transmitting data: {%d}\n", packet->data.status_uint32);
      break;
    case TEMPCMD_GETCONVRATE :
      packet->data.status_uint32 = tempData.tmp102_convRate;
      INFO_PRINT("TEMPCMD_GETCONVRATE cmd received | Transmitting data: {%d}\n", packet->data.status_uint32);
      break;
    case TEMPCMD_GETOVERTEMPSTATE:
      packet->data.status_uint32 = tempData.overTempState;
      INFO_PRINT("TEMPCMD_GETOVERTEMPSTATE cmd received | Transmitting data: {%d}\n", packet->data.status_uint32);
      break;
      */
    case LIGHTCMD_GETLUXDATA :
      packet->data.status_float = lightData.apds9301_luxData;
      INFO_PRINT("LIGHTCMD_GETLUXDATA cmd received | Transmitting data: {%f}\n", packet->data.status_float);
      break;
    case LIGHTCMD_GETDEVPARTNO :
      packet->data.status_uint32 = lightData.apds9301_devicePartNo;
      INFO_PRINT("LIGHTCMD_GETDEVPARTNO cmd received | Transmitting data: {0x%x}\n", packet->data.status_uint32);
      break;
    case LIGHTCMD_GETDEVREVNO :
      packet->data.status_uint32 = lightData.apds9301_deviceRevNo;
      INFO_PRINT("LIGHTCMD_GETDEVREVNO cmd received | Transmitting data: {0x%x}\n", packet->data.status_uint32);
      break;
    case LIGHTCMD_GETPOWERCTRL :
      packet->data.status_uint32 = lightData.apds9301_powerControl;
      INFO_PRINT("LIGHTCMD_GETPOWERCTRL cmd received | Transmitting data: {0x%x}\n", packet->data.status_uint32);
      break;
    case LIGHTCMD_GETTIMINGGAIN :
      packet->data.status_uint32 = lightData.apds9301_timingGain;
      INFO_PRINT("LIGHTCMD_GETTIMINGGAIN cmd received | Transmitting data: {0x%x}\n", packet->data.status_uint32);
      break;
    case LIGHTCMD_GETTIMINGINTEGRATION :
      packet->data.status_uint32 = lightData.apds9301_timingIntegration;
      INFO_PRINT("LIGHTCMD_GETTIMINGINTEGRATION cmd received | Transmitting data: {0x%x}\n", packet->data.status_uint32);
      break;
    case LIGHTCMD_GETINTSELECT :
      packet->data.status_uint32 = lightData.apds9301_intSelect;
      INFO_PRINT("LIGHTCMD_GETINTSELECT cmd received | Transmitting data: {0x%x}\n", packet->data.status_uint32);
      break;
    case LIGHTCMD_GETINTPERSIST :
      packet->data.status_uint32 = lightData.apds9301_intPersist;
      INFO_PRINT("LIGHTCMD_GETINTPERSIST cmd received | Transmitting data: {0x%x}\n", packet->data.status_uint32);
      break;
    case LIGHTCMD_GETLOWTHRES :
      packet->data.status_uint32 = lightData.apds9301_intThresLow;
      INFO_PRINT("LIGHTCMD_GETLOWTHRES cmd received | Transmitting data: {0x%x}\n", packet->data.status_uint32);
      break;
    case LIGHTCMD_GETHIGHTHRES :
      packet->data.status_uint32 = lightData.apds9301_intThresHigh;
      INFO_PRINT("LIGHTCMD_GETHIGHTHRES cmd received | Transmitting data: {0x%x}\n", packet->data.status_uint32);
      break;
    case LIGHTCMD_GETLIGHTSTATE:
      packet->data.status_uint32 = lightData.lightState;
      INFO_PRINT("LIGHTCMD_GETLIGHTSTATE cmd received | Transmitting data: {0x%x}\n", packet->data.status_uint32);
      break;
    default:
      ERROR_PRINT("cmd received with value {%d} not recognized - cmd ignored\n", packet->cmd);
      LOG_REMOTE_HANDLING_EVENT(REMOTE_EVENT_INVALID_RECV);
      return;
  }

  /* Log which command was received */
  LOG_REMOTE_CMD_EVENT(packet->cmd);

  return;
}

/*---------------------------------------------------------------------------------*/
