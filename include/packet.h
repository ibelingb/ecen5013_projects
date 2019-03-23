/***********************************************************************************
 * @author Brian Ibeling
 * Brian.Ibeling@colorado.edu
 * Advanced Embedded Software Development
 * ECEN5013 - Rick Heidebrecht
 * @date March 8, 2019
 * arm-linux-gnueabi (Buildroot)
 * gcc (Ubuntu)
 ************************************************************************************
 *
 * @file packet.h
 * @brief Header to capture all packets used between threads
 *
 ************************************************************************************
 */

#ifndef PACKET_H_
#define PACKET_H_

#include <stdint.h>
#include <pthread.h>
#include "tempSensor.h"
#include "lightSensor.h"
#include "logger_types.h"

#define STATUS_MSG_QUEUE_MSG_SIZE   (sizeof(TaskStatusPacket)) // bytes
#define STATUS_MSG_QUEUE_DEPTH      (4) // total messages

#define MSG_QUEUE_MSG_SIZE        (sizeof(LogMsgPacket)) // bytes
#define MSG_QUEUE_DEPTH           (10) // total messages
#define LOG_MSG_FILENAME_SIZE     (32) // bytes
#define LOG_MSG_PAYLOAD_SIZE      (128) // bytes
#define IPC_NAME_SIZE             (30)

/* Identifies start of a packet */
#define PKT_HEADER (0xABCD)

/* ------------------------------------------------------------- */
/*** ENUM DEFINITIONS ***/


typedef enum ProcessId_e
{
  PID_LIGHT = 0,
  PID_TEMP,
  PID_REMOTE,
  PID_LOGGING,
  PID_END
} ProcessId_e;

/* set by main, not self */
typedef enum TaskState_e
{
  STATE_IDLE = 0,
  STATE_RUNNING,
  STATE_KILLED,
  STATE_END
} TaskState_e;

/* set by self, reported to main */
typedef enum TaskStatus_e
{
  STATUS_OK = 0,
  STATUS_ERROR,
  STATUS_TERMINATED,
  STATUS_END
} TaskStatus_e;

// TODO - Needed? Could just reuse ProcessId_e or have this separate enum
typedef enum Task_e
{
  TASK_LIGHT = 0,
  TASK_TEMP,
} Task_e;

// TODO - move to header
typedef enum {
    MAIN_EVENT_STARTED_THREADS = 0,
    MAIN_EVENT_THREAD_UNRESPONSIVE,
    MAIN_EVENT_RESTART_THREAD,
    MAIN_EVENT_LOG_QUEUE_FULL,
    MAIN_EVENT_ISSUING_EXIT_CMD,
    MAIN_EVENT_END
} MainEvent_e;

// TODO - move to header
typedef enum {
    REMOTE_EVENT_STARTED = 0,
    REMOTE_EVENT_CNCT_ACCEPTED,
    REMOTE_EVENT_CNCT_LOST,
    REMOTE_EVENT_CMD_RECV,
    REMOTE_EVENT_INVALID_RECV,
    REMOTE_EVENT_ERROR,
    REMOTE_EVENT_EXITING,
    REMOTE_EVENT_END
} RemoteEvent_e;

// TODO - move to header
typedef enum {
    LIGHT_EVENT_STARTED = 0,
    LIGHT_EVENT_DAY,
    LIGHT_EVENT_NIGHT,
    LIGHT_EVENT_ERROR,
    LIGHT_EVENT_EXITING,
    LIGHT_EVENT_END
} LightEvent_e;

typedef enum {
  ERROR_CODE_NONE = 0,
  ERROR_CODE_TIMEOUT,
  ERROR_CODE_RECVD_NULL_PTR,
  /* TODO - put other general errors here */

  /* start of user unique errors, to be type casted */
  ERROR_CODE_USER0 = 128,
  ERROR_CODE_USER1 = 129,
  ERROR_CODE_USER2 = 130,
  ERROR_CODE_USER3 = 131,
  ERROR_CODE_USER4 = 132,
  ERROR_CODE_USER5 = 133,
  ERROR_CODE_USER6 = 134,
  ERROR_CODE_USER7 = 135,
  ERROR_CODE_USER8 = 136,
  ERROR_CODE_USER9 = 137,
  ERROR_CODE_USER10 = 138,
  ERROR_CODE_USER11 = 139,
  ERROR_CODE_USER12 = 140,
  ERROR_CODE_USER13 = 141,
  ERROR_CODE_USER14 = 142,
  ERROR_CODE_USER15 = 143,
  ERROR_CODE_END = 255
} ErrorCode_e;

typedef enum {
  MAIN_ACTION_NONE = 0,
  MAIN_ACTION_NOTIFY_USER_ONLY,
  MAIN_ACTION_RESTART_THREAD,
  MAIN_ACTION_TERMINATE_THREAD,
  MAIN_ACTION_TERMINATE_ALL,
  MAIN_ACTION_END
} MainAction_e;

typedef enum LogLevel_e
{
  LOGLVL_INFO = 0,
  LOGLVL_WARNING,
  LOGLVL_ERROR
} LogLevel_e;

/* ------------------------------------------------------------- */
/*** PACKET/STRUCT DEFINITIONS ***/
typedef struct TaskStatusPacket
{
  uint16_t header;
  uint32_t timestamp;
  ProcessId_e processId;
  TaskState_e taskState;
  TaskStatus_e taskStatus;
  uint8_t errorCode;
} TaskStatusPacket;

typedef struct LogMsgPacket
{
  logMsg_e logMsgId;
  uint8_t filename[LOG_MSG_FILENAME_SIZE];
  uint16_t lineNum;
  uint32_t timestamp;
 	uint32_t payloadLength;
	uint8_t payload[LOG_MSG_PAYLOAD_SIZE];
  uint16_t sourceId;
	uint32_t checksum;  
} LogMsgPacket;

/* This struct will be used within shared memory to define data structure to read/write btw threads */
typedef struct TempDataStruct
{
  float tmp102_temp;
  float tmp102_lowThreshold;
  float tmp102_highThreshold;
  float tmp102_config;
  float tmp102_tempResolution;
  Tmp102_FaultCount_e tmp102_fault;
  Tmp102_AddrMode_e tmp102_extendedMode;
  Tmp102_Shutdown_e tmp102_shutdownMode;
  Tmp102_Alert_e tmp102_alert;
  Tmp102_ConvRate_e tmp102_convRate;
} TempDataStruct;

/* This struct will be used within shared memory to define data structure to read/write btw threads */
typedef struct LightDataStruct
{
  float apds9301_luxData;
  uint8_t apds9301_devicePartNo;
  uint8_t apds9301_deviceRevNo;
  Apds9301_PowerCtrl_e apds9301_powerControl;
  Apds9301_TimingGain_e apds9301_timingGain;
  Apds9301_TimingInt_e apds9301_timingIntegration;
  Apds9301_IntSelect_e apds9301_intSelect;
  Apds9301_IntPersist_e apds9301_intPersist;
  uint16_t apds9301_intThresLow;
  uint16_t apds9301_intThresHigh;
} LightDataStruct;

/* ------------------------------------------------------------- */
/*** THREAD INFO STRUCT DEFINITIONS ***/
/* Struct defining the set of arguments passed to each thread when it's created */

typedef struct SensorThreadInfo
{
  char heartbeatMsgQueueName[IPC_NAME_SIZE];
  char logMsgQueueName[IPC_NAME_SIZE];
  char sensorSharedMemoryName[IPC_NAME_SIZE];
  pthread_mutex_t* sharedMemMutex;
  pthread_mutex_t* i2cBusMutex;
  int sharedMemSize;
  int tempDataOffset;  /* Offset into SharedMemory for TempDataStruct (in bytes) */
  int lightDataOffset; /* Offset into SharedMemory for LightDataStruct (in bytes) */
} SensorThreadInfo;

typedef struct LogThreadInfo
{
  char heartbeatMsgQueueName[IPC_NAME_SIZE];
  char logMsgQueueName[IPC_NAME_SIZE];
  char logFileName[64];
} LogThreadInfo;

#endif // PACKET_H_
