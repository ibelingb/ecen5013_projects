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
#define TMP102_TEMP_START_BIT	(4)
#define TMP102_TLOW_START_BIT	(TMP102_TEMP_START_BIT)
#define TMP102_THIGH_START_BIT	(TMP102_TEMP_START_BIT)

#define TMP102_GET_ADDR_MODE(TEMP_REG)	((TEMP_REG % 2) == 1 ? TMP102_EXTENDED_MODE : TMP102_NORMAL_MODE)
#define TMP102_EXTENDED_MODE	(1)
#define TMP102_NORMAL_MODE		(0)
#define TMP102_BITS_TO_TEMPC(BITS, SHIFT) (((float)((0xFFFF & (BITS)) >> SHIFT)) * TMP102_TEMP_SCALEFACTOR)

/* private functions */
int8_t tmp102_getReg(uint8_t file, uint16_t *pReg, uint8_t REG);

/**
 * @brief get temperature from sensor in celcius.
 * 
 * @param file handle for i2c bus
 * @param pTemp pointer to temperature variable / return value
 * @return int8_t status, EXIT_SUCCESS or EXIT_FAILURE
 */
int8_t tmp102_getTempC(uint8_t file, float *pTemp)
{
	/* validate inputs */
	if(pTemp == NULL)
		return EXIT_FAILURE;

	/* get register value */
	uint16_t tmp;
	if(EXIT_FAILURE == tmp102_getReg(file, &tmp, TMP102_TEMP_REG))
		return EXIT_FAILURE;

	/* if extended address mode shift one less bit */
	uint8_t shiftValue = TMP102_TEMP_START_BIT;
	if(TMP102_GET_ADDR_MODE(tmp) == TMP102_EXTENDED_MODE)
		--shiftValue;

	/* convert bits to Degrees C */
	*pTemp = TMP102_BITS_TO_TEMPC(tmp, shiftValue);
	return EXIT_SUCCESS;
}

/**
 * @brief 
 * 
 * @param file 
 * @param pLow 
 * @return int8_t 
 */
int8_t tmp102_getLowThreshold(uint8_t file, float *pLow)
{
	/* validate inputs */
	if(pLow == NULL)
		return EXIT_FAILURE;

	/* get temp reg to test for extended address mode */
	uint16_t tmp;
	if(EXIT_FAILURE == tmp102_getReg(file, &tmp, TMP102_TEMP_REG))
	return EXIT_FAILURE;

	/* if extended address mode shift one less bit */
	uint8_t shiftValue = TMP102_TLOW_START_BIT;
	if(TMP102_GET_ADDR_MODE(tmp) == TMP102_EXTENDED_MODE)
		--shiftValue;

	/* get TLow register value */
	if(EXIT_FAILURE == tmp102_getReg(file, &tmp, TMP102_TLOW_REG))
		return EXIT_FAILURE;

	*pLow = TMP102_BITS_TO_TEMPC(tmp, shiftValue);
	return EXIT_SUCCESS;
}

/**
 * @brief 
 * 
 * @param file 
 * @param pHigh 
 * @return int8_t 
 */
int8_t tmp102_getHighThreshold(uint8_t file, float *pHigh)
{
	/* validate inputs */
	if(pHigh == NULL)
		return EXIT_FAILURE;

	/* get temp reg to test for extended address mode */
	uint16_t tmp;
	if(EXIT_FAILURE == tmp102_getReg(file, &tmp, TMP102_TEMP_REG))
	return EXIT_FAILURE;

	/* if extended address mode shift one less bit */
	uint8_t shiftValue = TMP102_THIGH_START_BIT;
	if(TMP102_GET_ADDR_MODE(tmp) == TMP102_EXTENDED_MODE)
		--shiftValue;

	/* get THigh register value */
	if(EXIT_FAILURE == tmp102_getReg(file, &tmp, TMP102_THIGH_REG))
		return EXIT_FAILURE;

	*pHigh = TMP102_BITS_TO_TEMPC(tmp, shiftValue);
	return EXIT_SUCCESS;
}

/**
 * @brief 
 * 
 * @param file 
 * @param pRes 
 * @return int8_t 
 */
int8_t tmp102_getTempResolution(uint8_t file, uint8_t *pRes)
{
	/* validate inputs */
	if(pRes == NULL)
		return EXIT_FAILURE;
	
	return EXIT_SUCCESS;
}

int8_t tmp102_getFault(uint8_t file, uint8_t *pFaults)
{
	/* validate inputs */
	if(pFaults == NULL)
		return EXIT_FAILURE;
	
	return EXIT_SUCCESS;
}

int8_t tmp102_getExtendedMode(uint8_t file, uint8_t *pEM)
{
	/* validate inputs */
	if(pEM == NULL)
		return EXIT_FAILURE;
	
	return EXIT_SUCCESS;
}

int8_t tmp102_getShutdownMode(uint8_t file, uint8_t *pBit)
{
	/* validate inputs */
	if(pBit == NULL)
		return EXIT_FAILURE;
	
	return EXIT_SUCCESS;
}

int8_t tmp102_getAlert(uint8_t file, uint8_t *pAlert)
{
	/* validate inputs */
	if(pAlert == NULL)
		return EXIT_FAILURE;
	
	return EXIT_SUCCESS;
}

int8_t tmp102_getConvRate(uint8_t file, uint8_t *pConvRt)
{
	/* validate inputs */
	if(pConvRt == NULL)
		return EXIT_FAILURE;
	
	return EXIT_SUCCESS;
}

int8_t tmp102_setLowThreshold(uint8_t file, float low)
{
	setIicRegister(file, TMP102_ADDR, TMP102_TLOW_REG, (uint32_t)low, TMP102_REG_SIZE, TMP102_ENDIANNESS);
	return EXIT_SUCCESS;
}

int8_t tmp102_setHighThreshold(uint8_t file, float high)
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


/** private functions ******************************************************************/

int8_t tmp102_getReg(uint8_t file, uint16_t *pConfig, uint8_t REG)
{
	/* validate inputs */
	if(pConfig == NULL)
		return EXIT_FAILURE;

	/* get register value */
	uint32_t tmp;
	if(EXIT_FAILURE == getIicRegister(file, TMP102_ADDR, REG, &tmp, TMP102_REG_SIZE, TMP102_ENDIANNESS))
		return EXIT_FAILURE;

	*pConfig = (uint16_t)(0xFFFF & tmp);
	return EXIT_SUCCESS;
}
