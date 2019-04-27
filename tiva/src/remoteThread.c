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
#include "main.h"

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

#define DIAGNOISTIC_PRINTS  (1)

/*---------------------------------------------------------------------------------*/
static uint8_t keepAlive;
static TaskHandle_t remoteStatusTaskHandle = NULL;
static TaskHandle_t remoteLogTaskHandle = NULL;
static TaskHandle_t remoteCmdTaskHandle = NULL;
static TaskHandle_t remoteDataTaskHandle = NULL;

/*---------------------------------------------------------------------------------*/
BaseType_t InitIPStack(void);
BaseType_t sendSocketData(Socket_t *pSocket, uint8_t *pData, size_t length);
BaseType_t readSocketData(Socket_t *pSocket, uint8_t *pData, size_t length);
void printConnectionStatus(BaseType_t ret);

void remoteStatusTask(void *pvParameters);
void remoteLogTask(void *pvParameters);
void remoteCmdTask(void *pvParameters);
void remoteDataTask(void *pvParameters);

/*---------------------------------------------------------------------------------*/
void remoteTask(void *pvParameters)
{
    LOG_REMOTE_CLIENT_EVENT(REMOTE_EVENT_STARTED);
    INFO_PRINT("THREAD CREATED, remoteTask #: %d\n\r", getTaskNum());

    /*-------------------------------------------------------------------------------------*/
    /* Initialize IP Stack and Network Driver  */
    /*-------------------------------------------------------------------------------------*/
    if(InitIPStack() != pdPASS) {
        INFO_PRINT("InitIPStack() failed\n");
    }
    else {
        /*------------------------------------------------------------------------------------*/
        /* create child threads */
        /*------------------------------------------------------------------------------------*/

        xTaskCreate(remoteStatusTask, (const portCHAR *)"RemoteStatus", configMINIMAL_STACK_SIZE, pvParameters, 1, &remoteStatusTaskHandle);
        setTaskNum(remoteStatusTaskHandle, PID_REMOTE_CLIENT_STATUS);
        xTaskCreate(remoteLogTask, (const portCHAR *)"RemoteLog", configMINIMAL_STACK_SIZE, pvParameters, 1, &remoteLogTaskHandle);
        setTaskNum(remoteLogTaskHandle, PID_REMOTE_CLIENT_LOG);
        xTaskCreate(remoteCmdTask, (const portCHAR *)"RemoteCmd", configMINIMAL_STACK_SIZE, pvParameters, 1, &remoteCmdTaskHandle);
        setTaskNum(remoteCmdTaskHandle, PID_REMOTE_CLIENT_CMD);
        xTaskCreate(remoteDataTask, (const portCHAR *)"RemoteData", configMINIMAL_STACK_SIZE, pvParameters, 1, &remoteDataTaskHandle);
        setTaskNum(remoteDataTaskHandle, PID_REMOTE_CLIENT_DATA);
    }
    LOG_REMOTE_CLIENT_EVENT(REMOTE_EVENT_EXITING);
    INFO_PRINT("remoteTask Exiting\n");
    vTaskDelete(NULL);
}

/*---------------------------------------------------------------------------------*/
/*
 *
 */
void remoteStatusTask(void *pvParameters)
{
    uint8_t count = 0;
    const TickType_t xDelay = LOG_QUEUE_RECV_WAIT_DELAY / portTICK_PERIOD_MS;
    TaskStatusPacket statusMsg;
    keepAlive = 1;
    static const TickType_t xTimeOut = pdMS_TO_TICKS(5000);
    struct freertos_sockaddr xServerAddress;
    socklen_t xSize = sizeof(struct freertos_sockaddr);
    uint8_t connectionLost;
    BaseType_t ret;
    SensorThreadInfo info = *((SensorThreadInfo *)pvParameters);
    Socket_t xClientSocket;

    LOG_REMOTE_CLIENT_EVENT(REMOTE_EVENT_STARTED);
    INFO_PRINT("THREAD CREATED, remoteStatusTask #: %d\n\r", getTaskNum());

    /*clear struct */
    memset(&statusMsg, 0,sizeof(TaskStatusPacket));

    /* Set destination */
    xServerAddress.sin_addr = FreeRTOS_inet_addr(SERVER_IP_ADDRESS_STR);
    xServerAddress.sin_port = FreeRTOS_htons(STATUS_PORT);

    /* create TCP socket */
    xClientSocket = FreeRTOS_socket(FREERTOS_AF_INET, FREERTOS_SOCK_STREAM, FREERTOS_IPPROTO_TCP);
    if(xClientSocket == FREERTOS_INVALID_SOCKET) {
        LOG_REMOTE_CLIENT_EVENT(REMOTE_BIST_COMPLETE);
        LOG_REMOTE_CLIENT_EVENT(REMOTE_INIT_ERROR);
        ERROR_PRINT("ERROR remoteStatusTask: Failed to create socket\n");
        vTaskDelay(2000);
    }
    else {
        /* set timeouts */
        FreeRTOS_setsockopt(xClientSocket, 0, FREERTOS_SO_SNDTIMEO, &xTimeOut, sizeof(xTimeOut));
        FreeRTOS_setsockopt(xClientSocket, 0, FREERTOS_SO_RCVTIMEO, &xTimeOut, sizeof(xTimeOut));

        /* Bind the socket */
        if(FreeRTOS_bind(xClientSocket, &xServerAddress, xSize) != 0) {
            LOG_REMOTE_CLIENT_EVENT(REMOTE_BIST_COMPLETE);
            LOG_REMOTE_CLIENT_EVENT(REMOTE_INIT_ERROR);
            ERROR_PRINT("ERROR remoteStatusTask: Failed to create socket\n");
            vTaskDelay(2000);
        }
        /*--------------------------------------------------------------------------*/
        /* Socket Created Successfully! */
        /*--------------------------------------------------------------------------*/
        else {
            LOG_REMOTE_CLIENT_EVENT(REMOTE_BIST_COMPLETE);
            LOG_REMOTE_CLIENT_EVENT(REMOTE_INIT_SUCCESS);

            /* set before entering so loop tries to connect */
            connectionLost = 1;

            /* send status msgs to Control Node */
            while(keepAlive)
            {
                ++count;

                /*--------------------------------------------------------------------------*/
                /* Connect Lost State */
                /*--------------------------------------------------------------------------*/
                if(connectionLost) {
                    ret = 0;
                    do {
                        ret = FreeRTOS_connect(xClientSocket, &xServerAddress, xSize);
                        printConnectionStatus(ret);
                    } while(ret != 0);
                    connectionLost = 0;
                }
                /*--------------------------------------------------------------------------*/
                /* Connected State */
                /*--------------------------------------------------------------------------*/
                else {
                    /* get thread status msgs */
                    if(xQueueReceive(info.statusFd, (void *)&statusMsg, xDelay) != pdFALSE) {
                        /* send status msgs to Control Node */
                        if(sendSocketData(&xClientSocket, (uint8_t *)&statusMsg, sizeof(statusMsg)) == pdFREERTOS_ERRNO_ENOTCONN) {
                            connectionLost = 1;
                        }
                        else {
                            /* for diagnostics */
                            //if(DIAGNOISTIC_PRINTS) {PRINT_STATUS_MSG_HEADER(&statusMsg);}
                        }
                    }
                    else {
                        LOG_REMOTE_CLIENT_EVENT(REMOTE_STATUS_QUEUE_ERROR);
                    }
                }
            }
        }

        /* gracefully shutdown socket */
        FreeRTOS_shutdown( xClientSocket, FREERTOS_SHUT_RDWR );

        /* The socket has shut down and is safe to close */
        FreeRTOS_closesocket( xClientSocket );
    }
    LOG_REMOTE_CLIENT_EVENT(REMOTE_EVENT_EXITING);
    INFO_PRINT("remoteStatusTask Exiting\n");
    vTaskDelete(NULL);
}

/*---------------------------------------------------------------------------------*/
/*
 *
 */
void remoteLogTask(void *pvParameters)
{
    uint8_t count = 0;
    const TickType_t xDelay = LOG_QUEUE_RECV_WAIT_DELAY / portTICK_PERIOD_MS;
    LogMsgPacket logMsg;
    keepAlive = 1;
    static const TickType_t xTimeOut = pdMS_TO_TICKS(5000);
    struct freertos_sockaddr xServerAddress;
    socklen_t xSize = sizeof(struct freertos_sockaddr);
    uint8_t connectionLost;
    BaseType_t ret;
    SensorThreadInfo info = *((SensorThreadInfo *)pvParameters);
    Socket_t xClientSocket;

    LOG_REMOTE_CLIENT_EVENT(REMOTE_EVENT_STARTED);
    INFO_PRINT("THREAD CREATED, remoteLogTask #: %d\n\r", getTaskNum());

    /* clear structure */
    memset(&logMsg, 0,sizeof(LogMsgPacket));

    /* Set destination */
    xServerAddress.sin_addr = FreeRTOS_inet_addr(SERVER_IP_ADDRESS_STR);
    xServerAddress.sin_port = FreeRTOS_htons(LOG_PORT);

    /* create TCP socket */
    xClientSocket = FreeRTOS_socket(FREERTOS_AF_INET, FREERTOS_SOCK_STREAM, FREERTOS_IPPROTO_TCP);
    if(xClientSocket == FREERTOS_INVALID_SOCKET) {
        LOG_REMOTE_CLIENT_EVENT(REMOTE_BIST_COMPLETE);
        LOG_REMOTE_CLIENT_EVENT(REMOTE_INIT_ERROR);
        ERROR_PRINT("ERROR remoteStatusTask: Failed to create socket\n");
        vTaskDelay(2000);
    }
    else {
        /* set timeouts */
        FreeRTOS_setsockopt(xClientSocket, 0, FREERTOS_SO_SNDTIMEO, &xTimeOut, sizeof(xTimeOut));
        FreeRTOS_setsockopt(xClientSocket, 0, FREERTOS_SO_RCVTIMEO, &xTimeOut, sizeof(xTimeOut));

        /* Bind the socket */
        if(FreeRTOS_bind(xClientSocket, &xServerAddress, xSize) != 0) {
            LOG_REMOTE_CLIENT_EVENT(REMOTE_BIST_COMPLETE);
            LOG_REMOTE_CLIENT_EVENT(REMOTE_INIT_ERROR);
            ERROR_PRINT("ERROR remoteLogTask: Failed to create socket\n");
            vTaskDelay(2000);
        }
        /*--------------------------------------------------------------------------*/
        /* Socket Created Successfully! */
        /*--------------------------------------------------------------------------*/
        else {
            LOG_REMOTE_CLIENT_EVENT(REMOTE_BIST_COMPLETE);
            LOG_REMOTE_CLIENT_EVENT(REMOTE_INIT_SUCCESS);

            /* set before entering so loop tries to connect */
            connectionLost = 1;

            /* send status msgs to Control Node */
            while(keepAlive)
            {
                ++count;

                /*--------------------------------------------------------------------------*/
                /* Connect Lost State */
                /*--------------------------------------------------------------------------*/
                if(connectionLost) {
                    ret = 0;
                    do {
                        ret = FreeRTOS_connect(xClientSocket, &xServerAddress, xSize);
                        printConnectionStatus(ret);
                    } while(ret != 0);
                    connectionLost = 0;
                }
                /*--------------------------------------------------------------------------*/
                /* Connected State */
                /*--------------------------------------------------------------------------*/
                else {
                    /* get log msgs */
                    if(xQueueReceive(info.logFd, (void *)&logMsg, xDelay) != pdFALSE)
                    {
                        /* Transmit data to Control Node */
                        if(sendSocketData(&xClientSocket, (uint8_t *)&logMsg, sizeof(logMsg)) == pdFREERTOS_ERRNO_ENOTCONN) {
                            connectionLost = 1;
                        }
                        /* for diagnostics */
                        if(DIAGNOISTIC_PRINTS) {PRINT_LOG_MSG_HEADER(&logMsg);}
                    }
                    else {
                        LOG_REMOTE_CLIENT_EVENT(REMOTE_LOG_QUEUE_ERROR);
                    }
                }
            }
        }

        /* gracefully shutdown socket */
        FreeRTOS_shutdown( xClientSocket, FREERTOS_SHUT_RDWR );

        /* The socket has shut down and is safe to close */
        FreeRTOS_closesocket( xClientSocket );
    }
    LOG_REMOTE_CLIENT_EVENT(REMOTE_EVENT_EXITING);
    INFO_PRINT("remoteLogTask Exiting\n");
    vTaskDelete(NULL);
}

/*---------------------------------------------------------------------------------*/
/*
 *
 */
void remoteDataTask(void *pvParameters)
{
    uint8_t count = 0;
    RemoteDataPacket sensorData;
    keepAlive = 1;
    static const TickType_t xTimeOut = pdMS_TO_TICKS(5000);
    struct freertos_sockaddr xServerAddress;
    socklen_t xSize = sizeof(struct freertos_sockaddr);
    uint8_t connectionLost;
    BaseType_t ret;
    SensorThreadInfo info = *((SensorThreadInfo *)pvParameters);
    Socket_t xClientSocket;

    LOG_REMOTE_CLIENT_EVENT(REMOTE_EVENT_STARTED);
    INFO_PRINT("THREAD CREATED, remoteDataTask #: %d\n\r", getTaskNum());

    /* clear structure */
    memset(&sensorData, 0,sizeof(RemoteDataPacket));

    /* Set destination */
    xServerAddress.sin_addr = FreeRTOS_inet_addr(SERVER_IP_ADDRESS_STR);
    xServerAddress.sin_port = FreeRTOS_htons(DATA_PORT);

    /* create TCP socket */
    xClientSocket = FreeRTOS_socket(FREERTOS_AF_INET, FREERTOS_SOCK_STREAM, FREERTOS_IPPROTO_TCP);
    if(xClientSocket == FREERTOS_INVALID_SOCKET) {
        LOG_REMOTE_CLIENT_EVENT(REMOTE_BIST_COMPLETE);
        LOG_REMOTE_CLIENT_EVENT(REMOTE_INIT_ERROR);
        ERROR_PRINT("ERROR remoteDataTask: Failed to create socket\n");
        vTaskDelay(2000);
    }
    else {
        /* set timeouts */
        FreeRTOS_setsockopt(xClientSocket, 0, FREERTOS_SO_SNDTIMEO, &xTimeOut, sizeof(xTimeOut));
        FreeRTOS_setsockopt(xClientSocket, 0, FREERTOS_SO_RCVTIMEO, &xTimeOut, sizeof(xTimeOut));

        /* Bind the socket */
        if(FreeRTOS_bind(xClientSocket, &xServerAddress, xSize) != 0) {
            LOG_REMOTE_CLIENT_EVENT(REMOTE_BIST_COMPLETE);
            LOG_REMOTE_CLIENT_EVENT(REMOTE_INIT_ERROR);
            ERROR_PRINT("ERROR remoteDataTask: Failed to create socket\n");
            vTaskDelay(2000);
        }
        /*--------------------------------------------------------------------------*/
        /* Socket Created Successfully! */
        /*--------------------------------------------------------------------------*/
        else {
            LOG_REMOTE_CLIENT_EVENT(REMOTE_BIST_COMPLETE);
            LOG_REMOTE_CLIENT_EVENT(REMOTE_INIT_SUCCESS);

            /* set before entering so loop tries to connect */
            connectionLost = 1;

            /* send status msgs to Control Node */
            while(keepAlive)
            {
                ++count;

                /*--------------------------------------------------------------------------*/
                /* Connect Lost State */
                /*--------------------------------------------------------------------------*/
                if(connectionLost) {
                    ret = 0;
                    do {
                        ret = FreeRTOS_connect(xClientSocket, &xServerAddress, xSize);
                        printConnectionStatus(ret);
                    } while(ret != 0);
                    connectionLost = 0;
                }
                /*--------------------------------------------------------------------------*/
                /* Connected State */
                /*--------------------------------------------------------------------------*/
                else {
                    /* try to read sensor data from shmem */
                    if(xSemaphoreTake(info.shmemMutex, THREAD_MUTEX_DELAY) == pdTRUE)
                    {
                        /* read data from shmem */
                        if(info.pShmem != NULL) {
                            sensorData.luxData = info.pShmem->lightData.apds9301_luxData;
                            sensorData.moistureData = info.pShmem->moistData.moistureLevel;
                        }

                        /* release mutex ASAP so others can use */
                        xSemaphoreGive(info.shmemMutex);

                        /* Transmit data to Control Node */
                        if(sendSocketData(&xClientSocket, (uint8_t *)&sensorData, sizeof(sensorData)) == pdFREERTOS_ERRNO_ENOTCONN) {
                            connectionLost = 1;
                        }
                        /* for diagnostics */
                        if(DIAGNOISTIC_PRINTS) {
                            //INFO_PRINT("Sensor data, luxData: %d, moistureData: %d\n", (int16_t)sensorData.luxData, (int16_t)sensorData.moistureData);
                        }
                    }
                }
            }
            vTaskDelay(500);
        }

        /* gracefully shutdown socket */
        FreeRTOS_shutdown( xClientSocket, FREERTOS_SHUT_RDWR );

        /* The socket has shut down and is safe to close */
        FreeRTOS_closesocket( xClientSocket );
    }
    LOG_REMOTE_CLIENT_EVENT(REMOTE_EVENT_EXITING);
    INFO_PRINT("remoteDataTask Exiting\n");
    vTaskDelete(NULL);
}

/*---------------------------------------------------------------------------------*/
/*
 *
 */
void remoteCmdTask(void *pvParameters)
{

    /*clear struct */
    //memset(&sensorData, 0,sizeof(RemoteDataPacket));
    //readSocketData(&xClientSocket, recvData, sizeof(recvData));
    INFO_PRINT("remoteCmdTask Exiting\n");
    LOG_REMOTE_CLIENT_EVENT(REMOTE_EVENT_EXITING);
    vTaskDelete(NULL);
}

/*---------------------------------------------------------------------------------*/
/*
 *
 */
void killRemoteTask(void)
{
    keepAlive = 0;
}

/*---------------------------------------------------------------------------------*/
/*
 *
 */
BaseType_t InitIPStack(void)
{
    BaseType_t ret;
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

    ret = FreeRTOS_IPInit(ucIPAddress, ucNetMask, ucGatewayAddress,
                          ucDNSServerAddress, ucMACAddress);
    return ret;
}

/*---------------------------------------------------------------------------------*/
/*
 *
 */
BaseType_t readSocketData(Socket_t *pSocket, uint8_t *pData, size_t length)
{
    BaseType_t ret;
    configASSERT(pSocket);

    /* try to read data */
    ret = FreeRTOS_recv(*pSocket, pData, length, 0);

    if(ret == length) {
        return RETURN_SUCCESS;
    }

    /* FreeRTOS uses negative value to indicate obsolete error code */
    ret = ret < 0 ? -1 * ret : ret;

    if(ret == pdFREERTOS_ERRNO_ENOMEM ) {
        /* The no-memory error has priority above the non-connected error.
        Both are fatal and will elad to closing the socket. */
        ERROR_PRINT("ERROR in readSocketData: pdFREERTOS_ERRNO_ENOMEM (no memory)\n");
    }
    if(ret == pdFREERTOS_ERRNO_ENOTCONN ) {
        ERROR_PRINT("ERROR in readSocketData: pdFREERTOS_ERRNO_ENOTCONN (socket not connected)\n");
    }
    if(ret == pdFREERTOS_ERRNO_EINTR ) {
        ERROR_PRINT("ERROR in readSocketData: pdFREERTOS_ERRNO_EINTR\n");
    }
    if(ret == pdFREERTOS_ERRNO_EINVAL ) {
        /* Socket invalid, not TCP or not bound */
        ERROR_PRINT("ERROR in readSocketData: pdFREERTOS_ERRNO_EINVAL(invalid socket) \n");
    }
    if(ret == 0) {
        ERROR_PRINT("readSocketData TIMEOUT\n");
    }
    return RETURN_ERROR;
}

/*---------------------------------------------------------------------------------*/
/*
 *
 */
BaseType_t sendSocketData(Socket_t *pSocket, uint8_t *pData, size_t length)
{
    BaseType_t ret;
    configASSERT(pSocket);

    /* send data */
    ret = FreeRTOS_send(*pSocket, pData, length, 0);

    /* if all bytes sent, success! */
    if(ret == length) {
        return ret;
    }

    /* FreeRTOS uses negative value to indicate obsolete error code */
    ret = ret < 0 ? -1 * ret : ret;

    if(ret == pdFREERTOS_ERRNO_ENOMEM) {
        ERROR_PRINT("ERROR in readSocketData: pdFREERTOS_ERRNO_ENOMEM (out of memory)\n");
    }
    if(ret == pdFREERTOS_ERRNO_ENOTCONN) {
        ERROR_PRINT("ERROR in readSocketData: pdFREERTOS_ERRNO_ENOTCONN(socket not connected)\n");
    }
    if(ret == pdFREERTOS_ERRNO_EINVAL) {
        ERROR_PRINT("ERROR in readSocketData: pdFREERTOS_ERRNO_EINVAL \n");
    }
    if(ret == pdFREERTOS_ERRNO_ENOSPC) {
        ERROR_PRINT("readSocketData TIMEOUT\n");
    }
    return ret;
}

/*---------------------------------------------------------------------------------*/
/*
 *
 */
void printConnectionStatus(BaseType_t ret)
{
    /* FreeRTOS uses negative value to indicate obsolete error code */
    ret = ret < 0 ? -1 * ret : ret;

    if(ret == pdFREERTOS_ERRNO_EBADF) {
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
}
