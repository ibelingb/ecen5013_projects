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


#define DEFAULT_SERV_ADDR "10.0.0.93"
#define SENSOR_PORT (5008)

/*---------------------------------------------------------------------------------*/
int main(int argc, char *argv[])
{
  int sockfd;
  struct sockaddr_in servAddr;
  char *ipAddress = DEFAULT_SERV_ADDR;
  char rxData[] = "cnt0";
  size_t rxLen = sizeof(rxData) / sizeof(rxData[0]);
  uint8_t cnt;

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
  servAddr.sin_port = htons((int)SENSOR_PORT);
  if(inet_pton(AF_INET, ipAddress, &servAddr.sin_addr) != 1){
    printf("ERROR: remoteClient Application IP address provided of {%s} is invalid - exiting.\n", ipAddress);
    return -1;
  }

  if(connect(sockfd, (struct sockaddr *)&servAddr, sizeof(servAddr)) == -1) {
    printf("ERROR: remoteClient Application failed to successfully connect to server - exiting.\n");
    return -1;
  }

  /* Log SensorThread successfully created */
  printf("Successfully connected remoteClient on port %d.\n", SENSOR_PORT);

  while(1) 
  {
    uint8_t ret;
      ++cnt;
      cnt %= 6;
      rxData[3] = '0' + cnt;

      /* Transmit received cmd to server app */
      ret = send(sockfd, &rxData, rxLen, 0);
      printf("sent msg w/ %d bytes\n", ret);

      /* Receive response from server app and print data */
      //recv(sockfd, &cmdPacket, sizeof(struct RemoteCmdPacket), 0);

      sleep(3);
  }

  /* Cleanup */
  close(sockfd);

  return 0;
}
