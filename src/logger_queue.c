/***********************************************************************************
 * @author Josh Malburg
 * joshua.malburg@colorado.edu
 * Advanced Embedded Software Development
 * ECEN5013 - Rick Heidebrecht
 * @date March 15, 2019
 * arm-linux-gnueabi (Buildroot)
 * gcc (Ubuntu)
 ************************************************************************************
 *
 * @file logger_queue.c
 * @brief functions for queueing, etc to posix logger queue
 *
 ************************************************************************************
 */

#include <stdio.h>
#include <mqueue.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>	// for write, open, etc
#include <time.h>	// for timespec
#include <pthread.h>
#include <sys/syscall.h>

#include "my_debug.h"
#include "logger_types.h"
#include "packet.h"

#include "conversion.h"

/* globals */
static mqd_t logQueue = -1;

/* private functions */
uint8_t log_data(uint8_t *pData, uint32_t len, int fileFd);
uint8_t log_string(uint8_t *pStr, int fileFd);
uint8_t log_integer(int32_t num, int fileFd);
uint8_t log_byte(uint8_t value, int fileFd);

uint8_t init_queue_logger(void *pArg)
{
	/* re-open msg queue for read/write */
	LogThreadInfo args = *(LogThreadInfo *)pArg;
	logQueue = mq_open(args.logMsgQueueName, O_RDWR, 0, NULL);
	if(logQueue < 0)
	{
        ERROR_PRINT("mq_open failed, err#%d (%s)\n\r", errno, strerror(errno));
        return LOG_STATUS_NOTOK;
    }
	return LOG_STATUS_OK;
}

uint8_t log_queue_item(logItem_t *pLogItem)
{
	LogMsgPacket newItem;

	if(logQueue < 0)
	{
        ERROR_PRINT("log msg queue not initialized\n");
        return LOG_STATUS_NOTOK;
    }
	newItem.logMsgId = pLogItem->logMsgId;
	newItem.lineNum = pLogItem->lineNum;
	newItem.timestamp = pLogItem->time;
	newItem.payloadLength = pLogItem->payloadLength;
	newItem.sourceId = pLogItem->sourceId;
	newItem.checksum = pLogItem->checksum;
	
	/* copy filename string */
	if(strcpy((char *)newItem.filename, (char *)pLogItem->pFilename) == NULL)
		return LOG_STATUS_NOTOK;

	/* copy payload */
	if(memcpy(newItem.payload, pLogItem->pPayload, pLogItem->payloadLength) == NULL)
		return LOG_STATUS_NOTOK;

	/* send, use 7 as priority */
	if(mq_send(logQueue, (char *)&newItem, sizeof(LogMsgPacket), 7) < 0)
	{
        ERROR_PRINT("mq_receive failed, err#%d (%s)\n\r", errno, strerror(errno));
        return LOG_STATUS_NOTOK;
    }
	return LOG_STATUS_OK;
}

void log_queue_flush(void)
{
	if(logQueue < 0)
	{
        ERROR_PRINT("log msg queue not initialized\n");
        return;
    }
	return;
}

uint8_t log_dequeue_item(logItem_t *pLogItem)
{
	LogMsgPacket newItem;
	struct timespec rxTimeout;
	size_t bytesRead;
	struct mq_attr Attr;

	if(logQueue < 0)
	{
        ERROR_PRINT("log msg queue not initialized\n");
        return LOG_STATUS_NOTOK;
    }

	/* check queue length diagonistics */
	mq_getattr(logQueue, &Attr);
	MUTED_PRINT("logMsgQueue count:%ld\n", Attr.mq_curmsgs);
	if(Attr.mq_curmsgs > ((MSG_QUEUE_DEPTH * 3) / 4)) {
		if(Attr.mq_curmsgs >= MSG_QUEUE_DEPTH - 1) {
			ERROR_PRINT("logQueue full\n");
		}
		WARN_PRINT("logQueue have 3/4 full \n");
	}

	clock_gettime(CLOCK_REALTIME, &rxTimeout);
	rxTimeout.tv_sec += 1;

	/* read oldest highest priority queue */
	bytesRead = mq_timedreceive(logQueue, (char *)&newItem, sizeof(LogMsgPacket), NULL, &rxTimeout);
	if(bytesRead == -1)
    {
        if((errno == EINTR) || (errno == ETIMEDOUT))
		{
			MUTED_PRINT("mq_timedreceive timeout or interrupted: err#%d (%s)\n\r", errno, strerror(errno));
			return LOG_STATUS_OK;
		}
		ERROR_PRINT("mq_timedreceive failed, err#%d (%s)\n\r", errno, strerror(errno));
        return LOG_STATUS_NOTOK;
    }
	if(bytesRead == sizeof(LogMsgPacket))
	{
		pLogItem->logMsgId = newItem.logMsgId;
		pLogItem->lineNum = newItem.lineNum;
		pLogItem->time = newItem.timestamp;
		pLogItem->payloadLength = newItem.payloadLength;
		pLogItem->sourceId = newItem.sourceId;
		pLogItem->checksum = newItem.checksum;
		
		/* copy filename string */
		if(strcpy((char *)pLogItem->pFilename, (char *)newItem.filename) == NULL)
			return LOG_STATUS_NOTOK;

		/* copy payload */
		if(memcpy(pLogItem->pPayload, newItem.payload, pLogItem->payloadLength) == NULL)
			return LOG_STATUS_NOTOK;
			
		return LOG_STATUS_OK;
	}
	return LOG_STATUS_NOTOK;
}

uint8_t log_write_item(logItem_t *pLogItem, int fileFd)
{
	if(log_byte(FRAME_START_BYTE, fileFd) == LOG_STATUS_OK){
		if(log_integer(pLogItem->logMsgId, fileFd) == LOG_STATUS_OK){
			if(log_string(pLogItem->pFilename, fileFd) == LOG_STATUS_OK){
				if(log_integer(pLogItem->lineNum, fileFd) == LOG_STATUS_OK){
					if(log_integer(pLogItem->time, fileFd) == LOG_STATUS_OK){
						if(log_integer(pLogItem->payloadLength, fileFd) == LOG_STATUS_OK){
							if(log_data(pLogItem->pPayload, pLogItem->payloadLength, fileFd) == LOG_STATUS_OK){
								if(log_integer(pLogItem->sourceId, fileFd) == LOG_STATUS_OK){
									if(log_integer(pLogItem->checksum, fileFd) == LOG_STATUS_OK){
										if(log_byte(FRAME_STOP_BYTE, fileFd) == LOG_STATUS_OK){
											{
												return LOG_STATUS_OK;
											}
										}
									}
								}
							}
						}
					}
				}
			}
		}
	}
	ERROR_PRINT("error writing logItem\n");
	return LOG_STATUS_NOTOK;
}

/***** private functions *****/

uint8_t log_data(uint8_t *pData, uint32_t len, int fileFd)
{
	if(pData == NULL)
		return LOG_STATUS_NOTOK;

	if(write(fileFd, pData, len) < len) {
		return LOG_STATUS_NOTOK;
	}
	return LOG_STATUS_OK;
}

uint8_t log_byte(uint8_t value, int fileFd)
{
	if(write(fileFd, &value, 1) < 1) {
		return LOG_STATUS_NOTOK;
	}
	return LOG_STATUS_OK;
}

uint8_t log_string(uint8_t *pStr, int fileFd)
{
	uint8_t *pChar = pStr;

	while(*pChar != '\0')
	{
		if(write(fileFd, pChar++, 1) < 1) {
			return LOG_STATUS_NOTOK;
		}
	}
	if(write(fileFd, pChar, 1) < 1) {
		return LOG_STATUS_NOTOK;
	}

	return LOG_STATUS_OK;
}

uint8_t log_integer(int32_t num, int fileFd)
{
	uint8_t numStr[MAX_INT_STRING_SIZE];

	uint8_t len = my_itoa(num, numStr, HEX_BASE);

	if(len == 0)
		return LOG_STATUS_NOTOK;

	uint32_t ind = 0;
	uint8_t *pChar = &numStr[0];
	for(;ind < len; ++ind)
	{
		if(write(fileFd, pChar++, 1) < 1) {
			return LOG_STATUS_NOTOK;
		}
	}
	return LOG_STATUS_OK;
}
