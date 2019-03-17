/* Remote client applciation to interface with remoteThread from sensor application */

#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "remoteThread.h"
#include "packet.h"

// TODO: Update dynamically as command line argument
#define SERV_ADDR "192.168.1.220"

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
    scanf("%d", &inputCmd);
    cmdPacket.cmd = inputCmd;

    /* Transmit received cmd to server app */
    send(sockfd, &cmdPacket, sizeof(struct RemoteCmdPacket), 0);

    /* Receive response from server app and print data */
    recv(sockfd, &cmdPacket, sizeof(struct RemoteCmdPacket), 0);
    getCmdResponse(&cmdPacket);
  }

  /* Cleanup */
  close(sockfd);

  return 0;
}

/*---------------------------------------------------------------------------------*/
/* HELPER METHODS */
void appUsage(){
  printf("\nEnter a value to specify a command to send to the Sensor Application:\n"
         "\t1 = TEMPCMD_GETTEMP\n"
         "\t2 = TEMPCMD_GETLOWTHRES\n"
         "\t3 = TEMPCMD_GETHIGHTHRES\n"
         "\t4 = TEMPCMD_GETCONFIG\n"
         "\t5 = TEMPCMD_GETRESOLUTION\n"
         "\t6 = TEMPCMD_GETFAULT\n"
         "\t7 = TEMPCMD_GETEXTMODE\n"
         "\t8 = TEMPCMD_GETSHUTDOWNMODE\n"
         "\t9 = TEMPCMD_GETALERT\n"
         "\t10 = TEMPCMD_GETCONVRATE\n"
         "\t11 = LIGHTCMD_GETLUXLOW\n"
         "\t12 = LIGHTCMD_GETLUXHIGH\n"
         "\t13 = LIGHTCMD_GETDEVID\n"
         "\t14 = LIGHTCMD_GETCTRL\n"
         "\t15 = LIGHTCMD_GETTIMING\n"
         "\t16 = LIGHTCMD_GETLOWTHRES\n"
         "\t17 = LIGHTCMD_GETHIGHTHRES\n"
  );
}

static void getCmdResponse(RemoteCmdPacket *packet){
  printf("Response received from server\n");

  /* Based on received command, populate response to provide back to client */
  switch(packet->cmd) {
    case TEMPCMD_GETTEMP :
      printf("TEMPCMD_GETTEMP data received - {%.3f}\n", packet->data0.status_float);
      break;
    case TEMPCMD_GETLOWTHRES :
      printf("TEMPCMD_GETLOWTHRES data received - {%.3f}\n", packet->data0.status_float);
      break;
    case TEMPCMD_GETHIGHTHRES :
      printf("TEMPCMD_GETHIGHTHRES data received - {%.3f}\n", packet->data0.status_float);
      break;
    case TEMPCMD_GETCONFIG :
      printf("TEMPCMD_GETCONFIG data received - {%.3f}\n", packet->data0.status_float);
      break;
    case TEMPCMD_GETRESOLUTION :
      printf("TEMPCMD_GETRESOLUTION data received - {%.3f}\n", packet->data0.status_float);
      break;
    case TEMPCMD_GETFAULT :
      printf("TEMPCMD_GETFAULT data received - {%d}\n", packet->data0.status_uint32);
      break;
    case TEMPCMD_GETEXTMODE :
      printf("TEMPCMD_GETEXTMODE data received - {%d}\n", packet->data0.status_uint32);
      break;
    case TEMPCMD_GETSHUTDOWNMODE :
      printf("TEMPCMD_GETSHUTDOWNMODE data received - {%d}\n", packet->data0.status_uint32);
      break;
    case TEMPCMD_GETALERT :
      printf("TEMPCMD_GETALERT data received - {%d}\n", packet->data0.status_uint32);
      break;
    case TEMPCMD_GETCONVRATE :
      printf("TEMPCMD_GETCONVRATE data received - {%d}\n", packet->data0.status_uint32);
      break;
    case LIGHTCMD_GETLUXLOW :
      printf("LIGHTCMD_GETLUXLOW data received - [0]:{%d} | [1]:{%d}\n", 
              packet->data0.status_uint32, packet->data1.status_uint32);
      break;
    case LIGHTCMD_GETLUXHIGH :
      printf("LIGHTCMD_GETLUXHIGH data received - [0]:{%d} | [1]:{%d}\n", 
              packet->data0.status_uint32, packet->data1.status_uint32);
      break;
    case LIGHTCMD_GETDEVID :
      printf("LIGHTCMD_GETDEVID data received - {%d}\n", packet->data0.status_uint32);
      break;
    case LIGHTCMD_GETCTRL :
      printf("LIGHTCMD_GETCTRL data received - {%d}\n", packet->data0.status_uint32);
      break;
    case LIGHTCMD_GETTIMING :
      printf("LIGHTCMD_GETTIMING data received - {%d}\n", packet->data0.status_uint32);
      break;
    case LIGHTCMD_GETLOWTHRES :
      printf("LIGHTCMD_GETLOWTHRES data received - [0]:{%d} | [1]:{%d}\n", 
              packet->data0.status_uint32, packet->data1.status_uint32);
      break;
    case LIGHTCMD_GETHIGHTHRES :
      printf("LIGHTCMD_GETHIGHTHRES data received - [0]:{%d} | [1]:{%d}\n", 
              packet->data0.status_uint32, packet->data1.status_uint32);
      break;
    default:
      printf("Returned response has unrecognized command. Data ignored.");
      break;
  }

  return;
}
/*---------------------------------------------------------------------------------*/
