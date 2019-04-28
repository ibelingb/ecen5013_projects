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
 * @file remoteCmdThread.c
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
void remoteCmdGetAliveFlag(uint8_t *pAlive);

/* Define static and global variables */
static SensorThreadInfo sensorInfo;
static uint8_t aliveFlag = 1;

/*---------------------------------------------------------------------------------*/
void* remoteCmdThreadHandler(void* threadInfo)
{
  /* Validate input */
  if(threadInfo == NULL){
    ERROR_PRINT("remoteCmdThread Failed to initialize; NULL pointer for provided threadInfo parameter - exiting.\n");
    return NULL;
  }

  sensorInfo = *(SensorThreadInfo *)threadInfo;
  RemoteCmdPacket cmdPacket = {0};
  mqd_t logMsgQueue; /* logger MessageQueue */
  mqd_t hbMsgQueue;  /* main heartbeat MessageQueue */
  mqd_t cmdMsgQueue;  /* Cmd MessageQueue */
  struct mq_attr mqAttr;
  int sockfdCmdServer, sockfdCmdClient, socketCmdFlags;
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
  timer_interval.tv_nsec = REMOTE_LOOP_TIME_NSEC;
  timer_interval.tv_sec = REMOTE_LOOP_TIME_SEC;
  setupTimer(&set, &timerid, signum, &timer_interval);

  /* block SIGRTs signals */
  sigemptyset(&mask);
  for(ind = 0; ind < NUM_THREADS; ++ind)
  {
    if(ind != (uint8_t)PID_REMOTE_CMD)
      sigaddset(&mask, SIGRTMIN + ind);
  }
  pthread_sigmask(SIG_BLOCK, &mask, NULL);

  LOG_REMOTE_CMD_EVENT(REMOTE_EVENT_STARTED);

  /* Open FDs for Main and Logging Message queues */
  logMsgQueue = mq_open(sensorInfo.logMsgQueueName, O_RDWR, 0666, mqAttr);
  hbMsgQueue = mq_open(sensorInfo.heartbeatMsgQueueName, O_RDWR, 0666, mqAttr);
  cmdMsgQueue = mq_open(sensorInfo.cmdMsgQueueName, O_RDWR | O_NONBLOCK, 0666, mqAttr);
  if(logMsgQueue == -1){
    ERROR_PRINT("remoteCmdThread Failed to Open Logging MessageQueue - exiting.\n");
    LOG_REMOTE_CMD_EVENT(REMOTE_LOG_QUEUE_ERROR);
    return NULL;
  }
  if(hbMsgQueue == -1) {
    ERROR_PRINT("remoteCmdThread Failed to Open heartbeat MessageQueue - exiting.\n");
    LOG_REMOTE_CMD_EVENT(REMOTE_STATUS_QUEUE_ERROR);
    return NULL;
  }
  if(cmdMsgQueue == -1) {
    ERROR_PRINT("remoteCmdThread Failed to Open Command MessageQueue - exiting.\n");
    LOG_REMOTE_CMD_EVENT(REMOTE_STATUS_QUEUE_ERROR);
    return NULL;
  }

  /** Establish connection on remote socket **/
  /* Create Server Socket Interfaces */
  sockfdCmdServer = socket(AF_INET, SOCK_STREAM, 0);
  if(sockfdCmdServer == -1){
    ERROR_PRINT("remoteCmdThread failed to create Data Socket - exiting.\n");
    LOG_REMOTE_CMD_EVENT(REMOTE_SERVER_SOCKET_ERROR);
    return NULL;
  }

  /* Update Socket Server connections to be non-blocking */
  socketCmdFlags = fcntl(sockfdCmdServer, F_GETFL);
  fcntl(sockfdCmdServer, F_SETFL, socketCmdFlags | O_NONBLOCK);

  /* Update Socket Client connections to be non-blocking */
  struct timeval timeout;
  timeout.tv_usec = REMOTE_CLIENT_TIMEOUT_USEC;
  timeout.tv_sec = 0;

  /* Set properties and bind socket */
  servAddr.sin_family = AF_INET;
  servAddr.sin_addr.s_addr = INADDR_ANY;
  servAddr.sin_port = htons((int)CMD_PORT);
  if(bind(sockfdCmdServer, (struct sockaddr*)&servAddr, sizeof(servAddr)) == -1) {
    ERROR_PRINT("remoteCmdThread failed to bind Data Socket - exiting.\n");
    LOG_REMOTE_CMD_EVENT(REMOTE_SERVER_SOCKET_ERROR);
    return NULL;
  }

  /* Listen for Client Connection */
  if(listen(sockfdCmdServer, MAX_CLIENTS) == -1) {
    ERROR_PRINT("remoteCmdThread failed to successfully listen for Cmd Client connection - exiting.\n");
    LOG_REMOTE_CMD_EVENT(REMOTE_SERVER_SOCKET_ERROR);
    return NULL;
  }

  /* Log remoteCmdThread successfully created */
  INFO_PRINT("Created remoteCmdThread to listen on port {%d}\n", CMD_PORT);
  MUTED_PRINT("remoteCmdThread started successfully, pid: %d, SIGRTMIN+PID_e: %d\n",(pid_t)syscall(SYS_gettid), SIGRTMIN + PID_REMOTE_CMD);

  /* BIST/Power-on Test 
   *    - Waiting for client connection at this point;
   *    - Verify able to lock mutex and communicate with Shared Memory.
   */
  // TODO: TBD
  LOG_REMOTE_CMD_EVENT(REMOTE_INIT_SUCCESS);

  int status = 0;

  while(aliveFlag) {
    SEND_STATUS_MSG(hbMsgQueue, PID_REMOTE_CMD, STATUS_OK, ERROR_CODE_USER_NONE0);
    sigwait(&set, &signum);

    /* If cmd received, transmit packet to Remote Node */
    if(mq_receive(cmdMsgQueue, (char *)&cmdPacket, cmdPacketSize, NULL) == cmdPacketSize)
    {
      if(clientResponse != 0) {
        status = send(sockfdCmdClient, &cmdPacket, cmdPacketSize, 0);
        printf("remoteCmd sent. Cmd Packet: cmd: %d | Data: %d | Status: %d\n", cmdPacket.cmd, cmdPacket.data, status);
      }
      else {
        ERROR_PRINT("Failed to send CmdPacket to Remote Node - client socket connection unavailable\n");
        // TODO LOG failed to send cmd
      }

      /* Log cmd packet transmitted to remote node */
      // TODO
    }

    /* Accept Client Connection for Sensor data */
    if(clientResponse == 0)
    {
      sockfdCmdClient = accept(sockfdCmdServer, (struct sockaddr*)&cliAddr, &cliLen);
      if(sockfdCmdClient == -1){
        /* Add non-blocking logic to allow remoteCmdThread to report status while waiting for client conn */
        if(errno == EWOULDBLOCK) {
          continue;
        }

        /* Report error if client fails to connect to server */
        ERROR_PRINT("remoteCmdThread failed to accept client connection for socket - exiting.\n");
        LOG_REMOTE_CMD_EVENT(REMOTE_CLIENT_SOCKET_ERROR);
        continue;
      } else if(sockfdCmdClient > 0) {
        /* Log remoteCmdThread successfully Connected to client */
        printf("Connected remoteCmdThread to external Client on port %d.\n", CMD_PORT);
        LOG_REMOTE_CMD_EVENT(REMOTE_EVENT_CNCT_ACCEPTED);

        /* Update Socket Client connections to be non-blocking */
        setsockopt(sockfdCmdClient, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof(struct timeval));
      }
    } 

    /* Read from socket to determine if client disconnect occurred */
    clientResponse = recv(sockfdCmdClient, &cmdPacket, cmdPacketSize, 0);
    if(clientResponse == 0) {
      /* Handle disconnect from client socket */
      printf("remoteCmdThread connection lost with client on port %d.\n", CMD_PORT);
      LOG_REMOTE_CMD_EVENT(REMOTE_EVENT_CNCT_LOST);
      continue;
    }
  }

  /* Thread Cleanup */
  LOG_REMOTE_CMD_EVENT(REMOTE_EVENT_EXITING);
  ERROR_PRINT("Remote thread exiting\n");
  timer_delete(timerid);
  mq_close(logMsgQueue);
  mq_close(hbMsgQueue);
  mq_close(cmdMsgQueue);
  close(sockfdCmdClient);
  close(sockfdCmdServer);

  return NULL;
}

/*---------------------------------------------------------------------------------*/
void remoteCmdSigHandler(int signo, siginfo_t *info, void *extra)
{
  if((info != NULL) && (extra != NULL))
  {
    INFO_PRINT("remoteCmdSigHandler, signum: %d\n",info->si_signo);
    aliveFlag = 0;
  }
}

/*---------------------------------------------------------------------------------*/
void remoteCmdGetAliveFlag(uint8_t *pAlive)
{
  if(pAlive != NULL)
    *pAlive = aliveFlag;
}

/*---------------------------------------------------------------------------------*/
/* HELPER METHODS */
/*---------------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------------*/
