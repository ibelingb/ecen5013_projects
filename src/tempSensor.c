/* temp sensor library */

#include "tempSensor.h"
#include "lu_iic.h"
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>

#define TMP102_ADDR				(0x48)

#define TMP102_TEMP_REG			(0x00)
#define TMP102_CONFIG_REG		(0x01)
#define TMP102_TLOW_REG			(0x02)
#define TMP102_THIGH_REG		(0x03)

#define TMP102_REG_SIZE			(2)
#define TMP102_ENDIANNESS		(TMP102_LITTLE_ENDIAN)
#define TMP102_LITTLE_ENDIAN	(0)
#define TMP102_TEMP_SCALEFACTOR	(0.0625f)
#define TMP102_TEMP_START_BIT	(3)

int8_t tmp102_getTemp(uint8_t file, float *pTemp)
{
	if(pTemp == NULL)
		return EXIT_FAILURE;

	uint32_t tmp;
	if(EXIT_FAILURE == getIicRegister(file, TMP102_ADDR, TMP102_TEMP_REG, &tmp, TMP102_REG_SIZE, TMP102_ENDIANNESS))
		return EXIT_FAILURE;

	/* if extended mode, only shift 2-bits */
	uint8_t shiftValue = TMP102_TEMP_START_BIT;
	if(tmp % 2 == 1)
	{
		--shiftValue;
	}

	*pTemp = ((float)((0xFFFF & tmp) >> shiftValue)) * TMP102_TEMP_SCALEFACTOR;
	return EXIT_SUCCESS;
}

int8_t tmp102_getLowThreshold(uint8_t file, uint16_t *pLow)
{
	if(pLow == NULL)
		return EXIT_FAILURE;

	uint32_t tmp;
	if(EXIT_FAILURE == getIicRegister(file, TMP102_ADDR, TMP102_TLOW_REG, &tmp, TMP102_REG_SIZE, TMP102_ENDIANNESS))
		return EXIT_FAILURE;

	*pLow = (uint16_t)(0xFFFF & tmp);
	return EXIT_SUCCESS;
}

int8_t tmp102_getHighThreshold(uint8_t file, uint16_t *pHigh)
{
	if(pHigh == NULL)
		return EXIT_FAILURE;

	uint32_t tmp;
	if(EXIT_FAILURE == getIicRegister(file, TMP102_ADDR, TMP102_THIGH_REG, &tmp, TMP102_REG_SIZE, TMP102_ENDIANNESS))
		return EXIT_FAILURE;

	*pHigh = (uint16_t)(0xFFFF & tmp);
	return EXIT_SUCCESS;
}

int8_t tmp102_getConfiguration(uint8_t file, uint16_t *pConfig)
{
	if(pConfig == NULL)
		return EXIT_FAILURE;

	uint32_t tmp;
	if(EXIT_FAILURE == getIicRegister(file, TMP102_ADDR, TMP102_CONFIG_REG, &tmp, TMP102_REG_SIZE, TMP102_ENDIANNESS))
		return EXIT_FAILURE;

	*pConfig = (uint16_t)(0xFFFF & tmp);
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

int8_t tmp102_setLowThreshold(uint8_t file, uint16_t low)
{
	setIicRegister(file, TMP102_ADDR, TMP102_TLOW_REG, (uint32_t)low, TMP102_REG_SIZE, TMP102_ENDIANNESS);
	return EXIT_SUCCESS;
}

int8_t tmp102_setHighThreshold(uint8_t file, uint16_t high)
{
	setIicRegister(file, TMP102_ADDR, TMP102_THIGH_REG, (uint32_t)high, TMP102_REG_SIZE, TMP102_ENDIANNESS);
	return EXIT_SUCCESS;
}

int8_t tmp102_setConfiguration(uint8_t file, uint16_t config)
{
	setIicRegister(file, TMP102_ADDR, TMP102_CONFIG_REG, (uint32_t)config, TMP102_REG_SIZE, TMP102_ENDIANNESS);
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
