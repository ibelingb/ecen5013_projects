/***********************************************************************************
 * @author Brian Ibeling
 * brian.ibeling@colorado.edu
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
#include <unistd.h>
#include <string.h>
#include <mqueue.h>
#include <signal.h>
#include <pthread.h>
#include <sys/shm.h>
#include <sys/mman.h>
#include <errno.h>

#include "tempThread.h"
#include "lightThread.h"
#include "remoteThread.h"
#include "loggingThread.h"
#include "packet.h"
#include "bbgLeds.h"
#include "debug.h"
#include "logger.h"
#include "cmn_timer.h"
#include "platform.h"
#include "healthMonitor.h"
#define FOUND_GPIO_LIB


#define MAIN_LOG_EXIT_DELAY   (100 * 1000)
#define USR_LED_53            (53)

/* private functions */
void set_sig_handlers(void);
void mainCleanup(int sig);

/* Define static and global variables */
pthread_t gThreads[NUM_THREADS];
pthread_mutex_t gSharedMemMutex; // TODO: Could have separate mutexes for each sensor writing to SHM
pthread_mutex_t gI2cBusMutex; 

static uint8_t gExit = 1;

int main(int argc, char *argv[]){
  char *heartbeatMsgQueueName = "/heartbeat_mq";
  char *logMsgQueueName        = "/logging_mq";
  char *sensorSharedMemoryName = "/sensor_sm";
  char *logFile = "/usr/bin/log.bin";
  SensorThreadInfo sensorThreadInfo;
  LogThreadInfo logThreadInfo;
  LogMsgPacket logPacket;
  struct mq_attr mqAttr;
  mqd_t logMsgQueue;
  mqd_t heartbeatMsgQueue;
  int sharedMemSize = (sizeof(struct TempDataStruct) + sizeof(struct LightDataStruct));
  int sharedMemFd = 0;
  char ind;
  uint8_t newError;

  /* parse cmdline args */
  if(argc >= 2) {
    logFile = argv[1];
  }
  printf("logfile: %s\n", logFile);

  /* timer variables */
  static timer_t timerid;
  sigset_t set;
  int signum = SIGALRM;
  struct timespec timer_interval;

  /* set signal handlers and actions */
	set_sig_handlers();
  struct sigaction sigAction;
  sigAction.sa_handler = mainCleanup;
  sigAction.sa_flags = 0;
  sigaction(SIGINT, &sigAction, NULL);

  /* Initialize created structs and packets to be 0-filled */
  memset(&sensorThreadInfo, 0, sizeof(struct SensorThreadInfo));
  memset(&logThreadInfo,    0, sizeof(struct LogThreadInfo));
  memset(&logPacket,        0, sizeof(struct LogMsgPacket));
  memset(&mqAttr,           0, sizeof(struct mq_attr));

  /* Ensure log MQ properly cleaned up before starting */
  mq_unlink(logMsgQueueName);

  if(remove(logMsgQueueName) == -1 && errno != ENOENT)
	  ERRNO_PRINT("main() couldn't delete log queue path");

  /* Define MQ attributes */
  mqAttr.mq_flags   = 0;
  mqAttr.mq_maxmsg  = MSG_QUEUE_DEPTH;
  mqAttr.mq_msgsize = MSG_QUEUE_MSG_SIZE;
  mqAttr.mq_curmsgs = 0;

  /*** get logger up and running before initializing the rest of the system ***/
  /* Create MessageQueue for logger to receive log messages */
  logMsgQueue = mq_open(logMsgQueueName, O_CREAT, 0666, &mqAttr);
  if(logMsgQueue == -1)
  {
    ERROR_PRINT("ERROR: main() failed to create MessageQueue for Logger - exiting.\n");
    return EXIT_FAILURE;
  }

  /* initialize thread args for logging thread */
  strcpy(logThreadInfo.heartbeatMsgQueueName, heartbeatMsgQueueName);
  strcpy(logThreadInfo.logMsgQueueName, logMsgQueueName);
  strcpy(logThreadInfo.logFileName, logFile);

  /* initialize the logger (i.e. queue & writeability) 
    * main should probably do this before creating
    * other threads to ensure logger is working before
    * main or other threads start send msgs to mog queue */
  if(LOG_INIT(&logThreadInfo) != LOG_STATUS_OK)
  {
    ERROR_PRINT("failed to init logger\n");
    return EXIT_FAILURE;
  }

  /* logger (i.e. queue writer) init complete */
  LOG_LOGGER_INITIALIZED();

  /* create logger thread */
  if(pthread_create(&gThreads[0], NULL, logThreadHandler, (void*)&logThreadInfo) != 0)
  {
    ERROR_PRINT("ERROR: Failed to create Logging Thread - exiting main().\n");
    return EXIT_FAILURE;
  }

  /*** initialize rest of IPC resources ***/
  /* Define MQ attributes */
  mqAttr.mq_flags   = 0;
  mqAttr.mq_maxmsg  = STATUS_MSG_QUEUE_DEPTH;
  mqAttr.mq_msgsize = STATUS_MSG_QUEUE_MSG_SIZE;
  mqAttr.mq_curmsgs = 0;

  /* Ensure MQs and Shared Memory properly cleaned up before starting */
  mq_unlink(heartbeatMsgQueueName);
  shm_unlink(sensorSharedMemoryName);

  if(remove(heartbeatMsgQueueName) == -1 && errno != ENOENT)
  { ERRNO_PRINT("main() couldn't delete log queue path"); return EXIT_FAILURE; }

  /* Create MessageQueue to receive thread status from all children */
  heartbeatMsgQueue = mq_open(heartbeatMsgQueueName, O_CREAT | O_RDWR | O_NONBLOCK, 0666, &mqAttr);
  if(heartbeatMsgQueue == -1)
  {
    ERROR_PRINT("ERROR: main() failed to create MessageQueue for Main TaskStatus reception - exiting.\n");
    return EXIT_FAILURE;
  }

  /* Create Shared Memory for data sharing between SensorThreads and RemoteThread */
  sharedMemFd = shm_open(sensorSharedMemoryName, O_CREAT | O_RDWR, 0666);
  if(sharedMemFd == -1)
  {
    ERROR_PRINT("ERROR: main() failed to create shared memory for sensor and remote threads - exiting.\n");
    return EXIT_FAILURE;
  }

  /* Configure size of SensorSharedMemory */
  if(ftruncate(sharedMemFd, sharedMemSize) == -1)
  {
    ERROR_PRINT("ERROR: main() failed to configure size of shared memory - exiting.\n");
    return EXIT_FAILURE;
  }

  /* MemoryMap shared memory */
  if(*(int*)mmap(0, sharedMemSize, PROT_READ | PROT_WRITE, MAP_SHARED, sharedMemFd, 0) == -1)
  {
    ERROR_PRINT("ERROR: main() failed to complete memory mapping for shared memory - exiting.\n");
    return EXIT_FAILURE;
  }

  /* Populate ThreadInfo objects to pass names for created IPC pieces to threads */
  strcpy(sensorThreadInfo.heartbeatMsgQueueName, heartbeatMsgQueueName);
  strcpy(sensorThreadInfo.logMsgQueueName, logMsgQueueName);
  strcpy(sensorThreadInfo.sensorSharedMemoryName, sensorSharedMemoryName);
  sensorThreadInfo.sharedMemSize = sharedMemSize;
  sensorThreadInfo.lightDataOffset = sizeof(TempDataStruct);
  sensorThreadInfo.sharedMemMutex = &gSharedMemMutex;
  sensorThreadInfo.i2cBusMutex = &gI2cBusMutex;

  pthread_mutex_init(&gSharedMemMutex, NULL);
  pthread_mutex_init(&gI2cBusMutex, NULL);

  /* Create other threads */
  if(pthread_create(&gThreads[1], NULL, remoteThreadHandler, (void*)&sensorThreadInfo))
  {
    ERROR_PRINT("ERROR: Failed to create Remote Thread - exiting main().\n");
    return EXIT_FAILURE;
  }
  
  if(pthread_create(&gThreads[2], NULL, tempSensorThreadHandler, (void*)&sensorThreadInfo))
  {
    ERROR_PRINT("ERROR: Failed to create TempSensor Thread - exiting main().\n");
    return EXIT_FAILURE;
  }
  
  if(pthread_create(&gThreads[3], NULL, lightSensorThreadHandler, (void*)&sensorThreadInfo))
  {
    ERROR_PRINT("ERROR: Failed to create LightSensor Thread - exiting main().\n");
    return EXIT_FAILURE;
  }
  
  LOG_MAIN_EVENT(MAIN_EVENT_STARTED_THREADS);

  /* Clear memory objects */
  memset(&set, 0, sizeof(sigset_t));
  memset(&timerid, 0, sizeof(timer_t));

  timer_interval.tv_nsec = MAIN_LOOP_TIME_NSEC;
  timer_interval.tv_sec = MAIN_LOOP_TIME_SEC;
  setupTimer(&set, &timerid, signum, &timer_interval);

  /* initialize status LED */
  initLed();

  /* system is now fully functional */
  LOG_SYSTEM_INITIALIZED();
  MUTED_PRINT("main started successfully, pid: %d\n",(pid_t)syscall(SYS_gettid));

  /* Parent thread Asymmetrical - running concurrently with children threads */
  /* Periodically get thread status, send to logging thread */
  while(gExit) 
  {
    /* wait on signal timer */
    sigwait(&set, &signum);

    //LOG_HEARTBEAT();
    newError = 0;
    monitorHealth(&heartbeatMsgQueue, &gExit, &newError);
    MUTED_PRINT("newError: %d\n", newError);
    setStatusLed(newError);
  }
  LOG_SYSTEM_HALTED();

  /* wait to kill log so exit msgs get logged */
  usleep(MAIN_LOG_EXIT_DELAY);

  /* Kill logger; delay to allow Log Exiting to be written to log */
  pthread_kill(gThreads[0], SIGRTMIN + (uint8_t)PID_LOGGING);
  usleep(MAIN_LOG_EXIT_DELAY);

  /* join to clean up children */
  for(ind = 0; ind < NUM_THREADS; ++ind)
    pthread_join(gThreads[(uint8_t)ind], NULL);
  
  /* Cleanup */
  printf("main() Cleanup.\n");
  timer_delete(timerid);
  mq_unlink(heartbeatMsgQueueName);
  mq_unlink(logMsgQueueName);
  shm_unlink(sensorSharedMemoryName);
  mq_close(heartbeatMsgQueue);
  mq_close(logMsgQueue);
  pthread_mutex_destroy(&gSharedMemMutex);
  pthread_mutex_destroy(&gI2cBusMutex);
  close(sharedMemFd);
}

void mainCleanup(int sig){
  /* Send signal to all children threads to terminate */
  pthread_kill(gThreads[1], SIGRTMIN + (uint8_t)PID_REMOTE);
  pthread_kill(gThreads[2], SIGRTMIN + (uint8_t)PID_TEMP);
  pthread_kill(gThreads[3], SIGRTMIN + (uint8_t)PID_LIGHT);
  sleep(3);

  /* Trigger while-loop in main to exit; cleanup allocated resources */
  gExit = 0;
}
