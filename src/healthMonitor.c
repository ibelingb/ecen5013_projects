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
 * @file healthMonitor.c
 * @brief check thread status / state and take appropriate course of action
 *
 ************************************************************************************
 */

#include <stdint.h>
#include <mqueue.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <syscall.h>

#include "debug.h"
#include "packet.h"
#include "platform.h"

#include "loggingThread.h"
#include "tempThread.h"
#include "lightThread.h"
#include "remoteThread.h"
#include "healthMonitor.h"

#define THREAD_MISSING_COUNT        (3)
#define STATUS_MSG_PROCESS_LIMIT    (NUM_THREADS * 20 + 1)   /* 2 is because main is 1/2 as fast as other loops */

/* private functions */
int8_t getStatusMsg(mqd_t *pQueue, TaskStatusPacket *pPacket);
MainAction_e callArbitor(TaskStatusPacket *pStatus);
MainAction_e lightErrorArbitor(TaskStatusPacket *pStatus);
MainAction_e tempErrorArbitor(TaskStatusPacket *pStatus);
MainAction_e remoteErrorArbitor(TaskStatusPacket *pStatus);
MainAction_e loggingErrorArbitor(TaskStatusPacket *pStatus);
void processAction(ProcessId_e processId, MainAction_e action, uint8_t *pExit);
int8_t threadKiller(ProcessId_e Id);

void set_sig_handlers(void)
{
    struct sigaction action;

    action.sa_flags = SA_SIGINFO;
    
    action.sa_sigaction = lightSigHandler;
    if (sigaction(SIGRTMIN + (uint8_t)PID_LIGHT, &action, NULL) == -1) {
        perror("sigusr: sigaction");
        _exit(1);
    }
    action.sa_sigaction = tempSigHandler;
    if (sigaction(SIGRTMIN + (uint8_t)PID_TEMP, &action, NULL) == -1) {
        perror("sigusr: sigaction");
        _exit(1);
    }
    action.sa_sigaction = remoteSigHandler;
    if (sigaction(SIGRTMIN + (uint8_t)PID_REMOTE, &action, NULL) == -1) {
        perror("sigusr: sigaction");
        _exit(1);
    }
    action.sa_sigaction = loggingSigHandler;
    if (sigaction(SIGRTMIN + (uint8_t)PID_LOGGING, &action, NULL) == -1) {
        perror("sigusr: sigaction");
        _exit(1);
    }
}

int8_t monitorHealth(mqd_t * pQueue, uint8_t *pExit, uint8_t *newError)
{
    TaskStatusPacket status;
    MainAction_e action;
    static uint8_t threadMissingCount[NUM_THREADS + 1];     /* 1 is to provide slot for PID_END enum value */
    uint8_t missingFlag[NUM_THREADS + 1];                   /* 1 is to provide slot for PID_END enum value */
    uint8_t ind, msgCount, prevErrorCount;
    static uint8_t errorCount = 0;
    prevErrorCount = errorCount;

    if((pQueue == NULL) || (pExit == NULL)) {
        return EXIT_FAILURE;
    }

    /* determine state of threads (alive, zombie, etc) */
    for(ind = 0; ind < NUM_THREADS; ++ind) {
        /* TODO - get thread status */
    }

    /* set missing flags before looping on status queue;
     * there should be at least one message from each thread.
     * if a msg is received, this flag will get set to zero,
     * else this flag will increment the missing count by one */
    for(ind = 0; ind < NUM_THREADS; ++ind) {
        missingFlag[ind] = 1;
    }

    /* cycle through status messages */
    msgCount = 0;
    do
    {
        memset(&status, 0, sizeof(struct TaskStatusPacket));
        
        if(getStatusMsg(pQueue, &status) == EXIT_SUCCESS)
        {
            /* clear recvFlag so missing count doesn't increment */
            missingFlag[status.processId] = 0;             
            threadMissingCount[status.processId] = 0;

            /* determine error course of action */
            if(status.processId != PID_END)
            {
                ++msgCount;
                if(status.taskStatus == STATUS_ERROR)
                {
                    action = callArbitor(&status);
                    processAction(status.processId, action, pExit);
                    ++errorCount;
                }
            }
            if(msgCount > STATUS_MSG_PROCESS_LIMIT)
            {
                ERROR_PRINT("Stopped processing status msgs, some how health monitor got behind\n");
                break;
            }
            if(*pExit == 0) {
                INFO_PRINT("health monitor exiting, pExit: %d\n", *pExit);
                if(prevErrorCount != errorCount) {
                    *newError = 1;
                }
                return EXIT_SUCCESS;
            }
                
        }
    } while (status.processId != PID_END);

    /* update missingCount */
    for(ind = 0; ind < NUM_THREADS; ++ind) {
        threadMissingCount[ind] += missingFlag[ind];

        /* if missing exceeds limit, set error status to timeout and call arbitor again */
        if(threadMissingCount[ind] >= THREAD_MISSING_COUNT)
        {
            threadMissingCount[ind] = THREAD_MISSING_COUNT;
            status.processId = ind;
            status.errorCode = (uint8_t)ERROR_CODE_TIMEOUT;
            status.timestamp = log_get_time();
            action = callArbitor(&status);
            processAction(status.processId, action, pExit);
        }
    }
    MUTED_PRINT("health monitor exiting, pExit: %d\n", *pExit);
    if(prevErrorCount != errorCount) {
        *newError = 1;
    }
    return EXIT_SUCCESS;
}
/**
 * @brief perform action on process, clear exit flag if necessary
 * 
 * @param processId recipient of action
 * @param action action to perform
 * @param pExit pointer to exit flag
 */
void processAction(ProcessId_e processId, MainAction_e action, uint8_t *pExit)
{
    switch(action) {
        case MAIN_ACTION_TERMINATE_ALL:
        *pExit = 0;
        threadKiller(PID_END);
        break;
        case MAIN_ACTION_TERMINATE_THREAD:
        threadKiller(processId);
        break;
        case MAIN_ACTION_RESTART_THREAD:
        /* place holder, could do something here */
        default:
        break;
    }
}
/**
 * @brief set signal to cause 'Id' thread to exit
 * 
 * @param Id thread to kill
 * @return int8_t result of attempted kill
 */
int8_t threadKiller(ProcessId_e Id)
{
    union sigval sigargs;
    int8_t ind;

    sigargs.sival_int = 1;

    /* set signal(s) used to kill other threads */
    if(Id == PID_END){
        MUTED_PRINT("main says kill all\n");
        for(ind = 0; ind < PID_END; ++ind)
            sigqueue(getpid(), SIGRTMIN + ind, sigargs);
        
    }
    else {
        /* kill some thread */
        MUTED_PRINT("main says kill id#%d\n", Id);
        sigqueue(getpid(), SIGRTMIN + Id, sigargs);
    }
    return EXIT_SUCCESS;
}
/**
 * @brief get one status msg from queue
 * 
 * @param pQueue pointer to status queue
 * @param pPacket pointer to return element
 * @return int8_t results of get operation
 */
int8_t getStatusMsg(mqd_t *pQueue, TaskStatusPacket *pPacket)
{
    int bytesRead;
    TaskStatusPacket tmp;
    memset(&tmp, 0, sizeof(struct TaskStatusPacket));

    /* read a message from queue */
    bytesRead = mq_receive(*pQueue, (char *)&tmp, sizeof(struct TaskStatusPacket), NULL);

    /* check for read error */
    if(bytesRead == -1) {
        if(errno == EAGAIN) { 
            tmp.processId = PID_END;    /* no msgs, return */
            *pPacket = tmp;
            return EXIT_SUCCESS;
        }
        else { 
            tmp.processId = PID_END;    /* no msgs, return */
            *pPacket = tmp;
            ERRNO_PRINT("after read from mq_receive"); 
            return EXIT_FAILURE;
        }
    }

    /* ignore partial msgs */
    if(bytesRead == sizeof(TaskStatusPacket)) {    	        
        *pPacket = tmp;
        if(tmp.processId > PID_END)
        {
            tmp.processId = PID_END;
            return EXIT_FAILURE;
        }
        return EXIT_SUCCESS;
    }

    /* shouldn't get here */
    tmp.processId = PID_END;
    *pPacket = tmp;
    ERROR_PRINT("shouldn't have gotten here\n");
    return EXIT_SUCCESS;
}
/**
 * @brief call the appropriate arbitor based on status
 * 
 * @param processId thread Id
 * @param errorCode error of thread Id
 * @return MainAction_e action to perform
 */
MainAction_e callArbitor(TaskStatusPacket *pStatus)
{
    MainAction_e action;
    PRINT_STATUS_MSG(pStatus);

    switch(pStatus->processId){
        case PID_LIGHT:
            action = lightErrorArbitor(pStatus);
        break;
        case PID_TEMP:
            action = tempErrorArbitor(pStatus);
        break;
        case PID_REMOTE:
            action = remoteErrorArbitor(pStatus);
        break;
        case PID_LOGGING:
            action = loggingErrorArbitor(pStatus);
        break;
        default:
            action = MAIN_ACTION_NONE;
        break;
    }

    return action;
}
/**
 * @brief determine course of action of error for light thread
 * 
 * @param errorCode 
 * @return MainAction_e 
 */
MainAction_e lightErrorArbitor(TaskStatusPacket *pStatus)
{
    MainAction_e action;

    switch(pStatus->errorCode){
        /* do nothing */
        case ERROR_CODE_USER_NONE0:
        case ERROR_CODE_USER_NONE1:
        case ERROR_CODE_USER_NONE2:
        case ERROR_CODE_USER_NONE3:
        case ERROR_CODE_USER_NONE4:
        case ERROR_CODE_USER_NONE5:
        case ERROR_CODE_USER_NONE6:
        case ERROR_CODE_USER_NONE7:
            action = MAIN_ACTION_NONE;
            break;

        /* just notify */
        case ERROR_CODE_TIMEOUT:
        case ERROR_CODE_USER_NOTIFY0:
        case ERROR_CODE_USER_NOTIFY1:
        case ERROR_CODE_USER_NOTIFY2:
        case ERROR_CODE_USER_NOTIFY3:
        case ERROR_CODE_USER_NOTIFY4:
        case ERROR_CODE_USER_NOTIFY5:
        case ERROR_CODE_USER_NOTIFY6:
        case ERROR_CODE_USER_NOTIFY7:
            action = MAIN_ACTION_NOTIFY_USER_ONLY;
            break;       

        /* kill thread */
        case ERROR_CODE_USER_TERMTHREAD0:
        case ERROR_CODE_USER_TERMTHREAD1:
        case ERROR_CODE_USER_TERMTHREAD2:
        case ERROR_CODE_USER_TERMTHREAD3:
        case ERROR_CODE_USER_TERMTHREAD4:
        case ERROR_CODE_USER_TERMTHREAD5:
        case ERROR_CODE_USER_TERMTHREAD6:
        case ERROR_CODE_USER_TERMTHREAD7:
            action = MAIN_ACTION_TERMINATE_THREAD;
            break;

        /* kill all */
        case ERROR_CODE_USER_TERMALL0:
        case ERROR_CODE_USER_TERMALL1:
        case ERROR_CODE_USER_TERMALL2:
        case ERROR_CODE_USER_TERMALL3:
        case ERROR_CODE_USER_TERMALL4:
        case ERROR_CODE_USER_TERMALL5:
        case ERROR_CODE_USER_TERMALL6:
        case ERROR_CODE_USER_TERMALL7:
        default:
            action = MAIN_ACTION_TERMINATE_ALL;
            break;
    }
    return action;
}
/**
 * @brief determine course of action of error for temp thread
 * 
 * @param errorCode 
 * @return MainAction_e 
 */
MainAction_e tempErrorArbitor(TaskStatusPacket *pStatus)
{
    MainAction_e action;

    switch(pStatus->errorCode){
        /* do nothing */
        case ERROR_CODE_USER_NONE0:
        case ERROR_CODE_USER_NONE1:
        case ERROR_CODE_USER_NONE2:
        case ERROR_CODE_USER_NONE3:
        case ERROR_CODE_USER_NONE4:
        case ERROR_CODE_USER_NONE5:
        case ERROR_CODE_USER_NONE6:
        case ERROR_CODE_USER_NONE7:
            action = MAIN_ACTION_NONE;
            break;

        /* just notify */
        case ERROR_CODE_TIMEOUT:
        case ERROR_CODE_USER_NOTIFY0:
        case ERROR_CODE_USER_NOTIFY1:
        case ERROR_CODE_USER_NOTIFY2:
        case ERROR_CODE_USER_NOTIFY3:
        case ERROR_CODE_USER_NOTIFY4:
        case ERROR_CODE_USER_NOTIFY5:
        case ERROR_CODE_USER_NOTIFY6:
        case ERROR_CODE_USER_NOTIFY7:
            action = MAIN_ACTION_NOTIFY_USER_ONLY;
            break;       

        /* kill thread */
        case ERROR_CODE_USER_TERMTHREAD0:
        case ERROR_CODE_USER_TERMTHREAD1:
        case ERROR_CODE_USER_TERMTHREAD2:
        case ERROR_CODE_USER_TERMTHREAD3:
        case ERROR_CODE_USER_TERMTHREAD4:
        case ERROR_CODE_USER_TERMTHREAD5:
        case ERROR_CODE_USER_TERMTHREAD6:
        case ERROR_CODE_USER_TERMTHREAD7:
            action = MAIN_ACTION_TERMINATE_THREAD;
            break;

        /* kill all */
        case ERROR_CODE_USER_TERMALL0:
        case ERROR_CODE_USER_TERMALL1:
        case ERROR_CODE_USER_TERMALL2:
        case ERROR_CODE_USER_TERMALL3:
        case ERROR_CODE_USER_TERMALL4:
        case ERROR_CODE_USER_TERMALL5:
        case ERROR_CODE_USER_TERMALL6:
        case ERROR_CODE_USER_TERMALL7:
        default:
            action = MAIN_ACTION_TERMINATE_ALL;
            break;
    }
    return action;
}
/**
 * @brief determine course of action of error for remote thread
 * 
 * @param errorCode 
 * @return MainAction_e 
 */
MainAction_e remoteErrorArbitor(TaskStatusPacket *pStatus)
{
    MainAction_e action;

    switch(pStatus->errorCode){
        /* do nothing */
        case ERROR_CODE_USER_NONE0:
        case ERROR_CODE_USER_NONE1:
        case ERROR_CODE_USER_NONE2:
        case ERROR_CODE_USER_NONE3:
        case ERROR_CODE_USER_NONE4:
        case ERROR_CODE_USER_NONE5:
        case ERROR_CODE_USER_NONE6:
        case ERROR_CODE_USER_NONE7:
            action = MAIN_ACTION_NONE;
            break;

        /* just notify */
        case ERROR_CODE_TIMEOUT:
        case ERROR_CODE_USER_NOTIFY0:
        case ERROR_CODE_USER_NOTIFY1:
        case ERROR_CODE_USER_NOTIFY2:
        case ERROR_CODE_USER_NOTIFY3:
        case ERROR_CODE_USER_NOTIFY4:
        case ERROR_CODE_USER_NOTIFY5:
        case ERROR_CODE_USER_NOTIFY6:
        case ERROR_CODE_USER_NOTIFY7:
            action = MAIN_ACTION_NOTIFY_USER_ONLY;
            break;       

        /* kill thread */
        case ERROR_CODE_USER_TERMTHREAD0:
        case ERROR_CODE_USER_TERMTHREAD1:
        case ERROR_CODE_USER_TERMTHREAD2:
        case ERROR_CODE_USER_TERMTHREAD3:
        case ERROR_CODE_USER_TERMTHREAD4:
        case ERROR_CODE_USER_TERMTHREAD5:
        case ERROR_CODE_USER_TERMTHREAD6:
        case ERROR_CODE_USER_TERMTHREAD7:
            action = MAIN_ACTION_TERMINATE_THREAD;
            break;

        /* kill all */
        case ERROR_CODE_USER_TERMALL0:
        case ERROR_CODE_USER_TERMALL1:
        case ERROR_CODE_USER_TERMALL2:
        case ERROR_CODE_USER_TERMALL3:
        case ERROR_CODE_USER_TERMALL4:
        case ERROR_CODE_USER_TERMALL5:
        case ERROR_CODE_USER_TERMALL6:
        case ERROR_CODE_USER_TERMALL7:
        default:
            action = MAIN_ACTION_TERMINATE_ALL;
            break;
    }
    return action;
}
/**
 * @brief determine course of action of error for logging thread
 * 
 * @param errorCode 
 * @return MainAction_e 
 */
MainAction_e loggingErrorArbitor(TaskStatusPacket *pStatus)
{
    MainAction_e action;

    switch(pStatus->errorCode){
        /* do nothing */
        case ERROR_CODE_USER_NONE0:
        case ERROR_CODE_USER_NONE1:
        case ERROR_CODE_USER_NONE2:
        case ERROR_CODE_USER_NONE3:
        case ERROR_CODE_USER_NONE4:
        case ERROR_CODE_USER_NONE5:
        case ERROR_CODE_USER_NONE6:
        case ERROR_CODE_USER_NONE7:
            action = MAIN_ACTION_NONE;
            break;

        /* just notify */
        case ERROR_CODE_TIMEOUT:
        case ERROR_CODE_USER_NOTIFY0:
        case ERROR_CODE_USER_NOTIFY1:
        case ERROR_CODE_USER_NOTIFY2:
        case ERROR_CODE_USER_NOTIFY3:
        case ERROR_CODE_USER_NOTIFY4:
        case ERROR_CODE_USER_NOTIFY5:
        case ERROR_CODE_USER_NOTIFY6:
        case ERROR_CODE_USER_NOTIFY7:
            action = MAIN_ACTION_NOTIFY_USER_ONLY;
            break;       

        /* kill thread */
        case ERROR_CODE_USER_TERMTHREAD0:
        case ERROR_CODE_USER_TERMTHREAD1:
        case ERROR_CODE_USER_TERMTHREAD2:
        case ERROR_CODE_USER_TERMTHREAD3:
        case ERROR_CODE_USER_TERMTHREAD4:
        case ERROR_CODE_USER_TERMTHREAD5:
        case ERROR_CODE_USER_TERMTHREAD6:
        case ERROR_CODE_USER_TERMTHREAD7:
            action = MAIN_ACTION_TERMINATE_THREAD;
            break;

        /* kill all */
        case ERROR_CODE_USER_TERMALL0:
        case ERROR_CODE_USER_TERMALL1:
        case ERROR_CODE_USER_TERMALL2:
        case ERROR_CODE_USER_TERMALL3:
        case ERROR_CODE_USER_TERMALL4:
        case ERROR_CODE_USER_TERMALL5:
        case ERROR_CODE_USER_TERMALL6:
        case ERROR_CODE_USER_TERMALL7:
        default:
            action = MAIN_ACTION_TERMINATE_ALL;
            break;
    }
    return action;
}
