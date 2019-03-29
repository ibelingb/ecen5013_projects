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

// TODO: update
#define LED1 (0x00)
#define LED2 (0x01)
#define LED3 (0x02)
#define LED4 (0x03)

/*---------------------------------------------------------------------------------*/
int8_t setStatusLed(uint8_t newError);
int8_t initLed(void);

/*---------------------------------------------------------------------------------*/
#endif /* BBG_LEDS_H_ */
