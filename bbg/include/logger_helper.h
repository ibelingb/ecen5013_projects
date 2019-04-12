/***********************************************************************************
 * @author Joshua Malburg (joma0364)
 * joshua.malburg@colorado.edu
 * Principles of Embedded Software
 * ECEN5813 - Alex Fosdick
 * @date April 15, 2018
 * gcc (Ubuntu 7.2.0-8ubuntu3)
 * arm-linux-gnueabi (Ubuntu/Linaro 7.2.0-6ubuntu1)
 * arm-none-eabi-gcc (5.4.1 20160919)
 ************************************************************************************
 *
 * @file logger_helper.h
 * @brief helper functions of blocking and queue type loggers
 *
 ************************************************************************************
 */


#ifndef LOGGER_HELPER_H
#define	LOGGER_HELPER_H

#include <stdint.h>

/**
 * @brief return microseconds since first call
 * 
 * @return uint32_t 
 */
uint32_t log_get_time(void);
void log_set_checksum(logItem_t *pLogItem);
uint32_t log_strlen(uint8_t *pStr);

#endif	/* LOGGER_HELPER_H */
