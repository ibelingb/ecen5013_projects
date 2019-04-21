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
 * @brief main for project 2 - Multithreaded sensor sampling application with connected 
                               TIVA Board
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

#include "remoteThread.h"
#include "loggingThread.h"
#include "packet.h"
#include "bbgLeds.h"
#include "my_debug.h"
#include "logger.h"
#include "cmn_timer.h"
#include "platform.h"
#include "healthMonitor.h"

#define FOUND_GPIO_LIB
#define MAIN_LOG_EXIT_DELAY (100 * 1000)
#define USR_LED_53          (53)
#define BUFFER_SIZE         (3)

/* private functions */
void set_sig_handlers(void);
void sigintHandler(int sig);
void displayControlMenu();
int8_t handleConsoleCmd(uint8_t cmd, mqd_t cmdMsgQueue);
int readInputNonBlock();

/* Define static and global variables */
pthread_t gThreads[NUM_THREADS];

static uint8_t gExit = 1;

int main(int argc, char *argv[]){
  char *heartbeatMsgQueueName = "/heartbeat_mq";
  char *logMsgQueueName = "/logging_mq";
  char *dataMsgQueueName = "/data_mq";
  char *cmdMsgQueueName = "/cmd_mq";
  char *logFile = "/usr/bin/log.bin";
  SensorThreadInfo sensorThreadInfo;
  LogThreadInfo logThreadInfo;
  LogMsgPacket logPacket;
  struct mq_attr mqAttr;
  mqd_t logMsgQueue;
  mqd_t heartbeatMsgQueue;
  mqd_t dataMsgQueue;
  mqd_t cmdMsgQueue;
  char ind;
  uint8_t newError;
 
  char inputCmdBuffer[BUFFER_SIZE];
  int inputCmd = 0;

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
  sigAction.sa_handler = sigintHandler;
  sigAction.sa_flags = 0;
  sigaction(SIGINT, &sigAction, NULL);

  /* Initialize created structs and packets to be 0-filled */
  memset(&sensorThreadInfo, 0, sizeof(struct SensorThreadInfo));
  memset(&logThreadInfo,    0, sizeof(struct LogThreadInfo));
  memset(&logPacket,        0, sizeof(struct LogMsgPacket));
  memset(&mqAttr,           0, sizeof(struct mq_attr));

  /* Ensure MQs properly cleaned up before starting */
  mq_unlink(logMsgQueueName);
  if(remove(logMsgQueueName) == -1 && errno != ENOENT)
	  ERRNO_PRINT("main() couldn't delete log msg queue path");

  mq_unlink(heartbeatMsgQueueName);
  if(remove(heartbeatMsgQueueName) == -1 && errno != ENOENT)
  { ERRNO_PRINT("main() couldn't delete heartbeat msg queue path"); return EXIT_FAILURE; }

  mq_unlink(dataMsgQueueName);
  if(remove(dataMsgQueueName) == -1 && errno != ENOENT)
	  ERRNO_PRINT("main() couldn't delete data msg queue path");

  mq_unlink(cmdMsgQueueName);
  if(remove(cmdMsgQueueName) == -1 && errno != ENOENT)
	  ERRNO_PRINT("main() couldn't delete cmd msg queue path");

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
  /* Initialize Data MQ to receive sensor data from RemoteNode */
  dataMsgQueue = mq_open(dataMsgQueueName, O_CREAT | O_RDWR | O_NONBLOCK, 0666, &mqAttr);
  if(dataMsgQueue == -1)
  {
    ERROR_PRINT("ERROR: main() failed to create MessageQueue for Main Remote Data reception - exiting.\n");
    return EXIT_FAILURE;
  }

  /* Initialize Cmd MQ to send commands to RemoteNode */
  mqAttr.mq_flags   = 0;
  mqAttr.mq_maxmsg  = MSG_QUEUE_DEPTH;
  mqAttr.mq_msgsize = CMD_MSG_QUEUE_MSG_SIZE;
  mqAttr.mq_curmsgs = 0;
  cmdMsgQueue = mq_open(cmdMsgQueueName, O_CREAT | O_RDWR, 0666, &mqAttr);
  if(cmdMsgQueue == -1)
  {
    ERROR_PRINT("ERROR: main() failed to create MessageQueue for Main Cmd handling - exiting.\n");
    return EXIT_FAILURE;
  }

  /* Define MQ attributes */
  mqAttr.mq_flags   = 0;
  mqAttr.mq_maxmsg  = STATUS_MSG_QUEUE_DEPTH;
  mqAttr.mq_msgsize = STATUS_MSG_QUEUE_MSG_SIZE;
  mqAttr.mq_curmsgs = 0;

  /* Create MessageQueue to receive thread status from all children */
  heartbeatMsgQueue = mq_open(heartbeatMsgQueueName, O_CREAT | O_RDWR | O_NONBLOCK, 0666, &mqAttr);
  if(heartbeatMsgQueue == -1)
  {
    ERROR_PRINT("ERROR: main() failed to create MessageQueue for Main TaskStatus reception - exiting.\n");
    return EXIT_FAILURE;
  }

  /* Populate ThreadInfo objects to pass names for created IPC pieces to threads */
  strcpy(sensorThreadInfo.heartbeatMsgQueueName, heartbeatMsgQueueName);
  strcpy(sensorThreadInfo.logMsgQueueName, logMsgQueueName);
  strcpy(sensorThreadInfo.cmdMsgQueueName, cmdMsgQueueName);
  strcpy(sensorThreadInfo.dataMsgQueueName, dataMsgQueueName);

  /* Create other threads */
  if(pthread_create(&gThreads[1], NULL, remoteLogThreadHandler, (void*)&sensorThreadInfo))
  {
    ERROR_PRINT("ERROR: Failed to create Remote Log Thread - exiting main().\n");
    return EXIT_FAILURE;
  }
 
  if(pthread_create(&gThreads[2], NULL, remoteStatusThreadHandler, (void*)&sensorThreadInfo))
  {
    ERROR_PRINT("ERROR: Failed to create Remote Status Thread - exiting main().\n");
    return EXIT_FAILURE;
  }
 
  if(pthread_create(&gThreads[3], NULL, remoteDataThreadHandler, (void*)&sensorThreadInfo))
  {
    ERROR_PRINT("ERROR: Failed to create Remote Data Thread - exiting main().\n");
    return EXIT_FAILURE;
  }

  if(pthread_create(&gThreads[4], NULL, remoteCmdThreadHandler, (void*)&sensorThreadInfo))
  {
    ERROR_PRINT("ERROR: Failed to create Remote Cmd Thread - exiting main().\n");
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

  /* Display menu to UART console; Receive cmd from user */
  displayControlMenu();

  /* Parent thread Asymmetrical - running concurrently with children threads */
  /* Periodically get thread status, send to logging thread */
  while(gExit) 
  {
    /* Determine if input has been received by user */
    if(readInputNonBlock() == 1) {
      /* Convert received input from ascii to int */
      scanf("%s", inputCmdBuffer);
      inputCmd = atoi(inputCmdBuffer);

      /* Process received cmd from user; display cmd menu again */
      handleConsoleCmd(inputCmd, cmdMsgQueue);
      displayControlMenu();
    }

    /* wait on signal timer */
    sigwait(&set, &signum);

    /* If wish to log each heartbeat event monitored by main, uncomment below */
    //LOG_HEARTBEAT();

    newError = 0;
    //monitorHealth(&heartbeatMsgQueue, &gExit, &newError);
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
  mq_unlink(dataMsgQueueName);
  mq_unlink(cmdMsgQueueName);
  mq_close(heartbeatMsgQueue);
  mq_close(logMsgQueue);
  mq_close(dataMsgQueue);
  mq_close(cmdMsgQueue);
}

/*---------------------------------------------------------------------------------*/
/**
 * @brief Handler for SIGINT signal being received by main() - Kill application
 *
 * When SIGINT signal (ctrl+c) received by application, send kill signals to 
 * terminate Remote, Temp, and Light children threads and have main exit from 
 * processing loop to allow for gracefully termination of application.
 *
 * @param sig - param required by sigAction struct.
 * @return void
 */
void sigintHandler(int sig){
  /* Send signal to all children threads to terminate */
  pthread_kill(gThreads[1], SIGRTMIN + (uint8_t)PID_REMOTE_LOG);
  pthread_kill(gThreads[2], SIGRTMIN + (uint8_t)PID_REMOTE_STATUS);
  pthread_kill(gThreads[3], SIGRTMIN + (uint8_t)PID_REMOTE_DATA);
  pthread_kill(gThreads[4], SIGRTMIN + (uint8_t)PID_REMOTE_CMD);
  printf("\nDelay to allow threads to terminate gracefully and log exit events...\n\n");
  sleep(3);

  /* Trigger while-loop in main to exit; cleanup allocated resources */
  gExit = 0;
}

/*---------------------------------------------------------------------------------*/
/**
 * @brief Prints a menu of actions to the user to send to the BBG Control Node via
 *        the serial interface.
 * 
 * @return void
 */
void displayControlMenu()
{
  printf("\nEnter a value to specify a command to send to the Sensor Application:\n"
         "\t1 = Water Plant\n"
         "\t2 = Schedule Periodic Watering Cycle\n"
         "\t3 = Schedule One-Shot Watering Event\n"
         "\t4 = Get Latest Sensor Data\n"
         "\t5 = Get Device/Actuator State\n"
         "\t6 = Enable TIVA Device2 (LED2 On)\n"
         "\t7 = Disable TIVA Device2 (LED2 Off)\n"
         //"\t8 = \n"
         //"\t9 = \n"
         //"\t10 = \n"
         //"\t11 = \n"
  );
}

/*---------------------------------------------------------------------------------*/
/**
 * @brief Handle command received from user via UART console 
 * 
 * @return success of failure via EXIT_SUCCESS or EXIT_FAILURE
 */
int8_t handleConsoleCmd(uint8_t cmd, mqd_t cmdMsgQueue) {
  /* Verify received cmd is valid */
  if((cmd >= CMD_MAX_CMDS)) {
    printf("Invalid command received of {%d} - ignoring cmd\n", cmd);
    return EXIT_FAILURE;
  }

  switch((ConsoleCmd_e)cmd)
  {
    case CMD_WATER_PLANT :
      /* Populate packet and push onto cmdQueue to tx to Remote Node */
      printf("CMD_WATER_PLANT\n");

      RemoteCmdPacket cmdPacket = {0};
      cmdPacket.cmd = cmd;
      mq_send(cmdMsgQueue, (char *)&cmdPacket, sizeof(struct RemoteCmdPacket), 1);

      break;
    case CMD_SCHED_PERIODIC :
      printf("CMD_SCHED_PERIODIC\n");
      // TODO
      break;
    case CMD_SCHED_ONESHOT :
      printf("CMD_SCHED_ONESHOT\n");
      // TODO
      break;
    case CMD_GET_SENSOR_DATA :
      printf("CMD_GET_SENSOR_DATA\n");
      // TODO
      break;
    case CMD_GET_APP_STATE :
      printf("CMD_GET_APP_STATE\n");
      // TODO
      break;
    case CMD_EN_DEV2 :
      printf("CMD_EN_DEV2\n");
      // TODO
      break;
    case CMD_DS_DEV2 :
      printf("CMD_DS_DEV2\n");
      // TODO
      break;
    /*
    case CMD_ :
      // TODO
      break;
    */

    default:
      printf("Unrecognized command received. Request ignored.\n");
      return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}

/*---------------------------------------------------------------------------------*/
/**
 * @brief Read input from UART console in non-blocking manner
 *
 * @return value 1 if user input received; otherwise 0
 */
int readInputNonBlock()
{
  struct timeval tv = { 0L, 0L };
  fd_set fds;
  FD_ZERO(&fds);
  FD_SET(STDIN_FILENO, &fds);
  select(STDIN_FILENO+1, &fds, NULL, NULL, &tv);
  return (FD_ISSET(0, &fds));
}
