/***********************************************************************************
 * @author Joshua Malburg
 * joshua.malburg@colorado.edu
 * Advanced Embedded Software Development
 * ECEN5013 - Rick Heidebrecht
 * @date March 7, 2019
 * arm-linux-gnueabi (Buildroot)
 * gcc (Ubuntu)
 ************************************************************************************
 *
 * @file tempSensor.h
 * @brief temp sensor library
 *
 ************************************************************************************
 */

#ifndef TEMP_SENSOR_H_
#define TEMP_SENSOR_H_

#include <stdint.h>

typedef enum 
{
    TMP102_CONV_RATE_0P25HZ,
    TMP102_CONV_RATE_1HZ,
    TMP102_CONV_RATE_4HZ,
    TMP102_CONV_RATE_8HZ
} Tmp102_ConvRate_e;

typedef enum
{
    TMP102_ADDR_MODE_NORMAL,
    TMP102_ADDR_MODE_EXTENDED
    
} Tmp102_AddrMode_e;

typedef enum
{
    TMP102_ALERT_ACTIVE,
    TMP102_ALERT_OFF
} Tmp102_Alert_e;

typedef enum
{
    TMP102_REQ_ONE_FAULT,
    TMP102_REQ_TWO_FAULT,
    TMP102_REQ_FOUR_FAULT,
    TMP102_REQ_SIX_FAULT,
} Tmp102_FaultCount_e;

typedef enum
{
    TMP102_DEVICE_IN_NORMAL,
    TMP102_DEVICE_IN_SHUTDOWN
} Tmp102_Shutdown_e;

int8_t tmp102_getTempC(uint8_t file, float *pTemp);
int8_t tmp102_getLowThreshold(uint8_t file, float *pLow);
int8_t tmp102_getHighThreshold(uint8_t file, float *pHigh);
int8_t tmp102_setLowThreshold(uint8_t file, float low);
int8_t tmp102_setHighThreshold(uint8_t file, float high);

int8_t tmp102_getFaultQueueSize(uint8_t file, Tmp102_FaultCount_e *pFaults);
int8_t tmp102_getExtendedMode(uint8_t file, Tmp102_AddrMode_e *pMode);
int8_t tmp102_getShutdownState(uint8_t file, Tmp102_Shutdown_e *pState);
int8_t tmp102_getAlert(uint8_t file, Tmp102_Alert_e *pAlert);
int8_t tmp102_getConvRate(uint8_t file, Tmp102_ConvRate_e *pConvRt);

int8_t tmp102_setExtendedMode(uint8_t file, Tmp102_AddrMode_e mode);
int8_t tmp102_setShutdownState(uint8_t file, Tmp102_Shutdown_e state);
int8_t tmp102_setConvRate(uint8_t file, uint8_t *pConvRt);

/* unit conversions */
#define TMP_DEGC_TO_DEGK(DEG)    (((float)(DEG)) + 273.15f)
#define TMP_DEGC_TO_DEGF(DEG)    ((((float)(DEG)) * (9.0f / 5.0f)) + 32.0f)
#define TMP_DEGF_TO_DEGC(DEG)    ((((float)(DEG)) - 32.0f) * (5.0f / 9.0f))
#define TMP_DEGF_TO_DEGK(DEG)    (TMP_DEGC_TO_DEGK(TMP_DEGF_TO_DEGC(DEG)))
#define TMP_DEGK_TO_DEGC(DEG)    (((float)(DEG)) - 273.15f)
#define TMP_DEGK_TO_DEGF(DEG)    (TMP_DEGC_TO_DEGF(TMP_DEGK_TO_DEGC(DEG)))

#endif /* TEMP_SENSOR_H_ */
