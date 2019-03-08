/* temp sensor library */

#include "temp.h"
#include "lu_iic.h"
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>

#define TMP102_ADDR			(0x48)

#define TMP102_TEMP_REG		(0x00)
#define TMP102_CONFIG_REG	(0x01)
#define TMP102_TLOW_REG		(0x02)
#define TMP102_THIGH_REG	(0x03)

int8_t tmp102_getTemp(uint8_t file, uint8_t *pTemp)
{	
	if(pTemp == NULL)
		return EXIT_FAILURE;

	getIicRegister(file, TMP102_ADDR, TMP102_TEMP_REG, pTemp);
	return EXIT_SUCCESS;
}

int8_t tmp102_getLowThreshold(uint8_t file, uint8_t *pLow)
{
	if(pLow == NULL)
		return EXIT_FAILURE;

	getIicRegister(file, TMP102_ADDR, TMP102_TLOW_REG, pLow);
	return EXIT_SUCCESS;
}

int8_t tmp102_getHighThreshold(uint8_t file, uint8_t *pHigh)
{
	if(pHigh == NULL)
		return EXIT_FAILURE;

	getIicRegister(file, TMP102_ADDR, TMP102_THIGH_REG, pHigh);
	return EXIT_SUCCESS;
}

int8_t tmp102_getConfiguration(uint8_t file, uint8_t *pConfig)
{
	if(pConfig == NULL)
		return EXIT_FAILURE;

	getIicRegister(file, TMP102_ADDR, TMP102_CONFIG_REG, pConfig);
	return EXIT_SUCCESS;
}

int8_t tmp102_getTempResolution(uint8_t file, uint8_t *pRes)
{
	if(pRes == NULL)
		return EXIT_FAILURE;
	
	return EXIT_SUCCESS;
}

int8_t tmp102_getFault(uint8_t file, uint8_t *pFaults)
{
	if(pFaults == NULL)
		return EXIT_FAILURE;
	
	return EXIT_SUCCESS;
}

int8_t tmp102_getExtendedMode(uint8_t file, uint8_t *pEM)
{
	if(pEM == NULL)
		return EXIT_FAILURE;
	
	return EXIT_SUCCESS;
}

int8_t tmp102_getShutdownMode(uint8_t file, uint8_t *pBit)
{
	if(pBit == NULL)
		return EXIT_FAILURE;
	
	return EXIT_SUCCESS;
}

int8_t tmp102_getAlert(uint8_t file, uint8_t *pAlert)
{
	if(pAlert == NULL)
		return EXIT_FAILURE;
	
	return EXIT_SUCCESS;
}

int8_t tmp102_getConvRate(uint8_t file, uint8_t *pConvRt)
{
	if(pConvRt == NULL)
		return EXIT_FAILURE;
	
	return EXIT_SUCCESS;
}

int8_t tmp102_setLowThreshold(uint8_t file, uint8_t low)
{
	setIicRegister(file, TMP102_ADDR, TMP102_TLOW_REG, low);
	return EXIT_SUCCESS;
}

int8_t tmp102_setHighThreshold(uint8_t file, uint8_t high)
{
	setIicRegister(file, TMP102_ADDR, TMP102_THIGH_REG, high);
	return EXIT_SUCCESS;
}

int8_t tmp102_setConfiguration(uint8_t file, uint8_t config)
{
	setIicRegister(file, TMP102_ADDR, TMP102_CONFIG_REG, config);
	return EXIT_SUCCESS;
}

int8_t tmp102_setTempResolution(uint8_t file, uint8_t res)
{
	return EXIT_SUCCESS;
}

int8_t tmp102_setExtendedMode(uint8_t file, uint8_t *pEM)
{
	return EXIT_SUCCESS;
}

int8_t tmp102_setShutdownMode(uint8_t file, uint8_t bit)
{
	return EXIT_SUCCESS;
}

int8_t tmp102_setConvRate(uint8_t file, uint8_t *pConvRt)
{
	return EXIT_SUCCESS;
}