/***********************************************************************************
 * @author Brian Ibeling
 * brian.ibeling@colorado.edu
 * Advanced Embedded Software Development
 * ECEN5013 - Rick Heidebrecht
 * @date March 8, 2019
 * arm-linux-gnueabi (Buildroot)
 * gcc (Ubuntu)
 ************************************************************************************
 *
 * @file remoteThread.h
 * @brief Remote Socket handling thread
 *
 * Resources Used:
 *  - https://jameshfisher.com/2017/04/05/set_socket_nonblocking/
 *
 ************************************************************************************
 */

#ifndef REMOTE_CMD_THREAD_H_
#define REMOTE_CMD_THREAD_H_

#include <stdint.h>
#include <signal.h>

#include "remoteThread.h"

/*---------------------------------------------------------------------------------*/

/**
 * @brief - Remote Socket Server Thread used to connect application via socket to an
 *          external client. Receive requests for data and pass back info back to client.
 *
 * @param threadInfo - Struct containing resources allocated and filenames established by main()
 *
 * @return void*
 */
void* remoteCmdThreadHandler(void* threadInfo);

/**
 * @brief 
 *
 * @param signo - Signal number
 * @param info - Struct used to pass data through to Signal Handler.
 * @param extra - Additional params if needed by sigAction struct when setting sa_sigaction
 * @return void
 */
void remoteCmdSigHandler(int signo, siginfo_t *info, void *extra);

/*---------------------------------------------------------------------------------*/
#endif /* REMOTE_CMD_THREAD_H_ */
