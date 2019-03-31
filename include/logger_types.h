/***********************************************************************************
 * @author Joshua Malburg (joma0364)
 * joshua.malburg@colorado.edu
 * Principles of Embedded Software
 * ECEN5813 - Alex Fosdick
 * @date April 14, 2018
 * gcc (Ubuntu 7.2.0-8ubuntu3)
 * arm-linux-gnueabi (Ubuntu/Linaro 7.2.0-6ubuntu1)
 * arm-none-eabi-gcc (5.4.1 20160919)
 ************************************************************************************
 *
 * @file logger_types.h
 * @brief special types for logger functions
 *
 ************************************************************************************
 */

#ifndef LOGGER_TYPES_H
#define	LOGGER_TYPES_H

#include <stdint.h>

#define LOG_STATUS_OK 			(1)
#define LOG_STATUS_NOTOK		(0)
#define LOG_UNINITIALIZED		(2)
#define LOG_STATUS_BUF_FULL 	(3)

#define MAX_INT_STRING_SIZE		(12)

#define FRAME_START_BYTE 	('<')
#define FRAME_STOP_BYTE		('>')

typedef enum {
	LOG_MSG_NONE = 0,
	LOG_MSG_SYSTEM_ID = 1,
	LOG_MSG_SYSTEM_VERSION = 2,
	LOG_MSG_LOGGER_INITIALIZED = 3,
	LOG_MSG_GPIO_INITIALIZED = 4,
	LOG_MSG_SYSTEM_INITIALIZED = 5,
	LOG_MSG_SYSTEM_HALTED = 6,
	LOG_MSG_INFO = 7,
	LOG_MSG_WARNING = 8,
	LOG_MSG_ERROR = 9,
	LOG_MSG_PROFILING_STARTED = 10,
	LOG_MSG_PROFILING_RESULT = 11,
	LOG_MSG_PROFILING_COMPLETED = 12,
	LOG_MSG_DATA_RECEIVED = 13,
	LOG_MSG_DATA_ANALYSIS_STARTED = 14,
	LOG_MSG_DATA_ALPHA_COUNT = 15,
	LOG_MSG_DATA_NUM_COUNT = 16,
	LOG_MSG_DATA_PUNC_COUNT = 17,
	LOG_MSG_DATA_MISC_COUNT = 18,
	LOG_MSG_DATA_ANALYSIS_COMPLETED = 19,
	LOG_MSG_HEARTBEAT = 20,
	LOG_MSG_CORE_DUMP = 21,
	LOG_MSG_THREAD_STATUS = 22,
	LOG_MSG_LIGHT_SENSOR_EVENT = 23,
	LOG_MSG_TEMP_SENSOR_EVENT = 24,
	LOG_MSG_REMOTE_HANDLING_EVENT = 25,
	LOG_MSG_REMOTE_CMD_EVENT = 26,
	LOG_MSG_LOG_EVENT = 27,
	LOG_MSG_MAIN_EVENT = 28,
	LOG_MSG_END = 29
} logMsg_e;

typedef struct {
	logMsg_e logMsgId;
	uint8_t *pFilename;
	uint32_t lineNum;
	uint32_t time;
	uint32_t payloadLength;
	uint8_t *pPayload;
	uint16_t sourceId;
	uint32_t checksum;
} logItem_t;

#endif	/* LOGGER_TYPES_H */
