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
 * @file remoteStatusThread.c
 * @brief Remote Status Thread Library
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
void remoteStatusGetAliveFlag(uint8_t *pAlive);

/* Define static and global variables */
static SensorThreadInfo sensorInfo;
static uint8_t aliveFlag = 1;

/*---------------------------------------------------------------------------------*/
void* remoteStatusThreadHandler(void* threadInfo)
{
  /* Validate input */
  if(threadInfo == NULL){
    ERROR_PRINT("remoteStatusThread Failed to initialize; NULL pointer for provided threadInfo parameter - exiting.\n");
    return NULL;
  }

  sensorInfo = *(SensorThreadInfo *)threadInfo;
  TaskStatusPacket statusPacket = {0};
  mqd_t logMsgQueue; /* logger MessageQueue */
  mqd_t hbMsgQueue;  /* main heartbeat MessageQueue */
  struct mq_attr mqAttr;
  int sockfdStatusServer, sockfdStatusClient, socketStatusFlags;
  struct sockaddr_in servAddr, cliAddr;
  unsigned int cliLen = sizeof(cliAddr);
  size_t statusPacketSize = sizeof(struct TaskStatusPacket);
  ssize_t clientResponse = 0; /* Used to determine if client has disconnected from server */
  uint8_t ind, statusMsgCount;
	sigset_t mask;
  struct timespec currentTime, lastStatusMsgTime;     /* to calc delta time */
  float deltaTime;                                    /* delta time since last sent status msg */

  /* how often healthMonitor expects status msgs */
  const float otherThreadTime = TEMP_LOOP_TIME_SEC + (TEMP_LOOP_TIME_NSEC * 1e-9);


  /* timer variables */
  timer_t timerid;
  sigset_t set;
  struct timespec timer_interval;
  int signum = SIGALRM;

  /* Setup Timer */
  memset(&set, 0, sizeof(sigset_t));
  memset(&timerid, 0, sizeof(timer_t));
  timer_interval.tv_nsec = REMOTE_LOOP_TIME_NSEC/4;
  timer_interval.tv_sec = REMOTE_LOOP_TIME_SEC/4;
  setupTimer(&set, &timerid, signum, &timer_interval);

  /* block SIGRTs signals */
  sigemptyset(&mask);
  for(ind = 0; ind < NUM_THREADS; ++ind)
  {
    if(ind != (uint8_t)PID_REMOTE_STATUS)
      sigaddset(&mask, SIGRTMIN + ind);
  }
  pthread_sigmask(SIG_BLOCK, &mask, NULL);

  LOG_REMOTE_STATUS_EVENT(REMOTE_EVENT_STARTED);

  /* Open FDs for Main and Logging Message queues */
  logMsgQueue = mq_open(sensorInfo.logMsgQueueName, O_RDWR, 0666, mqAttr);
  hbMsgQueue = mq_open(sensorInfo.heartbeatMsgQueueName, O_RDWR, 0666, mqAttr);
  if(logMsgQueue == -1){
    ERROR_PRINT("remoteStatusThread Failed to Open Logging MessageQueue - exiting.\n");
    LOG_REMOTE_STATUS_EVENT(REMOTE_LOG_QUEUE_ERROR);
    return NULL;
  }
  if(hbMsgQueue == -1) {
    ERROR_PRINT("remoteStatusThread Failed to Open heartbeat MessageQueue - exiting.\n");
    LOG_REMOTE_STATUS_EVENT(REMOTE_STATUS_QUEUE_ERROR);
    return NULL;
  }

  /** Establish connection on remote socket **/
  /* Create Server Socket Interfaces */
  sockfdStatusServer = socket(AF_INET, SOCK_STREAM, 0);
  if(sockfdStatusServer == -1){
    ERROR_PRINT("remoteStatusThread failed to create Data Socket - exiting.\n");
    LOG_REMOTE_STATUS_EVENT(REMOTE_SERVER_SOCKET_ERROR);
    return NULL;
  }

  /* Update Socket Server connections to be non-blocking */
  socketStatusFlags = fcntl(sockfdStatusServer, F_GETFL);
  fcntl(sockfdStatusServer, F_SETFL, socketStatusFlags | O_NONBLOCK);

  /* Update Socket Client connections to be non-blocking */
  struct timeval timeout;
  timeout.tv_usec = REMOTE_CLIENT_TIMEOUT_USEC;
  timeout.tv_sec = 0;

  /* Set properties and bind socket */
  servAddr.sin_family = AF_INET;
  servAddr.sin_addr.s_addr = INADDR_ANY;
  servAddr.sin_port = htons((int)STATUS_PORT);
  if(bind(sockfdStatusServer, (struct sockaddr*)&servAddr, sizeof(servAddr)) == -1) {
    ERROR_PRINT("remoteStatusThread failed to bind Data Socket - exiting.\n");
    LOG_REMOTE_STATUS_EVENT(REMOTE_SERVER_SOCKET_ERROR);
    return NULL;
  }

  /* Listen for Client Connection */
  if(listen(sockfdStatusServer, MAX_CLIENTS) == -1) {
    ERROR_PRINT("remoteStatusThread failed to successfully listen for Sensor Client connection - exiting.\n");
    LOG_REMOTE_STATUS_EVENT(REMOTE_SERVER_SOCKET_ERROR);
    return NULL;
  }

  /* Log remoteStatusThread successfully created */
  INFO_PRINT("Created remoteStatusThread to listen on port {%d}\n", STATUS_PORT);
  MUTED_PRINT("remoteStatusThread started successfully, pid: %d, SIGRTMIN+PID_e: %d\n",(pid_t)syscall(SYS_gettid), SIGRTMIN + PID_REMOTE_STATUS);

  /* BIST/Power-on Test 
   *    - Waiting for client connection at this point;
   *    - Verify able to lock mutex and communicate with Shared Memory.
   */
  // TODO: TBD

  while(aliveFlag) {
    statusMsgCount = 0;

    /* Accept Client Connection for Sensor data */
    if(clientResponse == 0)
    {
      sockfdStatusClient = accept(sockfdStatusServer, (struct sockaddr*)&cliAddr, &cliLen);
      if(sockfdStatusClient == -1){
        /* Add non-blocking logic to allow remoteStatusThread to report status while waiting for client conn */
        if(errno == EWOULDBLOCK) {
          continue;
        }

        /* Report error if client fails to connect to server */
        ERROR_PRINT("remoteStatusThread failed to accept client connection for socket - exiting.\n");
        LOG_REMOTE_STATUS_EVENT(REMOTE_CLIENT_SOCKET_ERROR);
        continue;
      } else if(sockfdStatusClient > 0) {
        /* Log remoteStatusThread successfully Connected to client */
        printf("Connected remoteStatusThread to external Client on port %d.\n", STATUS_PORT);
        LOG_REMOTE_STATUS_EVENT(REMOTE_EVENT_CNCT_ACCEPTED);

        /* Update Socket Client connections to be non-blocking */
        setsockopt(sockfdStatusClient, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof(struct timeval));
      }
    }

    /* Check for incoming status packets from remote clients on socket port */
    clientResponse = recv(sockfdStatusClient, &statusPacket, statusPacketSize, 0);
    if (clientResponse == -1)
    {
      /* Non-blocking logic to allow remoteStatusThread to report status while waiting for client cmd */
      if(errno != EWOULDBLOCK) {
        ERRNO_PRINT("remoteStatusThread recv fail");
      
        /* Handle error with receiving data from client socket */
        ERROR_PRINT("remoteStatusThread failed to handle incoming status packet from remote node.\n");
        LOG_REMOTE_STATUS_EVENT(REMOTE_CLIENT_SOCKET_ERROR);
      }
    } 
    else if(clientResponse == 0) { 
      /* Handle disconnect from client socket */
      printf("remoteStatusThread connection lost with client on port %d.\n", STATUS_PORT);
      LOG_REMOTE_STATUS_EVENT(REMOTE_EVENT_CNCT_LOST);
    }
    else if(clientResponse != statusPacketSize){
      ERROR_PRINT("remoteStatusThread received cmd of invalid length from remote client.\n"
             "Expected {%d} | Received {%d}", statusPacketSize, clientResponse);
      LOG_REMOTE_STATUS_EVENT(REMOTE_EVENT_INVALID_RECV);
    }
    /* Received Msg is the expected size for a statusPacket */
    else {
      /* Receive status packets from TIVA tasks, push onto heartbeat queue */
      SEND_STATUS_MSG(hbMsgQueue, statusPacket.processId, statusPacket.taskStatus, statusPacket.errorCode);
    }

    /* calculate delta time (since last status msg TX) */
    clock_gettime(CLOCK_REALTIME, &currentTime);
    deltaTime = (currentTime.tv_sec - lastStatusMsgTime.tv_sec) + ((currentTime.tv_nsec - lastStatusMsgTime.tv_nsec) * 1e-9);

    /* only send OK status at rate of other threads 
    * and if we didn't send error status yet */
    if((deltaTime > otherThreadTime) && (statusMsgCount == 0)) {
        SEND_STATUS_MSG(hbMsgQueue, PID_REMOTE_STATUS, STATUS_OK, ERROR_CODE_USER_NONE0);
        ++statusMsgCount;
        clock_gettime(CLOCK_REALTIME, &lastStatusMsgTime);
    }

    sigwait(&set, &signum);
  }

  /* Thread Cleanup */
  LOG_REMOTE_STATUS_EVENT(REMOTE_EVENT_EXITING);
  ERROR_PRINT("Remote Status Thread exiting\n");
  timer_delete(timerid);
  mq_close(logMsgQueue);
  mq_close(hbMsgQueue);
  close(sockfdStatusClient);
  close(sockfdStatusServer);

  return NULL;
}

/*---------------------------------------------------------------------------------*/
void remoteStatusSigHandler(int signo, siginfo_t *info, void *extra)
{
  if((info != NULL) && (extra != NULL))
  {
    INFO_PRINT("remoteStatusSigHandler, signum: %d\n",info->si_signo);
    aliveFlag = 0;
  }
}

/*---------------------------------------------------------------------------------*/
void remoteStatusGetAliveFlag(uint8_t *pAlive)
{
  if(pAlive != NULL)
    *pAlive = aliveFlag;
}

/*---------------------------------------------------------------------------------*/
/* HELPER METHODS */
/*---------------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------------*/
