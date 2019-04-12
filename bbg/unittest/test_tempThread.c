/***********************************************************************************
 * @author Josh Malburg
 * joshua.malburg@colorado.edu
 * Advanced Embedded Software Development
 * ECEN5013 - Rick Heidebrecht
 * @date March 20, 2019
 * arm-linux-gnueabi (Buildroot)
 * gcc (Ubuntu)
 ************************************************************************************
 *
 * @file test_tempThread.c
 * @brief verify tempThread (sensor setup/query, shmem writes, status feedback
 * to main and exit signalling)
 *
 ************************************************************************************
 */

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <stdint.h>
#include <pthread.h>
#include <mqueue.h>
#include <errno.h>
#include <string.h>         // for strerror()
#include <sys/syscall.h>
#include <fcntl.h>          //for O_RDWR
#include <sys/shm.h>
#include <sys/mman.h>

#include "my_debug.h"
#include "tempThread.h"
#include "lightThread.h"
#include "healthMonitor.h"

#define NUM_TEST_THREADS                 (2)
#define INC_HB_QUEUE

/* global for causing threads to exit */
int gExitSig = 1;

pthread_mutex_t gSharedMemMutex; // TODO: Could have separate mutexes for each sensor writing to SHM
pthread_mutex_t gI2cBusMutex; 

static void *remoteThread(void *pArg);
int8_t getQueueError(mqd_t * pQueue, TaskStatusPacket *pPacket);
int8_t checkSystemHealth(mqd_t * pQueue, TaskStatusPacket *pPacket);

int main(void)
{
    pthread_t pThread[NUM_TEST_THREADS];
    char *heartbeatMsgQueueName = "/heartbeat_mq";
    char *logMsgQueueName        = "/logging_mq";
    char *sensorSharedMemoryName = "/sensor_sm";
    TaskStatusPacket recvThreadStatus;
    SensorThreadInfo sensorThreadInfo;
    int sharedMemSize = (sizeof(struct TempDataStruct) + sizeof(struct LightDataStruct));
    int sharedMemFd = 0;

    #ifdef INC_HB_QUEUE
        struct mq_attr mqAttr;
        mqd_t heartbeatMsgQueue;
    #endif

    /* Initialize created structs and packets to be 0-filled */
    memset(&recvThreadStatus, 0, sizeof(struct TaskStatusPacket));
    memset(&sensorThreadInfo, 0, sizeof(struct SensorThreadInfo));
    #ifdef INC_HB_QUEUE
        memset(&mqAttr,           0, sizeof(struct mq_attr));
    #endif

    /* Ensure MQs and Shared Memory properly cleaned up before starting */
    mq_unlink(logMsgQueueName);
    mq_unlink(heartbeatMsgQueueName);
    shm_unlink(sensorSharedMemoryName);

    if(remove(logMsgQueueName) == -1 && errno != ENOENT)
        ERROR_PRINT("ERROR: main() couldn't delete log queue path, err#%d (%s)\n\r", errno, strerror(errno));

    #ifdef INC_HB_QUEUE
        if(remove(heartbeatMsgQueueName) == -1 && errno != ENOENT)
            ERROR_PRINT("ERROR: main() couldn't delete log queue path, err#%d (%s)\n\r", errno, strerror(errno));

        /* Define MQ attributes */
        mqAttr.mq_flags   = 0;
        mqAttr.mq_maxmsg  = STATUS_MSG_QUEUE_DEPTH;
        mqAttr.mq_msgsize = STATUS_MSG_QUEUE_MSG_SIZE;
        mqAttr.mq_curmsgs = 0;

        /* Create MessageQueue to receive thread status from all children */
        heartbeatMsgQueue = mq_open(heartbeatMsgQueueName, O_CREAT | O_RDWR, 0666, &mqAttr);
        if(heartbeatMsgQueue == -1)
        { ERROR_PRINT("ERROR: main() failed to create MessageQueue for Main TaskStatus reception - exiting.\n"); return EXIT_FAILURE; }
    #endif


    /* Create Shared Memory for data sharing between SensorThreads and RemoteThread */
    sharedMemFd = shm_open(sensorSharedMemoryName, O_CREAT | O_RDWR | O_NONBLOCK, 0666);
    if(sharedMemFd == -1)
    { printf("ERROR: main() failed to create shared memory for sensor and remote threads - exiting.\n"); return EXIT_FAILURE; }

    /* Configure size of SensorSharedMemory */
    if(ftruncate(sharedMemFd, sharedMemSize) == -1)
    { printf("ERROR: main() failed to configure size of shared memory - exiting.\n"); return EXIT_FAILURE; }

    /* MemoryMap shared memory */
    if(*(int*)mmap(0, sharedMemSize, PROT_READ | PROT_WRITE, MAP_SHARED, sharedMemFd, 0) == -1)
    { printf("ERROR: main() failed to complete memory mapping for shared memory - exiting.\n"); return EXIT_FAILURE; }

    /* Populate ThreadInfo objects to pass names for created IPC pieces to threads */
    strcpy(sensorThreadInfo.heartbeatMsgQueueName, heartbeatMsgQueueName);
    strcpy(sensorThreadInfo.logMsgQueueName, logMsgQueueName);
    strcpy(sensorThreadInfo.sensorSharedMemoryName, sensorSharedMemoryName);
    sensorThreadInfo.sharedMemSize = sharedMemSize;
    sensorThreadInfo.lightDataOffset = sizeof(TempDataStruct);
    sensorThreadInfo.sharedMemMutex = &gSharedMemMutex;
    sensorThreadInfo.i2cBusMutex = &gI2cBusMutex;

    pthread_mutex_init(&gSharedMemMutex, NULL);
    pthread_mutex_init(&gI2cBusMutex, NULL);

    pthread_create(&pThread[0], NULL, tempSensorThreadHandler, &sensorThreadInfo);
    pthread_create(&pThread[1], NULL, remoteThread, &sensorThreadInfo);

    /* do stuff, wait for exit signal 
    (e.g. in this case, when count to expire) */
    uint8_t loopCount = 0, exit = 1, newError = 0;
    while((loopCount++ < 100) && (exit))
    {
        monitorHealth(&heartbeatMsgQueue, &exit, &newError);
        sleep(1);
    }

    /* trigger thread exit */
    gExitSig = 0;

    for(uint8_t ind; ind < NUM_TEST_THREADS; ++ind)
        pthread_join(pThread[ind], NULL);

    /* clean up */
    #ifdef INC_HB_QUEUE
        mq_close(heartbeatMsgQueue);
    #endif
    mq_unlink(heartbeatMsgQueueName);
    mq_unlink(logMsgQueueName);
    shm_unlink(sensorSharedMemoryName);
    pthread_mutex_destroy(&gSharedMemMutex);
    pthread_mutex_destroy(&gI2cBusMutex);
    close(sharedMemFd);

    INFO_PRINT("main exiting\n");
    return EXIT_SUCCESS;
}

static void *remoteThread(void *threadInfo)
{
    SensorThreadInfo sensorInfo = *(SensorThreadInfo *)threadInfo;
    #ifdef INC_HB_QUEUE
        mqd_t hbMsgQueue;
    #endif
    void* sharedMemPtr = NULL;
    int sharedMemFd;
    TempDataStruct tempData;  

    INFO_PRINT("remote tid: %d\n",(pid_t)syscall(SYS_gettid));

    #ifdef INC_HB_QUEUE
        /* Open FDs for Status Msg queues */
        hbMsgQueue = mq_open(sensorInfo.heartbeatMsgQueueName, O_RDWR, 0666, NULL);
        if(hbMsgQueue == -1) 
        { ERROR_PRINT("ERROR: remoteThread Failed to Open heartbeat MessageQueue - exiting.\n"); return NULL; }
    #endif

    /* Setup Shared memory for thread */
    sharedMemFd = shm_open(sensorInfo.sensorSharedMemoryName, O_RDWR, 0666);
    if(sharedMemFd == -1) 
    { ERROR_PRINT("ERROR: remoteThread Failed to Open heartbeat MessageQueue - exiting.\n"); return NULL; }

    /* Memory map the shared memory */
    sharedMemPtr = mmap(0, sensorInfo.sharedMemSize, PROT_READ | PROT_WRITE, MAP_SHARED, sharedMemFd, 0);
    if(*(int *)sharedMemPtr == -1) 
    { ERROR_PRINT("ERROR: remoteThread Failed to complete memory mapping of shared memory - exiting\n"); return NULL; }

    while(gExitSig) 
    {
        /* Read from Shared Memory and pass requested data back to client */
        pthread_mutex_lock(sensorInfo.sharedMemMutex);
        memcpy(&tempData, (sharedMemPtr+sensorInfo.tempDataOffset), sizeof(struct TempDataStruct));
        INFO_PRINT("tempC: %f\n",tempData.tmp102_temp);
        pthread_mutex_unlock(sensorInfo.sharedMemMutex);
        sleep(2);
    }

    /* Thread Cleanup */
    INFO_PRINT("remote thread exiting\n");
    #ifdef INC_HB_QUEUE
        mq_close(hbMsgQueue);
    #endif
    close(sharedMemFd);
    return NULL;
}

