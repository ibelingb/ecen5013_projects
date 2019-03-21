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

#define APDS9301_PARTNO (0x05)

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

/*---------------------------------------------------------------------------------*/
/**
 * @brief 
 *
 * @param file
 * @param luxData
 *
 * @return 
 */
int8_t apds9301_getLuxData(uint8_t file, float *luxData);

/**
 * @brief TODO
 *
 * @param file
 * @param config
 *
 * @return 
 */ // TODO: Update name to be apds9301_getPowerState
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
int8_t apds9301_getDeviceId(uint8_t file, uint8_t *partNo, uint8_t *revNo);


/**
 * @brief 
 *
 * @param file
 * @param intThreshold
 *
 * @return 
 */
int8_t apds9301_getLowIntThreshold(uint8_t file, uint16_t *intThreshold);

/**
 * @brief 
 *
 * @param file
 * @param intThreshold
 *
 * @return 
 */
int8_t apds9301_getHighIntThreshold(uint8_t file, uint16_t *intThreshold);

/**
 * @brief 
 *
 * @param file
 * @param intSelect
 * @param persist
 *
 * @return 
 */
int8_t apds9301_getInterruptControl(uint8_t file, Apds9301_IntSelect_e *intSelect, Apds9301_IntPersist_e *persist);
/*---------------------------------------------------------------------------------*/

/**
 * @brief TODO
 *
 * @param file
 * @param control
 *
 * @return 
 */ // TODO: Update name to be apds9301_setPowerState
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
 * @brief * * @param file
 * @param intSelect
 * @param persist
 *
 * @return 
 */
int8_t apds9301_setInterruptControl(uint8_t file, Apds9301_IntSelect_e intSelect, Apds9301_IntPersist_e persist);

/**
 * @brief 
 *
 * @param file
 *
 * @return 
 */
int8_t apds9301_clearInterrupt(uint8_t file);

/**
 * @brief 
 *
 * @param file
 * @param intThreshold
 *
 * @return 
 */
int8_t apds9301_setLowIntThreshold(uint8_t file, uint16_t intThreshold);

/**
 * @brief 
 *
 * @param file
 * @param intThreshold
 *
 * @return 
 */
int8_t apds9301_setHighIntThreshold(uint8_t file, uint16_t intThreshold);

#endif /* LIGHT_SENSOR_H_ */
