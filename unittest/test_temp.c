/***********************************************************************************
 * @author Brian Ibeling and Josh Malburg
 * 
 * Advanced Embedded Software Development
 * ECEN5013 - Rick Heidebrecht
 * @date March 8, 2019
 * arm-linux-gnueabi (Buildroot)
 * gcc (Ubuntu)
 ************************************************************************************
 *
 * @file test_temp.c
 * @brief test cases for temperature library / sensor
 *
 ************************************************************************************
 */

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <stdint.h>

#include "lu_iic.h"
#include "tempSensor.h"
#include "debug.h"

/* global file descriptor */
int fd;
uint8_t testCount = 0;

/* test cases */
int8_t readTempConv(void);
int8_t rwr_LowThreshold(void);
int8_t rwr_HighThreshold(void);

int main(void)
{
	uint8_t testFails = 0;

	printf("test cases for TMP02 temp sensor library\n");

	/* init bus */
	fd = initIic("/dev/i2c-2");
	if(fd < 0)
		printf("i2c init failed\n");

	sleep(2);

	testFails += rwr_LowThreshold();
	testFails += rwr_HighThreshold();
	testFails += readTempConv();

	printf("\n\nTEST RESULTS, %d of %d failed tests\n", testFails, testCount);

	return EXIT_SUCCESS;
}

int8_t readTempConv(void)
{
	uint8_t ind;
	float temp, start;
	testCount++;

	for(ind = 0; ind < 10; ++ind)
	{
		if(tmp102_getTempC(fd, &temp) < 0)
		{
			ERROR_PRINT("tmp102_getTempC failed\n");
			return EXIT_FAILURE;
		}
		INFO_PRINT("got temp value: %f degC, %f degF, %f degK\n", temp, TMP_DEGC_TO_DEGF(temp), TMP_DEGC_TO_DEGK(temp));			
	sleep(1);
	}
	start = temp;
	temp = TMP_DEGC_TO_DEGF(temp);
	temp = TMP_DEGF_TO_DEGK(temp);
	temp = TMP_DEGK_TO_DEGC(temp);
	temp = TMP_DEGC_TO_DEGK(temp);
	temp = TMP_DEGK_TO_DEGF(temp);
	temp = TMP_DEGF_TO_DEGC(temp);
	INFO_PRINT("%f DegC-->DegF-->DegK-->DegC-->DegK-->DegF--> %f DegC\n", start, temp);
	
	return EXIT_SUCCESS;
}

int8_t rwr_LowThreshold(void)
{
	float Tlow, start, CHANGE = 5.0f;
	testCount++;

	/*read threshold value */
	if(EXIT_FAILURE == tmp102_getLowThreshold(fd, &Tlow))
	{ ERROR_PRINT("rwr_LowThreshold write failed\n"); return EXIT_FAILURE; }
	else { INFO_PRINT("got Tlow value: %f\n", Tlow); }

	/* change it */
	start = Tlow;
	if(EXIT_FAILURE == tmp102_setLowThreshold(fd, Tlow - CHANGE))
	{ ERROR_PRINT("rwr_LowThreshold read failed\n"); return EXIT_FAILURE; }

	/* read it back */
	if(EXIT_FAILURE == tmp102_getLowThreshold(fd, &Tlow))
	{ ERROR_PRINT("rwr_LowThreshold write failed\n"); return EXIT_FAILURE; }
	else { INFO_PRINT("got Tlow value: %f\n", Tlow); }

	if((start - CHANGE) != Tlow)
	{ 
		ERROR_PRINT("rwr_LowThreshold read doesn't match written\n");
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}

int8_t rwr_HighThreshold(void)
{
	float Thigh, start, CHANGE = 5.0f;
	testCount++;

	/*read threshold value */
	if(EXIT_FAILURE == tmp102_getHighThreshold(fd, &Thigh))
	{ ERROR_PRINT("rwr_HighThreshold write failed\n"); return EXIT_FAILURE; }
	else { INFO_PRINT("got Thigh value: %f\n", Thigh); }

	/* change it */
	start = Thigh;
	if(EXIT_FAILURE == tmp102_setHighThreshold(fd, Thigh - CHANGE))
	{ ERROR_PRINT("rwr_HighThreshold read failed\n"); return EXIT_FAILURE; }

	/* read it back */
	if(EXIT_FAILURE == tmp102_getHighThreshold(fd, &Thigh))
	{ ERROR_PRINT("rwr_HighThreshold write failed\n"); return EXIT_FAILURE; }
	else { INFO_PRINT("got Thigh value: %f\n", Thigh); }

	if((start - CHANGE) != Thigh)
	{ 
		ERROR_PRINT("rwr_HighThreshold read doesn't match written\n");
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}
