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

typedef enum {
    LOG_STATUS_NOTOK = 0,
    LOG_STATUS_OK,
    LOG_STATUS_UNINITIALIZED,
    LOG_STATUS_BUF_FULL,
	LOG_STATUS_TIMEOUT,
    LOG_STATUS_END
} LogStatus_e;

#define MAX_INT_STRING_SIZE		(12)
#ifdef __linux__
#define FRAME_START_BYTE 	('<')
#define FRAME_STOP_BYTE		('>')
#endif

typedef enum  __attribute__ ((__packed__)) {
	LOG_MSG_NONE = 0,
	LOG_MSG_SYSTEM_ID = 1,
	LOG_MSG_SYSTEM_VERSION = 2,
	LOG_MSG_LOGGER_INITIALIZED = 3,
	LOG_MSG_SYSTEM_INITIALIZED = 5,
	LOG_MSG_SYSTEM_HALTED = 6,
	LOG_MSG_INFO = 7,
	LOG_MSG_WARNING = 8,
	LOG_MSG_ERROR = 9,
	LOG_MSG_HEARTBEAT = 10,
	LOG_MSG_THREAD_STATUS = 11,
	LOG_MSG_LIGHT_SENSOR_EVENT = 12,
	LOG_MSG_TEMP_SENSOR_EVENT = 13,
	LOG_MSG_REMOTE_LOG_EVENT = 14,
	LOG_MSG_REMOTE_STATUS_EVENT = 15,
	LOG_MSG_REMOTE_DATA_EVENT = 16,
	LOG_MSG_REMOTE_CMD_EVENT = 17,
	LOG_MSG_LOG_EVENT = 18,
	LOG_MSG_MAIN_EVENT = 19,
	LOG_MSG_SOLENOID_EVENT = 20,
	LOG_MSG_MOIST_EVENT = 21,
	LOG_MSG_OBSERVER_EVENT = 22,
	LOG_MSG_REMOTE_CLIENT_EVENT = 23,
	LOG_MSG_END = 24
} logMsg_e;

typedef struct {
	logMsg_e logMsgId;
	uint8_t *pFilename;
	uint16_t lineNum;
	uint32_t time;
	uint32_t payloadLength;
	uint8_t *pPayload;
	uint16_t sourceId;
	uint32_t checksum;
} logItem_t;

#endif	/* LOGGER_TYPES_H */
