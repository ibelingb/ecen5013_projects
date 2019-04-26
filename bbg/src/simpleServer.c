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

/* global to kill loop */
uint8_t gExit = 1;

void sigintHandler(int sig);
void set_sig_handlers(void);

int main( int argc, char *argv[] ) 
{
   	int sockfd, newsockfd, portno;
   	socklen_t clilen;
   	char buffer[256];
   	struct sockaddr_in serv_addr, cli_addr;
   	int  n, noConnection;
   	int strLen=20;
	char str[strLen];

   	/* set signal handlers and actions */
	set_sig_handlers();
  	struct sigaction sigAction;
  	sigAction.sa_handler = sigintHandler;
 	sigAction.sa_flags = 0;
  	sigaction(SIGINT, &sigAction, NULL);
   
   	/* First call to socket() function */
   	sockfd = socket(AF_INET, SOCK_STREAM, 0);
   
   	if (sockfd < 0) {
      	perror("ERROR opening socket");
      	exit(1);
   	}
   	printf("socket created\n");
   	
   	/* Initialize socket structure */
   	bzero((char *) &serv_addr, sizeof(serv_addr));
   	portno = 5008;
   
   	serv_addr.sin_family = AF_INET;
   	serv_addr.sin_addr.s_addr = INADDR_ANY;
   	serv_addr.sin_port = htons(portno);
   
   	/* Now bind the host address using bind() call.*/
   	if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
      	perror("ERROR on binding");
      	exit(1);
   	}
   	printf("socket bound\n");   
   	/* Now start listening for the clients, here process will
     * go in sleep mode and will wait for the incoming connection
   	 */
   	printf("listening...\n");
   	listen(sockfd, 50);
   	clilen = sizeof(cli_addr);

   	/* Update Socket Client connections to be non-blocking */
  	struct timeval timeout;
  	timeout.tv_usec = 1000;
  	timeout.tv_sec = 0;

   	/* Accept actual connection from the client */
   	noConnection = 1;
   	while(noConnection && gExit)
   	{
   		newsockfd = accept(sockfd, (struct sockaddr *)&cli_addr, &clilen);
   		inet_ntop(AF_INET, &(cli_addr.sin_addr), str, strLen);
		printf("connection accepted from: %s\n", str);	
   		if (newsockfd > 0) {
   			noConnection = 0;
   		}
   	}
   	if(gExit != 0)
   	{
   		/* Update Socket Client connections to be non-blocking */
	    setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof(struct timeval));

	    /* If connection is established then start communicating */
	   	bzero(buffer,256);
	   	while(gExit)
	   	{

	   	   	n = read(newsockfd,buffer, 5);
	   
		   	if (n < 0) {
		      	printf("ERROR reading from socket\n");
		      	continue;
		   	}
		   	printf(" %s\n", buffer);
	   	}
   	}

    /* Cleanup */
    printf("closing socket, exiting\n");
  	close(sockfd);
    
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

void set_sig_handlers(void)
{
    // struct sigaction action;

    // action.sa_flags = SA_SIGINFO;
   
    // action.sa_sigaction = remoteLogSigHandler;
    // if (sigaction(SIGRTMIN + (uint8_t)PID_REMOTE_LOG, &action, NULL) == -1) {
    //     perror("sigusr: sigaction");
    //     _exit(1);
    // }
    // action.sa_sigaction = remoteStatusSigHandler;
    // if (sigaction(SIGRTMIN + (uint8_t)PID_REMOTE_STATUS, &action, NULL) == -1) {
    //     perror("sigusr: sigaction");
    //     _exit(1);
    // }
    // action.sa_sigaction = remoteDataSigHandler;
    // if (sigaction(SIGRTMIN + (uint8_t)PID_REMOTE_DATA, &action, NULL) == -1) {
    //     perror("sigusr: sigaction");
    //     _exit(1);
    // }
    // action.sa_sigaction = remoteCmdSigHandler;
    // if (sigaction(SIGRTMIN + (uint8_t)PID_REMOTE_CMD, &action, NULL) == -1) {
    //     perror("sigusr: sigaction");
    //     _exit(1);
    // }
    // action.sa_sigaction = loggingSigHandler;
    // if (sigaction(SIGRTMIN + (uint8_t)PID_LOGGING, &action, NULL) == -1) {
    //     perror("sigusr: sigaction");
    //     _exit(1);
    // }
}