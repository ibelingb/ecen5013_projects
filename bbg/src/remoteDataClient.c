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
static void getCmdResponse(RemoteDataPacket *packet);

/* Define static and global variables */

/*---------------------------------------------------------------------------------*/
int main(int argc, char *argv[])
{
  RemoteDataPacket dataPacket = {0};
  int sockfd;
  struct sockaddr_in servAddr;
  int inputCmd = 0;
  char *ipAddress = DEFAULT_SERV_ADDR;
  char inputBuffer[BUFFER_SIZE];

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
  servAddr.sin_port = htons((int)DATA_PORT);
  if(inet_pton(AF_INET, ipAddress, &servAddr.sin_addr) != 1){
    printf("ERROR: remoteClient Application IP address provided of {%s} is invalid - exiting.\n", ipAddress);
    return -1;
  }

  if(connect(sockfd, (struct sockaddr *)&servAddr, sizeof(servAddr)) == -1) {
    printf("ERROR: remoteClient Application failed to successfully connect to server - exiting.\n");
    return -1;
  }

  /* Log SensorThread successfully created */
  printf("Successfully connected remoteDataClient on port %d.\n", DATA_PORT);

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

      if(inputCmd == 1) {
        dataPacket.luxData = 60;
        dataPacket.moistureData = 100;
      } else if(inputCmd == 2) {
        dataPacket.luxData = 30;
        dataPacket.moistureData = 100;
      } else if(inputCmd == 3) {
        dataPacket.luxData = 60;
        dataPacket.moistureData = 800;
      } else if(inputCmd == 4) {
        dataPacket.luxData = 30;
        dataPacket.moistureData = 800;
      }

      /* Transmit received cmd to server app */
      send(sockfd, &dataPacket, sizeof(struct RemoteDataPacket), 0);
      printf("Data Packet Tx.\n");

      /* Receive response from server app and print data */
      //recv(sockfd, &dataPacket, sizeof(struct RemoteDataPacket), 0);
      getCmdResponse(&dataPacket);
    }
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
  printf("\nEnter a value to specify a command to send to the Sensor Application:\n"
         "\t1  = LuxData=60, MoistureData=100\n"
         "\t2  = LuxData=30, MoistureData=100\n"
         "\t3  = LuxData=60, MoistureData=800\n"
         "\t4  = LuxData=30, MoistureData=800\n"
         //"\t5  = LuxData=60, MoistureData=100\n"
         //"\t6  = LuxData=60, MoistureData=100\n"
  );
}

/**
 * @brief - Process data received from sensor application to print returned data.
 *
 * @param packet - Data packet received from Sensor Application to process and print results.
 * @return void
 */
static void getCmdResponse(RemoteDataPacket *packet){
  if(packet == NULL)
  {
    printf("getCmdResponse() received NULL for packet argument - Cmd response couldn't be processed.\n");
    return;
  }

  /* Based on received command, populate response to provide back to client */
  //switch(packet->cmd) {
    //case TEMPCMD_GETTEMP :
      //printf("TEMPCMD_GETTEMP data received - {%.3f}\n", packet->data.status_float);
      //break;
    //default:
      //printf("Unrecognized command received. Request ignored.\n");
      //break;
  //}

  return;
}
/*---------------------------------------------------------------------------------*/
