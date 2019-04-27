/***********************************************************************************
 * @author Josh Malburg
 * joshua.malburg@colorado.edu
 * Advanced Embedded Software Development
 * ECEN5013 - Rick Heidebrecht
 * @date April 25, 2019
 * arm-linux-gnueabi (Buildroot)
 * gcc (Ubuntu)
 ************************************************************************************
 *
 * @file simpleServer.c
 * @brief simple server that waits for a client then prints received bytes
 * 
 * reference:
 * https://stackoverflow.com/questions/12730477/close-is-not-closing-socket-properly
 *
 ************************************************************************************
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>

#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <signal.h>

#include "remoteThread.h"
#include "packet.h"
#include "healthMonitor.h"
#include "logger.h"

#define THREAD_COUNT		(3)

/* global to kill loop */
static uint8_t gExit = 1;
pthread_t gThreads[THREAD_COUNT];

void sigintHandler(int sig);
void* getRemoteStatusTask(void* threadInfo);
void* getRemoteLogTask(void* threadInfo);
void* getRemoteDataTask(void* threadInfo);
int getSO_ERROR(int fd);
void closeSocket(int fd);

int main( int argc, char *argv[] ) 
{
	  /* Create other threads */
  if(pthread_create(&gThreads[0], NULL, getRemoteStatusTask, NULL))
  {
    ERROR_PRINT("ERROR: Failed to create getRemoteStatusTask Thread - exiting main().\n");
    return EXIT_FAILURE;
  }
		  /* Create other threads */
  if(pthread_create(&gThreads[1], NULL, getRemoteLogTask, NULL))
  {
    ERROR_PRINT("ERROR: Failed to create getRemoteLogTask Thread - exiting main().\n");
    return EXIT_FAILURE;
  }
		  /* Create other threads */
  if(pthread_create(&gThreads[2], NULL, getRemoteDataTask, NULL))
  {
    ERROR_PRINT("ERROR: Failed to create getRemoteDataTask Thread - exiting main().\n");
    return EXIT_FAILURE;
  }

	  /* join to clean up children */
  for(uint8_t ind = 0; ind < THREAD_COUNT; ++ind)
    pthread_join(gThreads[(uint8_t)ind], NULL);

	return EXIT_SUCCESS;
}

void* getRemoteStatusTask(void* threadInfo)
{
   	int myServerSocketFd, statusSocketFd;
   	socklen_t clilen;
		struct sockaddr_in serv_addr, cli_addr;
   	int  n, noConnection;
   	int strLen=20;
		char str[strLen];
		TaskStatusPacket statusMsg;
		int socketDataFlags;

   	/* set signal handlers and actions */
  	struct sigaction sigAction;
  	sigAction.sa_handler = sigintHandler;
 	  sigAction.sa_flags = 0;
  	sigaction(SIGINT, &sigAction, NULL);
   
   	/* First call to socket() function */
   	myServerSocketFd = socket(AF_INET, SOCK_STREAM, 0);

		/* Update Socket Server connections to be non-blocking */
  	socketDataFlags = fcntl(myServerSocketFd, F_GETFL);
  	fcntl(myServerSocketFd, F_SETFL, socketDataFlags | O_NONBLOCK);
   
   	if (myServerSocketFd < 0) {
      	perror("getRemoteStatusTask ERROR opening socket");
      	exit(1);
   	}
   	printf("getRemoteStatusTask socket created\n");
   	
   	/* Initialize socket structure */
   	bzero((char *) &serv_addr, sizeof(serv_addr));
   
   	serv_addr.sin_family = AF_INET;
   	serv_addr.sin_addr.s_addr = INADDR_ANY;
   	serv_addr.sin_port = htons(STATUS_PORT);
   
   	/* Now bind the host address using bind() call.*/
   	if (bind(myServerSocketFd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
      	perror("getRemoteStatusTask ERROR on binding");
      	exit(1);
   	}
   	printf("getRemoteStatusTask socket bound\n");   
   	/* Now start listening for the clients, here process will
     * go in sleep mode and will wait for the incoming connection
   	 */
   	printf("getRemoteStatusTask listening...\n");
   	listen(myServerSocketFd, 50);
   	clilen = sizeof(cli_addr);

   	/* Update Socket Client connections to be non-blocking */
  	struct timeval timeout;
  	timeout.tv_usec = 10000;
  	timeout.tv_sec = 0;

   	/* Accept actual connection from the client */
   	noConnection = 1;
   	while(noConnection && (gExit != 0))
   	{
   		statusSocketFd = accept(myServerSocketFd, (struct sockaddr *)&cli_addr, &clilen);
			if(statusSocketFd == -1) {
          continue;
      }
   		inet_ntop(AF_INET, &(cli_addr.sin_addr), str, strLen);
			printf("getRemoteStatusTask connection accepted from: %s\n", str);	
   		if (statusSocketFd > 0) {
   			noConnection = 0;
   		}
   	}
   	if(gExit != 0)
   	{
   		/* Update Socket Client connections to be non-blocking */
	    setsockopt(myServerSocketFd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof(struct timeval));

	   	while((gExit != 0))
	   	{
	   	   	n = read(statusSocketFd, &statusMsg, sizeof(statusMsg));
	   
		   	if (n < 0) {
		      	printf("getRemoteStatusTask ERROR reading from socket\n");
		      	continue;
		   	}
		   	PRINT_STATUS_MSG_HEADER(&statusMsg);
	   	}
   	}

    /* Cleanup */
    printf("closing getRemoteStatusTask socket, exiting\n");
  	closeSocket(myServerSocketFd);
		closeSocket(statusSocketFd);
    
   return 0;
}

void* getRemoteLogTask(void* threadInfo)
{
	  int myServerSocketFd, statusSocketFd;
   	socklen_t clilen;
   	struct sockaddr_in serv_addr, cli_addr;
   	int  n, noConnection;
   	int strLen=20;
		char str[strLen];
    LogMsgPacket logMsg;
		int socketDataFlags;

   	/* set signal handlers and actions */
  	struct sigaction sigAction;
  	sigAction.sa_handler = sigintHandler;
 	  sigAction.sa_flags = 0;
  	sigaction(SIGINT, &sigAction, NULL);
   
   	/* First call to socket() function */
   	myServerSocketFd = socket(AF_INET, SOCK_STREAM, 0);

		/* Update Socket Server connections to be non-blocking */
  	socketDataFlags = fcntl(myServerSocketFd, F_GETFL);
  	fcntl(myServerSocketFd, F_SETFL, socketDataFlags | O_NONBLOCK);
   
   	if (myServerSocketFd < 0) {
      	perror("getRemoteLogTask ERROR opening socket");
      	exit(1);
   	}
   	printf("getRemoteLogTask socket created\n");
   	
   	/* Initialize socket structure */
   	bzero((char *) &serv_addr, sizeof(serv_addr));
   
   	serv_addr.sin_family = AF_INET;
   	serv_addr.sin_addr.s_addr = INADDR_ANY;
   	serv_addr.sin_port = htons(LOG_PORT);
   
   	/* Now bind the host address using bind() call.*/
   	if (bind(myServerSocketFd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
      	perror("getRemoteLogTask ERROR on binding");
      	exit(1);
   	}
   	printf("getRemoteLogTask socket bound\n");   
   	/* Now start listening for the clients, here process will
     * go in sleep mode and will wait for the incoming connection
   	 */
   	printf("getRemoteLogTask listening...\n");
   	listen(myServerSocketFd, 50);
   	clilen = sizeof(cli_addr);

   	/* Update Socket Client connections to be non-blocking */
  	struct timeval timeout;
  	timeout.tv_usec = 10000;
  	timeout.tv_sec = 0;

   	/* Accept actual connection from the client */
   	noConnection = 1;
   	while(noConnection && (gExit != 0))
   	{
   			statusSocketFd = accept(myServerSocketFd, (struct sockaddr *)&cli_addr, &clilen);
				if(statusSocketFd == -1) {
          	continue;
      	}
   			inet_ntop(AF_INET, &(cli_addr.sin_addr), str, strLen);
				printf("getRemoteLogTask connection accepted from: %s\n", str);	
   			if (statusSocketFd > 0) {
   					noConnection = 0;
   			}
   	}
   	if(gExit != 0)
   	{
   			/* Update Socket Client connections to be non-blocking */
	    	setsockopt(myServerSocketFd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof(struct timeval));

	   		while((gExit != 0))
	   		{
	   	   		n = read(statusSocketFd, &logMsg, sizeof(LogMsgPacket));
	   
		   			if (n != sizeof(LogMsgPacket)) {
		      			INFO_PRINT("ERROR reading from getRemoteLogTask socket, expected: %u, got: %d, logMsg_e size: %u\n", 
								(uint16_t)sizeof(LogMsgPacket), n, (uint16_t)sizeof(logMsg_e));
		      			continue;
		   			}
		   			PRINT_LOG_MSG_HEADER(&logMsg);
	   		}
   	}

    /* Cleanup */
    printf("closing getRemoteLogTask socket, exiting\n");
  	closeSocket(myServerSocketFd);
		closeSocket(statusSocketFd);
    
	  return NULL;
}
void* getRemoteDataTask(void* threadInfo)
{
		int myServerSocketFd, statusSocketFd;
   	socklen_t clilen;
   	char buffer[256];
   	struct sockaddr_in serv_addr, cli_addr;
   	int  n, noConnection;
   	int strLen=20;
		char str[strLen];
		RemoteDataPacket dataMsg;
		int socketDataFlags;

   	/* set signal handlers and actions */
  	struct sigaction sigAction;
  	sigAction.sa_handler = sigintHandler;
 	  sigAction.sa_flags = 0;
  	sigaction(SIGINT, &sigAction, NULL);
   
   	/* First call to socket() function */
   	myServerSocketFd = socket(AF_INET, SOCK_STREAM, 0);

		/* Update Socket Server connections to be non-blocking */
  	socketDataFlags = fcntl(myServerSocketFd, F_GETFL);
  	fcntl(myServerSocketFd, F_SETFL, socketDataFlags | O_NONBLOCK);
   
   	if (myServerSocketFd < 0) {
      	perror("getRemoteDataTask ERROR opening socket");
      	exit(1);
   	}
   	printf("getRemoteDataTask socket created\n");
   	
   	/* Initialize socket structure */
   	bzero((char *) &serv_addr, sizeof(serv_addr));
   
   	serv_addr.sin_family = AF_INET;
   	serv_addr.sin_addr.s_addr = INADDR_ANY;
   	serv_addr.sin_port = htons(DATA_PORT);
   
   	/* Now bind the host address using bind() call.*/
   	if (bind(myServerSocketFd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
      	perror("getRemoteDataTask ERROR on binding");
      	exit(1);
   	}
   	printf("getRemoteDataTask socket bound\n");   
   	/* Now start listening for the clients, here process will
     * go in sleep mode and will wait for the incoming connection
   	 */
   	printf("getRemoteDataTask listening...\n");
   	listen(myServerSocketFd, 50);
   	clilen = sizeof(cli_addr);

   	/* Update Socket Client connections to be non-blocking */
  	struct timeval timeout;
  	timeout.tv_usec = 10000;
  	timeout.tv_sec = 0;

   	/* Accept actual connection from the client */
   	noConnection = 1;
   	while(noConnection && (gExit != 0))
   	{
   			statusSocketFd = accept(myServerSocketFd, (struct sockaddr *)&cli_addr, &clilen);
				if(statusSocketFd == -1) {
						continue;
				}				 
   			inet_ntop(AF_INET, &(cli_addr.sin_addr), str, strLen);
				printf("getRemoteDataTask, connection accepted from: %s\n", str);	
   			if (statusSocketFd > 0) {
   					noConnection = 0;
   			}
   	}
   	if(gExit != 0)
   	{
   			/* Update Socket Client connections to be non-blocking */
	    setsockopt(myServerSocketFd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof(struct timeval));

	    	/* If connection is established then start communicating */
	   		bzero(buffer,256);
	   		while((gExit != 0))
	   		{
	   	   		n = read(statusSocketFd, &dataMsg, sizeof(RemoteDataPacket));
	   
						if (n < 0) {
								printf("ERROR reading from getRemoteDataTask socket\n");
								continue;
						}
		   			printf("Sensor data,luxData: %f, moistureData: %f\n", dataMsg.luxData, dataMsg.moistureData);
	   	}
   	}

    /* Cleanup */
    printf("closing getRemoteDataTask socket, exiting\n");
  	closeSocket(myServerSocketFd);
		closeSocket(statusSocketFd);
    
   	return 0;
}
/*---------------------------------------------------------------------------------*/
/**
 * @brief Handler for SIGINT signal being received by main() - Kill application
 *
 * When SIGINT signal (ctrl+c) received by application, send kill signals to 
 * terminate Remote, Temp, and Light children threads and have main exit from 
 * processing loop to allow for gracefully termination of application.
 *
 * @param sig - param required by sigAction struct.
 * @return void
 */
void sigintHandler(int sig)
{
		printf("kill loop\n");
  	/* Trigger while-loop in main to exit; cleanup allocated resources */
  	gExit = 0;
}

int getSO_ERROR(int fd) {
   int err = 1;
   socklen_t len = sizeof err;
   if (-1 == getsockopt(fd, SOL_SOCKET, SO_ERROR, (char *)&err, &len))
      ERRNO_PRINT("getSO_ERROR");
   if (err)
      errno = err;              // set errno to the socket SO_ERROR
   return err;
}

void closeSocket(int fd) {      // *not* the Windows closesocket()
   if (fd >= 0) {
      getSO_ERROR(fd); // first clear any errors, which can cause close to fail
      if (shutdown(fd, SHUT_RDWR) < 0) // secondly, terminate the 'reliable' delivery
         if (errno != ENOTCONN && errno != EINVAL) // SGI causes EINVAL
            ERRNO_PRINT("shutdown\n");
      if (close(fd) < 0) // finally call close()
         ERRNO_PRINT("close");
   }
}