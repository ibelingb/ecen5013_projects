/***********************************************************************************
 * @author Brian Ibeling
 * brian.ibeling@colorado.edu
 * Advanced Embedded Software Development
 * ECEN5013 - Rick Heidebrecht
 * @date March 14, 2019
 * arm-linux-gnueabi (Buildroot)
 * gcc (Ubuntu)
 ************************************************************************************
 *
 * @file bbgLeds.c
 * @brief Interface for on-board BBG LEDs
 *
 * references: 
 * https://www.teachmemicro.com/beaglebone-black-blink-led-using-c/
 * https://vadl.github.io/beagleboneblack/2016/07/29/setting-up-bbb-gpio
 ************************************************************************************
 */

#include "bbgLeds.h"
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

/**
 * @brief initialize status led (gpio21)
 * 
 * @param value desired value of led
 * @return int function sucess
 */
int led1_set_value(uint8_t value);
int led2_set_value(uint8_t value);

/*---------------------------------------------------------------------------------*/
int8_t setStatusLed(uint8_t state)
{
  switch (state) {
    case 0: /* DEGRADED state (one LED) */
      led1_set_value(1);
      led2_set_value(0);
      break;
    case 1: /* FAULT state (two LED) */
      led1_set_value(1);
      led2_set_value(1);
      break;
    case 2: /* NOMINAL state (no LED) */
      led1_set_value(0);
      led2_set_value(0);
      break;
  }

  return EXIT_SUCCESS;
}

/*---------------------------------------------------------------------------------*/
/* HELPER METHODS */
int8_t initLed(void)
{
  FILE *export_file = NULL; // declare pointers
  FILE *IO_direction = NULL;
  FILE *IO_value = NULL;
  char str1[] = "0";
  char str3[] = "out";
  char str[] = "53";
  char str2[] = "54";
  //this part here exports gpio21
  export_file = fopen ("/sys/class/gpio/export", "w");
  fwrite (str, 1, sizeof(str), export_file);
  fclose (export_file);

  //this part here sets the direction of the pin
  IO_direction = fopen("/sys/class/gpio/gpio53/direction", "w");
  fwrite(str3, 1, sizeof(str3), IO_direction); //set the pin to HIGH
  fclose(IO_direction);
  usleep (1000000);

  IO_value = fopen ("/sys/class/gpio/gpio53/value", "w");
  fwrite (str1, 1, sizeof(str1), IO_value);
  fclose (IO_value);

  //this part here exports gpio22
  export_file = fopen ("/sys/class/gpio/export", "w");
  fwrite (str2, 1, sizeof(str2), export_file);
  fclose (export_file);

  //this part here sets the direction of the pin
  IO_direction = fopen("/sys/class/gpio/gpio54/direction", "w");
  fwrite(str3, 1, sizeof(str3), IO_direction); //set the pin to HIGH
  fclose(IO_direction);
  usleep (1000000);

  IO_value = fopen ("/sys/class/gpio/gpio54/value", "w");
  fwrite (str1, 1, sizeof(str1), IO_value);

  return 0;
}

int led1_set_value(uint8_t value)
{
  FILE *IO_value = NULL;
  char str1[] = "0";
  char str2[] = "1";
  IO_value = fopen ("/sys/class/gpio/gpio53/value", "w");

  fwrite (value == 0 ? str1 : str2, 1, 
  sizeof(str2), IO_value);
  fclose (IO_value);

  return 0;
}

/*---------------------------------------------------------------------------------*/
int led2_set_value(uint8_t value)
{
  FILE *IO_value = NULL;
  char str1[] = "0";
  char str2[] = "1";
  IO_value = fopen ("/sys/class/gpio/gpio54/value", "w");

  fwrite (value == 0 ? str1 : str2, 1, 
  sizeof(str2), IO_value);
  fclose (IO_value);

  return 0;
}

/*---------------------------------------------------------------------------------*/
