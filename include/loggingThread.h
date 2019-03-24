/***********************************************************************************
 * @author Josh Malburg
 * joshua.malburg@colorado.edu
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

typedef enum {
    LOG_EVENT_STARTED = 0,
    LOG_EVENT_FILE_OPEN,
    LOG_EVENT_WRITE_ERROR,
    LOG_EVENT_OPEN_ERROR,
    LOG_EVENT_EXITING,
    LOG_EVENT_END
} LogEvent_e;

extern int gExitLog;

/*---------------------------------------------------------------------------------*/
void* logThreadHandler(void* threadInfo);
void loggingSigHandler(int signo, siginfo_t *info, void *extra);

/*---------------------------------------------------------------------------------*/
#endif /* LOGGING_THREAD_H_ */
