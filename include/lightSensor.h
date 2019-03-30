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
 * https://learn.sparkfun.com/tutorials/apds-9301-sensor-hookup-guide
 *
 ************************************************************************************
 */

#ifndef LIGHT_SENSOR_H_
#define LIGHT_SENSOR_H_

#include <stdint.h>
#include <stdbool.h>
#include <math.h>

#define APDS9301_PARTNO (0x05)
#define LIGHT_DARK_THRESHOLD (150) /* Lux sensor value to transition between Light and Dark states */

/*---------------------------------------------------------------------------------*/
/* */
typedef enum
{
  APDS9301_CTRL_POWERDOWN = 0,
  APDS9301_CTRL_POWERUP = 3
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

typedef enum
{
  APDS9301_INT_SELECT_LEVEL_DISABLE = 0, /* Level Interrupt output disabled */
  APDS9301_INT_SELECT_LEVEL_ENABLE       /* Level Interrupt output enabled */
} Apds9301_IntSelect_e;

typedef enum
{
  APDS9301_INT_PERSIST_EVERY_CYCLE = 0, /* Every ADC cycle generates interrupt */
  APDS9301_INT_PERSIST_OUTSIDE_CYCLE, /* Any value outside of threshold range */
  APDS9301_INT_PERSIST_OUTSIDE_2P,  /* 2 integration time periods out of range */
  APDS9301_INT_PERSIST_OUTSIDE_3P,  /* 3 integration time periods out of range */
  APDS9301_INT_PERSIST_OUTSIDE_4P,  /* 4 integration time periods out of range */
  APDS9301_INT_PERSIST_OUTSIDE_5P,  /* 5 integration time periods out of range */
  APDS9301_INT_PERSIST_OUTSIDE_6P,  /* 6 integration time periods out of range */
  APDS9301_INT_PERSIST_OUTSIDE_7P,  /* 7 integration time periods out of range */
  APDS9301_INT_PERSIST_OUTSIDE_8P,  /* 8 integration time periods out of range */
  APDS9301_INT_PERSIST_OUTSIDE_9P,  /* 9 integration time periods out of range */
  APDS9301_INT_PERSIST_OUTSIDE_10P,  /* 10 integration time periods out of range */
  APDS9301_INT_PERSIST_OUTSIDE_11P,  /* 11 integration time periods out of range */
  APDS9301_INT_PERSIST_OUTSIDE_12P,  /* 12 integration time periods out of range */
  APDS9301_INT_PERSIST_OUTSIDE_13P,  /* 13 integration time periods out of range */
  APDS9301_INT_PERSIST_OUTSIDE_14P,  /* 14 integration time periods out of range */
  APDS9301_INT_PERSIST_OUTSIDE_15P,  /* 15 integration time periods out of range */
} Apds9301_IntPersist_e;

typedef enum
{
  LUX_STATE_LIGHT = 0,
  LUX_STATE_DARK
} LightState_e;

/*---------------------------------------------------------------------------------*/
/**
 * @brief Get Lux data from connected APDS9301 device via I2C bus.
 *
 * @param file - File handle for I2C bus.
 * @param luxData - Pointer to return converted Lux data calculated from APDS9301 Sensor
 *
 * @return - Success or Failure status of get method
 */
int8_t apds9301_getLuxData(uint8_t file, float *luxData);

/**
 * @brief Get Config register data from connected APDS9301 device via I2C bus.
 *
 * @param file - File handle for I2C bus.
 * @param config - Pointer to return Power bits from Control Register.
 *
 * @return - Success or Failure status of get method
 */ // TODO: Update name to be apds9301_getPowerState
int8_t apds9301_getControl(uint8_t file, Apds9301_PowerCtrl_e *control);

/**
 * @brief Get Timing register Integration data from connected APDS9301 device via I2C bus.
 *
 * @param file - File handle for I2C bus.
 * @param integration - Pointer to return Intergration bits from Timing register.
 *
 * @return - Success or Failure status of get method
 */
int8_t apds9301_getTimingIntegration(uint8_t file, Apds9301_TimingInt_e *integration);

/**
 * @brief Get Timing register Gain data from connected APDS9301 device via I2C bus.
 *
 * @param file - File handle for I2C bus.
 * @param gain - Pointer to return Gain bits from Timing register.
 *
 * @return - Success or Failure status of get method
 */
int8_t apds9301_getTimingGain(uint8_t file, Apds9301_TimingGain_e *gain);

/**
 * @brief Get Device register Part and Rev numbers from connected APDS9301 device via I2C bus.
 *
 * @param file - File handle for I2C bus.
 * @param partNo - Pointer to return Part Number of APDS9301 from Device ID register
 * @param revNo - Pointer to return Revision Number of APDS9301 from Device ID register
 *
 * @return - Success or Failure status of get method
 */
int8_t apds9301_getDeviceId(uint8_t file, uint8_t *partNo, uint8_t *revNo);


/**
 * @brief Get Device Low Interrupt Threshold from connected APDS9301 device via I2C bus.
 *
 * @param file - File handle for I2C bus.
 * @param intThreshold - Pointer to return 16-bit Low Interrupt Threshold value set.
 *
 * @return - Success or Failure status of get method
 */
int8_t apds9301_getLowIntThreshold(uint8_t file, uint16_t *intThreshold);

/**
 * @brief Get Device High Interrupt Threshold from connected APDS9301 device via I2C bus.
 *
 * @param file - File handle for I2C bus.
 * @param intThreshold - Pointer to return 16-bit High Interrupt Threshold value set.
 *
 * @return - Success or Failure status of get method
 */
int8_t apds9301_getHighIntThreshold(uint8_t file, uint16_t *intThreshold);

/**
 * @brief Get Device Interrupt Control Select and Persist register data from connected APDS9301 
 *        device via I2C bus.
 *
 * @param file - File handle for I2C bus.
 * @param intSelect - Pointer to return Interrupt Level Select bits for enable/disable setting.
 * @param persist - Pointer to return Interrupt Persist bits set for number of integration cycles
                    outside of threshold window to trigger interrupt.
 *
 * @return - Success or Failure status of get method
 */
int8_t apds9301_getInterruptControl(uint8_t file, Apds9301_IntSelect_e *intSelect, Apds9301_IntPersist_e *persist);
/*---------------------------------------------------------------------------------*/

/**
 * @brief - Sets Power bits of Control Register to power on/off device.
 *
 * @param file - File handle for I2C bus.
 * @param control - Power state value to set to Control Register.
 *
 * @return - Success or Failure status of set method
 */ // TODO: Update name to be apds9301_setPowerState
int8_t apds9301_setControl(uint8_t file, Apds9301_PowerCtrl_e control);

/**
 * @brief - Set Integration Gain on APDS9301 device to be high (x16) or low (x1).
 *
 * @param file - File handle for I2C bus.
 * @param gain - Timing Gain value to set to Timing Register.
 *
 * @return - Success or Failure status of set method
 */
int8_t apds9301_setTimingGain(uint8_t file, Apds9301_TimingGain_e gain);

/**
 * @brief - Set Integration Timing on APDS9301 device.
 *
 * @param file - File handle for I2C bus.
 * @param integration - Timing Integration value to set to Timing Register.
 *
 * @return - Success or Failure status of set method
 */
int8_t apds9301_setTimingIntegration(uint8_t file, Apds9301_TimingInt_e integration);

/**
 * @brief - Set Interrupt controls for Select bits and Persist bits.
 *
 * @param file - File handle for I2C bus.
 * @param intSelect - Interrupt Select value to set to Interrupt Control register.
 * @param persist - Interrupt Persist value to set to Interrupt Control register.
 *
 * @return - Success or Failure status of set method
 */
int8_t apds9301_setInterruptControl(uint8_t file, Apds9301_IntSelect_e intSelect, Apds9301_IntPersist_e persist);

/**
 * @brief - Clears interrupt on APDS9301 by writing to CLEAR bit of Command register.
 *
 * @param file - File handle for I2C bus.
 *
 * @return - Success or Failure status of set method
 */
int8_t apds9301_clearInterrupt(uint8_t file);

/**
 * @brief - Set Low Interrupt threshold value to trigger interrupt from Lux data value.
 *
 * @param file - File handle for I2C bus.
 * @param intThreshold - High interrupt threshold value to set.
 *
 * @return - Success or Failure status of set method
 */
int8_t apds9301_setLowIntThreshold(uint8_t file, uint16_t intThreshold);

/**
 * @brief - Set High Interrupt threshold value to trigger interrupt from Lux data value.
 *
 * @param file - File handle for I2C bus.
 * @param intThreshold - High interrupt threshold value to set.
 *
 * @return - Success or Failure status of set method
 */
int8_t apds9301_setHighIntThreshold(uint8_t file, uint16_t intThreshold);

#endif /* LIGHT_SENSOR_H_ */
