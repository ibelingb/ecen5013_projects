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
 * @file remoteLogThread.c
 * @brief Remote Log Thread Library
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
void remoteLogGetAliveFlag(uint8_t *pAlive);

/* Define static and global variables */
static SensorThreadInfo sensorInfo;
static uint8_t aliveFlag = 1;

/*---------------------------------------------------------------------------------*/
void* remoteLogThreadHandler(void* threadInfo)
{
  /* Validate input */
  if(threadInfo == NULL){
    ERROR_PRINT("remoteLogThread Failed to initialize; NULL pointer for provided threadInfo parameter - exiting.\n");
    return NULL;
  }

  sensorInfo = *(SensorThreadInfo *)threadInfo;
  LogMsgPacket logPacket = {0};
  mqd_t logMsgQueue; /* logger MessageQueue */
  mqd_t hbMsgQueue;  /* main heartbeat MessageQueue */
  struct mq_attr mqAttr;
  int sockfdLogServer, sockfdLogClient, socketLogFlags;
  struct sockaddr_in servAddr, cliAddr;
  unsigned int cliLen = sizeof(cliAddr);
  size_t logPacketSize = sizeof(LogMsgPacket);
  ssize_t clientResponse = 0; /* Used to determine if client has disconnected from server */
  uint8_t ind;
	sigset_t mask;
  logItem_t tmpItem;

  /* timer variables */
  timer_t timerid;
  sigset_t set;
  struct timespec timer_interval;
  int signum = SIGALRM;

  /* Setup Timer */
  memset(&set, 0, sizeof(sigset_t));
  memset(&timerid, 0, sizeof(timer_t));
  memset(&tmpItem, 0, sizeof(logItem_t));
  timer_interval.tv_nsec = REMOTE_LOOP_TIME_NSEC;
  timer_interval.tv_sec = REMOTE_LOOP_TIME_SEC;
  setupTimer(&set, &timerid, signum, &timer_interval);

  /* block SIGRTs signals */
  sigemptyset(&mask);
  for(ind = 0; ind < NUM_THREADS; ++ind)
  {
    if(ind != (uint8_t)PID_REMOTE_LOG)
      sigaddset(&mask, SIGRTMIN + ind);
  }
  pthread_sigmask(SIG_BLOCK, &mask, NULL);

  LOG_REMOTE_LOG_EVENT(REMOTE_EVENT_STARTED);

  /* Open FDs for Main and Logging Message queues */
  logMsgQueue = mq_open(sensorInfo.logMsgQueueName, O_RDWR, 0666, mqAttr);
  hbMsgQueue = mq_open(sensorInfo.heartbeatMsgQueueName, O_RDWR, 0666, mqAttr);
  if(logMsgQueue == -1){
    ERROR_PRINT("remoteLogThread Failed to Open Logging MessageQueue - exiting.\n");
    LOG_REMOTE_LOG_EVENT(REMOTE_LOG_QUEUE_ERROR);
    return NULL;
  }
  if(hbMsgQueue == -1) {
    ERROR_PRINT("remoteLogThread Failed to Open heartbeat MessageQueue - exiting.\n");
    LOG_REMOTE_LOG_EVENT(REMOTE_STATUS_QUEUE_ERROR);
    return NULL;
  }

  /** Establish connection on remote socket **/
  /* Create Server Socket Interfaces */
  sockfdLogServer = socket(AF_INET, SOCK_STREAM, 0);
  if(sockfdLogServer == -1){
    ERROR_PRINT("remoteLogThread failed to create Log Socket - exiting.\n");
    LOG_REMOTE_LOG_EVENT(REMOTE_SERVER_SOCKET_ERROR);
    return NULL;
  }

  /* Update Socket Server connections to be non-blocking */
  socketLogFlags = fcntl(sockfdLogServer, F_GETFL);
  fcntl(sockfdLogServer, F_SETFL, socketLogFlags | O_NONBLOCK);

  /* Update Socket Client connections to be non-blocking */
  struct timeval timeout;
  timeout.tv_usec = REMOTE_CLIENT_TIMEOUT_USEC;
  timeout.tv_sec = 0;

  /* Set properties and bind socket */
  servAddr.sin_family = AF_INET;
  servAddr.sin_addr.s_addr = INADDR_ANY;
  servAddr.sin_port = htons((int)LOG_PORT);
  if(bind(sockfdLogServer, (struct sockaddr*)&servAddr, sizeof(servAddr)) == -1) {
    ERROR_PRINT("remoteLogThread failed to bind Log Socket - exiting.\n");
    LOG_REMOTE_LOG_EVENT(REMOTE_SERVER_SOCKET_ERROR);
    return NULL;
  }

  /* Listen for Client Connection */
  if(listen(sockfdLogServer, MAX_CLIENTS) == -1) {
    ERROR_PRINT("remoteLogThread failed to successfully listen for Sensor Client connection - exiting.\n");
    LOG_REMOTE_LOG_EVENT(REMOTE_SERVER_SOCKET_ERROR);
    return NULL;
  }

  /* Log remoteLogThread successfully created */
  INFO_PRINT("Created remoteLogThread to listen on port {%d}\n", LOG_PORT);
  MUTED_PRINT("remoteLogThread started successfully, pid: %d, SIGRTMIN+PID_e: %d\n",(pid_t)syscall(SYS_gettid), SIGRTMIN + PID_REMOTE_LOG);

  /* BIST/Power-on Test 
   *    - Waiting for client connection at this point;
   *    - Verify able to lock mutex and communicate with Shared Memory.
   */
  // TODO: TBD

  while(aliveFlag) {
    SEND_STATUS_MSG(hbMsgQueue, PID_REMOTE_LOG, STATUS_OK, ERROR_CODE_USER_NONE0);
    sigwait(&set, &signum);

    /* Accept Client Connection for Log data */
    if(clientResponse == 0)
    {
      sockfdLogClient = accept(sockfdLogServer, (struct sockaddr*)&cliAddr, &cliLen);
      if(sockfdLogClient == -1){
        /* Add non-blocking logic to allow remoteLogThread to report status while waiting for client conn */
        if(errno == EWOULDBLOCK) {
          continue;
        }

        /* Report error if client fails to connect to server */
        ERROR_PRINT("remoteLogThread failed to accept client connection for socket - exiting.\n");
        LOG_REMOTE_LOG_EVENT(REMOTE_CLIENT_SOCKET_ERROR);
        continue;
      } else if(sockfdLogClient > 0) {
        /* Log remoteLogThread successfully Connected to client */
        printf("Connected remoteLogThread to external Client on port %d.\n", LOG_PORT);
        LOG_REMOTE_LOG_EVENT(REMOTE_EVENT_CNCT_ACCEPTED);

        /* Update Socket Client connections to be non-blocking */
        setsockopt(sockfdLogClient, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof(struct timeval));
      }
    }

    /* Check for incoming commands from remote clients on socket port */
    clientResponse = recv(sockfdLogClient, &logPacket, logPacketSize, 0);
    if (clientResponse == -1) 
    {
      /* Non-blocking logic to allow remoteLogThread to report status while waiting for client cmd */
      if(errno == EWOULDBLOCK) {
        continue;
      }
      ERRNO_PRINT("remoteLogThread recv fail");

      /* Handle error with receiving data from client socket */
      ERROR_PRINT("remoteLogThread failed to handle incoming command from remote client.\n");
      LOG_REMOTE_LOG_EVENT(REMOTE_CLIENT_SOCKET_ERROR);
      continue;
    } else if(clientResponse == 0) { 
      /* Handle disconnect from client socket */
      printf("remoteLogThread connection lost with client on port %d.\n", LOG_PORT);
      LOG_REMOTE_LOG_EVENT(REMOTE_EVENT_CNCT_LOST);
      continue;
    }

    /* Verify bytes received is the expected size for a logPacket */
    if(clientResponse != logPacketSize){
      ERROR_PRINT("remoteLogThread received cmd of invalid length from remote client.\n"
                  "Expected {%d} | Received {%d}", logPacketSize, clientResponse);
      LOG_REMOTE_LOG_EVENT(REMOTE_EVENT_INVALID_RECV);
      continue;
    }
    tmpItem.logMsgId = logPacket.logMsgId;
    tmpItem.lineNum = logPacket.lineNum;
    tmpItem.checksum = logPacket.checksum;
    tmpItem.payloadLength = logPacket.payloadLength;
    tmpItem.time = logPacket.timestamp;
    tmpItem.sourceId = logPacket.sourceId;
    tmpItem.pPayload = (uint8_t *)&logPacket.payload;
    tmpItem.pFilename = (uint8_t *)&logPacket.filename;

    /* Write received log packet from RemoteNode to logger */
    if(LOG_ITEM(&tmpItem) != LOG_STATUS_OK) {
      INFO_PRINT("LOG_ITEM write error\n");
    }
  }

  /* Thread Cleanup */
  LOG_REMOTE_LOG_EVENT(REMOTE_EVENT_EXITING);
  ERROR_PRINT("Remote Log Thread exiting\n");
  timer_delete(timerid);
  mq_close(logMsgQueue);
  mq_close(hbMsgQueue);
  shutdown(sockfdLogClient, SHUT_RDWR);
  shutdown(sockfdLogServer, SHUT_RDWR);
  sleep(1);
  uint8_t closeCnt = 0;
  while(closeCnt++ < 8) {
    close(sockfdLogClient);
    close(sockfdLogServer);
    usleep(100000);
  }

  return NULL;
}

/*---------------------------------------------------------------------------------*/
void remoteLogSigHandler(int signo, siginfo_t *info, void *extra)
{
  if((info != NULL) && (extra != NULL))
  {
    INFO_PRINT("remoteLogSigHandler, signum: %d\n",info->si_signo);
    aliveFlag = 0;
  }
}

/*---------------------------------------------------------------------------------*/
void remoteLogGetAliveFlag(uint8_t *pAlive)
{
  if(pAlive != NULL)
    *pAlive = aliveFlag;
}

/*---------------------------------------------------------------------------------*/
/* HELPER METHODS */
/*---------------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------------*/
