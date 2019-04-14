/***********************************************************************************
 * @author Brian Ibeling and Joshua Malburg
 * Brian.Ibeling@colorado.edu, joshua.malburg@colorad.edu
 * Advanced Embedded Software Development
 * ECEN5013 - Rick Heidebrecht
 * @date April 12, 2019
 * CCS  Version: 8.3.0.00009
 ************************************************************************************
 *
 * @file remoteThread.h
 * @brief
 *
 ************************************************************************************
 */

#ifndef REMOTETHREAD_H_
#define REMOTETHREAD_H_

typedef enum {
  REMOTE_EVENT_STARTED = 0,
  REMOTE_EVENT_CNCT_ACCEPTED,
  REMOTE_EVENT_CNCT_LOST,
  REMOTE_EVENT_CMD_RECV,
  REMOTE_EVENT_INVALID_RECV,
  REMOTE_LOG_QUEUE_ERROR,
  REMOTE_STATUS_QUEUE_ERROR,
  REMOTE_SHMEM_ERROR,
  REMOTE_SERVER_SOCKET_ERROR,
  REMOTE_CLIENT_SOCKET_ERROR,
  REMOTE_BIST_COMPLETE,
  REMOTE_INIT_SUCCESS,
  REMOTE_INIT_ERROR,
  REMOTE_EVENT_EXITING,
  REMOTE_EVENT_END
} RemoteEvent_e;

void remoteTask(void *pvParameters);

#endif /* REMOTETHREAD_H_ */
