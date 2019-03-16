/***********************************************************************************
 * @author Brian Ibeling and Josh Malburg
 * 
 * Advanced Embedded Software Development
 * ECEN5013 - Rick Heidebrecht
 * @date March 8, 2019
 * arm-linux-gnueabi (Buildroot)
 * gcc (Ubuntu)
 ************************************************************************************
 *
 * @file debug.h
 * @brief print statements, etc
 *
 ************************************************************************************
 */

#ifndef DEBUG_H_
#define DEBUG_H_

#include <stdio.h>

#define ERROR_PRINT(...)	(printf(__VA_ARGS__))
#define WARN_PRINT(...)		(printf(__VA_ARGS__))
#define INFO_PRINT(...)		(printf(__VA_ARGS__))
#define DEBUG_PRINT(...)	(printf(__VA_ARGS__))

#endif /* DEBUG_H_ */