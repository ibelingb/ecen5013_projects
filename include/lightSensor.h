/***********************************************************************************
 * @author Brian Ibeling
 * Brian.Ibeling@colorado.edu
 * Advanced Embedded Software Development
 * ECEN5013 - Rick Heidebrecht
 * @date March 8, 2019
 * arm-linux-gnueabi (Buildroot)
 * gcc (Ubuntu)
 ************************************************************************************
 *
 * @file lightSensor.h
 * @brief Light sensor library for APDS-9301 device
 *
 * https://www.sparkfun.com/products/14350
 *
 ************************************************************************************
 */

#ifndef LIGHT_SENSOR_H_
#define LIGHT_SENSOR_H_

#include <stdint.h>
#include <stdbool.h>

/*---------------------------------------------------------------------------------*/
/* */
typedef enum
{
  APDS9301_CTRL_POWERUP = 0,
  APDS9301_CTRL_POWERDOWN
} Apds9301_PowerCtrl_e;

typedef enum
{
  APDS9301_TIMING_GAIN_LOW = 0, /* Sets gain to be 1x */
  APDS9301_TIMING_GAIN_HIGH     /* Sets gain to be 16x */
} Apds9301_TimingGain_e;

typedef enum
{
  APDS9301_TIMING_INT_13P7 = 0, /* Nominal integration time of 13.7 msec */
  APDS9301_TIMING_INT_101,      /* Nominal integration time of 101 msec */
  APDS9301_TIMING_INT_402       /* Nominal integration time of 402 msec */
} Apds9301_TimingInt_e;

/* Enum used to specify which Interrupt Threshold byte is being set/get */
typedef enum
{
  APDS9301_LOWTHRES_LOWBYTE = 0,
  APDS9301_LOWTHRES_HIGHBYTE,
  APDS9301_HIGHTHRES_LOWBYTE,
  APDS9301_HIGHTHRES_HIGHBYTE,
} Apds9301_IntThresholdByte_e;

/*---------------------------------------------------------------------------------*/
/**
 * @brief 
 *
 * @param file
 * @param luxData0Low
 * @param luxData0High
 *
 * @return 
 */
int8_t apds9301_getLuxData0(uint8_t file, uint8_t *luxData0Low, uint8_t *luxData0High);

/**
 * @brief 
 *
 * @param file
 * @param luxData1Low
 * @param luxData1High
 *
 * @return 
 */
int8_t apds9301_getLuxData1(uint8_t file, uint8_t *luxData1Low, uint8_t *luxData1High);

/**
 * @brief TODO
 *
 * @param file
 * @param config
 *
 * @return 
 */
int8_t apds9301_getControl(uint8_t file, Apds9301_PowerCtrl_e *control);

/**
 * @brief TODO
 *
 * @param file
 * @param integration
 *
 * @return 
 */
int8_t apds9301_getTimingIntegration(uint8_t file, Apds9301_TimingInt_e *integration);

/**
 * @brief TODO
 *
 * @param file
 * @param gain 
 *
 * @return 
 */
int8_t apds9301_getTimingGain(uint8_t file, Apds9301_TimingGain_e *gain);

/**
 * @brief TODO
 *
 * @param file
 * @param deviceId
 *
 * @return 
 */
int8_t apds9301_getDeviceId(uint8_t file, uint8_t *deviceId);

/**
 * @brief TODO
 *
 * @param file
 * @param intThreshold
 *
 * @return 
 */
int8_t apds9301_getInterruptThreshold(uint8_t file, Apds9301_IntThresholdByte_e intByte, uint8_t *intThreshold);

/*---------------------------------------------------------------------------------*/

/**
 * @brief TODO
 *
 * @param file
 * @param cmd
 *
 * @return 
 */
int8_t apds9301_setCommand(uint8_t file, uint8_t cmd);

/**
 * @brief TODO
 *
 * @param file
 * @param control
 *
 * @return 
 */
int8_t apds9301_setControl(uint8_t file, Apds9301_PowerCtrl_e control);

/**
 * @brief 
 *
 * @param file
 * @param gain
 *
 * @return 
 */
int8_t apds9301_setTimingGain(uint8_t file, Apds9301_TimingGain_e gain);

/**
 * @brief 
 *
 * @param file
 * @param integration
 *
 * @return 
 */
int8_t apds9301_setTimingIntegration(uint8_t file, Apds9301_TimingInt_e integration);

/**
 * @brief TODO
 *
 * @param file
 * @param state
 *
 * @return 
 */
int8_t apds9301_setInterruptControl(uint8_t file, bool state);

/**
 * @brief TODO
 *
 * @param file
 * @param intThreshold
 *
 * @return 
 */
int8_t apds9301_setInterruptThreshold(uint8_t file, uint8_t intThreshold);

#endif /* LIGHT_SENSOR_H_ */
