/*
 * main.h
 *
 *  Created on: Apr 10, 2019
 *      Author: malbu
 */

#ifndef MAIN_H_
#define MAIN_H_

/* Define to trap errors during development. */
//#define configASSERT(x)     if((x) == 0 ) { for( ;; ); }

#define SYSTEM_CLOCK            (25000000U)
int16_t getTaskNum(void);
//int8_t setTaskNum(const char *pcName, ProcessId_e process);

#endif /* MAIN_H_ */
