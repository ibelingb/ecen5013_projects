/* APDS-9301 light sensor library */

#include "lightSensor.h"
#include "lu_iic.h"
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>

#define APDS9301_ADDR           (0x39)
#define APDS9301_CONTROL        (0x00)
#define APDS9301_TIMING         (0x01)
#define APDS9301_THRESHLOWLOW   (0x02)
#define APDS9301_THRESHLOWHIGH  (0x03)
#define APDS9301_THRESHHIGHLOW  (0x04)
#define APDS9301_THRESHHIGHHIGH (0x05)
#define APDS9301_INTERRUPT      (0x06)
#define APDS9301_CRC            (0x08)
#define APDS9301_ID             (0x0A)
#define APDS9301_DATA0LOW       (0x0C)
#define APDS9301_DATA0HIGH      (0x0D)
#define APDS9301_DATA1LOW       (0x0E)
#define APDS9301_DATA1HIGH      (0x0F)

/*---------------------------------------------------------------------------------*/
int8_t apds9301_getLuxData0(uint8_t file, uint8_t *luxData0)
{

  return EXIT_SUCCESS;
}

int8_t apds9301_getLuxData1(uint8_t file, uint8_t *luxData1)
{

  return EXIT_SUCCESS;
}

int8_t apds9301_getControl(uint8_t file, uint8_t *config)
{

  return EXIT_SUCCESS;
}

int8_t apds9301_getTiming(uint8_t file, uint8_t *Timing)
{

  return EXIT_SUCCESS;
}

int8_t apds9301_getDeviceId(uint8_t file, uint8_t *deviceId)
{

  return EXIT_SUCCESS;
}

int8_t apds9301_getInterruptThreshold(uint8_t file, IntThresholdByte_e intByte, uint8_t *intThreshold)
{

  return EXIT_SUCCESS;
}

int8_t apds9301_setCommand(uint8_t file, uint8_t cmd)
{

  return EXIT_SUCCESS;
}

int8_t apds9301_setConfig(uint8_t file, uint8_t config)
{

  return EXIT_SUCCESS;
}

int8_t apds9301_setTiming(uint8_t file, uint8_t timing)
{

  return EXIT_SUCCESS;
}

int8_t apds9301_setInterruptControl(uint8_t file, bool state)
{

  return EXIT_SUCCESS;
}

int8_t apds9301_setInterruptThreshold(uint8_t file, uint8_t intThreshold)
{

  return EXIT_SUCCESS;
}
