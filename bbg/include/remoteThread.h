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

#ifndef REMOTE_THREAD_H_
#define REMOTE_THREAD_H_

#include <stdint.h>
#include <signal.h>

#include "remoteLogThread.h"
#include "remoteStatusThread.h"
#include "remoteDataThread.h"
#include "remoteCmdThread.h"

#define LOG_PORT    (5001)
#define STATUS_PORT (5002)
#define CMD_PORT    (5003)
#define DATA_PORT   (5004)

typedef enum {
  REMOTE_EVENT_STARTED = 0,
  REMOTE_EVENT_CNCT_ACCEPTED,
  REMOTE_EVENT_CNCT_LOST,
  REMOTE_EVENT_CMD_RECV,
  REMOTE_EVENT_INVALID_RECV,
  REMOTE_LOG_QUEUE_ERROR,
  REMOTE_STATUS_QUEUE_ERROR,
  REMOTE_SERVER_SOCKET_ERROR,
  REMOTE_CLIENT_SOCKET_ERROR,
  REMOTE_BIST_COMPLETE,
  REMOTE_INIT_SUCCESS,
  REMOTE_INIT_ERROR,
  REMOTE_EVENT_EXITING,
  REMOTE_EVENT_END
} RemoteEvent_e;

/*---------------------------------------------------------------------------------*/

/**
 * @brief - Remote Socket Server Thread used to connect application via socket to an
 *          external client. Receive requests for data and pass back info back to client.
 *
 * @param threadInfo - Struct containing resources allocated and filenames established by main()
 *
 * @return void*
 */
void* remoteThreadHandler(void* threadInfo);

/**
 * @brief 
 *
 * @param signo - Signal number
 * @param info - Struct used to pass data through to Signal Handler.
 * @param extra - Additional params if needed by sigAction struct when setting sa_sigaction
 * @return void
 */
void remoteSigHandler(int signo, siginfo_t *info, void *extra);

/**
 * @brief Get method to receive lightThread's alive status.
 *
 * @param pAlive - Pointer to return alive status back to caller function to.
 * @return void
 */
void remoteGetAliveFlag(uint8_t *pAlive);

/*---------------------------------------------------------------------------------*/
#endif /* REMOTE_THREAD_H_ */
