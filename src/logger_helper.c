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
 * @file logger_helper.c
 * @brief blocking functions for sending data to logger
 *
 ************************************************************************************
 */


#include "logger_types.h"
#include <stddef.h>

#ifdef __linux__
#include <time.h>
#else
#include "rtc.h"
#endif


uint32_t log_get_time(void)
{
#ifdef __linux__
	return((uint32_t)clock());
#else
	return(rtc_get_seconds());
#endif
}

void log_set_checksum(logItem_t *pLogItem)
{
	uint32_t sum = pLogItem->logMsgId + pLogItem->lineNum
			+ pLogItem->time + pLogItem->payloadLength + pLogItem->sourceId;

	uint8_t *ptr = pLogItem->pFilename;
	if (ptr == NULL)
		return;

	while(*ptr != '\0')
	{
		sum += *ptr;
		++ptr;
	}

	uint32_t ind = 0;
	ptr = pLogItem->pPayload;
	for(;ind < pLogItem->payloadLength; ++ind)
	{
		sum += *ptr;
		++ptr;
	}

	pLogItem->checksum = sum;

	return;
}

uint32_t log_strlen(uint8_t *pStr)
{
	if(pStr == NULL)		/* check pointer */
		return 0;

	uint32_t length = 1;	/* track string length */
	while(*pStr++ != '\0')	/* increment string pointer */
		++length;

	return length;
}
