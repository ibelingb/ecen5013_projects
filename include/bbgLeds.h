/***********************************************************************************
 * @author Brian Ibeling
 * brian.ibeling@colorado.edu
 * Advanced Embedded Software Development
 * ECEN5013 - Rick Heidebrecht
 * @date March 14, 2019
 * arm-linux-gnueabi (Buildroot)
 * gcc (Ubuntu)
 ************************************************************************************
 *
 * @file bbgLeds.h
 * @brief Interface for on-board BBG LEDs
 *
 ************************************************************************************
 */

#ifndef BBG_LEDS_H_
#define BBG_LEDS_H_

#include <stdint.h>

/*---------------------------------------------------------------------------------*/
/**
 * @brief implements state machine to drive status led
 * 
 * @param newError new error flag
 * @return int8_t success of operation
 */
int8_t setStatusLed(uint8_t newError);

/**
 * @brief initializes led gpio
 * 
 * @return int8_t success of operation
 */
int8_t initLed(void);

/*---------------------------------------------------------------------------------*/
#endif /* BBG_LEDS_H_ */
