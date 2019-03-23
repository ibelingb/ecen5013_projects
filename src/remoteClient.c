/***********************************************************************************
 * @author Brian Ibeling
 * brian.ibeling@colorado.edu
 * Advanced Embedded Software Development
 * ECEN5013 - Rick Heidebrecht
 * @date March 14, 2019
 * arm-linux-gnueabi (Buildroot)
 * gcc (Ubuntu)
 ************************************************************************************
 *
 * @file lightSensor.c
 * @brief Remote client applciation to interface with remoteThread from sensor application
 *
 ************************************************************************************
 */

#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "remoteThread.h"
#include "packet.h"

// TODO: Update dynamically as command line argument
#define SERV_ADDR "192.168.1.220"
#define BUFFER_SIZE (3)

/* Prototypes for private/helper functions */
void appUsage();
static void getCmdResponse(RemoteCmdPacket *packet);

/* Define static and global variables */

/*---------------------------------------------------------------------------------*/
int main()
{
  RemoteCmdPacket cmdPacket = {0};
  int sockfd;
  struct sockaddr_in servAddr;
  int inputCmd = 0;
  char inputBuffer[BUFFER_SIZE];

  /** Establish connection on remote socket **/
  /* Open Socket */
  sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if(sockfd == -1){
    printf("ERROR: remoteClient Application failed to create socket - exiting.\n");
    return -1;
  }

  /* Establish Connection with Server */
  servAddr.sin_family = AF_INET;
  servAddr.sin_port = htons((int)PORT);
  inet_pton(AF_INET, SERV_ADDR, &servAddr.sin_addr);

  if(connect(sockfd, (struct sockaddr *)&servAddr, sizeof(servAddr)) == -1) {
    printf("ERROR: remoteClient Application failed to successfully connect to server - exiting.\n");
    return -1;
  }

  /* Log SensorThread successfully created */
  printf("Successfully connected remoteClient on port %d.\n", PORT);

  while(1) {
    /* Display usage info and wait for user input */
    appUsage();
    if(scanf("%s", inputBuffer) == 1){
      /* Convert received input and verify cmd is valid */
      inputCmd = atoi(inputBuffer);
      if((inputCmd >= MAX_CMDS) || inputCmd <= 0) {
        printf("Invalid command received of {%s} - ignoring cmd\n", inputBuffer);
        continue;
      }

      cmdPacket.cmd = inputCmd;

      /* Transmit received cmd to server app */
      send(sockfd, &cmdPacket, sizeof(struct RemoteCmdPacket), 0);

      /* Receive response from server app and print data */
      recv(sockfd, &cmdPacket, sizeof(struct RemoteCmdPacket), 0);
      getCmdResponse(&cmdPacket);
    }
  }

  /* Cleanup */
  close(sockfd);

  return 0;
}

/*---------------------------------------------------------------------------------*/
/* HELPER METHODS */
void appUsage(){
  printf("\nEnter a value to specify a command to send to the Sensor Application:\n"
         "\t1  = TEMPCMD_GETTEMP\n"
         "\t2  = TEMPCMD_GETLOWTHRES\n"
         "\t3  = TEMPCMD_GETHIGHTHRES\n"
         "\t4  = TEMPCMD_GETCONFIG\n"
         "\t5  = TEMPCMD_GETRESOLUTION\n"
         "\t6  = TEMPCMD_GETFAULT\n"
         "\t7  = TEMPCMD_GETEXTMODE\n"
         "\t8  = TEMPCMD_GETSHUTDOWNMODE\n"
         "\t9  = TEMPCMD_GETALERT\n"
         "\t10 = TEMPCMD_GETCONVRATE\n"
         "\t11 = LIGHTCMD_GETLUXDATA\n"
         "\t12 = LIGHTCMD_GETPOWERCTRL\n"
         "\t13 = LIGHTCMD_GETDEVPARTNO\n"
         "\t14 = LIGHTCMD_GETDEVREVNO\n"
         "\t15 = LIGHTCMD_GETTIMINGGAIN\n"
         "\t16 = LIGHTCMD_GETTIMINGINTEGRATION\n"
         "\t17 = LIGHTCMD_GETINTSELECT\n"
         "\t18 = LIGHTCMD_GETINTPERSIST\n"
         "\t19 = LIGHTCMD_GETLOWTHRES\n"
         "\t20 = LIGHTCMD_GETHIGHTHRES\n"
  );
}

static void getCmdResponse(RemoteCmdPacket *packet){
  /* Based on received command, populate response to provide back to client */
  switch(packet->cmd) {
    case TEMPCMD_GETTEMP :
      printf("TEMPCMD_GETTEMP data received - {%.3f}\n", packet->data.status_float);
      break;
    case TEMPCMD_GETLOWTHRES :
      printf("TEMPCMD_GETLOWTHRES data received - {%.3f}\n", packet->data.status_float);
      break;
    case TEMPCMD_GETHIGHTHRES :
      printf("TEMPCMD_GETHIGHTHRES data received - {%.3f}\n", packet->data.status_float);
      break;
    case TEMPCMD_GETCONFIG :
      printf("TEMPCMD_GETCONFIG data received - {%.3f}\n", packet->data.status_float);
      break;
    case TEMPCMD_GETRESOLUTION :
      printf("TEMPCMD_GETRESOLUTION data received - {%.3f}\n", packet->data.status_float);
      break;
    case TEMPCMD_GETFAULT :
      printf("TEMPCMD_GETFAULT data received - {%d}\n", packet->data.status_uint32);
      break;
    case TEMPCMD_GETEXTMODE :
      printf("TEMPCMD_GETEXTMODE data received - {%d}\n", packet->data.status_uint32);
      break;
    case TEMPCMD_GETSHUTDOWNMODE :
      printf("TEMPCMD_GETSHUTDOWNMODE data received - {%d}\n", packet->data.status_uint32);
      break;
    case TEMPCMD_GETALERT :
      printf("TEMPCMD_GETALERT data received - {%d}\n", packet->data.status_uint32);
      break;
    case TEMPCMD_GETCONVRATE :
      printf("TEMPCMD_GETCONVRATE data received - {%d}\n", packet->data.status_uint32);
      break;
    case LIGHTCMD_GETLUXDATA :
      printf("LIGHTCMD_GETLUXDATA data received - {%f}\n", packet->data.status_float);
      break;
    case LIGHTCMD_GETPOWERCTRL :
      printf("LIGHTCMD_GETPOWERCTRL data received - {0x%x}\n", packet->data.status_uint32);
      break;
    case LIGHTCMD_GETDEVPARTNO:
      printf("LIGHTCMD_GETDEVPARTNO data received - {0x%x}\n", packet->data.status_uint32);
      break;
    case LIGHTCMD_GETDEVREVNO :
      printf("LIGHTCMD_GETDEVREVNO data received - {0x%x}\n", packet->data.status_uint32);
      break;
    case LIGHTCMD_GETTIMINGGAIN :
      printf("LIGHTCMD_GETTIMINGGAIN data received - {0x%x}\n", packet->data.status_uint32);
      break;
    case LIGHTCMD_GETTIMINGINTEGRATION :
      printf("LIGHTCMD_GETTIMINGINTEGRATION data received - {0x%x}\n", packet->data.status_uint32);
      break;
    case LIGHTCMD_GETINTSELECT :
      printf("LIGHTCMD_GETINTSELECT data received - {0x%x}\n", packet->data.status_uint32);
      break;
    case LIGHTCMD_GETINTPERSIST :
      printf("LIGHTCMD_GETINTPERSIST data received - {0x%x}\n", packet->data.status_uint32);
      break;
    case LIGHTCMD_GETLOWTHRES :
      printf("LIGHTCMD_GETLOWTHRES data received - {0x%x}\n", packet->data.status_uint32);
      break;
    case LIGHTCMD_GETHIGHTHRES :
      printf("LIGHTCMD_GETHIGHTHRES data received - {0x%x}\n", packet->data.status_uint32);
      break;
    default:
      printf("Unrecognized command received. Request ignored.\n");
      break;
  }

  return;
}
/*---------------------------------------------------------------------------------*/
