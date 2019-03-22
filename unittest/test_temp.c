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
 * TODO:
 * 1. verify can handle neg numbers
 ************************************************************************************
 */

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <stdint.h>

#include "lu_iic.h"
#include "tempSensor.h"
#include "debug.h"

#define TEMP_SAMPLES	(4)

/* global file descriptor */
int fd;
uint8_t testCount = 0;

/* test cases */
int8_t test_LowThreshold(void);
int8_t test_HighThreshold(void);
int8_t test_FaultQueueSize(void);
int8_t test_ExtendedAddrMode(void);
int8_t test_Shutdown(void);
int8_t test_ConversionRate(void);
int8_t test_AlertPolarity(void);
int8_t test_TempConv(void);

int main(void)
{
	printf("test cases for TMP02 temp sensor library\n");
	
	/* keep track of failures */
	uint8_t testFails = 0;

	/* init handle for i2c bus */
	fd = initIic("/dev/i2c-2");
	if(fd < 0)
		printf("i2c init failed\n");

	sleep(2);

	testFails += test_TempConv();
	testFails += test_LowThreshold();
	testFails += test_HighThreshold();
	testFails += test_FaultQueueSize();
	testFails += test_ExtendedAddrMode();
	testFails += test_TempConv();
	testFails += test_Shutdown();
	testFails += test_TempConv();
	testFails += test_ConversionRate();
	testFails += test_TempConv();
	testFails += test_AlertPolarity();

	printf("\n\nTEST RESULTS, %d of %d failed tests\n", testFails, testCount);

	return EXIT_SUCCESS;
}

/**
 * @brief read temp and verify conversions to/from C/F/K 
 * use all temp conversions
 * 
 * @return int8_t test pass / fail (EXIT_FAILURE)
 */
int8_t test_TempConv(void)
{
	uint8_t ind;
	float temp, start;
	testCount++;
	INFO_PRINT("\nstart of test_TempConv, test #%d\n",testCount);

	for(ind = 0; ind < TEMP_SAMPLES; ++ind)
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
/**
 * @brief read, write and verify write to low temp threshold register
 * 
 * @return int8_t test pass / fail (EXIT_FAILURE)
 */
int8_t test_LowThreshold(void)
{
	float Tlow, start, CHANGE = 5.0f;
	testCount++;
	INFO_PRINT("\nstart of test_LowThreshold, test #%d\n",testCount);

	/*read threshold value */
	if(EXIT_FAILURE == tmp102_getLowThreshold(fd, &Tlow))
	{ ERROR_PRINT("test_LowThreshold write failed\n"); return EXIT_FAILURE; }
	else { INFO_PRINT("got Tlow value: %f\n", Tlow); }

	/* change it */
	start = Tlow;
	if(EXIT_FAILURE == tmp102_setLowThreshold(fd, Tlow - CHANGE))
	{ ERROR_PRINT("test_LowThreshold read failed\n"); return EXIT_FAILURE; }

	/* read it back */
	if(EXIT_FAILURE == tmp102_getLowThreshold(fd, &Tlow))
	{ ERROR_PRINT("test_LowThreshold write failed\n"); return EXIT_FAILURE; }
	else { INFO_PRINT("got Tlow value: %f\n", Tlow); }

	if((start - CHANGE) != Tlow)
	{ 
		ERROR_PRINT("test_LowThreshold read doesn't match written\n");
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}
/**
 * @brief read, write and verify write to high temp threshold register
 * 
 * @return int8_t test pass / fail (EXIT_FAILURE)
 */
int8_t test_HighThreshold(void)
{
	float Thigh, start, CHANGE = 5.0f;
	testCount++;
	INFO_PRINT("\nstart of test_HighThreshold, test #%d\n",testCount);

	/*read threshold value */
	if(EXIT_FAILURE == tmp102_getHighThreshold(fd, &Thigh))
	{ ERROR_PRINT("test_HighThreshold write failed\n"); return EXIT_FAILURE; }
	else { INFO_PRINT("got Thigh value: %f\n", Thigh); }

	/* change it */
	start = Thigh;
	if(EXIT_FAILURE == tmp102_setHighThreshold(fd, Thigh - CHANGE))
	{ ERROR_PRINT("test_HighThreshold read failed\n"); return EXIT_FAILURE; }

	/* read it back */
	if(EXIT_FAILURE == tmp102_getHighThreshold(fd, &Thigh))
	{ ERROR_PRINT("test_HighThreshold write failed\n"); return EXIT_FAILURE; }
	else { INFO_PRINT("got Thigh value: %f\n", Thigh); }

	if((start - CHANGE) != Thigh)
	{ 
		ERROR_PRINT("test_HighThreshold read doesn't match written\n");
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}
/**
 * @brief read, write and verify write to fault queue register
 * @return int8_t test pass / fail (EXIT_FAILURE)
 */
int8_t test_FaultQueueSize(void)
{
	Tmp102_FaultCount_e startCount, tmpCount, newCount;
	testCount++;
	INFO_PRINT("\nstart of test_FaultQueueSize, test #%d\n",testCount);
	
	/*read value */
	if(EXIT_FAILURE == tmp102_getFaultQueueSize(fd, &startCount))
	{ ERROR_PRINT("test_FaultQueueSize read failed\n"); return EXIT_FAILURE; }
	else { INFO_PRINT("got startCount value: %d\n", startCount); }

	/* set new value */
	if((((uint8_t)startCount) + 1) >= TMP102_REQ_FAULT_END)
	{
		newCount = (Tmp102_FaultCount_e)((((uint8_t)startCount) + 1) 
		% ((uint8_t)(TMP102_REQ_FAULT_END - 1)));
	}
	else {
		newCount = startCount + 1;
	}
	
	/* write new value */
	if(EXIT_FAILURE == tmp102_setFaultQueueSize(fd, newCount))
	{ ERROR_PRINT("test_FaultQueueSize write failed\n"); return EXIT_FAILURE; }

	/* read it back */
	if(EXIT_FAILURE == tmp102_getFaultQueueSize(fd, &tmpCount))
	{ ERROR_PRINT("test_FaultQueueSize read failed\n"); return EXIT_FAILURE; }
	else { INFO_PRINT("got tmpCount value: %d\n", tmpCount); }

	/* verify read back value match write value */
	if(newCount != tmpCount)
	{ 
		ERROR_PRINT("test_FaultQueueSize read doesn't match written\n");
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}
/**
 * @brief set extended addressMode and verify temp reading is 
 * still correct, try to get temp above 128 DegC?
 * 
 * @return int8_t test pass / fail (EXIT_FAILURE)
 */
int8_t test_ExtendedAddrMode(void)
{
	Tmp102_AddrMode_e startMode, tmpMode, newMode;
	testCount++;
	INFO_PRINT("\nstart of test_ExtendedAddrMode, test #%d\n",testCount);
	
	/*read value */
	if(EXIT_FAILURE == tmp102_getExtendedMode(fd, &startMode))
	{ ERROR_PRINT("test_ExtendedAddrMode read failed\n"); return EXIT_FAILURE; }
	else { INFO_PRINT("got startMode value: %d\n", startMode); }

	/* set new value */
	if((((uint8_t)startMode) + 1) >= TMP102_ADDR_MODE_END)
	{
		newMode = (Tmp102_AddrMode_e)((((uint8_t)startMode) + 1) 
		% ((uint8_t)(TMP102_ADDR_MODE_END - 1)));
	}
	else {
		newMode = startMode + 1;
	}
	
	/* write new value */
	if(EXIT_FAILURE == tmp102_setExtendedMode(fd, newMode))
	{ ERROR_PRINT("test_ExtendedAddrMode write failed\n"); return EXIT_FAILURE; }

	/* read it back */
	if(EXIT_FAILURE == tmp102_getExtendedMode(fd, &tmpMode))
	{ ERROR_PRINT("test_ExtendedAddrMode read failed\n"); return EXIT_FAILURE; }
	else { INFO_PRINT("got tmpMode value: %d\n", tmpMode); }

	/* verify read back value match write value */
	if(newMode != tmpMode)
	{ 
		ERROR_PRINT("test_ExtendedAddrMode read doesn't match written\n");
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}
/**
 * @brief put sensor in shutdown mode, verify temp conversion
 * is disabled; verify conversion returns to normal when 
 * commanded out of shutdown mode.
 * 
 * @return int8_t test pass / fail (EXIT_FAILURE)
 */
int8_t test_Shutdown(void)
{
	Tmp102_Shutdown_e startState, tmpState, newState;
	testCount++;
	INFO_PRINT("\nstart of test_Shutdown, test #%d\n",testCount);
	
	/*read value */
	if(EXIT_FAILURE == tmp102_getShutdownState(fd, &startState))
	{ ERROR_PRINT("test_Shutdown read failed\n"); return EXIT_FAILURE; }
	else { INFO_PRINT("got startState value: %d\n", startState); }

	/* set new value */
	if((((uint8_t)startState) + 1) >= TMP102_DEVICE_IN_END)
	{
		newState = (Tmp102_Shutdown_e)((((uint8_t)startState) + 1) 
		% ((uint8_t)(TMP102_DEVICE_IN_END - 1)));
	}
	else {
		newState = startState + 1;
	}
	
	/* write new value */
	if(EXIT_FAILURE == tmp102_setShutdownState(fd, newState))
	{ ERROR_PRINT("test_Shutdown write failed\n"); return EXIT_FAILURE; }

	/* read it back */
	if(EXIT_FAILURE == tmp102_getShutdownState(fd, &tmpState))
	{ ERROR_PRINT("test_Shutdown read failed\n"); return EXIT_FAILURE; }
	else { INFO_PRINT("got tmpState value: %d\n", tmpState); }

	/* verify read back value match write value */
	if(newState != tmpState)
	{ 
		ERROR_PRINT("test_Shutdown read doesn't match written\n");
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}
/**
 * @brief change the conversion rate; verify by sampling at
 * high rate and inspect samples to see if they change when expected
 * 
 * @return int8_t test pass / fail (EXIT_FAILURE)
 */
int8_t test_ConversionRate(void)
{
	Tmp102_ConvRate_e startRate, tmpRate, newRate;
	testCount++;
	INFO_PRINT("\nstart of test_ConversionRate, test #%d\n",testCount);
	
	/*read value */
	if(EXIT_FAILURE == tmp102_getConvRate(fd, &startRate))
	{ ERROR_PRINT("test_ConversionRate read failed\n"); return EXIT_FAILURE; }
	else { INFO_PRINT("got startRate value: %d\n", startRate); }

	/* set new value */
	if((((uint8_t)startRate) + 1) >= TMP102_CONV_RATE_END)
	{
		newRate = (Tmp102_ConvRate_e)((((uint8_t)startRate) + 1) 
		% ((uint8_t)(TMP102_CONV_RATE_END - 1)));
	}
	else {
		newRate = startRate + 1;
	}
	
	/* write new value */
	if(EXIT_FAILURE == tmp102_setConvRate(fd, newRate))
	{ ERROR_PRINT("test_ConversionRate write failed\n"); return EXIT_FAILURE; }

	/* read it back */
	if(EXIT_FAILURE == tmp102_getConvRate(fd, &tmpRate))
	{ ERROR_PRINT("test_ConversionRate read failed\n"); return EXIT_FAILURE; }
	else { INFO_PRINT("got tmpRate value: %d\n", tmpRate); }

	/* verify read back value match write value */
	if(newRate != tmpRate)
	{ 
		ERROR_PRINT("test_ConversionRate read doesn't match written\n");
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}
/**
 * @brief set/clear polarity to verify alert active level changes
 * 
 * @return int8_t test pass / fail (EXIT_FAILURE)
 */
int8_t test_AlertPolarity(void)
{
	uint8_t startPol, tmpPol, newPol;
	Tmp102_Alert_e startAlert, newAlert;

	testCount++;
	INFO_PRINT("\nstart of test_AlertPolarity, test #%d\n",testCount);
	
	/*read start polarity value */
	if(EXIT_FAILURE == tmp102_getPolarity(fd, &startPol))
	{ ERROR_PRINT("test_AlertPolarity read failed\n"); return EXIT_FAILURE; }
	else { INFO_PRINT("got startPol value: %d\n", startPol); }

	/*read start alert value */
	if(EXIT_FAILURE == tmp102_getAlert(fd, &startAlert))
	{ ERROR_PRINT("test_AlertPolarity read failed\n"); return EXIT_FAILURE; }
	else { INFO_PRINT("got startAlert value: %d\n", startAlert); }

	/* set new value */
	newPol = !startPol;
	
	/* write new value */
	if(EXIT_FAILURE == tmp102_setPolarity(fd, newPol))
	{ ERROR_PRINT("test_AlertPolarity write failed\n"); return EXIT_FAILURE; }

	/* read it back */
	if(EXIT_FAILURE == tmp102_getPolarity(fd, &tmpPol))
	{ ERROR_PRINT("test_AlertPolarity read failed\n"); return EXIT_FAILURE; }
	else { INFO_PRINT("got tmpPol value: %d\n", tmpPol); }

	/*read start alert value */
	if(EXIT_FAILURE == tmp102_getAlert(fd, &newAlert))
	{ ERROR_PRINT("test_AlertPolarity read failed\n"); return EXIT_FAILURE; }
	else { INFO_PRINT("got newAlert value: %d\n", newAlert); }

	/* verify read back value match write value */
	if(newPol != tmpPol)
	{ 
		ERROR_PRINT("test_AlertPolarity polarity read doesn't match written\n");
		return EXIT_FAILURE;
	}

	/* verify alert compensates for polarity flip */
	if(newAlert != startAlert)
	{ 
		ERROR_PRINT("test_AlertPolarity alert read doesn't match written\n");
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}
