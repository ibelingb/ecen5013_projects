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
 * @brief send log messages via queue
 *
 ************************************************************************************
 */


#ifndef LOGGER_QUEUE_H
#define	LOGGER_QUEUE_H

#include "logger_types.h"
#include <mqueue.h>

uint8_t init_queue_logger(void *pQueue);

uint8_t log_queue_flush(void);
uint8_t log_queue_item(logItem_t *pLogItem);
uint8_t log_dequeue_item(logItem_t *pLogItem);
uint8_t log_write_item(logItem_t *pLogItem, int filefd);

#endif	/* LOGGER_QUEUE_H */
