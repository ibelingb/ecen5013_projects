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
#include <sys/time.h>
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
#define BUFFER_SIZE         (6)

//#define HOUR_TO_SEC (3600)
#define HOUR_TO_SEC (1) // For testing, set to seconds

/* private functions */
void set_sig_handlers(void);
void sigintHandler(int sig);
void displayCommandMenu();
int8_t handleConsoleCmd(uint32_t cmd);
int readInputNonBlock();
void setPeriodicWaterSched(uint32_t hours);
void setOneshotWaterSched(uint32_t hours);
void cancelWaterSched();
static void waterDeviceTx();

/* Define static and global variables */
pthread_t gThreads[NUM_THREADS];
static uint8_t gExit = 1;

static RemoteCmd_e gCurrentCmd = 0; /* Tracks current user cmd if waiting for additional data */
static mqd_t cmdMsgQueue;
static timer_t waterTimerid;
uint32_t waterCyclePeriodHours = 0;
float luxData, moistureData = 0;
static mqd_t heartbeatMsgQueue;

/* Variables to track system operating state */
ControlLoopState_e controlLoopState = IDLE;
SystemState_e systemState = NOMINAL;

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

  mqd_t dataMsgQueue;
  char ind;
  uint8_t newError;

  /* Variables for handling user input cmds */
  char userInputBuffer[BUFFER_SIZE];
  uint32_t userInput = 0;

  /* Variables for tx/rx of data with remoteNode */
  RemoteDataPacket dataPacket = {0};
  size_t dataPacketSize = sizeof(struct RemoteDataPacket);

  /* parse cmdline args */
  if(argc >= 2) {
    logFile = argv[1];
  }
  printf("logfile: %s\n", logFile);

  /* Thread timer variables */
  static timer_t timerid;
  sigset_t set;
  int signum = SIGALRM;
  struct timespec timer_interval;

  /* Watering timer variables */
  struct sigevent waterSigEvent;

  /* set signal handlers and actions */
	set_sig_handlers();
  struct sigaction sigAction;
  sigAction.sa_handler = sigintHandler;
  sigAction.sa_flags = 0;
  sigaction(SIGINT, &sigAction, NULL);

  /* Initialize created structs and packets to be 0-filled */
  memset(&sensorThreadInfo, 0, sizeof(struct SensorThreadInfo));
  memset(&logThreadInfo,    0, sizeof(struct LogThreadInfo));
  memset(&logPacket,        0, sizeof(LogMsgPacket));
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
  mqAttr.mq_msgsize = DATA_MSG_QUEUE_MSG_SIZE;
  dataMsgQueue = mq_open(dataMsgQueueName, O_CREAT | O_RDWR | O_NONBLOCK, 0666, &mqAttr);
  if(dataMsgQueue == -1)
  {
    ERROR_PRINT("ERROR: main() failed to create MessageQueue for Main Remote Data reception - exiting.\n");
    return EXIT_FAILURE;
  }

  /* Initialize Cmd MQ to send commands to RemoteNode */
  mqAttr.mq_msgsize = CMD_MSG_QUEUE_MSG_SIZE;
  cmdMsgQueue = mq_open(cmdMsgQueueName, O_CREAT | O_RDWR, 0666, &mqAttr);
  if(cmdMsgQueue == -1)
  {
    ERROR_PRINT("ERROR: main() failed to create MessageQueue for Main Cmd handling - exiting.\n");
    return EXIT_FAILURE;
  }

  /* Create MessageQueue to receive thread status from all children */
  mqAttr.mq_maxmsg  = STATUS_MSG_QUEUE_DEPTH;
  mqAttr.mq_msgsize = STATUS_MSG_QUEUE_MSG_SIZE;
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

  /* Create main-loop timer */
  timer_interval.tv_nsec = MAIN_LOOP_TIME_NSEC;
  timer_interval.tv_sec = MAIN_LOOP_TIME_SEC;
  setupTimer(&set, &timerid, signum, &timer_interval);

  /* Create watering timer */
  waterSigEvent.sigev_notify = SIGEV_THREAD;
  waterSigEvent.sigev_value.sival_ptr = NULL;
  waterSigEvent.sigev_notify_function = waterDeviceTx;
  waterSigEvent.sigev_notify_attributes = NULL;
  timer_create(CLOCK_REALTIME, &waterSigEvent, &waterTimerid);

  /* initialize status LED */
  initLed();

  /* system is now fully functional */
  LOG_SYSTEM_INITIALIZED();
  MUTED_PRINT("main started successfully, pid: %d\n",(pid_t)syscall(SYS_gettid));

  /* Display menu to UART console; Receive cmd from user */
  displayCommandMenu();

  /* Parent thread Asymmetrical - running concurrently with children threads */
  /* Periodically get thread status, send to logging thread */
  while(gExit) 
  {
    /* Determine if input has been received by user */
    if(readInputNonBlock() == 1) {
      /* Convert received input from ascii to int */
      scanf("%s", userInputBuffer);
      userInput = atoi(userInputBuffer);

      /* Process received cmd from user; display cmd menu again */
      handleConsoleCmd(userInput);
      displayCommandMenu();
    }

    /* If data received from TIVA, write to local data */
    if(mq_receive(dataMsgQueue, (char *)&dataPacket, dataPacketSize, NULL) == dataPacketSize)
    {
      luxData = dataPacket.luxData;
      moistureData = dataPacket.moistureData;
    }

    /** Control Loop **/
    /* Based on current operating state, handle data returned from TIVA */
    /* If soil moisture exceed saturation level, reset timer of next water cycle */
    switch(controlLoopState) {
      case WATER_PERIODIC_SCHED:
        /* Waiting until next periodic watering cycle */
        /* If soil moisture exceeds threshold between scheduled watering cycles, reset timer */
        if(moistureData > SOIL_SATURATION_THRES)
        {
          setPeriodicWaterSched(waterCyclePeriodHours);
        }
        break;
      case WATER_ONESHOT_SCHED:
        /* Waiting until next one-shot watering cycle */
        break;
      case WATERING_PLANT:
        /* Plant should be getting watered - remain in watering state until soil moisture exceeds threshold */
        // TODO: Update below to use state of Solenoid/Simulated LED Device reported from TIVA
        //       --> If Solenoid is "open", watering plant
        if(moistureData < SOIL_SATURATION_THRES)
        {
          /* Continue watering plant */
        } 
        else {
          /* Watering complete, update current state */
          struct itimerspec waterTimeSpec;
          timer_gettime(waterTimerid, &waterTimeSpec);
          /* Check if timer scheduled; if zero, timer is disabled and set state to IDLE */
          if(((int)waterTimeSpec.it_value.tv_sec == 0) && 
             ((int)waterTimeSpec.it_value.tv_nsec == 0))
          {
            controlLoopState = IDLE;
          }
          else {
            controlLoopState = WATER_PERIODIC_SCHED;
          }
        }
        break;
      case IDLE:
      default:
        // TODO: TBD
        break;
    }

    /* If wish to log each heartbeat event monitored by main, uncomment below */
    //LOG_HEARTBEAT();

    newError = 0;
    monitorHealth(&heartbeatMsgQueue, &gExit, &newError);
    MUTED_PRINT("newError: %d\n", newError);
    setStatusLed(newError);

    /* wait on signal timer */
    sigwait(&set, &signum);
  }
  INFO_PRINT("Main loop exited\n");
  LOG_SYSTEM_HALTED();

  /* wait to kill log so exit msgs get logged */
  usleep(MAIN_LOG_EXIT_DELAY);
  sleep(3);

  /* Kill logger; delay to allow Log Exiting to be written to log */
  INFO_PRINT("killing logger %d\n", __libc_current_sigrtmax());
  pthread_kill(gThreads[0], SIGRTMIN + (uint8_t)PID_LOGGING);
  INFO_PRINT("kill signal sent\n");
  usleep(MAIN_LOG_EXIT_DELAY);
  sleep(1);

  /* join to clean up children */
  for(ind = 0; ind < NUM_THREADS; ++ind) {
    pthread_join(gThreads[(uint8_t)ind], NULL);
  }
  
  
  /* Cleanup */
  printf("main() Cleanup.\n");
  timer_delete(timerid);
  timer_delete(waterTimerid);
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
 * terminate Remote children threads and have main exit from 
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
  gExit = 0;
  
  /* Trigger while-loop in main to exit; cleanup allocated resources */

}

/*---------------------------------------------------------------------------------*/
/**
 * @brief Prints a menu of actions to the user to update the current operating state of the 
 *        BBG Control Node  or the TIVA Remote Node.
 * 
 * @return void
 */
void displayCommandMenu()
{
  switch(gCurrentCmd)
  {
    case CMD_SCHED_PERIODIC:
      printf("\nEnter a value to water the plant at a periodically date/time.\n");
      break;
    case CMD_SCHED_ONESHOT:
      printf("\nEnter a value to water the plant at a future date/time.\n");
      break;
    case CMD_SETMOISTURE_LOWTHRES:
      printf("\nEnter a value to set for the Moisture Sensor Low Threshold.\n");
      break;
    case CMD_SETMOISTURE_HIGHTHRES:
      printf("\nEnter a value to set for the Moisture Sensor High Threshold.\n");
      break;
    default:
      printf("\nEnter a value to specify a command to send to the Sensor Application:\n"
             "\t1 = Water Plant\n"
             "\t2 = Schedule Periodic Watering Cycle\n"
             "\t3 = Schedule One-Shot Watering Event\n"
             "\t4 = Display Sensor Data\n"
             "\t5 = Display Device/Actuator State\n"
             "\t6 = Enable TIVA Device2 (LED2 On)\n"
             "\t7 = Disable TIVA Device2 (LED2 Off)\n"
             "\t8 = Set Moisture Low Threshold\n"
             "\t9 = Set Moisture Threshold\n"
             "\t10 = Cancel Scheduled Watering Event\n"
             //"\t11 = \n"
            );
      break;
  }
}

/*---------------------------------------------------------------------------------*/
/**
 * @brief Handle command received from user via UART console 
 * 
 * @return success of failure via EXIT_SUCCESS or EXIT_FAILURE
 */
int8_t handleConsoleCmd(uint32_t userInput) {
  RemoteCmd_e txCmd = REMOTE_CMD_END;
  uint32_t data = 0;

  if(gCurrentCmd != 0) {
    data = userInput;
    userInput = gCurrentCmd;
  }

  /* Verify received cmd is valid */
  if(userInput >= CMD_MAX_CMDS) {
    gCurrentCmd = 0;
    printf("Invalid command received of {%d} - ignoring cmd\n", userInput);
    return EXIT_FAILURE;
  } 
  else {
    switch((ConsoleCmd_e)userInput)
    {
      case CMD_WATER_PLANT :
        /* Populate packet and push onto cmdQueue to tx to Remote Node */
        printf("CMD_WATER_PLANT\n");
        waterDeviceTx();
        break;
      case CMD_SCHED_PERIODIC :
        printf("CMD_SCHED_PERIODIC\n");
        gCurrentCmd = CMD_SCHED_PERIODIC;
        if(data != 0) {
          setPeriodicWaterSched(data);
          gCurrentCmd = 0;
        }
        break;
      case CMD_SCHED_ONESHOT :
        printf("CMD_SCHED_ONESHOT\n");
        gCurrentCmd = CMD_SCHED_ONESHOT;
        if(data != 0) {
          setOneshotWaterSched(data);
          gCurrentCmd = 0;
        }
        break;
      case CMD_GET_SENSOR_DATA :
        printf("CMD_GET_SENSOR_DATA\n");
        printf("LuxData: {%f} | MoistureData: {%f}\n", luxData, moistureData);
        break;
      case CMD_GET_APP_STATE :
        printf("CMD_GET_APP_STATE\n");
        printf("ControlLoopState: {%d} | SystemState: {%d}\n", controlLoopState, systemState);
        break;
      case CMD_EN_DEV2 :
        /* Populate packet and push onto cmdQueue to tx to Remote Node */
        printf("CMD_EN_DEV2\n");
        txCmd = REMOTE_ENDEV2;
        break;
      case CMD_DS_DEV2 :
        printf("CMD_DS_DEV2\n");
        txCmd = REMOTE_DSDEV2;
        break;
      case CMD_SETMOISTURE_LOWTHRES:
        printf("CMD_SETMOISTURE_LOWTHRES\n");
        gCurrentCmd = CMD_SETMOISTURE_LOWTHRES;
        if(data != 0) {
          /* Validate data received from user */
          if(data > SOIL_MOISTURE_MAX) {
            gCurrentCmd = 0;
            ERROR_PRINT("Invalid High Threshold value for Soil Moisture received.\n"
                        "Max value: {%d} | Received value: {%d}\n", SOIL_MOISTURE_MAX, data);
          }
          else {
            txCmd = REMOTE_SETMOISTURE_LOWTHRES;
          }
        }
        break;
      case CMD_SETMOISTURE_HIGHTHRES:
        printf("CMD_SETMOISTURE_HIGHTHRES\n");
        gCurrentCmd = CMD_SETMOISTURE_HIGHTHRES;

        if (data != 0) {
          /* Validate data received from user */
          if(data > SOIL_MOISTURE_MAX) {
            gCurrentCmd = 0;
            ERROR_PRINT("Invalid Low Threshold value for Soil Moisture received.\n"
                        "Max value: {%d} | Received value: {%d}\n", SOIL_MOISTURE_MAX, data);
          }
          else {
            txCmd = REMOTE_SETMOISTURE_HIGHTHRES;
          }
        }
        break;

      case CMD_SCHED_CANCEL :
        printf("CMD_SCHED_CANCEL\n");
        cancelWaterSched();
        break;

        /*
           case CMD_ :
        // TODO
        break;
         */

      default:
        ERROR_PRINT("Unrecognized command received. Request ignored.\n");
      return EXIT_FAILURE;
    }
  }

  /* If cmd received needs to be transmitted to the TIVA, populate packet and send */
  if(txCmd != REMOTE_CMD_END) {
    RemoteCmdPacket cmdPacket = {0};
    cmdPacket.cmd = txCmd;
    cmdPacket.data = data;

    printf("Cmd TX: cmd: %d | data: %d\n", cmdPacket.cmd, cmdPacket.data);

    mq_send(cmdMsgQueue, (char *)&cmdPacket, sizeof(struct RemoteCmdPacket), 1);
    gCurrentCmd = 0;
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

/*---------------------------------------------------------------------------------*/
void setPeriodicWaterSched(uint32_t hours) {
  /* Validate input watering schedule */
  if(hours < 8) {
    ERROR_PRINT("Periodic watering cycle must have a minimum interval of 8 hours - "
                "Setting periodic watering cycle failed.\n");
    return;
  }

  struct itimerspec timeSpec;
  timeSpec.it_value.tv_sec = hours*HOUR_TO_SEC;
  timeSpec.it_value.tv_nsec = 0;
  timeSpec.it_interval.tv_sec = hours*HOUR_TO_SEC;
  timeSpec.it_interval.tv_nsec = 0;

  timer_settime(waterTimerid, 0, &timeSpec, NULL);
  waterCyclePeriodHours = hours;
  
  /* Update controlLoopState if not currently watering plant */
  if(controlLoopState != WATERING_PLANT)
  {
    controlLoopState = WATER_PERIODIC_SCHED;
  }
}

/*---------------------------------------------------------------------------------*/
void setOneshotWaterSched(uint32_t hours) {
  struct itimerspec timeSpec;
  timeSpec.it_value.tv_sec = hours*HOUR_TO_SEC;
  timeSpec.it_value.tv_nsec = 0;
  timeSpec.it_interval.tv_sec = 0;
  timeSpec.it_interval.tv_nsec = 0;

  timer_settime(waterTimerid, 0, &timeSpec, NULL);

  /* Update controlLoopState if not currently watering plant */
  if(controlLoopState != WATERING_PLANT)
  {
    controlLoopState = WATER_ONESHOT_SCHED;
  }
}

/*---------------------------------------------------------------------------------*/
void cancelWaterSched() {
  struct itimerspec timeSpec;
  timeSpec.it_value.tv_sec = 0;
  timeSpec.it_value.tv_nsec = 0;
  timeSpec.it_interval.tv_sec = 0;
  timeSpec.it_interval.tv_nsec = 0;

  timer_settime(waterTimerid, 0, &timeSpec, NULL);

  /* Update controlLoopState if not currently watering plant */
  if(controlLoopState != WATERING_PLANT)
  {
    controlLoopState = IDLE;
  }
}

/*---------------------------------------------------------------------------------*/
static void waterDeviceTx() {
  RemoteCmdPacket cmdPacket = {0};
  cmdPacket.cmd = REMOTE_WATERPLANT;
  mq_send(cmdMsgQueue, (char *)&cmdPacket, sizeof(struct RemoteCmdPacket), 1);
  controlLoopState = WATERING_PLANT;
}
