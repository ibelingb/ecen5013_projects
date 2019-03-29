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

/*---------------------------------------------------------------------------------*/
void* lightSensorThreadHandler(void* threadInfo);
void lightSigHandler(int signo, siginfo_t *info, void *extra);
void lightGetAliveFlag(uint8_t *pAlive);


/*---------------------------------------------------------------------------------*/
#endif /* LIGHT_THREAD_H_ */
