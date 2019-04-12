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
 * @file memory.h
 * @brief platform-indepedent memory manipulation functions.
 * Functions for copying, setting, clearing, allocating and freeing 
 * memory in a platform independent manner.
 * 
 ************************************************************************************
 */

#ifndef MEMORY_H
#define MEMORY_H

#include <stdint.h>
#include <stdlib.h>

/*
 * @brief memory copy, handles overlap
 * will copy bytes from src to dst; will not corrupt memory if src & dst overlap
 * @param   src     memory location to copy from
 * @param   dst     memory location to copy to
 * @param   length  number of bytes to copy
 * @return  head of memory location copied to
 */
uint8_t * my_memmove(uint8_t * src, uint8_t * dst, size_t length);

/*
 * @brief memory copy, does not handles overlap
 * will copy bytes from src to dst; can't handle overlapping memory
 * @param   src     memory location to copy from
 * @param   dst     memory location to copy to
 * @param   length  number of bytes to copy
 * @return  head of memory location copied to
 */
uint8_t * my_memcpy(uint8_t * src, uint8_t * dst, size_t length);

/*
 * @brief set all bytes to 'value' param
 * will set all bytes in memory location 'src' to value.
 * @param   src     memory location to set
 * @param   length  number of bytes to set
 * @return  head of memory location set
 */
uint8_t * my_memset(uint8_t * src, size_t length, uint8_t value);

/*
 * @brief set all bytes to zero
 * will set all bytes in memory location 'src' to zero.
 * @param   src     memory location to set
 * @param   length  number of bytes to set
 * @return  head of memory location set
 */
uint8_t * my_memzero(uint8_t * src, size_t length);

/*
 * @brief reverse bytes of memory
 * will reverse the bytes in memory location 'src'.
 * @param   src     memory location
 * @param   length  number of bytes in src
 * @return  head of memory location
 */
uint8_t * my_reverse(uint8_t * src, size_t length);

/*
 * @brief dynamically allocate space for words
 * will allocation space dynamically for 'length' bytes
 * @param   length  number of bytes to allocate
 * @return  head of dynamically allocated memory or void if failed
 */
void * reserve_words(size_t length);

/*
 * @brief free dynamically allocated memory
 * free dynamically allocated memory
 * @param   src  pointer to dynamically allocated memory
 * @return  will return src or NULL if failed
 */
void * free_words(uint32_t * src);


#endif
