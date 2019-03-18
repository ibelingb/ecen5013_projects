/***********************************************************************************
 * @author Brian Ibeling
 * brian.ibeling@colorado.edu
 * Advanced Embedded Software Development
 * ECEN5013 - Rick Heidebrecht
 * @date March 16, 2019
 * arm-linux-gnueabi (Buildroot)
 * gcc (Ubuntu)
 ************************************************************************************
 *
 * @file test_light.c
 * @brief test cases for Light library / sensor
 *
 ************************************************************************************
 */

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <stdint.h>

#include "lu_iic.h"
#include "lightSensor.h"
#include "debug.h"

#define LIGHT_SAMPLES	(4)

/* global file descriptor */
int fd;
uint8_t testCount = 0;

/* test cases */
int8_t test_CommandWrite(void);
int8_t test_ControlReadWrite(void);
int8_t test_TimingReadWrite(void);
int8_t test_InterruptCtrlEnDs(void);
int8_t test_DeviceIdRead(void);
int8_t test_InterruptThresholdReadWrite(void);
int8_t test_LuxDataRead(void);

/*---------------------------------------------------------------------------------*/
int main(void)
{
	printf("test cases for APDS9301 light sensor library\n");
	
	/* keep track of failures */
	uint8_t testFails = 0;

	/* init handle for i2c bus */
	fd = initIic("/dev/i2c-2");
	if(fd < 0)
		printf("i2c init failed\n");

	sleep(2);

  //testFails += test_CommandWrite();
  testFails += test_ControlReadWrite();
  testFails += test_TimingReadWrite();
  //testFails += test_InterruptCtrlEnDs();
  testFails += test_DeviceIdRead();
  //testFails += test_InterruptThresholdReadWrite();
  //testFails += test_LuxDataRead();

	printf("\n\nTEST RESULTS, %d of %d failed tests\n", testFails, testCount);

	return EXIT_SUCCESS;
}

/*---------------------------------------------------------------------------------*/
/**
 * @brief TODO
 * @return int8_t test pass / fail (EXIT_FAILURE)
 */
int8_t test_CommandWrite(void)
{
  //uint8_t tmp = 0;
	testCount++;
	INFO_PRINT("start of test_CommandWrite, test #%d\n",testCount);

  //TODO
	
	return EXIT_SUCCESS;
}

/*---------------------------------------------------------------------------------*/
/**
 * @brief Unit Test to read and write the Control register on the APDS9301 device
 * @return int8_t test pass / fail (EXIT_FAILURE)
 */
int8_t test_ControlReadWrite(void)
{
  Apds9301_PowerCtrl_e controlReg;
	testCount++;
	INFO_PRINT("start of test_ControlReadWrite, test #%d\n",testCount);

	/* Set device to be powered off */
  controlReg = APDS9301_CTRL_POWERDOWN;
	if(EXIT_FAILURE == apds9301_setControl(fd, controlReg)) { 
    ERROR_PRINT("test_ControlReadWrite write failed\n"); 
    return EXIT_FAILURE; 
  }
	/* Read Control register */
	if(EXIT_FAILURE == apds9301_getControl(fd, &controlReg)) { 
    ERROR_PRINT("test_ControlReadWrite read failed\n"); 
    return EXIT_FAILURE; 
  } else { 
    INFO_PRINT("APDS9301 control register returned value: 0x%x\n", controlReg); 
  }
	/* Verify device is powered off */
	if(controlReg != APDS9301_CTRL_POWERDOWN)
	{ 
		ERROR_PRINT("test_ControlReadWrite failed to get APDS9301 state of 0x%x\n", APDS9301_CTRL_POWERDOWN);
		return EXIT_FAILURE;
	}


	/* Set device to be powered on */
  controlReg = APDS9301_CTRL_POWERUP;
	if(EXIT_FAILURE == apds9301_setControl(fd, controlReg)) { 
    ERROR_PRINT("test_ControlReadWrite write failed\n"); 
    return EXIT_FAILURE; 
  }
	/* Read Control register */
	if(EXIT_FAILURE == apds9301_getControl(fd, &controlReg)) { 
    ERROR_PRINT("test_ControlReadWrite read failed\n"); 
    return EXIT_FAILURE; 
  } else { 
    INFO_PRINT("APDS9301 control register returned value: 0x%x\n", controlReg); 
  }
	/* Verify device is powered on */
	if(controlReg != APDS9301_CTRL_POWERUP)
	{ 
		ERROR_PRINT("test_ControlReadWrite failed to get APDS9301 state of 0x%x\n", APDS9301_CTRL_POWERUP);
		return EXIT_FAILURE;
	}
	
	return EXIT_SUCCESS;
}

/*---------------------------------------------------------------------------------*/
/**
 * @brief TODO
 * @return int8_t test pass / fail (EXIT_FAILURE)
 */
int8_t test_TimingReadWrite(void)
{
  Apds9301_TimingGain_e timingGain;
  //Apds9301_TimingInt_e timingInt;
	testCount++;
	INFO_PRINT("start of test_TimingReadWrite, test #%d\n",testCount);

  INFO_PRINT("Test read/write to Timing Gain\n");
	/* Set device gain to HIGH */
  timingGain = APDS9301_TIMING_GAIN_HIGH;
	if(EXIT_FAILURE == apds9301_setTimingGain(fd, timingGain)) { 
    ERROR_PRINT("test_TimingReadWrite write failed\n"); 
    return EXIT_FAILURE; 
  }
	/* Read Control register */
	if(EXIT_FAILURE == apds9301_getTimingGain(fd, &timingGain)) { 
    ERROR_PRINT("test_TimingReadWrite read failed\n"); 
    return EXIT_FAILURE; 
  } else { 
    INFO_PRINT("APDS9301 Timing register returned value: 0x%x\n", timingGain); 
  }
	/* Verify device gain is high */
	if(timingGain != APDS9301_TIMING_GAIN_HIGH)
	{ 
		ERROR_PRINT("test_TimingReadWrite failed to set/get APDS9301 gain to be 0x%x\n", APDS9301_TIMING_GAIN_HIGH);
		return EXIT_FAILURE;
	}


  INFO_PRINT("Test read/write to Timing Integration\n");
  //TODO - Gain 
	
	return EXIT_SUCCESS;
}

/*---------------------------------------------------------------------------------*/
/**
 * @brief TODO
 * @return int8_t test pass / fail (EXIT_FAILURE)
 */
int8_t test_InterruptCtrlEnDs(void)
{
  //uint8_t tmp = 0;
	testCount++;
	INFO_PRINT("start of test_InterruptCtrlEnDs, test #%d\n",testCount);

  //TODO
	
	return EXIT_SUCCESS;
}

/*---------------------------------------------------------------------------------*/
/**
 * @brief TODO
 * @return int8_t test pass / fail (EXIT_FAILURE)
 */
int8_t test_DeviceIdRead(void)
{
  uint8_t deviceId = 0;
	testCount++;
	INFO_PRINT("start of test_DeviceIdRead, test #%d\n",testCount);
	
	/* Read Device ID */
	if(EXIT_FAILURE == apds9301_getDeviceId(fd, &deviceId)) { 
    ERROR_PRINT("test_DeviceIdRead read failed\n"); 
    return EXIT_FAILURE; 
  } else { 
    INFO_PRINT("APDS9301 returned DeviceID value: 0x%x\n", deviceId); 
  }

	/* verify read back value greater than 0 */
	if(deviceId != 0x50)
	{ 
		ERROR_PRINT("test_DeviceIdRead read failed to get APDS9301 Device ID of 0x50\n");
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}

/*---------------------------------------------------------------------------------*/
/**
 * @brief TODO
 * @return int8_t test pass / fail (EXIT_FAILURE)
 */
int8_t test_InterruptThresholdReadWrite(void)
{
  //uint8_t tmp = 0;
	testCount++;
	INFO_PRINT("start of test_InterruptThresholdReadWrite, test #%d\n",testCount);

  //TODO
	
	return EXIT_SUCCESS;
}

/*---------------------------------------------------------------------------------*/
/**
 * @brief TODO
 * @return int8_t test pass / fail (EXIT_FAILURE)
 */
int8_t test_LuxDataRead(void)
{
  //uint8_t tmp = 0;
	testCount++;
	INFO_PRINT("start of test_LuxDataRead, test #%d\n",testCount);
	
	return EXIT_SUCCESS;
}

/*---------------------------------------------------------------------------------*/
