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

#define DEFAULT_SERV_ADDR "192.168.1.27"
#define BUFFER_SIZE (3)

/* Prototypes for private/helper functions */
void appUsage();
static void getCmdResponse(RemoteCmdPacket *packet);

/* Define static and global variables */

/*---------------------------------------------------------------------------------*/
int main(int argc, char *argv[])
{
  RemoteCmdPacket cmdPacket = {0};
  int sockfd;
  struct sockaddr_in servAddr;
  char *ipAddress = DEFAULT_SERV_ADDR;

  /* If IP Address provided as cmdline arg, set as IP */
  if(argc >= 2) {
    ipAddress = argv[1];
  }

  /** Establish connection on remote socket **/
  /* Open Socket - verify ID is valid */
  sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if(sockfd == -1){
    printf("ERROR: remoteClient Application failed to create socket - exiting.\n");
    return -1;
  }

  /* Establish Connection with Server */
  servAddr.sin_family = AF_INET;
  servAddr.sin_port = htons((int)CMD_PORT);
  if(inet_pton(AF_INET, ipAddress, &servAddr.sin_addr) != 1){
    printf("ERROR: remoteClient Application IP address provided of {%s} is invalid - exiting.\n", ipAddress);
    return -1;
  }

  if(connect(sockfd, (struct sockaddr *)&servAddr, sizeof(servAddr)) == -1) {
    printf("ERROR: remoteClient Application failed to successfully connect to server - exiting.\n");
    return -1;
  }

  /* Log SensorThread successfully created */
  printf("Successfully connected remoteDataClient on port %d.\n", CMD_PORT);

  /* Display usage info and wait for user input */
  appUsage();

  while(1) {
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
/*---------------------------------------------------------------------------------*/
/**
 * @brief - Prints the remoteClient options to request data from the Sensor Application.
 * @return void
 */
void appUsage(){
  printf("\n%d Port opened, waiting to receive Cmd Packets from sensor Application\n", CMD_PORT);
}

/**
 * @brief - Process data received from sensor application to print returned data.
 *
 * @param packet - Data packet received from Sensor Application to process and print results.
 * @return void
 */
static void getCmdResponse(RemoteCmdPacket *packet){
  if(packet == NULL)
  {
    printf("getCmdResponse() received NULL for packet argument - Cmd response couldn't be processed.\n");
    return;
  }

  printf("Cmd: %d | Data: %d\n", packet->cmd, packet->data);

  return;
}
/*---------------------------------------------------------------------------------*/
