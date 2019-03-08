/***********************************************************************************
 * @author Joshua Malburg (joma0364)
 * joshua.malburg@colorado.edu
 * Advanced Embedded Software Development
 * ECEN5013 - Rick Feidebrecht
 * @date March 7, 2019
 * arm-linux-gnueabi (Buildroot)
 * gcc (Ubuntu)
 ************************************************************************************
 *
 * @file temp.h
 * @brief temp sensor library
 *
 ************************************************************************************
 */

#ifndef TEMP_H_
#define TEMP_H_

#include <stdint.h>

int8_t tmp102_getTemp(uint8_t file, uint8_t *pTemp);
int8_t tmp102_getLowThreshold(uint8_t file, uint8_t *pLow);
int8_t tmp102_getHighThreshold(uint8_t file, uint8_t *pHigh);

int8_t tmp102_getConfiguration(uint8_t file, uint8_t *pConfig);
int8_t tmp102_getTempResolution(uint8_t file, uint8_t *res);
int8_t tmp102_getFault(uint8_t file, uint8_t *pFaults);
int8_t tmp102_getExtendedMode(uint8_t file, uint8_t *pEM);
int8_t tmp102_getShutdownMode(uint8_t file, uint8_t *bit);
int8_t tmp102_getAlert(uint8_t file, uint8_t *pAlert);
int8_t tmp102_getConvRate(uint8_t file, uint8_t *pConvRt);

int8_t tmp102_setLowThreshold(uint8_t file, uint8_t low);
int8_t tmp102_setHighThreshold(uint8_t file, uint8_t high);

int8_t tmp102_setConfiguration(uint8_t file, uint8_t config);
int8_t tmp102_setTempResolution(uint8_t file, uint8_t res);
int8_t tmp102_setExtendedMode(uint8_t file, uint8_t *pEM);
int8_t tmp102_setShutdownMode(uint8_t file, uint8_t bit);
int8_t tmp102_setConvRate(uint8_t file, uint8_t *pConvRt);

#endif /* TEMP_H_ */