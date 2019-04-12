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
 * @file memory.c
 * @brief interface for platform-independent memory manipulation functions.
 * 
 ************************************************************************************
 */

#include "memory.h"

/* assumes a word is 4 bytes, where 1 byte = sizeof(char) */
#define SIZE_OF_WORD_BYTES 4

int null_ptr_check(uint8_t * src, uint8_t * dst)
{
	return((src == NULL) || (dst == NULL));
}
uint8_t * my_memcpy(uint8_t * src, uint8_t * dst, size_t length)
{
   /* verify appropriate input provided */
	if((length <= 0) || null_ptr_check(src,dst))
		return(NULL);

	uint8_t *end = src + length;     /* end = one passed last value to copy */
	uint8_t *dhead = dst;            /* head = first value to copy */

   /* increment src/dst until source reaches tail (last value to copy) */
	while (src < end)
	{
		*dst = *src;
		++src;
		++dst;
	}
   /* return head of dst */
	return(dhead);
}
uint8_t * my_memmove(uint8_t * src, uint8_t * dst, size_t length)
{
	/* verify appropriate input provided */
   if((length <= 0) || null_ptr_check(src,dst))
		return(NULL);

   /* if src is after dst, start at tail to prevent corruption */
	if(src < dst)
	{
      /* define tails of src and dst */
      uint8_t *stail = src + (length - 1);
      uint8_t *dtail = dst + (length - 1);

      /* increment src & dst from tail until head is reached */
		while(stail >= src)
		{
			*dtail = *stail;
         --stail;
         --dtail;
		}
		return(dst);
	}
	else

    /* don't duplicate code, call non-overlapping copy function */
	return (my_memcpy(src,dst,length));
}
uint8_t * my_memset(uint8_t * src, size_t length, uint8_t value)
{
   /* verify appropriate input provided */
	if((length <= 0) || null_ptr_check(src,src))
		return(NULL);

    uint8_t *head = src;             /* head = first value to copy */
	uint8_t *end = src + length;     /* end = one passed last value to copy */

   /* increment src until source reaches tail (last value to copy) */
	while (src < end)
	{
		*src = value;
		++src;
	}
   /* return head of src */
	return(head);
}
uint8_t * my_memzero(uint8_t * src, size_t length)
{
	/* don't duplicate code, call memset instead */
   return(my_memset(src,length,0));
}
uint8_t * my_reverse(uint8_t * src, size_t length)
{
   /* verify appropriate input provided */
	if ((length <= 0) || null_ptr_check(src,src))
		return(NULL);

	/* initialize swap variable and pointers for array tail & head */
	uint8_t temp;
	uint8_t *head = src;
	uint8_t *tail = src + (length - 1);

	/* determine if center of array has been reached */
	while (head < tail)
	{
		/* if not swap head and tail */
		temp = *head;
		*head = *tail;
		*tail = temp;

		/* increment toward center */
		++head;
		--tail;
	}
	return(src);
}
void * reserve_words(size_t length)
{
   void * pTemp = NULL;

   /* verify appropriate input provided */
   if (length <= 0)
      return(pTemp);

   /* dynamically allocate memory */
	pTemp = malloc(length * (SIZE_OF_WORD_BYTES * sizeof(char)));

	return(pTemp);
}
void * free_words(uint32_t * src)
{
   /* verify appropriate input provided */
	if((void *)src == NULL)
		return(NULL);

	free((void *)src);

	return((void *)src);
}
