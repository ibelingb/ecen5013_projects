/* Header to capture all packets used between threads */

#ifndef PACKET_H_
#define PACKET_H_

#include <stdint.h>
#include <pthread.h>

#define MSG_QUEUE_MSG_SIZE   (8192) // bytes
#define MSG_QUEUE_DEPTH      (20) // total messages
#define LOG_MSG_PAYLOAD_SIZE (128) // bytes
#define IPC_NAME_SIZE        (30)

/* Identifies start of a packet */
#define HEADER_BYTE1 (0xAB)
#define HEADER_BYTE2 (0xCD)

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

typedef enum LogMsg_e
{
  LOG_MSG_ERROR = 0,
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
  uint8_t header[2];
  uint32_t timestamp;
  ProcessId_e processId;
  TaskState_e taskState;
  TaskStatus_e taskStatus;
  uint32_t crc;
} TaskStatusPacket;

typedef struct LogMsgPacket
{
  uint32_t timestamp;
  LogLevel_e logLevel;
  ProcessId_e processId;
  LogMsg_e logMsgType;
  uint8_t payload[LOG_MSG_PAYLOAD_SIZE];
} LogMsgPacket;

/* This struct will be used within shared memory to define data structure to read/write btw threads */
typedef struct TempDataStruct
{
  uint8_t tmp102_temp;
  uint8_t tmp102_lowThreshold;
  uint8_t tmp102_highThreshold;
  uint8_t tmp102_config;
  uint8_t tmp102_tempResolution;
  uint8_t tmp102_fault;
  uint8_t tmp102_extendedMode;
  uint8_t tmp102_shutdownMode;
  uint8_t tmp102_alert;
  uint8_t tmp102_convRate;
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

typedef struct LogThreadInfo
{
  char heartbeatMsgQueueName[IPC_NAME_SIZE];
  char logMsgQueueName[IPC_NAME_SIZE];
  // TODO - Not sure if there's anything else needed here
} LogThreadInfo;

#endif // PACKET_H_
