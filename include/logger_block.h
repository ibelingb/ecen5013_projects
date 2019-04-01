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

/**
 * @brief  init logger
 * 
 * @param pArg stuff it needs
 * @return uint8_t 
 */
uint8_t init_logger_block(void * pArg);

/**
 * @brief put an item in logger
 * 
 * @param pLogItem item to enter
 * @return uint8_t success of operation
 */
uint8_t log_item(logItem_t *pLogItem);

/**
 * @brief flush the logger
 * 
 * @return uint8_t successof operation
 */
uint8_t log_flush(void);

#endif	/* LOGGER_BLOCK_H */
