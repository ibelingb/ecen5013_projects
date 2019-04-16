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

#define SENSOR_PORT (5001)
#define CMD_PORT (5002)
#define LOG_PORT (5003)

typedef union StatusData {
  uint32_t status_uint32;
  float status_float;
} StatusData;

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

typedef enum {
  TEMPCMD_GETTEMP = 1,
  TEMPCMD_GETLOWTHRES,
  TEMPCMD_GETHIGHTHRES,
  TEMPCMD_GETCONFIG,
  TEMPCMD_GETRESOLUTION,
  TEMPCMD_GETFAULT,
  TEMPCMD_GETEXTMODE,
  TEMPCMD_GETSHUTDOWNMODE,
  TEMPCMD_GETALERT,
  TEMPCMD_GETCONVRATE,
  TEMPCMD_GETOVERTEMPSTATE,
  LIGHTCMD_GETLUXDATA,
  LIGHTCMD_GETPOWERCTRL,
  LIGHTCMD_GETDEVPARTNO,
  LIGHTCMD_GETDEVREVNO,
  LIGHTCMD_GETTIMINGGAIN,
  LIGHTCMD_GETTIMINGINTEGRATION,
  LIGHTCMD_GETINTSELECT,
  LIGHTCMD_GETINTPERSIST,
  LIGHTCMD_GETLOWTHRES,
  LIGHTCMD_GETHIGHTHRES,
  LIGHTCMD_GETLIGHTSTATE,
  MAX_CMDS
} RemoteCmd_e;

typedef struct RemoteCmdPacket
{
  uint16_t header;
  RemoteCmd_e cmd;
  StatusData data;
  uint32_t crc;
} RemoteCmdPacket;

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
