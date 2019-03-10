/***********************************************************************************
 * @author Joshua Malburg (joma0364)
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

int8_t tmp102_getTempC(uint8_t file, float *pTemp);
int8_t tmp102_getLowThreshold(uint8_t file, float *pLow);
int8_t tmp102_getHighThreshold(uint8_t file, float *pHigh);
int8_t tmp102_setLowThreshold(uint8_t file, float low);
int8_t tmp102_setHighThreshold(uint8_t file, float high);

int8_t tmp102_getTempResolution(uint8_t file, uint8_t *res);
int8_t tmp102_getFault(uint8_t file, uint8_t *pFaults);
int8_t tmp102_getExtendedMode(uint8_t file, uint8_t *pEM);
int8_t tmp102_getShutdownMode(uint8_t file, uint8_t *bit);
int8_t tmp102_getAlert(uint8_t file, uint8_t *pAlert);
int8_t tmp102_getConvRate(uint8_t file, uint8_t *pConvRt);

int8_t tmp102_setConfiguration(uint8_t file, uint16_t config);
int8_t tmp102_setTempResolution(uint8_t file, uint8_t res);
int8_t tmp102_setExtendedMode(uint8_t file, uint8_t *pEM);
int8_t tmp102_setShutdownMode(uint8_t file, uint8_t bit);
int8_t tmp102_setConvRate(uint8_t file, uint8_t *pConvRt);

#endif /* TEMP_SENSOR_H_ */
