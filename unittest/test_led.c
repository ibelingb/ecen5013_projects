/***********************************************************************************
 * @author Josh Malburg
 * joshua.malburg@colorado.edu
 * Advanced Embedded Software Development
 * ECEN5013 - Rick Heidebrecht
 * @date March 29, 2019
 * arm-linux-gnueabi (Buildroot)
 * gcc (Ubuntu)
 ************************************************************************************
 *
 * @file test_led.c
 * @brief test cases for status led
 * 
 ************************************************************************************
 */

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <stdint.h>

#include "bbgLeds.h"

#define LOOP_DLY	(usleep(500e-3 * 1e6))
#define LOOP_COUNT	(24)

uint8_t testCount = 0;

/* test cases */
int8_t test_led(void);

int main(void)
{
	printf("test cases for led\n");
	
	/* keep track of failures */
	uint8_t testFails = 0;

	/* init gpio */
	initLed();
	usleep(10000);

	testFails += test_led();

	printf("\n\nTEST RESULTS, %d of %d failed tests\n", testFails, testCount);

	return EXIT_SUCCESS;
}

/**
 * @brief verify blink and sold states
 * 
 * @return int8_t results of test
 */
int8_t test_led(void)
{
	++testCount;
	uint8_t ind, newError;

	/* verify led is off */
	printf("verify led remains off..\n");
	newError = 0;
	for(ind = 0; ind < LOOP_COUNT; ++ind) {
		setStatusLed(newError);
		LOOP_DLY;
		newError = 0;
	}
	printf("done\n");

	/* verify transition to blink state */
	printf("verify led blinks then goes solid...\n");
	newError = 1;
	for(ind = 0; ind < LOOP_COUNT; ++ind) {
		setStatusLed(newError);
		LOOP_DLY;
		newError = 0;
	}
	printf("done\n");

	/* verify transition to on state */
	printf("verify led remains solid...\n");
	newError = 0;
	for(ind = 0; ind < LOOP_COUNT; ++ind) {
		setStatusLed(newError);
		LOOP_DLY;
		newError = 0;
	}
	printf("done\n");

	/* verify transition back to blink state */
	printf("verify led blinks then goes solid...\n");
	newError = 1;
	for(ind = 0; ind < LOOP_COUNT; ++ind) {
		setStatusLed(newError);
		LOOP_DLY;
		newError = 0;
	}
	printf("done\n");
	return EXIT_SUCCESS;
}