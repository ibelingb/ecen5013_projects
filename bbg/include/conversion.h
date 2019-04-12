/***********************************************************************************
 * @author Joshua Malburg (joma0364)
 * joshua.malburg@colorado.edu
 * Principles of Embedded Software
 * ECEN5813 - Alex Fosdick
 * @date January 31, 2018
 * gcc (Ubuntu 7.2.0-8ubuntu3)
 * arm-linux-gnueabi (Ubuntu/Linaro 7.2.0-6ubuntu1)
 * arm-none-eabi-gcc (5.4.1 20160919)
 ************************************************************************************
 * 
 * @file conversion.h
 * @brief interface for platform-indepedent Int to/from ASCII conversion functions.
 * 
 ************************************************************************************
 */

#ifndef CONVERSION_H
#define CONVERSION_H

#include <stdint.h>

#define DECIMAL_BASE	(10)
#define DEC_BASE		(10)
#define HEX_BASE		(16)
#define OCT_BASE		(8)
#define BINARY_BASE		(2)

/*
* @brief converts integer to ascii 
* will convert integer to ascii; the value will be represented in the string using base specified
* if base is 10 (i.e. decimal) a negative sign will be included in the string and length
* @param   data   integer value to convert
* @param   ptr    pointer to ascii string
* @param   base   base of value to represent
* @return  length of string
*/
uint8_t my_itoa(int32_t data, uint8_t * ptr, uint32_t base);

/*
* @brief converts ascii to integer
* will read ascii characters from ptr until null terminator is reached.  Value will use 
* base to convert to integer value
* @param    ptr      ointer to ascii string
* @param    digits   number of digits in string
* @param    base     base of ascii value
* @return   length of string
*/
int32_t my_atoi(uint8_t * ptr, uint8_t digits, uint32_t base);
#endif
