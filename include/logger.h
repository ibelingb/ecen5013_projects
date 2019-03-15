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

#define LOG_BLOCKING
#define LOG

#ifndef LOG

#define LOG_FLUSH()
#define LOG_INIT()					/* implemented */
#define SYSTEM_ID()					/* implemented */
#define SYSTEM_VERSION()			/* implemented */
#define LOGGER_INITIALIZED()		/* implemented */
#define GPIO_INITIALIZED()			/* implemented */
#define SYSTEM_INITIALIZED()		/* implemented */
#define SYSTEM_HALTED()				/* implemented */
#define INFO(pStr)					/* implemented */
#define WARNING(pStr)				/* implemented */
#define ERROR(pStr)					/* implemented */
#define PROFILING_STARTED()
#define PROFILING_RESULT(pArray, statPayloadLength)
#define PROFILING_COMPLETED()
#define DATA_RECEIVED(value)		/* implemented */
#define DATA_ANALYSIS_STARTED()		/* implemented */
#define DATA_ALPHA_COUNT(count)		/* implemented */
#define DATA_NUM_COUNT(count)		/* implemented */
#define DATA_PUNC_COUNT(count)		/* implemented */
#define DATA_MISC_COUNT(count)		/* implemented */
#define DATA_ANALYSIS_COMPLETED()	/* implemented */
#define HEARTBEAT()					/* implemented */
#define CORE_DUMP()

#else /* LOGging enabled */

#ifdef __linux__
#include "logger_block.h"
#define LOG_ITEM(pLogItem)	(log_item(pLogItem))
#define LOG_INIT()			(init_logger_block())
#define LOG_FLUSH()			(log_flush())
#else
#ifdef LOG_BLOCKING
#include "logger_block.h"
#define LOG_ITEM(pLogItem)	(log_item(pLogItem))
#define LOG_INIT()			(init_logger_block())
#define LOG_FLUSH()			(log_flush())
#else
#include "logger_queue.h"
#define LOG_ITEM(pLogItem)	(log_queue_item(pLogItem))
#define LOG_INIT()			(init_queue_logger())
#define LOG_FLUSH()			(log_queue_flush())
#endif
#endif

#ifndef __linux__
#define SYSTEM_ID()({\
	logItem_t logItem;\
	uint8_t fileStr[sizeof(__FILE__)] = __FILE__;\
	uint8_t numStr[32];\
	uint16_t deviceId = (SIM->SDID & 0xFFFF0000u) >> 16;\
	logItem.logEventId = LOG_EVENT_SYSTEM_ID;\
	logItem.pFilename = &fileStr[0];\
	logItem.lineNum = __LINE__;\
	logItem.time = log_get_time();\
	logItem.payloadLength = my_itoa(deviceId, &numStr[0], HEX_BASE);\
	logItem.pPayload = &numStr[0];\
	log_set_checksum(&logItem);\
	LOG_ITEM(&logItem);\
})

#define SYSTEM_VERSION()({\
	logItem_t logItem;\
	uint8_t fileStr[sizeof(__FILE__)] = __FILE__;\
	uint8_t numStr[32];\
	uint16_t versionId = (SIM->SDID & 0x0000FF00u) | (FW_VERSION & 0xFFu);\
	logItem.logEventId = LOG_EVENT_SYSTEM_VERSION;\
	logItem.pFilename = &fileStr[0];\
	logItem.lineNum = __LINE__;\
	logItem.time = log_get_time();\
	logItem.payloadLength = my_itoa(versionId, &numStr[0], HEX_BASE);\
	logItem.pPayload = &numStr[0];\
	log_set_checksum(&logItem);\
	LOG_ITEM(&logItem);\
})
#else
#define SYSTEM_ID()
#define SYSTEM_VERSION()
#endif

#define LOGGER_INITIALIZED()({\
	logItem_t logItem;\
	uint8_t fileStr[sizeof(__FILE__)] = __FILE__;\
	logItem.logEventId = LOG_EVENT_LOGGER_INITIALIZED;\
	logItem.pFilename = &fileStr[0];\
	logItem.lineNum = __LINE__;\
	logItem.time = log_get_time();\
	logItem.pPayload = NULL;\
	logItem.payloadLength = 0;\
	log_set_checksum(&logItem);\
	LOG_ITEM(&logItem);\
})

#define GPIO_INITIALIZED()({\
	logItem_t logItem;\
	uint8_t fileStr[sizeof(__FILE__)] = __FILE__;\
	logItem.logEventId = LOG_EVENT_GPIO_INITIALIZED;\
	logItem.pFilename = &fileStr[0];\
	logItem.lineNum = __LINE__;\
	logItem.time = log_get_time();\
	logItem.pPayload = NULL;\
	logItem.payloadLength = 0;\
	log_set_checksum(&logItem);\
	LOG_ITEM(&logItem);\
})

#define SYSTEM_INITIALIZED()({\
	logItem_t logItem;\
	uint8_t fileStr[sizeof(__FILE__)] = __FILE__;\
	logItem.logEventId = LOG_EVENT_SYSTEM_INITIALIZED;\
	logItem.pFilename = &fileStr[0];\
	logItem.lineNum = __LINE__;\
	logItem.time = log_get_time();\
	logItem.pPayload = NULL;\
	logItem.payloadLength = 0;\
	log_set_checksum(&logItem);\
	LOG_ITEM(&logItem);\
})

#define SYSTEM_HALTED()({\
	logItem_t logItem;\
	uint8_t fileStr[sizeof(__FILE__)] = __FILE__;\
	logItem.logEventId = LOG_EVENT_SYSTEM_HALTED;\
	logItem.pFilename = &fileStr[0];\
	logItem.lineNum = __LINE__;\
	logItem.time = log_get_time();\
	logItem.pPayload = NULL;\
	logItem.payloadLength = 0;\
	log_set_checksum(&logItem);\
	LOG_ITEM(&logItem);\
})

#define INFO(pStr)({\
	logItem_t logItem;\
	uint8_t fileStr[sizeof(__FILE__)] = __FILE__;\
	logItem.logEventId = LOG_EVENT_INFO;\
	logItem.pFilename = &fileStr[0];\
	logItem.lineNum = __LINE__;\
	logItem.time = log_get_time();\
	logItem.payloadLength = log_strlen((uint8_t *)pStr);\
	logItem.pPayload = (uint8_t *)pStr;\
	log_set_checksum(&logItem);\
	LOG_ITEM(&logItem);\
})

#define WARNING(pStr)({\
	logItem_t logItem;\
	uint8_t fileStr[sizeof(__FILE__)] = __FILE__;\
	logItem.logEventId = LOG_EVENT_WARNING;\
	logItem.pFilename = &fileStr[0];\
	logItem.lineNum = __LINE__;\
	logItem.time = log_get_time();\
	logItem.payloadLength = log_strlen((uint8_t *)pStr);\
	logItem.pPayload = (uint8_t *)pStr;\
	log_set_checksum(&logItem);\
	LOG_ITEM(&logItem);\
})

#define ERROR(pStr)({\
	logItem_t logItem;\
	uint8_t fileStr[sizeof(__FILE__)] = __FILE__;\
	logItem.logEventId = LOG_EVENT_ERROR;\
	logItem.pFilename = &fileStr[0];\
	logItem.lineNum = __LINE__;\
	logItem.time = log_get_time();\
	logItem.payloadLength = log_strlen((uint8_t *)pStr);\
	logItem.pPayload = (uint8_t *)pStr;\
	log_set_checksum(&logItem);\
	LOG_ITEM(&logItem);\
})

#define PROFILING_STARTED()({\
	logItem_t logItem;\
	uint8_t fileStr[sizeof(__FILE__)] = __FILE__;\
	logItem.logEventId = LOG_EVENT_PROFILING_STARTED;\
	logItem.pFilename = &fileStr[0];\
	logItem.lineNum = __LINE__;\
	logItem.time = log_get_time();\
	logItem.pPayload = NULL;\
	logItem.payloadLength = 0;\
	log_set_checksum(&logItem);\
	LOG_ITEM(&logItem);\
})

#define PROFILING_RESULT(pArray, statPayloadLength)({\
	logItem_t logItem;\
	uint8_t fileStr[sizeof(__FILE__)] = __FILE__;\
	logItem.logEventId = LOG_EVENT_PROFILING_RESULT;\
	logItem.pFilename = &fileStr[0];\
	logItem.lineNum = __LINE__;\
	logItem.time = log_get_time();\
	logItem.payloadLength = statPayloadLength;\
	logItem.pPayload = (uint8_t *)pArray;\
	log_set_checksum(&logItem);\
	LOG_ITEM(&logItem);\
})

#define PROFILING_COMPLETED()({\
	logItem_t logItem;\
	uint8_t fileStr[sizeof(__FILE__)] = __FILE__;\
	logItem.logEventId = LOG_EVENT_PROFILING_COMPLETED;\
	logItem.pFilename = &fileStr[0];\
	logItem.lineNum = __LINE__;\
	logItem.time = log_get_time();\
	logItem.pPayload = NULL;\
	logItem.payloadLength = 0;\
	log_set_checksum(&logItem);\
	LOG_ITEM(&logItem);\
})

#define DATA_RECEIVED(value)({\
	logItem_t logItem;\
	uint8_t fileStr[sizeof(__FILE__)] = __FILE__;\
	uint8_t numStr[2];\
	numStr[0] = value;\
	numStr[1] = '\0';\
	logItem.logEventId = LOG_EVENT_DATA_RECEIVED;\
	logItem.pFilename = &fileStr[0];\
	logItem.lineNum = __LINE__;\
	logItem.time = log_get_time();\
	logItem.payloadLength = 2;\
	logItem.pPayload = &numStr[0];\
	log_set_checksum(&logItem);\
	LOG_ITEM(&logItem);\
})

#define DATA_ANALYSIS_STARTED()({\
	logItem_t logItem;\
	uint8_t fileStr[sizeof(__FILE__)] = __FILE__;\
	logItem.logEventId = LOG_EVENT_DATA_ANALYSIS_STARTED;\
	logItem.pFilename = &fileStr[0];\
	logItem.lineNum = __LINE__;\
	logItem.time = log_get_time();\
	logItem.pPayload = NULL;\
	logItem.payloadLength = 0;\
	log_set_checksum(&logItem);\
	LOG_ITEM(&logItem);\
})

#define DATA_ALPHA_COUNT(count)({\
	logItem_t logItem;\
	uint8_t fileStr[sizeof(__FILE__)] = __FILE__;\
	uint8_t numStr[32];\
	logItem.logEventId = LOG_EVENT_DATA_ALPHA_COUNT;\
	logItem.pFilename = &fileStr[0];\
	logItem.lineNum = __LINE__;\
	logItem.time = log_get_time();\
	logItem.payloadLength = my_itoa(count, &numStr[0], HEX_BASE);\
	logItem.pPayload = &numStr[0];\
	log_set_checksum(&logItem);\
	LOG_ITEM(&logItem);\
})
#define DATA_NUM_COUNT(count)({\
	logItem_t logItem;\
	uint8_t fileStr[sizeof(__FILE__)] = __FILE__;\
	uint8_t numStr[32];\
	logItem.logEventId = LOG_EVENT_DATA_NUM_COUNT;\
	logItem.pFilename = &fileStr[0];\
	logItem.lineNum = __LINE__;\
	logItem.time = log_get_time();\
	logItem.payloadLength = my_itoa(count, &numStr[0], HEX_BASE);\
	logItem.pPayload = &numStr[0];\
	log_set_checksum(&logItem);\
	LOG_ITEM(&logItem);\
})
#define DATA_PUNC_COUNT(count)({\
	logItem_t logItem;\
	uint8_t fileStr[sizeof(__FILE__)] = __FILE__;\
	uint8_t numStr[32];\
	logItem.logEventId = LOG_EVENT_DATA_PUNC_COUNT;\
	logItem.pFilename = &fileStr[0];\
	logItem.lineNum = __LINE__;\
	logItem.time = log_get_time();\
	logItem.payloadLength = my_itoa(count, &numStr[0], HEX_BASE);\
	logItem.pPayload = &numStr[0];\
	log_set_checksum(&logItem);\
	LOG_ITEM(&logItem);\
})

#define DATA_MISC_COUNT(count)({\
	logItem_t logItem;\
	uint8_t fileStr[sizeof(__FILE__)] = __FILE__;\
	uint8_t numStr[32];\
	logItem.logEventId = LOG_EVENT_DATA_MISC_COUNT;\
	logItem.pFilename = &fileStr[0];\
	logItem.lineNum = __LINE__;\
	logItem.time = log_get_time();\
	logItem.payloadLength = my_itoa(count, &numStr[0], HEX_BASE);\
	logItem.pPayload = &numStr[0];\
	log_set_checksum(&logItem);\
	LOG_ITEM(&logItem);\
})

#define DATA_ANALYSIS_COMPLETED()({\
	logItem_t logItem;\
	uint8_t fileStr[sizeof(__FILE__)] = __FILE__;\
	logItem.logEventId = LOG_EVENT_DATA_ANALYSIS_COMPLETED;\
	logItem.pFilename = &fileStr[0];\
	logItem.lineNum = __LINE__;\
	logItem.time = log_get_time();\
	logItem.pPayload = NULL;\
	logItem.payloadLength = 0;\
	log_set_checksum(&logItem);\
	LOG_ITEM(&logItem);\
})

#define HEARTBEAT()({\
	logItem_t logItem;\
	uint8_t fileStr[sizeof(__FILE__)] = __FILE__;\
	logItem.logEventId = LOG_EVENT_HEARTBEAT;\
	logItem.pFilename = &fileStr[0];\
	logItem.lineNum = __LINE__;\
	logItem.time = log_get_time();\
	logItem.pPayload = NULL;\
	logItem.payloadLength = 0;\
	log_set_checksum(&logItem);\
	LOG_ITEM(&logItem);\
})

#define CORE_DUMP()
#endif  /* DEBUG */
#endif	/* LOGGER_H */
