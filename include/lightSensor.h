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

/*---------------------------------------------------------------------------------*/
/**
 * @brief TODO
 *
 * @param file
 * @param luxData0
 *
 * @return 
 */
int8_t apds9301_getLuxData0(uint8_t file, uint8_t luxData0);

/**
 * @brief TODO
 *
 * @param file
 * @param luxData1
 *
 * @return 
 */
int8_t apds9301_getLuxData1(uint8_t file, uint8_t luxData1);

/**
 * @brief TODO
 *
 * @param file
 * @param config
 *
 * @return 
 */
int8_t apds9301_getConfig(uint8_t file, uint8_t *config);

/**
 * @brief TODO
 *
 * @param file
 * @param Timing
 *
 * @return 
 */
int8_t apds9301_getTiming(uint8_t file, uint8_t *Timing);

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
int8_t apds9301_getInterruptThreshold(uint8_t file, uint8_t *intThreshold);

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
 * @param config
 *
 * @return 
 */
int8_t apds9301_setConfig(uint8_t file, uint8_t config);

/**
 * @brief Set APDS9301 device timing integration and gain
 *
 * @param file
 * @param timing
 *
 * @return 
 */
int8_t apds9301_setTiming(uint8_t file, uint8_t timing);

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
