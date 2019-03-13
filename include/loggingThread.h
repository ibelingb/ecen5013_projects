/***********************************************************************************
 * @author 
 * 
 * Advanced Embedded Software Development
 * ECEN5013 - Rick Heidebrecht
 * @date March 8, 2019
 * arm-linux-gnueabi (Buildroot)
 * gcc (Ubuntu)
 ************************************************************************************
 *
 * @file loggingThread.h
 * @brief Logging thread
 *
 ************************************************************************************
 */

#ifndef LOGGING_THREAD_H_
#define LOGGING_THREAD_H_

#include <stdint.h>

/*---------------------------------------------------------------------------------*/
void* logThreadHandler(void* threadInfo);

/*---------------------------------------------------------------------------------*/
#endif /* LOGGING_THREAD_H_ */
