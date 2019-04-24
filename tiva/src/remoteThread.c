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

/*---------------------------------------------------------------------------------*/
void InitIP(void);

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

    /* initialize sockets for sensor, status, and logging data channels */
//    Socket_t xSensorSocket, xStatusSocket, xLoggingSocket;
//    socklen_t xSize = sizeof( struct freertos_sockaddr );
//    static const TickType_t xTimeOut = pdMS_TO_TICKS( 2000 );

    /*-------------------------------------------------------------------------------------*/
    /* set up socket  */
    /*-------------------------------------------------------------------------------------*/
    Socket_t xListeningSocket;
    const TickType_t xTimeOut = 30 / portTICK_PERIOD_MS;
    char cString[ 50 ];
    uint8_t recvData[50];
    uint32_t ulCount = 0UL;
    struct freertos_sockaddr xDestinationAddress;
    socklen_t xSize = sizeof(struct freertos_sockaddr);
    uint8_t chksumEnable = 1;

    /* Set destination */
    xDestinationAddress.sin_addr = FreeRTOS_inet_addr( "10.0.0.144" );
    xDestinationAddress.sin_port = FreeRTOS_htons( 1021 );

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
//    FreeRTOS_setsockopt( xListeningSocket, 0, FREERTOS_SO_UDPCKSUM_OUT,
//                         &chksumEnable, 0 );

    /* Bind the socket to port 0x1234. */
    FreeRTOS_bind(xListeningSocket, &xDestinationAddress, xSize);

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

    /* try to connect */
    while(!(FreeRTOS_connect(xListeningSocket, &xDestinationAddress, xSize)));

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
        //FreeRTOS_send(xListeningSocket, cString, strlen(cString), 0);
        FreeRTOS_recv(xListeningSocket, recvData, sizeof(recvData), 0);

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
