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
 * @brief TMP102 temperature sensor library
 *
 ************************************************************************************
 */

#include "lu_iic.h"
#include "tempSensor.h"
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>

#define TMP102_ADDR				(0x48ul)					/* I2C address of sensor */

#define TMP102_TEMP_REG			(0x00ul)					/* temperature register */
#define TMP102_CONFIG_REG		(0x01ul)					/* configuration register */
#define TMP102_TLOW_REG			(0x02ul)					/* low threshold temp. register */
#define TMP102_THIGH_REG		(0x03ul)					/* high threshold temp. register */

#define TMP102_REG_SIZE			(2)							/* size of TMP102 register in bytes */
#define TMP102_ENDIANNESS		(TMP102_LITTLE_ENDIAN)		/* endianness of returned bytes */
#define TMP102_LITTLE_ENDIAN	(0)
#define TMP102_TEMP_SCALEFACTOR	(0.0625f)					/* LSB weight of temp and threshold registers */
#define TMP102_TEMP_START_BIT	(4)							/* offset for temperature data in register */
#define TMP102_TLOW_START_BIT	(TMP102_TEMP_START_BIT)		/* offset for Tlow data in register */
#define TMP102_THIGH_START_BIT	(TMP102_TEMP_START_BIT)		/* offset for Thigh data in register */

/* macro to determine address mode, normal vs extended */
#define TMP102_GET_ADDR_MODE(TEMP_REG)	((TEMP_REG % 2) == 1 ? TMP102_ADDR_MODE_EXTENDED : TMP102_ADDR_MODE_NORMAL)

/* FIXME: need to update for negative numbers */
#define TMP102_BITS_TO_TEMPC(BITS, SHIFT) (((float)((0xFFFF & (BITS)) >> SHIFT)) * TMP102_TEMP_SCALEFACTOR)
#define TMP102_TEMPC_TO_BITS(TEMPC,SHIFT) (((uint16_t)(TEMPC / TMP102_TEMP_SCALEFACTOR)) << SHIFT)

#define TMP102_CONFIG_REG_MASK_EXT_MODE		(0x0010ul)
#define TMP102_CONFIG_REG_MASK_ALERT		(0x0020ul)
#define TMP102_CONFIG_REG_MASK_CONVRATE		(0x00C0ul)
#define TMP102_CONFIG_REG_MASK_SHUTDOWN		(0x0100ul)
#define TMP102_CONFIG_REG_MASK_TM_MODE		(0x0200ul)
#define TMP102_CONFIG_REG_MASK_POL			(0x0400ul)
#define TMP102_CONFIG_REG_MASK_FLT_QUEUE	(0x1800ul)
#define TMP102_CONFIG_REG_MASK_CONV_RES		(0x6000ul)
#define TMP102_CONFIG_REG_MASK_ONE_SHOT		(0x8000ul)

#define TMP102_CONFIG_REG_OFFSET_EXT_MODE	(4)
#define TMP102_CONFIG_REG_OFFSET_ALERT		(5)
#define TMP102_CONFIG_REG_OFFSET_CONVRATE	(6)
#define TMP102_CONFIG_REG_OFFSET_SHUTDOWN	(8)
#define TMP102_CONFIG_REG_OFFSET_TM_MODE	(9)
#define TMP102_CONFIG_REG_OFFSET_POL		(10)
#define TMP102_CONFIG_REG_OFFSET_FLT_QUEUE	(11)
#define TMP102_CONFIG_REG_OFFSET_CONV_RES	(13)
#define TMP102_CONFIG_REG_OFFSET_ONE_SHOT	(15)

/* private functions */
int8_t tmp102_getReg(uint8_t file, uint16_t *pReg, uint8_t REG);
int8_t tmp102_getPolarity(uint8_t file, uint8_t *pBit);

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
	if(TMP102_GET_ADDR_MODE(tmp) == TMP102_ADDR_MODE_EXTENDED)
		--shiftValue;

	/* convert bits to Degrees C */
	*pTemp = TMP102_BITS_TO_TEMPC(tmp, shiftValue);
	return EXIT_SUCCESS;
}

/**
 * @brief get low temperature threshold value in celcius.
 * 
 * @param file handle to i2c bus
 * @param pLow pointer to store threshold value
 * @return int8_t status, EXIT_SUCCESS if succeeeds
 */
int8_t tmp102_getLowThreshold(uint8_t file, float *pLow)
{
	uint8_t shiftValue;
	Tmp102_AddrMode_e addressMode;
	uint16_t tmp;

	/* validate inputs */
	if(pLow == NULL)
		return EXIT_FAILURE;

	/* get extended mode */
	if(EXIT_FAILURE == tmp102_getExtendedMode(file, &addressMode))
		return EXIT_FAILURE;

	/* if extended address mode shift one less bit */
	shiftValue = TMP102_TLOW_START_BIT;
	if(addressMode == TMP102_ADDR_MODE_EXTENDED)
		--shiftValue;

	/* get TLow register value */
	if(EXIT_FAILURE == tmp102_getReg(file, &tmp, TMP102_TLOW_REG))
		return EXIT_FAILURE;

	/* convert bits to degrees celcius before returning */
	*pLow = TMP102_BITS_TO_TEMPC(tmp, shiftValue);
	return EXIT_SUCCESS;
}

/**
 * @brief get high temperature threshold value in celcius.
 * 
 * @param file handle to i2c bus
 * @param pLow pointer to store threshold value
 * @return int8_t status, EXIT_SUCCESS if succeeeds
 */
int8_t tmp102_getHighThreshold(uint8_t file, float *pHigh)
{
	uint8_t shiftValue;
	Tmp102_AddrMode_e addressMode;
	uint16_t tmp;

	/* validate inputs */
	if(pHigh == NULL)
		return EXIT_FAILURE;

	/* get extended mode */
	if(EXIT_FAILURE == tmp102_getExtendedMode(file, &addressMode))
		return EXIT_FAILURE;

	/* if extended address mode shift one less bit */
	shiftValue = TMP102_THIGH_START_BIT;
	if(addressMode == TMP102_ADDR_MODE_EXTENDED)
		--shiftValue;

	/* get THigh register value */
	if(EXIT_FAILURE == tmp102_getReg(file, &tmp, TMP102_THIGH_REG))
		return EXIT_FAILURE;

	/* convert bits to degrees celcius before returning */
	*pHigh = TMP102_BITS_TO_TEMPC(tmp, shiftValue);
	return EXIT_SUCCESS;
}

int8_t tmp102_getFaultQueueSize(uint8_t file, Tmp102_FaultCount_e *pFaults)
{
	/* validate inputs */
	if(pFaults == NULL)
		return EXIT_FAILURE;

	/* get register value */
	uint16_t tmp;
	if(EXIT_FAILURE == tmp102_getReg(file, &tmp, TMP102_CONFIG_REG))
		return EXIT_FAILURE;
	
	/* get data */
	*pFaults = (Tmp102_FaultCount_e)((tmp & TMP102_CONFIG_REG_MASK_FLT_QUEUE) 
	>> TMP102_CONFIG_REG_OFFSET_FLT_QUEUE);
	
	return EXIT_SUCCESS;
}

/**
 * @brief get address mode, normal (12-bits) or extended (13-bits)
 * 
 * @param file handle to i2c bus
 * @param pEM pointer to variable to store mode
 * @return int8_t status, returns EXIT_SUCCESS if succeeds
 */
int8_t tmp102_getExtendedMode(uint8_t file, Tmp102_AddrMode_e *pMode)
{
	/* validate inputs */
	if(pMode == NULL)
		return EXIT_FAILURE;

	/* get register value */
	uint16_t tmp;
	if(EXIT_FAILURE == tmp102_getReg(file, &tmp, TMP102_CONFIG_REG))
		return EXIT_FAILURE;

	/* get data */
	*pMode = (Tmp102_AddrMode_e)((tmp & TMP102_CONFIG_REG_MASK_EXT_MODE) 
	>> TMP102_CONFIG_REG_OFFSET_EXT_MODE);
	
	return EXIT_SUCCESS;
}

/**
 * @brief determine if sensor is in shutdown mode
 * 
 * @param file handle to i2c bus
 * @param pState pointer to state variable, to store results
 * @return int8_t status, EXIT_SUCCESS if succeeds
 */
int8_t tmp102_getShutdownState(uint8_t file, Tmp102_Shutdown_e *pState)
{
	/* validate inputs */
	if(pState == NULL)
		return EXIT_FAILURE;

	/* get register value */
	uint16_t tmp;
	if(EXIT_FAILURE == tmp102_getReg(file, &tmp, TMP102_CONFIG_REG))
		return EXIT_FAILURE;
	
	/* get data */
	*pState = (Tmp102_Shutdown_e)((tmp & TMP102_CONFIG_REG_MASK_SHUTDOWN) 
	>> TMP102_CONFIG_REG_OFFSET_SHUTDOWN);

	return EXIT_SUCCESS;
}

int8_t tmp102_getAlert(uint8_t file, Tmp102_Alert_e *pAlert)
{
	uint16_t reg;
	uint8_t tmp, polarity, ActiveIs;

	/* validate inputs */
	if(pAlert == NULL)
		return EXIT_FAILURE;

	/* first get polarity setting */
	if(EXIT_FAILURE == tmp102_getPolarity(file, &polarity))
		return EXIT_FAILURE;	

	/* use polarity to determine active alert state */
	ActiveIs = (polarity == 0) ? 0 : 1;

	/* get register value */
	if(EXIT_FAILURE == tmp102_getReg(file, &reg, TMP102_CONFIG_REG))
		return EXIT_FAILURE;

	/* get alert value */
	tmp = ((reg & TMP102_CONFIG_REG_MASK_ALERT) >> TMP102_CONFIG_REG_OFFSET_ALERT);

	if(tmp == ActiveIs)
		tmp = TMP102_ALERT_ACTIVE;
	else
		tmp = TMP102_ALERT_OFF;
	
	/* finally return results */
	*pAlert = (Tmp102_Alert_e)tmp;
	return EXIT_SUCCESS;
}

int8_t tmp102_getConvRate(uint8_t file, Tmp102_ConvRate_e *pConvRt)
{
	/* validate inputs */
	if(pConvRt == NULL)
		return EXIT_FAILURE;

	/* get register value */
	uint16_t tmp;
	if(EXIT_FAILURE == tmp102_getReg(file, &tmp, TMP102_CONFIG_REG))
		return EXIT_FAILURE;

	/* get data */
	*pConvRt = (Tmp102_ConvRate_e)((tmp & TMP102_CONFIG_REG_MASK_CONVRATE) 
	>> TMP102_CONFIG_REG_OFFSET_CONVRATE);
	
	return EXIT_SUCCESS;
}

/**
 * @brief change the low temperature threshold sensor value
 * 
 * @param file handle to i2c bus
 * @param high new threshold value to write
 * @return int8_t return status, EXIT_SUCCESS if succeeds
 */
int8_t tmp102_setLowThreshold(uint8_t file, float low)
{
	uint32_t low_bits;
	Tmp102_AddrMode_e addressMode;
	uint8_t shiftValue;
	
	/* get extended mode */
	if(EXIT_FAILURE == tmp102_getExtendedMode(file, &addressMode))
		return EXIT_FAILURE;

	/* if extended address mode shift one less bit */
	shiftValue = TMP102_TLOW_START_BIT;
	if(addressMode == TMP102_ADDR_MODE_EXTENDED)
		--shiftValue;

	/* convert float to integer */
	low_bits = TMP102_TEMPC_TO_BITS(low, shiftValue);

	/* set register value */
	if(EXIT_FAILURE == setIicRegister(file, TMP102_ADDR, TMP102_TLOW_REG, low_bits, TMP102_REG_SIZE, TMP102_ENDIANNESS))
		return EXIT_FAILURE;

	return EXIT_SUCCESS;
}

/**
 * @brief change the high temperature threshold sensor value
 * 
 * @param file handle to i2c bus
 * @param high new threshold value to write
 * @return int8_t return status, EXIT_SUCCESS if succeeds
 */
int8_t tmp102_setHighThreshold(uint8_t file, float high)
{
	uint32_t high_bits;
	Tmp102_AddrMode_e addressMode;
	uint8_t shiftValue;
	
	/* get extended mode */
	if(EXIT_FAILURE == tmp102_getExtendedMode(file, &addressMode))
		return EXIT_FAILURE;

	/* if extended address mode shift one less bit */
	shiftValue = TMP102_THIGH_START_BIT;
	if(addressMode == TMP102_ADDR_MODE_EXTENDED)
		--shiftValue;

	/* convert float to integer */
	high_bits = TMP102_TEMPC_TO_BITS(high, shiftValue);

	/* set register value */
	if(EXIT_FAILURE == setIicRegister(file, TMP102_ADDR, TMP102_THIGH_REG, high_bits, TMP102_REG_SIZE, TMP102_ENDIANNESS))
		return EXIT_FAILURE;

	return EXIT_SUCCESS;
}

int8_t tmp102_setTempResolution(uint8_t file, uint8_t res)
{
	return EXIT_SUCCESS;
}

int8_t tmp102_setExtendedMode(uint8_t file, Tmp102_AddrMode_e mode)
{
	return EXIT_SUCCESS;
}

int8_t tmp102_setShutdownState(uint8_t file, Tmp102_Shutdown_e state)
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

int8_t tmp102_getPolarity(uint8_t file, uint8_t *pBit)
{
	/* validate inputs */
	if(pBit == NULL)
		return EXIT_FAILURE;

	/* get register value */
	uint16_t tmp;
	if(EXIT_FAILURE == tmp102_getReg(file, &tmp, TMP102_CONFIG_REG))
		return EXIT_FAILURE;
	
	/* get data */
	*pBit = (uint8_t)((tmp & TMP102_CONFIG_REG_MASK_POL) 
	>> TMP102_CONFIG_REG_OFFSET_POL);

	return EXIT_SUCCESS;
}
