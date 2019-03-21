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
 * @file lightThread.h
 * @brief Light sensor thread for APDS-9301 device
 *
 * https://www.sparkfun.com/products/14350
 *
 ************************************************************************************
 */

#ifndef LIGHT_THREAD_H_
#define LIGHT_THREAD_H_

#include <stdint.h>
#include "lightSensor.h"
#include "packet.h"

#define APDS9301_ID (0x00) // TODO - update

/*---------------------------------------------------------------------------------*/
void* lightSensorThreadHandler(void* threadInfo);

/*---------------------------------------------------------------------------------*/
#endif /* LIGHT_THREAD_H_ */
