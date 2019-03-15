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
* @file conversion.c
* @brief implementation of platform-indepedent Int to/from ASCII conversion functions.
*
************************************************************************************
*/

#include "conversion.h"
#include "memory.h"
#include <math.h>

/*
* @brief converts integer to ascii
* will convert integer to ascii; the value will be represented in the string using base specified
* if base is 10 (i.e. decimal) a negative sign will be included in the string and length
* @param   data   integer value to convert
* @param   ptr    pointer to ascii string
* @param   base   base of value to represent
* @return  length of string
*/
uint8_t my_itoa(int32_t data, uint8_t * ptr, uint32_t base)
{
   /* verify appropriate input provided */
   if ((ptr == NULL) || (base < 2) || (base > 16))
      return 0;
   
   uint8_t length = 0;
   uint8_t digit = 0;

   /* for decimal display, print minus sign */
   if ((data < 0) && (base == 10))
   {
      *ptr = '-';
      ++ptr;
      data *= -1;
   }
   
   /* loop until zero to find value of each digit */
   uint8_t *firstDigit = ptr;
   do
   {
      /* remainder if value of digit */
      digit = data % base;

      /* digit character for digit */
      if (digit <= 9)
         *ptr = digit +'0';
      else
         *ptr = (digit - 10) +'A';

      /* update integer value & ptr for next iteration */
      data /= base;
      ++ptr;

      /* increment string digit char count */
      ++length;
   } 
   while (data != 0);

   /* previous loop stores digits backs */
   my_reverse(firstDigit, length);

   /* add null terminator */
   *ptr = '\0';
   ++length;

   return(length);
}

/*
* @brief converts ascii to integer
* will read ascii characters from ptr until null terminator is reached.  Value will use
* base to convert to integer value
* @param    ptr      ointer to ascii string
* @param    digits   number of digits in string
* @param    base     base of ascii value
* @return   length of string
*/
int32_t my_atoi(uint8_t * ptr, uint8_t digits, uint32_t base)
{
   /* verify appropriate input provided */
   if ((ptr == NULL) || (base < 2) || (base > 16) || (digits <= 0))
      return 0;
   
   int32_t value = 0;            /* cummlative integer value of string */
   int32_t num;                  /* value of current character */
   int32_t sign = 1;             /* value of */
   int32_t found1st = 0;         /* flag for first non-whitespace */
   int8_t count = 0;             /* number of non-whitespace characters */

   while (*ptr != '\0')
   {
      num = *ptr;

      /* ignore leading whitespace */
      if ((found1st == 0) && (num <= 32))
      {
         ++ptr;
         continue;
      }
      else
      {
         found1st = 1;
         ++count;
      }

      /* check for leading minus sign */
      if ((num == '-') && (count == 1))
      {
         sign = -1;
         num = 0;
      }
      else
      {
         /* convert character to integer value */
         if ((num >= 'A') && (num <= 'Z') && (base > 10))
            num -= 'A' - 10;
         else if ((num >= 'a') && (num <= 'z') && (base > 10))
            num -= 'a' - 10;
         else if ((num >= '0') && (num <= '9'))
            num -= '0';
         else
            break;
      }

      /* raise previous value by power then add current value */
      value = (value * base) + num;

      --digits;
      ++ptr;
   }
   return (sign * value);
}
