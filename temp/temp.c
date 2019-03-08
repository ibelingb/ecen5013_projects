/* temp sensor library */

#include "temp.h"
#include "lu_iic.h"
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>

int8_t tmp102_getTemp(uint8_t *pTemp)
{
	return EXIT_SUCCESS;
}

int8_t tmp102_getLowThreshold(uint8_t *pLow)
{
	return EXIT_SUCCESS;
}

int8_t tmp102_getHighThreshold(uint8_t *pHigh)
{
	return EXIT_SUCCESS;
}

int8_t tmp102_getConfiguration(uint8_t *pConfig)
{
	//setIicRegister(int IicFd, uint8_t slavAddr, uint8_t reg, uint8_t reg_value);
	//getIicRegister(int IicFd, uint8_t slavAddr, uint8_t reg, uint8_t *pReg_value);
	return EXIT_SUCCESS;
}

int8_t tmp102_getTempResolution(uint8_t res)
{
	return EXIT_SUCCESS;
}

int8_t tmp102_getFault(uint8_t *pFaults)
{
	return EXIT_SUCCESS;
}

int8_t tmp102_getExtendedMode(uint8_t *pEM)
{
	return EXIT_SUCCESS;
}

int8_t tmp102_getShutdownMode(uint8_t bit)
{
	return EXIT_SUCCESS;
}

int8_t tmp102_getAlert(uint8_t *pAlert)
{
	return EXIT_SUCCESS;
}

int8_t tmp102_getConvRate(uint8_t *pConvRt)
{
	return EXIT_SUCCESS;
}

int8_t tmp102_setLowThreshold(uint8_t low)
{
	return EXIT_SUCCESS;
}

int8_t tmp102_setHighThreshold(uint8_t high)
{
	return EXIT_SUCCESS;
}

int8_t tmp102_setConfiguration(uint8_t config)
{
	return EXIT_SUCCESS;
}

int8_t tmp102_setTempResolution(uint8_t res)
{
	return EXIT_SUCCESS;
}

int8_t tmp102_setExtendedMode(uint8_t *pEM)
{
	return EXIT_SUCCESS;
}

int8_t tmp102_setShutdownMode(uint8_t bit)
{
	return EXIT_SUCCESS;
}

int8_t tmp102_setConvRate(uint8_t *pConvRt)
{
	return EXIT_SUCCESS;
}