/***********************************************************************************
 * @author Joshua Malburg (joma0364)
 * joshua.malburg@colorado.edu
 * Principles of Embedded Software
 * ECEN5813 - Alex Fosdick
 * @date  April 14, 2018
 * gcc (Ubuntu 7.2.0-8ubuntu3)
 * arm-linux-gnueabi (Ubuntu/Linaro 7.2.0-6ubuntu1)
 * arm-none-eabi-gcc (5.4.1 20160919)
 ************************************************************************************
 *
 * @file logger.c
 * @brief blocking functions for sending data to logger
 *
 ************************************************************************************
 */

#include "logger_types.h"

#include <stdio.h>
#include "conversion.h"

/* private functions */
uint8_t log_data(uint8_t *pData, uint32_t len);
uint8_t log_string(uint8_t *pStr);
uint8_t log_integer(int32_t num);
uint8_t log_byte(uint8_t value);


uint8_t init_logger_block(void)
{
	return LOG_STATUS_OK;
}

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

uint8_t log_item(logItem_t *pLogItem)
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

void log_flush(void)
{
	return;
}
