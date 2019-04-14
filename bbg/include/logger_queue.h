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

#ifndef __linux__
#include "task.h"
#define LOG_QUEUE_SEND_WAIT_DELAY   ((TickType_t) (10  / portTICK_PERIOD_MS))
#define LOG_QUEUE_RECV_WAIT_DELAY   ((TickType_t) (10  / portTICK_PERIOD_MS))
#endif

/**
 * @brief initialize the posix queue logger
 * 
 * @param pQueue pointer to handle
 * @return uint8_t success of operation
 */
uint8_t init_queue_logger(void *pQueue);

/**
 * @brief flush the queue
 * 
 * @return uint8_t success of operation
 */
uint8_t log_queue_flush(void);

/**
 * @brief add item to queue
 * 
 * @param pLogItem item to add
 * @return uint8_t success of operation
 */
uint8_t log_queue_item(logItem_t *pLogItem);

/**
 * @brief remove item from queue
 * 
 * @param pLogItem pointer to container to store removed item
 * @return uint8_t success operation
 */
uint8_t log_dequeue_item(logItem_t *pLogItem);

/**
 * @brief write item to file
 * 
 * @param pLogItem item to write
 * @param filefd file pointer to write to
 * @return uint8_t  success of operation
 */
uint8_t log_write_item(logItem_t *pLogItem, int filefd);

#endif	/* LOGGER_QUEUE_H */
