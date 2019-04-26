/***********************************************************************************
 * @author Brian Ibeling and Joshua Malburg
 * Brian.Ibeling@colorado.edu, joshua.malburg@colorad.edu
 * Advanced Embedded Software Development
 * ECEN5013 - Rick Heidebrecht
 * @date April 12, 2019
 * CCS  Version: 8.3.0.00009
 ************************************************************************************
 *
 * @file remoteThread.c
 * @brief TODO
 *
 * Resources Utilized:
 *   - https://www.freertos.org/FreeRTOS-Plus/FreeRTOS_Plus_TCP/TCP_Networking_Tutorial_TCP_Client_and_Server.html
 *
 ************************************************************************************
 */

/* stdlib includes */
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

/* app specific includes */
#include "remoteThread.h"
#include "cmn_timer.h"
#include "uartstdio.h"
#include "packet.h"
#include "my_debug.h"
#include "healthMonitor.h"
#include "logger.h"

/* TivaWare includes */
#include "driverlib/sysctl.h"
#include "driverlib/debug.h"
#include "driverlib/gpio.h"
#include "driverlib/pin_map.h"
#include "inc/hw_memmap.h"

/* FreeRTOS includes */
#include "FreeRTOS.h"
#include "FreeRTOS_IP.h"
#include "FreeRTOS_Sockets.h"
#include "queue.h"
#include "task.h"
#include "semphr.h"

/*---------------------------------------------------------------------------------*/
static uint8_t keepAlive;   /* global to kill thread */
static Socket_t xListeningSocket;

/*---------------------------------------------------------------------------------*/
void InitIP(void);
int8_t sendSocketData(uint8_t *pData, size_t length);
int8_t readSocketData(uint8_t *pData, size_t length);

/*---------------------------------------------------------------------------------*/
void remoteTask(void *pvParameters)
{
    uint8_t count = 0;
    float luxData, moisture;
    const TickType_t xDelay = LOG_QUEUE_RECV_WAIT_DELAY / portTICK_PERIOD_MS;
    TaskStatusPacket statusMsg;
    LogMsgPacket logMsg;
    keepAlive = 1;

    LOG_REMOTE_CLIENT_EVENT(REMOTE_EVENT_STARTED);
    MUTED_PRINT("Remote Task #: %d\n\r", getTaskNum());

    InitIP();
    /*-------------------------------------------------------------------------------------*/
    /* set up socket  */
    /*-------------------------------------------------------------------------------------*/

    static const TickType_t xTimeOut = pdMS_TO_TICKS(5000);
    char cString[ 50 ];
    uint8_t recvData[30];
    uint32_t ulCount = 0UL;
    struct freertos_sockaddr xDestinationAddress;
    socklen_t xSize = sizeof(struct freertos_sockaddr);
    uint8_t chksumEnable = 1;
    BaseType_t ret;

    /* Set destination */
    xDestinationAddress.sin_addr = FreeRTOS_inet_addr( "10.0.0.93" );
    xDestinationAddress.sin_port = FreeRTOS_htons(5008);

#ifdef UDP_SEND_TEST
    Socket_t xSocket;
    const TickType_t x1000ms = 1000UL / portTICK_PERIOD_MS;


       /* Create the socket. */
       xSocket = FreeRTOS_socket( FREERTOS_AF_INET,
                                  FREERTOS_SOCK_DGRAM,/*FREERTOS_SOCK_DGRAM for UDP.*/
                                  FREERTOS_IPPROTO_UDP );

       /* Check the socket was created. */
       configASSERT( xSocket != FREERTOS_INVALID_SOCKET );

       /* NOTE: FreeRTOS_bind() is not called.  This will only work if
       ipconfigALLOW_SOCKET_SEND_WITHOUT_BIND is set to 1 in FreeRTOSIPConfig.h. */

       for( ;; )
       {
           /* Create the string that is sent. */
           sprintf( cString,
                    "Standard send message number %lu\r\n",
                    ulCount );

           /* Send the string to the UDP socket.  ulFlags is set to 0, so the standard
           semantics are used.  That means the data from cString[] is copied
           into a network buffer inside FreeRTOS_sendto(), and cString[] can be
           reused as soon as FreeRTOS_sendto() has returned. */
           FreeRTOS_sendto( xSocket,
                            cString,
                            strlen( cString ),
                            0,
                            &xDestinationAddress,
                            sizeof( xDestinationAddress ) );

           ulCount++;

           /* Wait until it is time to send again. */
           vTaskDelay( x1000ms );
       }

#else

    /* Attempt to open the TCP socket. */
    xListeningSocket = FreeRTOS_socket( FREERTOS_AF_INET,
                                        FREERTOS_SOCK_STREAM, FREERTOS_IPPROTO_TCP );

    /* Check for errors. */
    configASSERT( xListeningSocket != FREERTOS_INVALID_SOCKET );

    /* set timeouts and checksum flag */
    FreeRTOS_setsockopt( xListeningSocket, 0, FREERTOS_SO_SNDTIMEO,
                         &xTimeOut, sizeof( xTimeOut ) );
    FreeRTOS_setsockopt( xListeningSocket, 0, FREERTOS_SO_RCVTIMEO,
                         &xTimeOut, sizeof( xTimeOut ) );

    /* Bind the socket, but pass in NULL to let FreeRTOS+TCP choose the port number.
    See the next source code snipped for an example of how to bind to a specific
    port number. */
    FreeRTOS_bind(xListeningSocket, &xDestinationAddress, xSize);
#endif

    /*-------------------------------------------------------------------------------------*/

    /* set portion of statusMsg that does not change */
    memset(&logMsg, 0,sizeof(LogMsgPacket));

    /* get status queue handle, etc */
    SensorThreadInfo info = *((SensorThreadInfo *)pvParameters);

    /* TODO - set BIST error in logMsg if necessary */
    LOG_REMOTE_CLIENT_EVENT(REMOTE_BIST_COMPLETE);
    if(0) {
        LOG_REMOTE_CLIENT_EVENT(REMOTE_INIT_SUCCESS);
    }
    else {
        LOG_REMOTE_CLIENT_EVENT(REMOTE_INIT_ERROR);
    }
    while(!(FreeRTOS_connect(xListeningSocket, &xDestinationAddress, sizeof(xDestinationAddress))));
    /* try to connect */
    ret = 0;
//    do
//    {
//        ret = FreeRTOS_connect(xListeningSocket, &xDestinationAddress, xSize);
//    } while(ret != 0);

    if((ret == pdFREERTOS_ERRNO_EBADF) || (ret == (-1 * pdFREERTOS_ERRNO_EBADF))) {
        ERROR_PRINT("FreeRTOS_connect failed: pdFREERTOS_ERRNO_EBADF \n");
    }
    else if(ret == pdFREERTOS_ERRNO_EISCONN) {
        ERROR_PRINT("FreeRTOS_connect failed: pdFREERTOS_ERRNO_EISCONN \n");
    }
    else if(ret == pdFREERTOS_ERRNO_EINPROGRESS) {
        ERROR_PRINT("FreeRTOS_connect failed: pdFREERTOS_ERRNO_EINPROGRESS \n");
    }
    else if(ret == pdFREERTOS_ERRNO_EAGAIN) {
        ERROR_PRINT("FreeRTOS_connect failed: pdFREERTOS_ERRNO_EAGAIN \n");
    }
    else if(ret == pdFREERTOS_ERRNO_EWOULDBLOCK) {
        ERROR_PRINT("FreeRTOS_connect failed: pdFREERTOS_ERRNO_EWOULDBLOCK \n");
    }
    else if(ret == pdFREERTOS_ERRNO_ETIMEDOUT) {
        ERROR_PRINT("FreeRTOS_connect failed: pdFREERTOS_ERRNO_ETIMEDOUT \n");
    }
    else if (ret != 0) {
        ERROR_PRINT("FreeRTOS_connect failed: OTHER(%d)\n", ret);
    }
    else {
        INFO_PRINT("FreeRTOS_connect successed\n");
    }

    /* send data, log and status msgs to Control Node */
    while(keepAlive)
    {
        ++count;

        /* try to read sensor data from shmem */
        /* TODO - need to restrict how offet this is sent */
        if( xSemaphoreTake( info.shmemMutex, THREAD_MUTEX_DELAY ) == pdTRUE )
        {
            /* read data from shmem */
            /* TODO - read shmem method */
            luxData = info.pShmem->lightData.apds9301_luxData;
            moisture = info.pShmem->moistData.moistureLevel;
            luxData = luxData *= 1.0f;
            moisture *= 1.0f;

            /* release mutex */
            xSemaphoreGive(info.shmemMutex);
        }
        else {
            /* mutex timeout occurred */
            LOG_REMOTE_CLIENT_EVENT(REMOTE_SHMEM_ERROR);
        }
        /*---------------------------------------------------------------------*/
        /* TODO - send shmem data to Control Node */
        /*---------------------------------------------------------------------*/
        /* Create the string that is sent. */
        sprintf(cString, "Standard send message number %lu\r\n", ulCount);

        /* Send the string to the UDP socket */
        sendSocketData((uint8_t *)cString, strlen(cString));
 //       readSocketData(recvData, sizeof(recvData));

        ulCount++;
        /*-----------------------------------------------------------------------*/
        /* get thread status msgs */
        if(xQueueReceive(info.statusFd, (void *)&statusMsg, xDelay) != pdFALSE)
        {
            /* for development (verify queue send/recv) */
            //PRINT_STATUS_MSG_HEADER(&statusMsg);

            /* TODO - send status msgs to Control Node */
        }
        else {
            //LOG_REMOTE_CLIENT_EVENT(REMOTE_LOG_QUEUE_ERROR);
        }

        /* get log msgs */
        if(xQueueReceive(info.logFd, (void *)&logMsg, xDelay) != pdFALSE)
        {
            /* for development (verify queue send/recv) */
            PRINT_LOG_MSG_HEADER(&logMsg);

            /* TODO - send log msgs to Control Node */
        }
        else {
            //LOG_REMOTE_CLIENT_EVENT(REMOTE_LOG_QUEUE_ERROR);
        }
    }

    /* gracefully shutdown socket */
    FreeRTOS_shutdown( xListeningSocket, FREERTOS_SHUT_RDWR );

    /* The socket has shut down and is safe to close. */
    FreeRTOS_closesocket( xListeningSocket );
    LOG_REMOTE_CLIENT_EVENT(REMOTE_EVENT_EXITING);
}

/*---------------------------------------------------------------------------------*/
void killRemoteTask(void)
{
    keepAlive = 0;
}

/*---------------------------------------------------------------------------------*/
void InitIP(void)
{
    /* Define the network addressing.  These parameters will be used if either
    ipconfigUSE_DHCP is 0 or if ipconfigUSE_DHCP is 1 but DHCP auto configuration
    failed. */
    static const uint8_t ucIPAddress[ 4 ] = { 10, 0, 0, 202 };
    static const uint8_t ucNetMask[ 4 ] = { 255, 255, 255, 0 };
    static const uint8_t ucGatewayAddress[ 4 ] = { 10, 0, 0, 1 };

    /* The following is the address of an OpenDNS server. */
    static const uint8_t ucDNSServerAddress[ 4 ] = { 75, 75, 75, 75 };

    /* The MAC address array is not declared const as the MAC address will normally
    be read from an EEPROM and not hard coded (in real deployed applications).*/
    static uint8_t ucMACAddress[ 6 ] = { 0x00, 0x1a, 0xb6, 0x03, 0x59, 0x89 };

    if(FreeRTOS_IPInit(ucIPAddress, ucNetMask, ucGatewayAddress,
                       ucDNSServerAddress, ucMACAddress) != pdPASS) {
        ERROR_PRINT("IPInit error\n");
    }
}

int8_t readSocketData(uint8_t *pData, size_t length)
{
    BaseType_t ret;
    ret = FreeRTOS_recv(xListeningSocket, pData, length, 0);

    if(ret == length) {
        return RETURN_SUCCESS;
    }
    if(ret == pdFREERTOS_ERRNO_ENOMEM ) {
        ERROR_PRINT("ERROR in readSocketData: pdFREERTOS_ERRNO_ENOMEM\n");
    }
    if(ret == pdFREERTOS_ERRNO_ENOTCONN ) {
        ERROR_PRINT("ERROR in readSocketData: pdFREERTOS_ERRNO_ENOTCONN\n");
    }
    if(ret == pdFREERTOS_ERRNO_EINTR ) {
        ERROR_PRINT("ERROR in readSocketData: pdFREERTOS_ERRNO_EINTR\n");
    }
    if(ret == pdFREERTOS_ERRNO_EINVAL ) {
        ERROR_PRINT("ERROR in readSocketData: pdFREERTOS_ERRNO_EINVAL\n");
    }
    if(ret == 0) {
        ERROR_PRINT("readSocketData TIMEOUT\n");
    }
    return RETURN_ERROR;
}

int8_t sendSocketData(uint8_t *pData, size_t length)
{
    BaseType_t ret;
    ret = FreeRTOS_send(xListeningSocket, pData, length, 0);

    if(ret == length) {
        return RETURN_SUCCESS;
    }
    if(ret == pdFREERTOS_ERRNO_ENOMEM ) {
        ERROR_PRINT("ERROR in readSocketData: pdFREERTOS_ERRNO_ENOMEM\n");
    }
    if(ret == pdFREERTOS_ERRNO_ENOTCONN ) {
        ERROR_PRINT("ERROR in readSocketData: pdFREERTOS_ERRNO_ENOTCONN\n");
    }
    if(ret == pdFREERTOS_ERRNO_EINVAL  ) {
        ERROR_PRINT("ERROR in readSocketData: pdFREERTOS_ERRNO_EINVAL \n");
    }
    if(ret == pdFREERTOS_ERRNO_ENOSPC ) {
        ERROR_PRINT("readSocketData TIMEOUT\n");
    }
    return RETURN_ERROR;
}
