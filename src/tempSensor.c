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
	float maxTemp = 128.0f;
	if(TMP102_GET_ADDR_MODE(tmp) == TMP102_ADDR_MODE_EXTENDED) {
		--shiftValue;
		maxTemp = 150.0f;
	}
	/* test for negative value */
	float tmpFloat;
	if((tmp >> 15) != 0) {
		/* convert bits to Degrees C */
		tmpFloat = (maxTemp * 2.0f) - TMP102_BITS_TO_TEMPC(tmp, shiftValue);
	}	
	else {
		/* convert bits to Degrees C */
		tmpFloat = TMP102_BITS_TO_TEMPC(tmp, shiftValue);
	}
	*pTemp = tmpFloat;
	return EXIT_SUCCESS;
}

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
	float maxTemp = 128.0f;
	if(addressMode == TMP102_ADDR_MODE_EXTENDED) {
		--shiftValue;
		maxTemp = 150.0f;
	}

	/* get TLow register value */
	if(EXIT_FAILURE == tmp102_getReg(file, &tmp, TMP102_TLOW_REG))
		return EXIT_FAILURE;

	/* test for negative value */
	float tmpFloat;
	if((tmp >> 15) != 0) {
		/* convert bits to Degrees C */
		tmpFloat = (maxTemp * 2.0f) - TMP102_BITS_TO_TEMPC(tmp, shiftValue);
	}	
	else {
		/* convert bits to Degrees C */
		tmpFloat = TMP102_BITS_TO_TEMPC(tmp, shiftValue);
	}
	*pLow = tmpFloat;
	return EXIT_SUCCESS;
}

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
	float maxTemp = 128.0f;
	if(addressMode == TMP102_ADDR_MODE_EXTENDED) {
		--shiftValue;
		maxTemp = 150.0f;
	}

	/* test for negative */
	if(low < 0) {
		low = (maxTemp * 2.0f) - low;
	}

	/* convert float to integer */
	low_bits = TMP102_TEMPC_TO_BITS(low, shiftValue);

	/* set register value */
	if(EXIT_FAILURE == setIicRegister(file, TMP102_ADDR, TMP102_TLOW_REG, low_bits, TMP102_REG_SIZE, TMP102_ENDIANNESS))
		return EXIT_FAILURE;

	return EXIT_SUCCESS;
}

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
	float maxTemp = 128.0f;
	if(addressMode == TMP102_ADDR_MODE_EXTENDED) {
		--shiftValue;
		maxTemp = 150.0f;
	}

	/* get THigh register value */
	if(EXIT_FAILURE == tmp102_getReg(file, &tmp, TMP102_THIGH_REG))
		return EXIT_FAILURE;

	/* test for negative value */
	float tmpFloat;
	if((tmp >> 15) != 0) {
		/* convert bits to Degrees C */
		tmpFloat = (maxTemp * 2.0f) - TMP102_BITS_TO_TEMPC(tmp, shiftValue);
	}	
	else {
		/* convert bits to Degrees C */
		tmpFloat = TMP102_BITS_TO_TEMPC(tmp, shiftValue);
	}
	*pHigh = tmpFloat;
	return EXIT_SUCCESS;
}

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
	float maxTemp = 128.0f;
	if(addressMode == TMP102_ADDR_MODE_EXTENDED) {
		--shiftValue;
		maxTemp = 150.0f;
	}

	/* test for negative */
	if(high < 0) {
		high = (maxTemp * 2.0f) - high;
	}

	/* convert float to integer */
	high_bits = TMP102_TEMPC_TO_BITS(high, shiftValue);

	/* set register value */
	if(EXIT_FAILURE == setIicRegister(file, TMP102_ADDR, TMP102_THIGH_REG, high_bits, TMP102_REG_SIZE, TMP102_ENDIANNESS))
		return EXIT_FAILURE;

	return EXIT_SUCCESS;
}

int8_t tmp102_getFaultQueueSize(uint8_t file, Tmp102_FaultCount_e *pCounts)
{
	/* validate inputs */
	if(pCounts == NULL)
		return EXIT_FAILURE;

	/* get register value */
	uint16_t tmp;
	if(EXIT_FAILURE == tmp102_getReg(file, &tmp, TMP102_CONFIG_REG))
		return EXIT_FAILURE;
	
	/* get data */
	*pCounts = (Tmp102_FaultCount_e)((tmp & TMP102_CONFIG_REG_MASK_FLT_QUEUE) 
	>> TMP102_CONFIG_REG_OFFSET_FLT_QUEUE);
	
	return EXIT_SUCCESS;
}

int8_t tmp102_setFaultQueueSize(uint8_t file, Tmp102_FaultCount_e size)
{
	uint16_t tmp = 0, reg;

	/* first get current config register value */
	if(EXIT_FAILURE == tmp102_getReg(file, &reg, TMP102_CONFIG_REG))
		return EXIT_FAILURE;
	
	/* clear previous queue count value */
	reg &= ~TMP102_CONFIG_REG_MASK_FLT_QUEUE;

	/* set new value in tmp */
	tmp = (((uint16_t)size << TMP102_CONFIG_REG_OFFSET_FLT_QUEUE) 
	& TMP102_CONFIG_REG_MASK_FLT_QUEUE);

	/* write new register value */
	reg |= tmp;
	if(EXIT_FAILURE == setIicRegister(file, TMP102_ADDR, TMP102_CONFIG_REG, reg, TMP102_REG_SIZE, TMP102_ENDIANNESS))
		return EXIT_FAILURE;
	
	return EXIT_SUCCESS;
}

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

int8_t tmp102_setExtendedMode(uint8_t file, Tmp102_AddrMode_e mode)
{
	uint16_t tmp = 0, reg;

	/* first get current config register value */
	if(EXIT_FAILURE == tmp102_getReg(file, &reg, TMP102_CONFIG_REG))
		return EXIT_FAILURE;
	
	/* clear previous queue count value */
	reg &= ~TMP102_CONFIG_REG_MASK_EXT_MODE;

	/* set new value in tmp */
	tmp = (((uint16_t)mode << TMP102_CONFIG_REG_OFFSET_EXT_MODE) 
	& TMP102_CONFIG_REG_MASK_EXT_MODE);

	/* write new register value */
	reg |= tmp;
	if(EXIT_FAILURE == setIicRegister(file, TMP102_ADDR, TMP102_CONFIG_REG, reg, TMP102_REG_SIZE, TMP102_ENDIANNESS))
		return EXIT_FAILURE;
	
	return EXIT_SUCCESS;
}

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

int8_t tmp102_setShutdownState(uint8_t file, Tmp102_Shutdown_e state)
{
	uint16_t tmp = 0, reg;

	/* first get current config register value */
	if(EXIT_FAILURE == tmp102_getReg(file, &reg, TMP102_CONFIG_REG))
		return EXIT_FAILURE;
	
	/* clear previous queue count value */
	reg &= ~TMP102_CONFIG_REG_MASK_SHUTDOWN;

	/* set new value in tmp */
	tmp = (((uint16_t)state << TMP102_CONFIG_REG_OFFSET_SHUTDOWN) 
	& TMP102_CONFIG_REG_MASK_SHUTDOWN);

	/* write new register value */
	reg |= tmp;
	if(EXIT_FAILURE == setIicRegister(file, TMP102_ADDR, TMP102_CONFIG_REG, reg, TMP102_REG_SIZE, TMP102_ENDIANNESS))
		return EXIT_FAILURE;
	
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

int8_t tmp102_setConvRate(uint8_t file, Tmp102_ConvRate_e convrate)
{
	uint16_t tmp = 0, reg;

	/* first get current config register value */
	if(EXIT_FAILURE == tmp102_getReg(file, &reg, TMP102_CONFIG_REG))
		return EXIT_FAILURE;
	
	/* clear previous queue count value */
	reg &= ~TMP102_CONFIG_REG_MASK_CONVRATE;

	/* set new value in tmp */
	tmp = (((uint16_t)convrate << TMP102_CONFIG_REG_OFFSET_CONVRATE) 
	& TMP102_CONFIG_REG_MASK_CONVRATE);

	/* write new register value */
	reg |= tmp;
	if(EXIT_FAILURE == setIicRegister(file, TMP102_ADDR, TMP102_CONFIG_REG, reg, TMP102_REG_SIZE, TMP102_ENDIANNESS))
		return EXIT_FAILURE;
	
	return EXIT_SUCCESS;
}

int8_t tmp102_getPolarity(uint8_t file, uint8_t *pPol)
{
	/* validate inputs */
	if(pPol == NULL)
		return EXIT_FAILURE;

	/* get register value */
	uint16_t tmp;
	if(EXIT_FAILURE == tmp102_getReg(file, &tmp, TMP102_CONFIG_REG))
		return EXIT_FAILURE;
	
	/* get data */
	*pPol = (uint8_t)((tmp & TMP102_CONFIG_REG_MASK_POL) 
	>> TMP102_CONFIG_REG_OFFSET_POL);

	return EXIT_SUCCESS;
}

int8_t tmp102_setPolarity(uint8_t file, uint8_t pol)
{
	uint16_t tmp = 0, reg;

	/* first get current config register value */
	if(EXIT_FAILURE == tmp102_getReg(file, &reg, TMP102_CONFIG_REG))
		return EXIT_FAILURE;
	
	/* clear previous queue count value */
	reg &= ~TMP102_CONFIG_REG_MASK_POL;

	/* set new value in tmp */
	tmp = (((uint16_t)pol << TMP102_CONFIG_REG_OFFSET_POL) 
	& TMP102_CONFIG_REG_MASK_POL);

	/* write new register value */
	reg |= tmp;
	if(EXIT_FAILURE == setIicRegister(file, TMP102_ADDR, TMP102_CONFIG_REG, reg, TMP102_REG_SIZE, TMP102_ENDIANNESS))
		return EXIT_FAILURE;
	
	return EXIT_SUCCESS;
}

/** private functions ******************************************************************/

/**
 * @brief reads register from userspace driver and converts
 * to 16-bit register
 * 
 * @param file handle of i2c bus
 * @param pReg pointer to results variable 
 * @param REG address of register
 * @return int8_t status, EXIT_SUCCESS if succeeds
 */
int8_t tmp102_getReg(uint8_t file, uint16_t *pReg, uint8_t REG)
{
	/* validate inputs */
	if(pReg == NULL)
		return EXIT_FAILURE;

	/* get register value */
	uint32_t tmp;
	if(EXIT_FAILURE == getIicRegister(file, TMP102_ADDR, REG, &tmp, TMP102_REG_SIZE, TMP102_ENDIANNESS))
		return EXIT_FAILURE;

	*pReg = (uint16_t)(0xFFFF & tmp);
	return EXIT_SUCCESS;
}
