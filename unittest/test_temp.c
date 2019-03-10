/* unit tests */

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#include "lu_iic.h"
#include "tempSensor.h"
#include "debug.h"

/* global file descriptor */
int fd;
uint8_t testCount = 0;

/* test cases */
int8_t readTemp(void);
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

	testFails += rwr_LowThreshold();
	testFails += rwr_HighThreshold();
	testFails += readTemp();

	printf("test results, %d failed tests\n", testFails);

	return EXIT_SUCCESS;
}

int8_t readTemp(void)
{
	uint8_t ind;
	float temp;
	testCount++;

	for(ind = 0; ind < 10; ++ind)
	{
		if(tmp102_getTempC(fd, &temp) < 0)
		{
			ERROR_PRINT("tmp102_getTempC failed\n");
			return EXIT_FAILURE;
		}
		else
			INFO_PRINT("got temp value: %f\n", temp);
			
	sleep(1);
	}
	
	return EXIT_SUCCESS;
}

int8_t rwr_LowThreshold(void)
{
	float Tlow;
	testCount++;

	if(tmp102_getLowThreshold(fd, &Tlow) < 0)
	{
		ERROR_PRINT("tmp102_getLowThreshold failed\n");
		return EXIT_FAILURE;
	}
	else
		INFO_PRINT("got Tlow value: %f\n", Tlow);

	return EXIT_SUCCESS;
}

int8_t rwr_HighThreshold(void)
{
	float Thigh;
	testCount++;
	
	if(tmp102_getHighThreshold(fd, &Thigh) < 0)
	{
		ERROR_PRINT("tmp102_getLowThreshold failed\n");
		return EXIT_FAILURE;
	}
	else
		INFO_PRINT("got Thigh value: %f\n", Thigh);

	return EXIT_SUCCESS;
}
