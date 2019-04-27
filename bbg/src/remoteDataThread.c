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
 * @file remoteDataThread.c
 * @brief Remote Data Thread Library
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
void remoteDataGetAliveFlag(uint8_t *pAlive);

/* Define static and global variables */
static SensorThreadInfo sensorInfo;
static uint8_t aliveFlag = 1;

/*---------------------------------------------------------------------------------*/
void* remoteDataThreadHandler(void* threadInfo)
{
  /* Validate input */
  if(threadInfo == NULL){
    ERROR_PRINT("remoteDataThread Failed to initialize; NULL pointer for provided threadInfo parameter - exiting.\n");
    return NULL;
  }

  sensorInfo = *(SensorThreadInfo *)threadInfo;
  RemoteDataPacket dataPacket = {0};
  mqd_t logMsgQueue; /* logger MessageQueue */
  mqd_t hbMsgQueue;  /* main heartbeat MessageQueue */
  mqd_t dataMsgQueue;  /* Data MessageQueue */
  struct mq_attr mqAttr;
  int sockfdDataServer, sockfdDataClient, socketDataFlags;
  struct sockaddr_in servAddr, cliAddr;
  unsigned int cliLen = sizeof(cliAddr);
  size_t dataPacketSize = sizeof(struct RemoteDataPacket);
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
    if(ind != (uint8_t)PID_REMOTE_DATA)
      sigaddset(&mask, SIGRTMIN + ind);
  }
  pthread_sigmask(SIG_BLOCK, &mask, NULL);

  LOG_REMOTE_DATA_EVENT(REMOTE_EVENT_STARTED);

  /* Open FDs for Main and Logging Message queues */
  logMsgQueue = mq_open(sensorInfo.logMsgQueueName, O_RDWR, 0666, mqAttr);
  hbMsgQueue = mq_open(sensorInfo.heartbeatMsgQueueName, O_RDWR, 0666, mqAttr);
  dataMsgQueue = mq_open(sensorInfo.dataMsgQueueName, O_RDWR | O_NONBLOCK, 0666, mqAttr);
  if(logMsgQueue == -1){
    ERROR_PRINT("remoteDataThread Failed to Open Logging MessageQueue - exiting.\n");
    LOG_REMOTE_DATA_EVENT(REMOTE_LOG_QUEUE_ERROR);
    return NULL;
  }
  if(hbMsgQueue == -1) {
    ERROR_PRINT("remoteDataThread Failed to Open heartbeat MessageQueue - exiting.\n");
    LOG_REMOTE_DATA_EVENT(REMOTE_STATUS_QUEUE_ERROR);
    return NULL;
  }
  if(dataMsgQueue == -1) {
    ERROR_PRINT("remoteDataThread Failed to Open Data MessageQueue - exiting.\n");
    LOG_REMOTE_DATA_EVENT(REMOTE_STATUS_QUEUE_ERROR);
    return NULL;
  }

  /** Establish connection on remote socket **/
  /* Create Server Socket Interfaces */
  sockfdDataServer = socket(AF_INET, SOCK_STREAM, 0);
  if(sockfdDataServer == -1){
    ERROR_PRINT("remoteDataThread failed to create Data Socket - exiting.\n");
    LOG_REMOTE_DATA_EVENT(REMOTE_SERVER_SOCKET_ERROR);
    return NULL;
  }

  /* Update Socket Server connections to be non-blocking */
  socketDataFlags = fcntl(sockfdDataServer, F_GETFL);
  fcntl(sockfdDataServer, F_SETFL, socketDataFlags | O_NONBLOCK);

  /* Update Socket Client connections to be non-blocking */
  struct timeval timeout;
  timeout.tv_usec = REMOTE_CLIENT_TIMEOUT_USEC;
  timeout.tv_sec = 0;

  /* Set properties and bind socket */
  servAddr.sin_family = AF_INET;
  servAddr.sin_addr.s_addr = INADDR_ANY;
  servAddr.sin_port = htons((int)DATA_PORT);
  if(bind(sockfdDataServer, (struct sockaddr*)&servAddr, sizeof(servAddr)) == -1) {
    ERROR_PRINT("remoteDataThread failed to bind Data Socket - exiting.\n");
    LOG_REMOTE_DATA_EVENT(REMOTE_SERVER_SOCKET_ERROR);
    return NULL;
  }

  /* Listen for Client Connection */
  if(listen(sockfdDataServer, MAX_CLIENTS) == -1) {
    ERROR_PRINT("remoteDataThread failed to successfully listen for Sensor Client connection - exiting.\n");
    LOG_REMOTE_DATA_EVENT(REMOTE_SERVER_SOCKET_ERROR);
    return NULL;
  }

  /* Log remoteDataThread successfully created */
  INFO_PRINT("Created remoteDataThread to listen on port {%d}\n", DATA_PORT);
  MUTED_PRINT("remoteDataThread started successfully, pid: %d, SIGRTMIN+PID_e: %d\n",(pid_t)syscall(SYS_gettid), SIGRTMIN + PID_REMOTE_DATA);

  /* BIST/Power-on Test 
   *    - Waiting for client connection at this point;
   *    - Verify able to lock mutex and communicate with Shared Memory.
   */
  // TODO: TBD

  while(aliveFlag) {
    //SEND_STATUS_MSG(hbMsgQueue, PID_REMOTE_DATA, STATUS_OK, ERROR_CODE_USER_NONE0);
    sigwait(&set, &signum);

    /* Accept Client Connection for Sensor data */
    if(clientResponse == 0)
    {
      sockfdDataClient = accept(sockfdDataServer, (struct sockaddr*)&cliAddr, &cliLen);
      if(sockfdDataClient == -1){
        /* Add non-blocking logic to allow remoteDataThread to report status while waiting for client conn */
        if(errno == EWOULDBLOCK) {
          continue;
        }

        /* Report error if client fails to connect to server */
        ERROR_PRINT("remoteDataThread failed to accept client connection for socket - exiting.\n");
        LOG_REMOTE_DATA_EVENT(REMOTE_CLIENT_SOCKET_ERROR);
        continue;
      } else if(sockfdDataClient > 0) {
        /* Log remoteDataThread successfully Connected to client */
        printf("Connected remoteDataThread to external Client on port %d.\n", DATA_PORT);
        LOG_REMOTE_DATA_EVENT(REMOTE_EVENT_CNCT_ACCEPTED);

        /* Update Socket Client connections to be non-blocking */
        setsockopt(sockfdDataClient, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof(struct timeval));
      }
    }

    /* Check for incoming commands from remote clients on socket port */
    clientResponse = recv(sockfdDataClient, &dataPacket, dataPacketSize, 0);
    if (clientResponse == -1) 
    {
      /* Non-blocking logic to allow remoteDataThread to report status while waiting for Remote Node sensor data */
      if(errno == EWOULDBLOCK) {
        continue;
      }

      /* Handle error with receiving data from client socket */
      ERROR_PRINT("remoteDataThread failed to handle incoming command from remote client.\n");
      LOG_REMOTE_DATA_EVENT(REMOTE_CLIENT_SOCKET_ERROR);
      continue;
    } else if(clientResponse == 0) { 
      /* Handle disconnect from client socket */
      printf("remoteDataThread connection lost with client on port %d.\n", DATA_PORT);
      LOG_REMOTE_DATA_EVENT(REMOTE_EVENT_CNCT_LOST);
      continue;
    }

    /* Verify bytes received is the expected size for a dataPacket */
    if(clientResponse != dataPacketSize){
      ERROR_PRINT("remoteDataThread received data packet of invalid length from remote node.\n"
             "Expected {%d} | Received {%d}", dataPacketSize, clientResponse);
      LOG_REMOTE_DATA_EVENT(REMOTE_EVENT_INVALID_RECV);
      continue;
    }
    else {
      /* Log data received from Remote Node */
      // TODO
      printf("Data packet received: Lux: %f | Moist: %f\n", dataPacket.luxData, dataPacket.moistureData);

      /* Pass received data to main thread */
      mq_send(dataMsgQueue, (char *)&dataPacket, sizeof(struct RemoteDataPacket), 1);
    }
  }

  /* Thread Cleanup */
  LOG_REMOTE_DATA_EVENT(REMOTE_EVENT_EXITING);
  ERROR_PRINT("Remote thread exiting\n");
  timer_delete(timerid);
  mq_close(logMsgQueue);
  mq_close(hbMsgQueue);
  mq_close(dataMsgQueue);
  close(sockfdDataClient);
  close(sockfdDataServer);

  return NULL;
}

/*---------------------------------------------------------------------------------*/
void remoteDataSigHandler(int signo, siginfo_t *info, void *extra)
{
  if((info != NULL) && (extra != NULL))
  {
    INFO_PRINT("remoteSigHandler, signum: %d\n",info->si_signo);
    aliveFlag = 0;
  }
}

/*---------------------------------------------------------------------------------*/
/**
 * @brief Get method to receive remoteDataThread's alive status.
 *
 * @param pAlive - Pointer to return alive status back to caller function to.
 * @return void
 */
void remoteDataGetAliveFlag(uint8_t *pAlive)
{
  if(pAlive != NULL)
    *pAlive = aliveFlag;
}

/*---------------------------------------------------------------------------------*/
/* HELPER METHODS */
/*---------------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------------*/
