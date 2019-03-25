/***********************************************************************************
 * @author Josh Malburg
 * joshua.malburg@colorado.edu
 * Advanced Embedded Software Development
 * ECEN5013 - Rick Heidebrecht
 * @date March 23, 2019
 * arm-linux-gnueabi (Buildroot)
 * gcc (Ubuntu)
 ************************************************************************************
 *
 * @file test_healthMonitor.c
 * @brief verify health monitor properly reads/processes the status msg queue and
 * signalling for thread terminating, etc
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
#include <signal.h>

#include "debug.h"
#include "healthMonitor.h"
#include "tempThread.h"
#include "lightThread.h"
#include "remoteThread.h"
#include "loggingThread.h"

#define HEALTH_MONITOR_DELAY    (CHILD_THREAD_DELAY * 2)
#define THREAD_COUNT            (4)
#define CHILD_THREAD_DELAY      (50e-3 *1e6)
#define TEST_MULTITHREAD

/* globals */
uint8_t gExitSig = 1;       /* set by health monitor */
int gExitLog = 1;
uint8_t gExitReal = 1;      /* used by main to stop thread, test only */
mqd_t heartbeatMsgQueue;

static void *healthMonitorThread(void *pArg);
#ifdef TEST_MULTITHREAD
static void *DUMMY_lightThread(void *pArg);
static void *DUMMY_tempThread(void *pArg);
static void *DUMMY_remoteThread(void *pArg);
static void *DUMMY_loggingThread(void *pArg);
#endif

/* test cases */
uint8_t testCount = 0;
int8_t test_noneCoA(void);
int8_t test_notifyCoA(void);
int8_t test_termThreadCoA(void);
int8_t test_termAllCoA(void);
int8_t test_timeout(void);

/**
 * @brief create all resources (threads, IPC, etc) and run test cases
 * 
 * @return int 
 */
int main(void)
{
    pthread_t pThread[5];
    char *heartbeatMsgQueueName = "/heartbeat_mq";
    struct mq_attr mqAttr;
    memset(&mqAttr, 0, sizeof(struct mq_attr));

    /* set signal handlers and actions */
	set_sig_handlers();

    /* Define MQ attributes */
    mqAttr.mq_flags   = 0;
    mqAttr.mq_maxmsg  = STATUS_MSG_QUEUE_DEPTH;
    mqAttr.mq_msgsize = STATUS_MSG_QUEUE_MSG_SIZE;
    mqAttr.mq_curmsgs = 0;

    /* Ensure MQs and Shared Memory properly cleaned up before starting */
    mq_unlink(heartbeatMsgQueueName);

    if(remove(heartbeatMsgQueueName) == -1 && errno != ENOENT)
    { ERRNO_PRINT("main() couldn't delete log queue path"); return EXIT_FAILURE; }

    /* Create MessageQueue to receive thread status from all children */
    heartbeatMsgQueue = mq_open(heartbeatMsgQueueName, O_CREAT | O_RDWR | O_NONBLOCK, 0666, &mqAttr);
    if(heartbeatMsgQueue == -1)
    { ERROR_PRINT("ERROR: main() failed to create MessageQueue for Main TaskStatus reception - exiting.\n"); return EXIT_FAILURE; }

    pthread_create(&pThread[0], NULL, healthMonitorThread, NULL);
    #ifdef TEST_MULTITHREAD
        pthread_create(&pThread[1], NULL, DUMMY_lightThread, NULL);
        pthread_create(&pThread[2], NULL, DUMMY_tempThread, NULL);
        pthread_create(&pThread[3], NULL, DUMMY_remoteThread, NULL);
        pthread_create(&pThread[4], NULL, DUMMY_loggingThread, NULL);
    #endif

    printf("test cases for health monitor\n");
	
	/* keep track of failures */
	uint8_t testFails = 0;

    /* test cases */
    testFails += test_noneCoA();
    testFails += test_notifyCoA();
    testFails += test_termThreadCoA();
    testFails += test_termAllCoA();
    testFails += test_timeout();

    printf("\n\nTEST RESULTS, %d of %d failed tests\n", testFails, testCount);

    gExitReal = 0;

    /* clean up */
    pthread_join(pThread[0], NULL);

    #ifdef TEST_MULTITHREAD
        pthread_join(pThread[1], NULL);
        pthread_join(pThread[2], NULL);
        pthread_join(pThread[3], NULL);
        pthread_join(pThread[4], NULL);
    #endif
    mq_close(heartbeatMsgQueue);
    mq_unlink(heartbeatMsgQueueName);
    return EXIT_SUCCESS;
}

/**
 * @brief verify all errors with CoA of NONE are reported by all thread types
 * 
 * @return int8_t test results
 */
int8_t test_noneCoA(void)
{
    uint8_t ind, indj;
    testCount++;
    
    for(indj = ERROR_CODE_USER_NONE0; indj < ERROR_CODE_USER_NOTIFY0; ++indj)
    {
        for(ind = 0; ind < PID_END; ++ind)
        {
            SEND_STATUS_MSG(heartbeatMsgQueue, (ProcessId_e)ind, STATUS_ERROR, (ErrorCode_e)indj);
            usleep(CHILD_THREAD_DELAY / 4);
        }
    }
    return EXIT_SUCCESS;
}
/**
 * @brief verify all errors with CoA of NOTIFY are reported by all thread types
 * 
 * @return int8_t test results
 */
int8_t test_notifyCoA(void)
{
    uint8_t ind, indj;
    testCount++;
    for(indj = ERROR_CODE_USER_NOTIFY0; indj < ERROR_CODE_USER_TERMTHREAD0; ++indj)
    {
        for(ind = 0; ind < PID_END; ++ind)
        {
            SEND_STATUS_MSG(heartbeatMsgQueue, (ProcessId_e)ind, STATUS_ERROR, (ErrorCode_e)indj);
            usleep(CHILD_THREAD_DELAY / 4);
        }
    }
    return EXIT_SUCCESS;
}
/**
 * @brief verify all errors with CoA of THERMTHREAD are reported by all thread types
 * 
 * @return int8_t test results
 */
int8_t test_termThreadCoA(void)
{
    uint8_t ind, indj;
    testCount++;
    for(indj = ERROR_CODE_USER_TERMTHREAD0; indj < ERROR_CODE_USER_TERMALL0; ++indj)
    {
        for(ind = 0; ind < PID_END; ++ind)
        {
            SEND_STATUS_MSG(heartbeatMsgQueue, (ProcessId_e)ind, STATUS_ERROR, (ErrorCode_e)indj);
            usleep(CHILD_THREAD_DELAY / 4);
        }
    }
    return EXIT_SUCCESS;
}
/**
 * @brief verify all errors with CoA of TERMALL are reported by all thread types
 * 
 * @return int8_t test results
 */
int8_t test_termAllCoA(void)
{
    uint8_t ind, indj;
    testCount++;
    for(indj = ERROR_CODE_USER_TERMALL0; indj <= ERROR_CODE_USER_TERMALL7; ++indj)
    {
        for(ind = 0; ind < PID_END; ++ind)
        {
            SEND_STATUS_MSG(heartbeatMsgQueue, (ProcessId_e)ind, STATUS_ERROR, (ErrorCode_e)indj);
            usleep(CHILD_THREAD_DELAY / 4);
        }
    }
    return EXIT_SUCCESS;
}
/**
 * @brief verify health monitor detects a dead thread
 * 
 * @return int8_t 
 */
int8_t test_timeout(void)
{
    uint8_t ind;
    testCount++;
    for(ind = 0; ind < PID_END; ++ind)
    {
        //SEND_STATUS_MSG(heartbeatMsgQueue, (ProcessId_e)ind, STATUS_ERROR, ERROR_CODE_TIMEOUT);
        sleep(2);
    }
    return EXIT_SUCCESS;
}
/**
 * @brief call the health monitor
 * 
 * @param threadInfo not used
 * @return void* not used
 */
static void *healthMonitorThread(void *threadInfo)
{
    uint8_t ind;

    /* block SIGRTs signals */
	sigset_t mask;
	sigemptyset(&mask);

    for(ind = 0; ind < THREAD_COUNT; ++ind)
        sigaddset(&mask, SIGRTMIN + ind);

    pthread_sigmask(SIG_BLOCK, &mask, NULL);

    while(gExitReal) 
    {
        INFO_PRINT("health monitor thread alive\n");
        monitorHealth(&heartbeatMsgQueue, &gExitSig);
        usleep(HEALTH_MONITOR_DELAY);
    }

    /* Thread Cleanup */
    INFO_PRINT("health monitor thread exiting\n");
    return NULL;
}

#ifdef TEST_MULTITHREAD
/* does nothing but handle signal */
void *DUMMY_lightThread(void *pArg)
{
    uint8_t ind;

    /* block SIGRTs signals */
	sigset_t mask;
	sigemptyset(&mask);

    for(ind = 0; ind < THREAD_COUNT; ++ind)
    {
        if(ind != (uint8_t)PID_LIGHT)
            sigaddset(&mask, SIGRTMIN + ind);
    }
    pthread_sigmask(SIG_BLOCK, &mask, NULL);

    while(gExitReal) 
    {
        printf("light thread alive\n");
        sleep(1);
    }
    return NULL;
}

/* does nothing but handle signal */
void *DUMMY_tempThread(void *pArg)
{
    uint8_t ind;

    /* block SIGRTs signals */
	sigset_t mask;
	sigemptyset(&mask);

    for(ind = 0; ind < THREAD_COUNT; ++ind)
    {
        if(ind != (uint8_t)PID_LIGHT)
            sigaddset(&mask, SIGRTMIN + ind);
    }
    pthread_sigmask(SIG_BLOCK, &mask, NULL);

    while(gExitReal) 
    {
        printf("temp thread alive\n");
        sleep(1);
    }
    return NULL;
}

/* does nothing but handle signal */
void *DUMMY_remoteThread(void *pArg)
{
    uint8_t ind;

    /* block SIGRTs signals */
	sigset_t mask;
	sigemptyset(&mask);

    for(ind = 0; ind < THREAD_COUNT; ++ind)
    {
        if(ind != (uint8_t)PID_LIGHT)
            sigaddset(&mask, SIGRTMIN + ind);
    }
    pthread_sigmask(SIG_BLOCK, &mask, NULL);

    while(gExitReal) 
    {
        printf("remote thread alive\n");
        sleep(1);
    }
    return NULL;
}

/* does nothing but handle signal */
void *DUMMY_loggingThread(void *pArg)
{
    uint8_t ind;

    /* block SIGRTs signals */
	sigset_t mask;
	sigemptyset(&mask);

    for(ind = 0; ind < THREAD_COUNT; ++ind)
    {
        if(ind != (uint8_t)PID_LIGHT)
            sigaddset(&mask, SIGRTMIN + ind);
    }
    pthread_sigmask(SIG_BLOCK, &mask, NULL);

    while(gExitReal) 
    {
        printf("logging thread alive\n");
        sleep(1);
    }
    return NULL;
}
#endif