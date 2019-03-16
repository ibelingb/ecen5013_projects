/***********************************************************************************
 * @author Josh Malburg
 * 
 * Advanced Embedded Software Development
 * ECEN5013 - Rick Heidebrecht
 * @date March 15, 2019
 * arm-linux-gnueabi (Buildroot)
 * gcc (Ubuntu)
 ************************************************************************************
 *
 * @file test_logger_queue.c
 * @brief verify logger msgs & writes to queue
 *
 ************************************************************************************
 */

#include <stdio.h>
#include <mqueue.h>
#include <string.h>
#include <errno.h>
#include <time.h>	// for timespec

#include "debug.h"
#include "logger_types.h"
#include "packet.h"

#include "conversion.h"

/* globals */
mqd_t logQueue;
logItem_t sentItem;
logItem_t recvItem;

/* private functions */
uint8_t log_data(uint8_t *pData, uint32_t len);
uint8_t log_string(uint8_t *pStr);
uint8_t log_integer(int32_t num);
uint8_t log_byte(uint8_t value);

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

	if(mq_send(logQueue, (char *)&newItem, sizeof(LogMsgPacket), 1) < 0)
	{
        ERROR_PRINT("mq_receive failed, err#%d (%s)\n\r", errno, strerror(errno));
        return LOG_STATUS_NOTOK;
    }
	return LOG_STATUS_OK;
}

void log_queue_flush(void)
{
	return;
}

uint8_t log_dequeue_item(logItem_t *pLogItem)
{
	LogMsgPacket newItem;
	struct timespec rxTimeout;
	size_t bytesRead;
	clock_gettime(CLOCK_REALTIME, &rxTimeout);
	rxTimeout.tv_sec += 2;

	/* read oldest highest priority queue */
	bytesRead = mq_timedreceive(logQueue, (char *)&newItem, sizeof(LogMsgPacket), NULL, &rxTimeout);
	//bytesRead = mq_receive(logQueue, (char *)&newItem, sizeof(LogMsgPacket), NULL);
	if(bytesRead <= 0)
    {
        ERROR_PRINT("mq_receive failed, err#%d (%s)\n\r", errno, strerror(errno));
        return LOG_STATUS_NOTOK;
    }
	if(bytesRead > 0)
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
	}
	return LOG_STATUS_OK;
}

uint8_t log_write_item(logItem_t *pLogItem, int filefd)
{
	if(log_byte(FRAME_START_BYTE) != LOG_STATUS_OK)
		return LOG_STATUS_NOTOK;
	if(log_integer(pLogItem->logMsgId) != LOG_STATUS_OK)
		return LOG_STATUS_NOTOK;
	if(log_string(pLogItem->pFilename) != LOG_STATUS_OK)
		return LOG_STATUS_NOTOK;
	if(log_integer(pLogItem->lineNum) != LOG_STATUS_OK)
		return LOG_STATUS_NOTOK;
	if(log_integer(pLogItem->time) != LOG_STATUS_OK)
		return LOG_STATUS_NOTOK;
	if(log_integer(pLogItem->payloadLength) != LOG_STATUS_OK)
		return LOG_STATUS_NOTOK;
	if(log_data(pLogItem->pPayload, pLogItem->payloadLength) != LOG_STATUS_OK)
		return LOG_STATUS_NOTOK;
	if(log_integer(pLogItem->checksum) != LOG_STATUS_OK)
		return LOG_STATUS_NOTOK;
	if(log_byte(FRAME_STOP_BYTE) != LOG_STATUS_OK)
		return LOG_STATUS_NOTOK;
	printf("\n");
	return LOG_STATUS_OK;
}

/***** private functions *****/

uint8_t log_data(uint8_t *pData, uint32_t len)
{
	if(pData != NULL)
	{
		uint32_t ind = 0;
		for(;ind < len; ++ind)
			putchar(*pData++);
	}
	return LOG_STATUS_OK;
}

uint8_t log_byte(uint8_t value)
{
	putchar(value);
	return LOG_STATUS_OK;
}

uint8_t log_string(uint8_t *pStr)
{
	uint8_t *pChar = pStr;

	while(*pChar != '\0')
	{
		putchar(*pChar++);
	}
	putchar(*pChar);

	return LOG_STATUS_OK;
}

uint8_t log_integer(int32_t num)
{
	uint8_t numStr[MAX_INT_STRING_SIZE];

	uint8_t len = my_itoa(num, numStr, HEX_BASE);

	if(len == 0)
		return LOG_STATUS_NOTOK;

	uint32_t ind = 0;
	uint8_t *pChar = &numStr[0];
	for(;ind < len; ++ind)
		putchar(*pChar++);

	return LOG_STATUS_OK;
}

