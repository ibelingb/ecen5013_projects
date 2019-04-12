/***********************************************************************************
 * @author Brian Ibeling and Joshua Malburg
 * Brian.Ibeling@colorado.edu, joshua.malburg@colorado.edu
 * Advanced Embedded Software Development
 * ECEN5013 - Rick Heidebrecht
 * @date April 12, 2019
 * CCS  Version: 8.3.0.00009
 ************************************************************************************
 *
 * tiva_packet.h
 *
 ************************************************************************************
 */

#ifndef TIVA_PACKET_H_
#define TIVA_PACKET_H_

/* TODO - merge w/ BBG packet.h, only need one */

// FreeRTOS includes
#include "FreeRTOS.h"
#include "queue.h"

//typedef struct {
//    ProcessId_e msgId;  /* message Id */
//    uint32_t time;      /* in msec */
//    float temp;         /* times 1000 */
//    uint32_t count;     /* toggle count */
//    uint8_t name[8];    /* author's name */
//} StatusPacket_t;

typedef struct {
    QueueHandle_t logFd;
    uint32_t sysClock;
    TickType_t xStartTime;
} ThreadInfo_t;

typedef struct {
    uint16_t highThreshold;
    uint16_t lowThreshold;
    uint16_t moistureLevel;
} MoistureDataStruct;

typedef struct {
    uint8_t cmd;
    uint8_t state;
    uint16_t remainingOnTime;
} SolenoidDataStruct;

#endif /* TIVA_PACKET_H_ */
