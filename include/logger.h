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
 * @file logger.h
 * @brief blocking functions for sending data to logger
 *
 ************************************************************************************
 */

#ifndef LOGGER_H
#define	LOGGER_H

#include <stddef.h>
#include "logger_types.h"
#include "logger_helper.h"
#include "conversion.h"

#define LOG_MSG_QUEUE
#define LOG

#ifndef LOG

#define LOG_FLUSH()
#define LOG_INIT()						/* implemented */
#define LOG_SYSTEM_ID()					/* implemented */
#define LOG_SYSTEM_VERSION()			/* implemented */
#define LOG_LOGGER_INITIALIZED()		/* implemented */
#define LOG_GPIO_INITIALIZED()			/* implemented */
#define LOG_SYSTEM_INITIALIZED()		/* implemented */
#define LOG_SYSTEM_HALTED()				/* implemented */
#define LOG_INFO(pStr)					/* implemented */
#define LOG_WARNING(pStr)				/* implemented */
#define LOG_ERROR(pStr)					/* implemented */
#define LOG_PROFILING_STARTED()
#define LOG_PROFILING_RESULT(pArray, statPayloadLength)
#define LOG_PROFILING_COMPLETED()
#define LOG_DATA_RECEIVED(value)		/* implemented */
#define LOG_DATA_ANALYSIS_STARTED()		/* implemented */
#define LOG_DATA_ALPHA_COUNT(count)		/* implemented */
#define LOG_DATA_NUM_COUNT(count)		/* implemented */
#define LOG_DATA_PUNC_COUNT(count)		/* implemented */
#define LOG_DATA_MISC_COUNT(count)		/* implemented */
#define LOG_DATA_ANALYSIS_COMPLETED()	/* implemented */
#define LOG_HEARTBEAT()					/* implemented */
#define LOG_CORE_DUMP()
#define LOG_THREAD_STATUS(tid)
#define LOG_THREAD_EVENT(event_e, eventId)
#define LOG_TEMP_SENSOR_EVENT(event_e)
#define LOG_LIGHT_SENSOR_EVENT(event_e)	
#define LOG_REMOTE_HANDLING_EVENT(event_e)	
#define LOG_LOG_EVENT(event_e)				
#define LOG_MAIN_EVENT(event_e)	

#else /* LOGging enabled */

#ifdef __linux__
#ifdef LOG_MSG_QUEUE
#include "logger_queue.h"
#define LOG_ITEM(pLogItem)		(log_queue_item(pLogItem))
#define LOG_INIT(pArg)			(init_queue_logger(pArg))
#define LOG_FLUSH()				(log_queue_flush())
#else
#include "logger_block.h"
#define LOG_ITEM(pLogItem)		(log_item(pLogItem))
#define LOG_INIT(pArg)			(init_logger_block(pArg))
#define LOG_FLUSH()				(log_flush())
#endif
#else
#ifdef LOG_BLOCKING
#include "logger_block.h"
#define LOG_ITEM(pLogItem)		(log_item(pLogItem))
#define LOG_INIT()				(init_logger_block())
#define LOG_FLUSH()				(log_flush())
#else
#include "logger_queue.h"
#define LOG_ITEM(pLogItem)		(log_queue_item(pLogItem))
#define LOG_INIT()				(init_queue_logger())
#define LOG_FLUSH()				(log_queue_flush())
#endif
#endif

#ifndef __linux__
#define LOG_SYSTEM_ID()({\
	logItem_t logItem;\
	uint8_t fileStr[sizeof(__FILE__)] = __FILE__;\
	uint8_t numStr[32];\
	uint16_t deviceId = (SIM->SDID & 0xFFFF0000u) >> 16;\
	logItem.logMsgId = LOG_MSG_SYSTEM_ID;\
	logItem.pFilename = &fileStr[0];\
	logItem.lineNum = __LINE__;\
	logItem.time = log_get_time();\
	logItem.payloadLength = my_itoa(deviceId, &numStr[0], HEX_BASE);\
	logItem.pPayload = &numStr[0];\
	log_set_checksum(&logItem);\
	LOG_ITEM(&logItem);\
})

#define LOG_SYSTEM_VERSION()({\
	logItem_t logItem;\
	uint8_t fileStr[sizeof(__FILE__)] = __FILE__;\
	uint8_t numStr[32];\
	uint16_t versionId = (SIM->SDID & 0x0000FF00u) | (FW_VERSION & 0xFFu);\
	logItem.logMsgId = LOG_MSG_SYSTEM_VERSION;\
	logItem.pFilename = &fileStr[0];\
	logItem.lineNum = __LINE__;\
	logItem.time = log_get_time();\
	logItem.payloadLength = my_itoa(versionId, &numStr[0], HEX_BASE);\
	logItem.pPayload = &numStr[0];\
	log_set_checksum(&logItem);\
	LOG_ITEM(&logItem);\
})
#else
#define LOG_SYSTEM_ID()
#define LOG_SYSTEM_VERSION()
#endif

#define LOG_LOGGER_INITIALIZED()({\
	logItem_t logItem;\
	uint8_t fileStr[sizeof(__FILE__)] = __FILE__;\
	logItem.logMsgId = LOG_MSG_LOGGER_INITIALIZED;\
	logItem.pFilename = &fileStr[0];\
	logItem.lineNum = __LINE__;\
	logItem.time = log_get_time();\
	logItem.pPayload = NULL;\
	logItem.payloadLength = 0;\
	log_set_checksum(&logItem);\
	LOG_ITEM(&logItem);\
})

#define LOG_GPIO_INITIALIZED()({\
	logItem_t logItem;\
	uint8_t fileStr[sizeof(__FILE__)] = __FILE__;\
	logItem.logMsgId = LOG_MSG_GPIO_INITIALIZED;\
	logItem.pFilename = &fileStr[0];\
	logItem.lineNum = __LINE__;\
	logItem.time = log_get_time();\
	logItem.pPayload = NULL;\
	logItem.payloadLength = 0;\
	log_set_checksum(&logItem);\
	LOG_ITEM(&logItem);\
})

#define LOG_SYSTEM_INITIALIZED()({\
	logItem_t logItem;\
	uint8_t fileStr[sizeof(__FILE__)] = __FILE__;\
	logItem.logMsgId = LOG_MSG_SYSTEM_INITIALIZED;\
	logItem.pFilename = &fileStr[0];\
	logItem.lineNum = __LINE__;\
	logItem.time = log_get_time();\
	logItem.pPayload = NULL;\
	logItem.payloadLength = 0;\
	log_set_checksum(&logItem);\
	LOG_ITEM(&logItem);\
})

#define LOG_SYSTEM_HALTED()({\
	logItem_t logItem;\
	uint8_t fileStr[sizeof(__FILE__)] = __FILE__;\
	logItem.logMsgId = LOG_MSG_SYSTEM_HALTED;\
	logItem.pFilename = &fileStr[0];\
	logItem.lineNum = __LINE__;\
	logItem.time = log_get_time();\
	logItem.pPayload = NULL;\
	logItem.payloadLength = 0;\
	log_set_checksum(&logItem);\
	LOG_ITEM(&logItem);\
})

#define LOG_INFO(pStr)({\
	logItem_t logItem;\
	uint8_t fileStr[sizeof(__FILE__)] = __FILE__;\
	logItem.logMsgId = LOG_MSG_INFO;\
	logItem.pFilename = &fileStr[0];\
	logItem.lineNum = __LINE__;\
	logItem.time = log_get_time();\
	logItem.payloadLength = log_strlen((uint8_t *)pStr);\
	logItem.pPayload = (uint8_t *)pStr;\
	log_set_checksum(&logItem);\
	LOG_ITEM(&logItem);\
})

#define LOG_WARNING(pStr)({\
	logItem_t logItem;\
	uint8_t fileStr[sizeof(__FILE__)] = __FILE__;\
	logItem.logMsgId = LOG_MSG_WARNING;\
	logItem.pFilename = &fileStr[0];\
	logItem.lineNum = __LINE__;\
	logItem.time = log_get_time();\
	logItem.payloadLength = log_strlen((uint8_t *)pStr);\
	logItem.pPayload = (uint8_t *)pStr;\
	log_set_checksum(&logItem);\
	LOG_ITEM(&logItem);\
})

#define LOG_ERROR(pStr)({\
	logItem_t logItem;\
	uint8_t fileStr[sizeof(__FILE__)] = __FILE__;\
	logItem.logMsgId = LOG_MSG_ERROR;\
	logItem.pFilename = &fileStr[0];\
	logItem.lineNum = __LINE__;\
	logItem.time = log_get_time();\
	logItem.payloadLength = log_strlen((uint8_t *)pStr);\
	logItem.pPayload = (uint8_t *)pStr;\
	log_set_checksum(&logItem);\
	LOG_ITEM(&logItem);\
})

#define LOG_PROFILING_STARTED()({\
	logItem_t logItem;\
	uint8_t fileStr[sizeof(__FILE__)] = __FILE__;\
	logItem.logMsgId = LOG_MSG_PROFILING_STARTED;\
	logItem.pFilename = &fileStr[0];\
	logItem.lineNum = __LINE__;\
	logItem.time = log_get_time();\
	logItem.pPayload = NULL;\
	logItem.payloadLength = 0;\
	log_set_checksum(&logItem);\
	LOG_ITEM(&logItem);\
})

#define LOG_PROFILING_RESULT(pArray, statPayloadLength)({\
	logItem_t logItem;\
	uint8_t fileStr[sizeof(__FILE__)] = __FILE__;\
	logItem.logMsgId = LOG_MSG_PROFILING_RESULT;\
	logItem.pFilename = &fileStr[0];\
	logItem.lineNum = __LINE__;\
	logItem.time = log_get_time();\
	logItem.payloadLength = statPayloadLength;\
	logItem.pPayload = (uint8_t *)pArray;\
	log_set_checksum(&logItem);\
	LOG_ITEM(&logItem);\
})

#define LOG_PROFILING_COMPLETED()({\
	logItem_t logItem;\
	uint8_t fileStr[sizeof(__FILE__)] = __FILE__;\
	logItem.logMsgId = LOG_MSG_PROFILING_COMPLETED;\
	logItem.pFilename = &fileStr[0];\
	logItem.lineNum = __LINE__;\
	logItem.time = log_get_time();\
	logItem.pPayload = NULL;\
	logItem.payloadLength = 0;\
	log_set_checksum(&logItem);\
	LOG_ITEM(&logItem);\
})

#define LOG_DATA_RECEIVED(value)({\
	logItem_t logItem;\
	uint8_t fileStr[sizeof(__FILE__)] = __FILE__;\
	uint8_t numStr[2];\
	numStr[0] = value;\
	numStr[1] = '\0';\
	logItem.logMsgId = LOG_MSG_DATA_RECEIVED;\
	logItem.pFilename = &fileStr[0];\
	logItem.lineNum = __LINE__;\
	logItem.time = log_get_time();\
	logItem.payloadLength = 2;\
	logItem.pPayload = &numStr[0];\
	log_set_checksum(&logItem);\
	LOG_ITEM(&logItem);\
})

#define LOG_DATA_ANALYSIS_STARTED()({\
	logItem_t logItem;\
	uint8_t fileStr[sizeof(__FILE__)] = __FILE__;\
	logItem.logMsgId = LOG_MSG_DATA_ANALYSIS_STARTED;\
	logItem.pFilename = &fileStr[0];\
	logItem.lineNum = __LINE__;\
	logItem.time = log_get_time();\
	logItem.pPayload = NULL;\
	logItem.payloadLength = 0;\
	log_set_checksum(&logItem);\
	LOG_ITEM(&logItem);\
})

#define LOG_DATA_ALPHA_COUNT(count)({\
	logItem_t logItem;\
	uint8_t fileStr[sizeof(__FILE__)] = __FILE__;\
	uint8_t numStr[32];\
	logItem.logMsgId = LOG_MSG_DATA_ALPHA_COUNT;\
	logItem.pFilename = &fileStr[0];\
	logItem.lineNum = __LINE__;\
	logItem.time = log_get_time();\
	logItem.payloadLength = my_itoa(count, &numStr[0], HEX_BASE);\
	logItem.pPayload = &numStr[0];\
	log_set_checksum(&logItem);\
	LOG_ITEM(&logItem);\
})

#define LOG_DATA_NUM_COUNT(count)({\
	logItem_t logItem;\
	uint8_t fileStr[sizeof(__FILE__)] = __FILE__;\
	uint8_t numStr[32];\
	logItem.logMsgId = LOG_MSG_DATA_NUM_COUNT;\
	logItem.pFilename = &fileStr[0];\
	logItem.lineNum = __LINE__;\
	logItem.time = log_get_time();\
	logItem.payloadLength = my_itoa(count, &numStr[0], HEX_BASE);\
	logItem.pPayload = &numStr[0];\
	log_set_checksum(&logItem);\
	LOG_ITEM(&logItem);\
})

#define LOG_DATA_PUNC_COUNT(count)({\
	logItem_t logItem;\
	uint8_t fileStr[sizeof(__FILE__)] = __FILE__;\
	uint8_t numStr[32];\
	logItem.logMsgId = LOG_MSG_DATA_PUNC_COUNT;\
	logItem.pFilename = &fileStr[0];\
	logItem.lineNum = __LINE__;\
	logItem.time = log_get_time();\
	logItem.payloadLength = my_itoa(count, &numStr[0], HEX_BASE);\
	logItem.pPayload = &numStr[0];\
	log_set_checksum(&logItem);\
	LOG_ITEM(&logItem);\
})

#define LOG_DATA_MISC_COUNT(count)({\
	logItem_t logItem;\
	uint8_t fileStr[sizeof(__FILE__)] = __FILE__;\
	uint8_t numStr[32];\
	logItem.logMsgId = LOG_MSG_DATA_MISC_COUNT;\
	logItem.pFilename = &fileStr[0];\
	logItem.lineNum = __LINE__;\
	logItem.time = log_get_time();\
	logItem.payloadLength = my_itoa(count, &numStr[0], HEX_BASE);\
	logItem.pPayload = &numStr[0];\
	log_set_checksum(&logItem);\
	LOG_ITEM(&logItem);\
})

#define LOG_DATA_ANALYSIS_COMPLETED()({\
	logItem_t logItem;\
	uint8_t fileStr[sizeof(__FILE__)] = __FILE__;\
	logItem.logMsgId = LOG_MSG_DATA_ANALYSIS_COMPLETED;\
	logItem.pFilename = &fileStr[0];\
	logItem.lineNum = __LINE__;\
	logItem.time = log_get_time();\
	logItem.pPayload = NULL;\
	logItem.payloadLength = 0;\
	log_set_checksum(&logItem);\
	LOG_ITEM(&logItem);\
})

#define LOG_HEARTBEAT()({\
	logItem_t logItem;\
	uint8_t fileStr[sizeof(__FILE__)] = __FILE__;\
	logItem.logMsgId = LOG_MSG_HEARTBEAT;\
	logItem.pFilename = &fileStr[0];\
	logItem.lineNum = __LINE__;\
	logItem.time = log_get_time();\
	logItem.pPayload = NULL;\
	logItem.payloadLength = 0;\
	log_set_checksum(&logItem);\
	LOG_ITEM(&logItem);\
})

#define CORE_DUMP()

#define LOG_THREAD_STATUS(tid)({\
	logItem_t logItem;\
	uint8_t fileStr[sizeof(__FILE__)] = __FILE__;\
	uint8_t numStr[32];\
	logItem.logMsgId = LOG_MSG_DATA_ALPHA_COUNT;\
	logItem.pFilename = &fileStr[0];\
	logItem.lineNum = __LINE__;\
	logItem.time = log_get_time();\
	logItem.payloadLength = my_itoa(tid, &numStr[0], HEX_BASE);\
	logItem.pPayload = &numStr[0];\
	log_set_checksum(&logItem);\
	LOG_ITEM(&logItem);\
})

#define LOG_THREAD_EVENT(event_e, eventId)({\
	logItem_t logItem;\
	uint8_t fileStr[sizeof(__FILE__)] = __FILE__;\
	uint8_t numStr[32];\
	logItem.logMsgId = eventId;\
	logItem.pFilename = &fileStr[0];\
	logItem.lineNum = __LINE__;\
	logItem.time = log_get_time();\
	logItem.payloadLength = my_itoa((uint8_t)event_e, &numStr[0], HEX_BASE);\
	logItem.pPayload = &numStr[0];\
	log_set_checksum(&logItem);\
	LOG_ITEM(&logItem);\
})

#define LOG_TEMP_SENSOR_EVENT(event_e)		(LOG_THREAD_EVENT(event_e, LOG_MSG_TEMP_SENSOR_EVENT))
#define LOG_LIGHT_SENSOR_EVENT(event_e)		(LOG_THREAD_EVENT(event_e, LOG_MSG_LIGHT_SENSOR_EVENT))
#define LOG_REMOTE_HANDLING_EVENT(event_e)	(LOG_THREAD_EVENT(event_e, LOG_MSG_REMOTE_HANDLING_EVENT))
#define LOG_LOG_EVENT(event_e)				(LOG_THREAD_EVENT(event_e, LOG_MSG_LOG_EVENT))
#define LOG_MAIN_EVENT(event_e)				(LOG_THREAD_EVENT(event_e, LOG_MSG_MAIN_EVENT))

#endif  /* DEBUG */
#endif	/* LOGGER_H */
