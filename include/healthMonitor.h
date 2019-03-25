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
 * @file healthMonitor.h
 * @brief check thread status / state and take appropriate course of action
 *
 ************************************************************************************
 */

#ifndef HEALTHMONITOR_H_
#define HEALTHMONITOR_H_

#include <mqueue.h>
#include <stdint.h>
#include <syscall.h>

#include "packet.h"
#include "logger_helper.h"

//#define PRINT_NONE_ALSO             // for testing
//#define PRINT_ALL_STATUS_FIELDS     // for testing

/**
 * @brief check thread status / state and take appropriate course of action
 * 
 * @param pQueue pointer to status message queue
 * @param pExit indication to main that its time to terminate loop
 * @return int8_t status of call
 */
int8_t monitorHealth(mqd_t * pQueue, uint8_t *pExit);

/**
 * @brief register signal handlers used by health monitor to terminate threads 
 * 
 */
void set_sig_handlers(void);

#define SEND_STATUS_MSG(queue, Id, taskStat, errCode)({\
	TaskStatusPacket status;\
	status.timestamp = log_get_time();\
	status.header = ((pid_t)syscall(SYS_gettid));\
    status.taskStatus = taskStat;\
    status.errorCode = errCode;\
    status.processId = Id;\
    status.taskState = STATE_RUNNING;\
    if(mq_send(queue, (char *)&status, sizeof(TaskStatusPacket), 7) < 0)\
	{ ERRNO_PRINT("SEND_STATUS_MSG fail"); }\
})

__attribute__((always_inline)) inline const char *getPidString(ProcessId_e procId)
{
    switch (procId) {
        case PID_LIGHT:
            return "PID_LIGHT";
        break;
        case PID_TEMP:
            return "PID_TEMP";
        break;
        case PID_REMOTE:
            return "PID_REMOTE";
        break;
        case PID_LOGGING:
            return "PID_LOGGING";
        break;
        default:
            return "PID_END";
        break;
    }
}

__attribute__((always_inline)) inline void PRINT_STATUS_MSG_HEADER(TaskStatusPacket *pStatus)
{
    if((pStatus->errorCode >= ERROR_CODE_USER_NOTIFY0) && (pStatus->errorCode <= ERROR_CODE_USER_NOTIFY7))
        printf("recvd error(%d) w/ CoA = NOTIFY from %s at %d usec\n", pStatus->errorCode, 
        getPidString(pStatus->processId), pStatus->timestamp);
    else if((pStatus->errorCode >= ERROR_CODE_USER_TERMTHREAD0) && (pStatus->errorCode <= ERROR_CODE_USER_TERMTHREAD7))
        printf("recvd error(%d) w/ CoA = TERMTHREAD from %s at %d usec\n", pStatus->errorCode, 
        getPidString(pStatus->processId), pStatus->timestamp);
    else if((pStatus->errorCode >= ERROR_CODE_USER_TERMALL0) && (pStatus->errorCode <= ERROR_CODE_USER_TERMALL7))
        printf("recvd error(%d) w/ CoA = TERMALL from %s at %d usec\n", pStatus->errorCode, 
        getPidString(pStatus->processId), pStatus->timestamp);
    else if((pStatus->errorCode >= ERROR_CODE_USER_RESTARTTHREAD0) && (pStatus->errorCode <= ERROR_CODE_USER_RESTARTTHREAD7))
        printf("recvd error(%d) w/ CoA = RESTARTTHREAD from %s at %d usec\n", pStatus->errorCode, 
        getPidString(pStatus->processId), pStatus->timestamp);
    else if((pStatus->errorCode >= ERROR_CODE_USER_NONE0) && (pStatus->errorCode <= ERROR_CODE_USER_NONE7))
    #ifdef PRINT_NONE_ALSO
        printf("recvd error(%d) w/ CoA = NONE from %s at %d usec\n", pStatus->errorCode, 
        getPidString(pStatus->processId), pStatus->timestamp);
    #else
        printf(" ");
    #endif
    else if (pStatus->errorCode == ERROR_CODE_TIMEOUT)
        printf("recvd TIMEOUT error(%d) from %s at %d usec\n", pStatus->errorCode, getPidString(pStatus->processId), pStatus->timestamp);
    else
        printf("recvd error(%d) w/ CoA = OTHER from %s at %d usec\n", pStatus->errorCode, getPidString(pStatus->processId), pStatus->timestamp);

}

__attribute__((always_inline)) inline void PRINT_STATUS_MSG(TaskStatusPacket *pStatus)
{
    PRINT_STATUS_MSG_HEADER(pStatus);
    #ifdef PRINT_ALL_STATUS_FIELDS
    printf("header(%d)\t", pStatus->header);
    printf("timestamp(%d)\t", pStatus->timestamp);
    printf("processId(%d)\t", pStatus->processId);
    printf("taskState(%d)\t", pStatus->taskState);
    printf("taskStatus(%d)\t", pStatus->taskStatus);
    printf("errorCode(%d)\n", pStatus->errorCode);
    #endif
}
#endif /* HEALTHMONITOR_H_ */
