/* Header to capture all packets used between threads */

#include <stdint.h>
#include <linux/jiffies.h> // for Timestamp

#define LOG_MSG_PAYLOAD_SIZE (128) // bytes
#define IPC_NAME_SIZE        (30)

/* Identifies start of a packet */
uint8_t header[2] = 0xABCD;

/* ------------------------------------------------------------- */
/*** ENUM DEFINITIONS ***/
enum ProcessId_e
{
  LIGHT = 0,
  TEMP,
  REMOTE,
  LOGGING
};

enum TaskState_e
{
  IDLE = 0,
  RUNNING,
  KILLED
  // TODO
};

enum TaskStatus_e
{
  OK = 0,
  ERROR,
  TERMINATED
  // TODO
};

// TODO - Needed? Could just reuse ProcessId_e or have this separate enum
enum Task_e
{
  LIGHT = 0,
  TEMP,
};

enum TempCommand_e
{
  // TODO
};

enum LightCommand_e
{
  // TODO
};

enum LogMsg_e
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
};

num LogLevel_e
{
  INFO = 0,
  WARNING,
  ERROR
}

/* ------------------------------------------------------------- */
/*** PACKET DEFINITIONS ***/
struct TaskStatusPacket
{
  uint8_t header[2];
  ProcessId_e processId;
  TaskState_e taskState;
  TaskStatus_e taskStatus;
  uint32_t crc;
};

struct TempCmdPacket
{
  uint8_t header[2];
  TempCommand_e tempCmd;
  uint32_t data;
  uint32_t crc;
};

struct LightCmdPacket
{
  uint8_t header[2];
  LightCommand_e lightCmd;
  uint32_t data;
  uint32_t crc;
};

struct RemoteDataPacket
{
  uint8_t header[2];
  uint32_t response; // Cast to specific command type (temp or light) based on task when received by remote thread
  Task_e task;
  uint32_t data;
  uint32_t crc;
};

struct LogMsgPacket
{
  jiffies timestamp;
  LogLevel_e logLevel;
  ProcessId_e processId;
  LogMsg_e logMsgType;
  uint8_t payload[LOG_MSG_PAYLOAD_SIZE];
};


/* ------------------------------------------------------------- */
/*** THREAD INFO STRUCT DEFINITIONS ***/
/* Struct defining the set of arguments passed to each thread when it's created */

struct SensorThreadInfo
{
  char logMsgQueueName[IPC_NAME_SIZE];
  char cmdQueueName[IPC_NAME_SIZE];
  char dataQueueName[IPC_NAME_SIZE];
};

struct RemoteThreadInfo
{
  char logMsgQueueName[IPC_NAME_SIZE];
  char tempCmdQueueName[IPC_NAME_SIZE];
  char tempDataQueueName[IPC_NAME_SIZE];
  char lightCmdQueueName[IPC_NAME_SIZE];
  char lightDataQueueName[IPC_NAME_SIZE];
};

struct LogThreadInfo
{
  char logMsgQueueName[IPC_NAME_SIZE];
  // TODO - Not sure if there's anything else needed here
};
