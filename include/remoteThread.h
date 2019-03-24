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
 ************************************************************************************
 */

#ifndef REMOTE_THREAD_H_
#define REMOTE_THREAD_H_

#include <stdint.h>

#define PORT (5001)
#define REMOTE_MSG_PAYLOAD_SIZE (256) // Bytes

typedef union StatusData {
  uint32_t status_uint32;
  float status_float;
} StatusData;

typedef enum RemoteCmd_e
{
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
void* remoteThreadHandler(void* threadInfo);
void remoteSigHandler(int signo, siginfo_t *info, void *extra);
void remoteGetAliveFlag(uint8_t *pAlive);

/*---------------------------------------------------------------------------------*/
#endif /* REMOTE_THREAD_H_ */
