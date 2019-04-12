/***********************************************************************************
 * @author Brian Ibeling and Joshua Malburg
 * Brian.Ibeling@colorado.edu, joshua.malburg@colorado.edu
 * Advanced Embedded Software Development
 * ECEN5013 - Rick Heidebrecht
 * @date April 12, 2019
 * CCS  Version: 8.3.0.00009
 ************************************************************************************
 *
 * packet.h
 *
 ************************************************************************************
 */

#ifndef PACKET_H_
#define PACKET_H_

// FreeRTOS includes
#include "FreeRTOS.h"
#include "queue.h"

#define MSG_ID_TEMP             (0)
#define MSG_ID_LED              (1)

typedef struct {
    uint8_t msgId;      /* message Id */
    uint32_t time;      /* in msec */
    float temp;         /* times 1000 */
    uint32_t count;     /* toggle count */
    uint8_t name[8];    /* author's name */
} LogPacket_t;

typedef struct {
    QueueHandle_t logFd;
    uint32_t sysClock;
    TickType_t xStartTime;
} ThreadInfo_t;

#endif /* PACKET_H_ */
