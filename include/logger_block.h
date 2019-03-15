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


#ifndef LOGGER_BLOCK_H
#define	LOGGER_BLOCK_H

#include "logger_types.h"

uint8_t init_logger_block(void);
uint8_t log_item(logItem_t *pLogItem);
uint8_t log_flush(void);

#endif	/* LOGGER_BLOCK_H */
