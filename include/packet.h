/* Header to capture all packets used between threads */

#ifndef PACKET_H_
#define PACKET_H_

#include <stdint.h>
#include <pthread.h>
#include "tempSensor.h"

#define MSG_QUEUE_MSG_SIZE      (8192) // bytes
#define MSG_QUEUE_DEPTH         (20) // total messages
#define LOG_MSG_FILENAME_SIZE    (32) // bytes
#define LOG_MSG_PAYLOAD_SIZE    (128) // bytes
#define IPC_NAME_SIZE           (30)

/* Identifies start of a packet */
#define PKT_HEADER (0xABCD)

/* ------------------------------------------------------------- */
/*** ENUM DEFINITIONS ***/
typedef enum ProcessId_e
{
  PID_LIGHT = 0,
  PID_TEMP,
  PID_REMOTE,
  PID_LOGGING
} ProcessId_e;

typedef enum TaskState_e
{
  STATE_IDLE = 0,
  STATE_RUNNING,
  STATE_KILLED
  // TODO
} TaskState_e;

typedef enum TaskStatus_e
{
  STATUS_OK = 0,
  STATUS_ERROR,
  STATUS_TERMINATED
  // TODO
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

// TODO - delete, defined in logger_types.h
typedef enum LogMsg_e
{
  LOG_MSG_SYS_ERROR = 0,
  LOG_MSG_MAIN_START,
  LOG_MSG_TEMP_START,
  LOG_MSG_LIGHT_START,
  LOG_MSG_REMOTE_START,
  LOG_MSG_TEMP_EVENT,
  LOG_MSG_LIGHT_EVENT,
  LOG_MSG_REMOTE_EVENT,
  LOG_MSG_REMOTE_REQ
} LogMsg_e;

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
  uint32_t crc;
} TaskStatusPacket;

typedef struct LogMsgPacket
{
  logMsg_e logMsgId;
  uint8_t filename[LOG_MSG_FILENAME_SIZE];
  uint16_t lineNum;
  uint32_t timestamp;
 	uint32_t payloadLength;
	uint8_t payload[LOG_MSG_PAYLOAD_SIZE];
  uint8_t sourceId;
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
  //TODO - add fields for converted values and 16-bit fields
} TempDataStruct;

/* This struct will be used within shared memory to define data structure to read/write btw threads */
typedef struct LightDataStruct
{
  uint8_t apds9301_luxData0;
  uint8_t apds9301_luxData1;
  uint8_t apds9301_deviceId;
  uint8_t apds9301_controlReg;
  uint8_t apds9301_timingReg;
  uint8_t apds9301_intThresLowLow;  // Low Byte, low thres
  uint8_t apds9301_intThresLowHigh; // Low Byte, high thres
  uint8_t apds9301_intThresHighLow; // High Byte, low thres
  uint8_t apds9301_intThresHighHigh; // High Byte, high thres
  //TODO - Add any needed additional fields
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

typedef struct
{
  char heartbeatMsgQueueName[IPC_NAME_SIZE];
  char logMsgQueueName[IPC_NAME_SIZE];
  char logFileName[64];
} LogThreadInfo;

#endif // PACKET_H_
