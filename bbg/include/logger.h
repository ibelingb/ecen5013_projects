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
#include "logger_queue.h"
#include "conversion.h"
#include "packet.h"

#ifdef __linux__
    #include <stdio.h>
#else
    #include "FreeRTOS.h"
    #include "task.h"
    #include "uartstdio.h"
    #include "main.h"
#endif

#define SHORT_CIRCUIT_FOR_DEBUG (1)
#define DEBUG_TEST_ALL_MSG_TYPES (0)

#define LOG_MSG_QUEUE
//#define LOG

#ifndef LOG
    #define LOG_FLUSH()
    #define LOG_INIT(threadArgs)			(LOG_STATUS_OK)
    #define LOG_DEQUEUE_ITEM(pLogItem)		(LOG_STATUS_OK)
    #define LOG_WRITE_ITEM(pLogItem, fd)	(LOG_STATUS_OK)
    #define LOG_LOGGER_INITIALIZED()		/* implemented */
    #define LOG_SYSTEM_INITIALIZED()		/* implemented */
    #define LOG_SYSTEM_HALTED()				/* implemented */
    #define LOG_INFO(pStr)					/* implemented */
    #define LOG_WARNING(pStr)				/* implemented */
    #define LOG_ERROR(pStr)					/* implemented */
    #define LOG_HEARTBEAT()					/* implemented */
    #define LOG_THREAD_STATUS(tid)
    #define LOG_THREAD_EVENT(event_e, eventId)
    #define LOG_TEMP_SENSOR_EVENT(event_e)
    #define LOG_LIGHT_SENSOR_EVENT(event_e)
    #define LOG_REMOTE_HANDLING_EVENT(event_e)
    #define LOG_REMOTE_CMD_EVENT(event_e)
    #define LOG_LOG_EVENT(event_e)
    #define LOG_MAIN_EVENT(event_e)
    #define LOG_REMOTE_CLIENT_EVENT(event_e)
    #define LOG_SOLENOID_EVENT(event_e)
    #define LOG_OBSERVER_EVENT(event_e)
    #define LOG_MOISTURE_EVENT(event_e)
#else /* LOGging enabled */
    #define LOG_ITEM(pLogItem)              (log_queue_item(pLogItem))
    #define LOG_DEQUEUE_ITEM(pLogItem)      (log_dequeue_item(pLogItem))
    #define LOG_INIT(pArg)                  (init_queue_logger(pArg))
    #define LOG_FLUSH()                     (log_queue_flush())
    #define LOG_WRITE_ITEM(pLogItem, fd)    (log_write_item(pLogItem, fd))

    #ifdef __linux__
        #include <sys/syscall.h>
        #define LOG_GET_SRC_ID()				((pid_t)syscall(SYS_gettid))
    #else  // not LINUX
            #define LOG_GET_SRC_ID()            (getTaskNum())
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
        logItem.sourceId = LOG_GET_SRC_ID();\
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
        logItem.sourceId = LOG_GET_SRC_ID();\
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
        logItem.sourceId = LOG_GET_SRC_ID();\
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
        logItem.sourceId = LOG_GET_SRC_ID();\
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
        logItem.sourceId = LOG_GET_SRC_ID();\
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
        logItem.sourceId = LOG_GET_SRC_ID();\
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
        logItem.sourceId = LOG_GET_SRC_ID();\
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
        logItem.sourceId = LOG_GET_SRC_ID();\
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
        logItem.sourceId = LOG_GET_SRC_ID();\
        log_set_checksum(&logItem);\
        LOG_ITEM(&logItem);\
    })

    #define LOG_TEMP_SENSOR_EVENT(event_e)		    (LOG_THREAD_EVENT(event_e, LOG_MSG_TEMP_SENSOR_EVENT))
    #define LOG_LIGHT_SENSOR_EVENT(event_e)		    (LOG_THREAD_EVENT(event_e, LOG_MSG_LIGHT_SENSOR_EVENT))
    #define LOG_REMOTE_HANDLING_EVENT(event_e)	    (LOG_THREAD_EVENT(event_e, LOG_MSG_REMOTE_HANDLING_EVENT))
    #define LOG_REMOTE_CLIENT_EVENT(event_e)        (LOG_THREAD_EVENT(event_e, LOG_MSG_REMOTE_CLIENT_EVENT))
    #define LOG_REMOTE_CMD_EVENT(event_e)	        (LOG_THREAD_EVENT(event_e, LOG_MSG_REMOTE_CMD_EVENT))
    #define LOG_LOG_EVENT(event_e)				    (LOG_THREAD_EVENT(event_e, LOG_MSG_LOG_EVENT))
    #define LOG_MAIN_EVENT(event_e)				    (LOG_THREAD_EVENT(event_e, LOG_MSG_MAIN_EVENT))
    #define LOG_SOLENOID_EVENT(event_e)             (LOG_THREAD_EVENT(event_e, LOG_MSG_SOLENOID_EVENT))
    #define LOG_OBSERVER_EVENT(event_e)             (LOG_THREAD_EVENT(event_e, LOG_MSG_OBSERVER_EVENT))
    #define LOG_MOISTURE_EVENT(event_e)             (LOG_THREAD_EVENT(event_e, LOG_MSG_MOIST_EVENT))
#endif  /* LOG */

/**
 * @brief print log header info
 *
 */
__attribute__((always_inline)) inline void PRINT_LOG_MSG_HEADER(LogMsgPacket *pLog)
{
#ifdef __linux__
    printf("Recvd LOG MsgId: %d, from sourceId: %d at %d usec\n\r",pLog->logMsgId, pLog->sourceId, pLog->timestamp);
#else
    UARTprintf("Recvd LOG MsgId: %d, from sourceId: %d at %d usec\n\r",pLog->logMsgId, pLog->sourceId, pLog->timestamp);
#endif
}
#endif	/* LOGGER_H */
