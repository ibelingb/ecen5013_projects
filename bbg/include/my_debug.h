/***********************************************************************************
 * @author Josh Malburg
 * joshua.malburg@colorad.edu
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

#include <errno.h>
#include <string.h>

#ifdef __linux__
#include <stdio.h>

#define ERROR_PRINT(...)	(printf(__VA_ARGS__))
#define ERRNO_PRINT(str)    (ERROR_PRINT("%s, error in: %s, err#%d (%s)\n", str, __func__, errno, strerror(errno)))
#define WARN_PRINT(...)		(printf(__VA_ARGS__))
#define INFO_PRINT(...)		(printf(__VA_ARGS__))
#define MUTED_PRINT(...)	
#define DEBUG_PRINT(...)	//(printf(__VA_ARGS__))
#else
#include "uartstdio.h"
#define ERROR_PRINT(...)    (UARTprintf(__VA_ARGS__))
#define ERRNO_PRINT(str)    (ERROR_PRINT("%s, error in: %s, err#%d (%s)\n", str, __func__, errno, strerror(errno)))
#define WARN_PRINT(...)     (UARTprintf(__VA_ARGS__))
#define INFO_PRINT(...)     (UARTprintf(__VA_ARGS__))
#define MUTED_PRINT(...)
#define DEBUG_PRINT(...)    (UARTprintf(__VA_ARGS__))
#endif

#define RETURN_SUCCESS  (0)
#define RETURN_ERROR    (-1)
#endif /* DEBUG_H_ */
