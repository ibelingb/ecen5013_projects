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
int8_t test_TimingGainReadWrite(void);
int8_t test_TimingIntReadWrite(void);
int8_t test_InterruptControl(void);
int8_t test_DeviceIdRead(void);
int8_t test_InterruptThresholdReadWrite(void);
int8_t test_LuxDataRead(void);

// TODO: Add negative test cases

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

  testFails += test_ControlReadWrite();
  testFails += test_TimingGainReadWrite();
  testFails += test_TimingIntReadWrite();
  //testFails += test_InterruptControl();
  testFails += test_DeviceIdRead();
  //testFails += test_InterruptThresholdReadWrite();
  //testFails += test_LuxDataRead();

	printf("\n\nTEST RESULTS, %d of %d failed tests\n", testFails, testCount);

	return EXIT_SUCCESS;
}

/*---------------------------------------------------------------------------------*/
/**
 * @brief Unit Test to read and write the Control register on the APDS9301 device
 * @return int8_t test pass / fail (EXIT_FAILURE)
 */
int8_t test_ControlReadWrite(void)
{
  Apds9301_PowerCtrl_e controlRegWrite;
  Apds9301_PowerCtrl_e controlRegRead;
	testCount++;
	INFO_PRINT("start of test_ControlReadWrite, test #%d\n",testCount);

	/* Set device to be powered off */
  controlRegWrite = APDS9301_CTRL_POWERDOWN;
	if(EXIT_FAILURE == apds9301_setControl(fd, controlRegWrite)) { 
    ERROR_PRINT("test_ControlReadWrite write failed\n"); 
    return EXIT_FAILURE; 
  }
	/* Read Control register */
	if(EXIT_FAILURE == apds9301_getControl(fd, &controlRegRead)) { 
    ERROR_PRINT("test_ControlReadWrite read failed\n"); 
    return EXIT_FAILURE; 
  } else { 
    INFO_PRINT("APDS9301 control register returned value: 0x%x\n", controlRegRead); 
  }
	/* Verify device is powered off */
	if(controlRegRead != APDS9301_CTRL_POWERDOWN)
	{ 
		ERROR_PRINT("test_ControlReadWrite failed to get APDS9301 state of 0x%x\n", APDS9301_CTRL_POWERDOWN);
		return EXIT_FAILURE;
	}


	/* Set device to be powered on */
  controlRegWrite = APDS9301_CTRL_POWERUP;
	if(EXIT_FAILURE == apds9301_setControl(fd, controlRegWrite)) { 
    ERROR_PRINT("test_ControlReadWrite write failed\n"); 
    return EXIT_FAILURE; 
  }
	/* Read Control register */
	if(EXIT_FAILURE == apds9301_getControl(fd, &controlRegRead)) { 
    ERROR_PRINT("test_ControlReadWrite read failed\n"); 
    return EXIT_FAILURE; 
  } else { 
    INFO_PRINT("APDS9301 control register returned value: 0x%x\n", controlRegRead); 
  }
	/* Verify device is powered on */
	if(controlRegRead != APDS9301_CTRL_POWERUP)
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
int8_t test_TimingGainReadWrite(void)
{
  Apds9301_TimingGain_e timingGainWrite;
  Apds9301_TimingGain_e timingGainRead;
	testCount++;
	INFO_PRINT("start of test_TimingGainReadWrite, test #%d\n",testCount);

  INFO_PRINT("Test read/write to Timing Gain\n");
	/* Set device gain to HIGH */
  timingGainWrite = APDS9301_TIMING_GAIN_HIGH;
	if(EXIT_FAILURE == apds9301_setTimingGain(fd, timingGainWrite)) { 
    ERROR_PRINT("test_TimingGainReadWrite write failed\n"); 
    return EXIT_FAILURE; 
  }
	/* Read Timing register for gain bit */
	if(EXIT_FAILURE == apds9301_getTimingGain(fd, &timingGainRead)) { 
    ERROR_PRINT("test_TimingGainReadWrite read failed\n"); 
    return EXIT_FAILURE; 
  } else { 
    INFO_PRINT("APDS9301 Timing register returned gain value: 0x%x\n", timingGainRead); 
  }
	/* Verify device gain is high */
	if(timingGainRead != APDS9301_TIMING_GAIN_HIGH)
	{ 
		ERROR_PRINT("test_TimingGainReadWrite failed to set/get APDS9301 gain to be 0x%x\n", APDS9301_TIMING_GAIN_HIGH);
		return EXIT_FAILURE;
	}

	/* Set device gain to LOW */
  timingGainWrite = APDS9301_TIMING_GAIN_LOW;
	if(EXIT_FAILURE == apds9301_setTimingGain(fd, timingGainWrite)) { 
    ERROR_PRINT("test_TimingGainReadWrite write failed\n"); 
    return EXIT_FAILURE; 
  }
	/* Read Timing register for gain bit */
	if(EXIT_FAILURE == apds9301_getTimingGain(fd, &timingGainRead)) { 
    ERROR_PRINT("test_TimingGainReadWrite read failed\n"); 
    return EXIT_FAILURE; 
  } else { 
    INFO_PRINT("APDS9301 Timing register returned gain value: 0x%x\n", timingGainRead); 
  }
	/* Verify device gain is high */
	if(timingGainRead != APDS9301_TIMING_GAIN_LOW)
	{ 
		ERROR_PRINT("test_TimingGainReadWrite failed to set/get APDS9301 gain to be 0x%x\n", APDS9301_TIMING_GAIN_LOW);
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}

/*---------------------------------------------------------------------------------*/
/**
 * @brief TODO
 * @return int8_t test pass / fail (EXIT_FAILURE)
 */
int8_t test_TimingIntReadWrite(void)
{
  Apds9301_TimingInt_e timingIntWrite;
  Apds9301_TimingInt_e timingIntRead;
	testCount++;
	INFO_PRINT("start of test_TimingIntReadWrite, test #%d\n",testCount);

  INFO_PRINT("Test read/write to Timing Integration\n");
	/* Set device Integration time to 13.7 msec */
  timingIntWrite = APDS9301_TIMING_INT_13P7;
	if(EXIT_FAILURE == apds9301_setTimingIntegration(fd, timingIntWrite)) { 
    ERROR_PRINT("test_TimingIntReadWrite write failed\n"); 
    return EXIT_FAILURE; 
  }
	/* Read Timing register for integration bit */
	if(EXIT_FAILURE == apds9301_getTimingIntegration(fd, &timingIntRead)) { 
    ERROR_PRINT("test_TimingIntReadWrite read failed\n"); 
    return EXIT_FAILURE; 
  } else { 
    INFO_PRINT("APDS9301 Timing register returned integration value: 0x%x\n", timingIntRead); 
  }
	/* Verify device integration is 13.7 msec */
	if(timingIntRead != APDS9301_TIMING_INT_13P7)
	{ 
		ERROR_PRINT("test_TimingIntReadWrite failed to set/get APDS9301 integration to be 0x%x\n", APDS9301_TIMING_INT_13P7);
		return EXIT_FAILURE;
	}

	/* Set device Integration time to 101 msec */
  timingIntWrite = APDS9301_TIMING_INT_101;
	if(EXIT_FAILURE == apds9301_setTimingIntegration(fd, timingIntWrite)) { 
    ERROR_PRINT("test_TimingIntReadWrite write failed\n"); 
    return EXIT_FAILURE; 
  }
	/* Read Timing register for integration bit */
	if(EXIT_FAILURE == apds9301_getTimingIntegration(fd, &timingIntRead)) { 
    ERROR_PRINT("test_TimingIntReadWrite read failed\n"); 
    return EXIT_FAILURE; 
  } else { 
    INFO_PRINT("APDS9301 Timing register returned integration value: 0x%x\n", timingIntRead); 
  }
	/* Verify device integration is 101 msec */
	if(timingIntRead != APDS9301_TIMING_INT_101)
	{
		ERROR_PRINT("test_TimingIntReadWrite failed to set/get APDS9301 integration to be 0x%x\n", APDS9301_TIMING_INT_101);
		return EXIT_FAILURE;
	}

	/* Set device Integration time to 402 msec */
  timingIntWrite = APDS9301_TIMING_INT_402;
	if(EXIT_FAILURE == apds9301_setTimingIntegration(fd, timingIntWrite)) { 
    ERROR_PRINT("test_TimingIntReadWrite write failed\n"); 
    return EXIT_FAILURE; 
  }
	/* Read Timing register for integration bit */
	if(EXIT_FAILURE == apds9301_getTimingIntegration(fd, &timingIntRead)) { 
    ERROR_PRINT("test_TimingIntReadWrite read failed\n"); 
    return EXIT_FAILURE; 
  } else { 
    INFO_PRINT("APDS9301 Timing register returned integration value: 0x%x\n", timingIntRead); 
  }
	/* Verify device integration is 402 msec */
	if(timingIntRead != APDS9301_TIMING_INT_402)
	{ 
		ERROR_PRINT("test_TimingIntReadWrite failed to set/get APDS9301 integration to be 0x%x\n", APDS9301_TIMING_INT_402);
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}
/*---------------------------------------------------------------------------------*/
/**
 * @brief TODO
 * @return int8_t test pass / fail (EXIT_FAILURE)
 */
int8_t test_InterruptControl(void)
{
  Apds9301_IntSelect_e intSelect;
  Apds9301_IntPersist_e intPersist;

	testCount++;
	INFO_PRINT("start of test_InterruptControl, test #%d\n",testCount);
  INFO_PRINT("Test read/write to Interrupt Control\n");

	/* Set device Interrupt Control to have Level disabled, Interrupt every 15 periods */
  intSelect = APDS9301_INT_SELECT_LEVEL_DISABLE;
  intPersist = APDS9301_INT_PERSIST_OUTSIDE_15P;
	if(EXIT_FAILURE == apds9301_setInterruptControl(fd, intSelect, intPersist)) { 
    ERROR_PRINT("test_InterruptControl  write failed\n"); 
    return EXIT_FAILURE; 
  }
	/* Read Interrupt Control reg to receive Interrupt select and persist info */
	if(EXIT_FAILURE == apds9301_getInterruptControl(fd, &intSelect, &intPersist)) { 
    ERROR_PRINT("test_InterruptControl read failed\n"); 
    return EXIT_FAILURE; 
  } else { 
    INFO_PRINT("APDS9301 Interrupt Control register returned Select value: 0x%x | Persist value: 0x%x\n", intSelect, intPersist); 
  }
	/* Verify device Interrupt select and Persist set to specified values */
	if(intSelect != APDS9301_INT_SELECT_LEVEL_DISABLE)
	{ 
		ERROR_PRINT("test_InterruptControl failed to set/get APDS9301 Interrupt Select value to be 0x%x\n", APDS9301_INT_SELECT_LEVEL_DISABLE);
		return EXIT_FAILURE;
	}
	if(intPersist != APDS9301_INT_PERSIST_OUTSIDE_15P)
	{ 
		ERROR_PRINT("test_InterruptControl failed to set/get APDS9301 Interrupt Persist value to be 0x%x\n", APDS9301_INT_PERSIST_EVERY_CYCLE);
		return EXIT_FAILURE;
	}
	

	/* Set device Interrupt Control to have Level enabled, Interrupt every 7 periods */
  intSelect = APDS9301_INT_SELECT_LEVEL_ENABLE;
  intPersist = APDS9301_INT_PERSIST_OUTSIDE_7P;
	if(EXIT_FAILURE == apds9301_setInterruptControl(fd, intSelect, intPersist)) { 
    ERROR_PRINT("test_InterruptControl  write failed\n"); 
    return EXIT_FAILURE; 
  }
	/* Read Interrupt Control reg to receive Interrupt select and persist info */
	if(EXIT_FAILURE == apds9301_getInterruptControl(fd, &intSelect, &intPersist)) { 
    ERROR_PRINT("test_InterruptControl read failed\n"); 
    return EXIT_FAILURE; 
  } else { 
    INFO_PRINT("APDS9301 Interrupt Control register returned Select value: 0x%x | Persist value: 0x%x\n", intSelect, intPersist); 
  }
	/* Verify device Interrupt select and Persist set to specified values */
	if(intSelect != APDS9301_INT_SELECT_LEVEL_ENABLE)
	{ 
		ERROR_PRINT("test_InterruptControl failed to set/get APDS9301 Interrupt Select value to be 0x%x\n", APDS9301_INT_SELECT_LEVEL_DISABLE);
		return EXIT_FAILURE;
	}
	if(intPersist != APDS9301_INT_PERSIST_OUTSIDE_7P)
	{ 
		ERROR_PRINT("test_InterruptControl failed to set/get APDS9301 Interrupt Persist value to be 0x%x\n", APDS9301_INT_PERSIST_EVERY_CYCLE);
		return EXIT_FAILURE;
	}

	/* Set device Interrupt Control to have Level disabled, Interrupt every cycle */
  intSelect = APDS9301_INT_SELECT_LEVEL_DISABLE;
  intPersist = APDS9301_INT_PERSIST_EVERY_CYCLE;
	if(EXIT_FAILURE == apds9301_setInterruptControl(fd, intSelect, intPersist)) { 
    ERROR_PRINT("test_InterruptControl  write failed\n"); 
    return EXIT_FAILURE; 
  }
	/* Read Interrupt Control reg to receive Interrupt select and persist info */
	if(EXIT_FAILURE == apds9301_getInterruptControl(fd, &intSelect, &intPersist)) { 
    ERROR_PRINT("test_InterruptControl read failed\n"); 
    return EXIT_FAILURE; 
  } else { 
    INFO_PRINT("APDS9301 Interrupt Control register returned Select value: 0x%x | Persist value: 0x%x\n", intSelect, intPersist); 
  }
	/* Verify device Interrupt select and Persist set to specified values */
	if(intSelect != APDS9301_INT_SELECT_LEVEL_DISABLE)
	{ 
		ERROR_PRINT("test_InterruptControl failed to set/get APDS9301 Interrupt Select value to be 0x%x\n", APDS9301_INT_SELECT_LEVEL_DISABLE);
		return EXIT_FAILURE;
	}
	if(intPersist != APDS9301_INT_PERSIST_EVERY_CYCLE)
	{ 
		ERROR_PRINT("test_InterruptControl failed to set/get APDS9301 Interrupt Persist value to be 0x%x\n", APDS9301_INT_PERSIST_EVERY_CYCLE);
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}

/*---------------------------------------------------------------------------------*/
/**
 * @brief TODO
 * @return int8_t test pass / fail (EXIT_FAILURE)
 */
int8_t test_DeviceIdRead(void)
{
  uint8_t partNo = 0;
  uint8_t revNo = 0;
	testCount++;
	INFO_PRINT("start of test_DeviceIdRead, test #%d\n",testCount);
	
	/* Read Device ID */
	if(EXIT_FAILURE == apds9301_getDeviceId(fd, &partNo, &revNo)) { 
    ERROR_PRINT("test_DeviceIdRead read failed\n"); 
    return EXIT_FAILURE; 
  } else { 
    INFO_PRINT("APDS9301 returned Part No value: 0x%x | Rev No value: 0x%x\n", partNo, revNo); 
  }

	/* verify values returned */
	if(partNo != APDS9301_PARTNO)
	{ 
		ERROR_PRINT("test_DeviceIdRead read failed to get APDS9301 Part Number of 0x%x\n", APDS9301_PARTNO);
		return EXIT_FAILURE;
	}
	if(revNo != 0x00)
	{ 
		ERROR_PRINT("test_DeviceIdRead read failed to get APDS9301 Rev Number of 0x%x\n", APDS9301_PARTNO);
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
