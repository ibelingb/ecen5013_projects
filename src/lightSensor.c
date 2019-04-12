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
 * @file lightSensor.c
 * @brief APDS-9301 light sensor library
 *
 * Provides API for APDS9301 Light Sensor to read and write data from device over I2C
 * interface.
 *
 ************************************************************************************
 */

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#include "lightSensor.h"
#include "lu_iic.h"
#include "my_debug.h"

#define APDS9301_I2C_ADDR           (0x39ul)
#define APDS9301_CMD_WORD_BIT       (0x20)
#define APDS9301_CMD_INT_CLEAR_BIT  (0x40)

#define APDS9301_CONTROL_REG        (0x80ul)
#define APDS9301_TIMING_REG         (0x81ul)
#define APDS9301_THRESHLOWLOW       (0x82ul)
#define APDS9301_THRESHLOWHIGH      (0x83ul)
#define APDS9301_THRESHHIGHLOW      (0x84ul)
#define APDS9301_THRESHHIGHHIGH     (0x85ul)
#define APDS9301_INTERRUPT_CTRL_REG (0x86ul)
#define APDS9301_ID_REG             (0x8Aul)
#define APDS9301_DATA0LOW_REG       (0x8Cul)
#define APDS9301_DATA0HIGH_REG      (0x8Dul)
#define APDS9301_DATA1LOW_REG       (0x8Eul)
#define APDS9301_DATA1HIGH_REG      (0x8Ful)

#define APDS9301_CONTROL_MASK         (0x03)
#define APDS9301_DEV_PARTNO_MASK      (0xF0)
#define APDS9301_DEV_PARTNO_OFFSET    (0x04)
#define APDS9301_DEV_REVNO_MASK       (0x0F)
#define APDS9301_DEV_REVNO_OFFSET     (0x00)
#define APDS9301_TIMING_REG_MASK      (0x1B)
#define APDS9301_TIMING_GAIN_MASK     (0x10)
#define APDS9301_TIMING_GAIN_OFFSET   (0x04)
#define APDS9301_TIMING_MANUAL_MASK   (0x08)
#define APDS9301_TIMING_MANUAL_OFFSET (0x03)
#define APDS9301_TIMING_INT_MASK      (0x03)
#define APDS9301_TIMING_INT_OFFSET    (0x00)

#define APDS9301_INT_CONTROL_SEL_MASK       (0x30)
#define APDS9301_INT_CONTROL_SEL_OFFSET     (0x04)
#define APDS9301_INT_CONTROL_PERSIST_MASK   (0x0F)
#define APDS9301_INT_CONTROL_PERSIST_OFFSET (0x00)

#define APDS9301_REG_SIZE   (1) /* Size of APDS9301 register in bytes */
#define APDS9301_WORD_SIZE  (2) /* Size of APDS9301 Word in bytes */
#define APDS9301_ENDIANNESS (0) /* APDS9301 device endianness of returned data */

/* Prototypes for private/helper functions */
int8_t apds9301_getReg(uint8_t file, uint8_t *pReg, uint8_t REG);
int8_t apds9301_getWord(uint8_t file, uint16_t *pWord, uint8_t REG);
int8_t apds9301_writeWord(uint8_t file, uint16_t word, uint8_t REG);

/* Define static and global variables */

/*---------------------------------------------------------------------------------*/
int8_t apds9301_getLuxData(uint8_t file, float *luxData)
{
  uint16_t data0;
  uint16_t data1;
  float luxRatio;
  float sensorLux;

  /* Validate inputs */
  if(luxData == NULL)
    return EXIT_FAILURE;

  /* Verify device is powered on and responsive */
  Apds9301_PowerCtrl_e powerCtrl;
  apds9301_setControl(file, APDS9301_CTRL_POWERUP);
  apds9301_getControl(file, &powerCtrl);
  if(powerCtrl == APDS9301_CTRL_POWERDOWN)
  {
    ERROR_PRINT("apds9301_getLuxData() failed to power on device via apds9301_setControl() - read failed.\n");
    return EXIT_FAILURE;
  }

  /* Get Lux Data0 Low and High register values */
  if(EXIT_FAILURE == apds9301_getWord(file, &data0, APDS9301_DATA0LOW_REG))
    return EXIT_FAILURE;
  /* Get Lux Data1 Low and High register values */
  if(EXIT_FAILURE == apds9301_getWord(file, &data1, APDS9301_DATA1LOW_REG))
    return EXIT_FAILURE;

  /* Get Lux Calculation value based on device settings */ 
  /* See Note 8 on Page 3 of APDS9301 datasheet for below calculation */
  luxRatio = (float)data1/data0;
  if((luxRatio > 0) && (luxRatio <= 0.50)){
    sensorLux = ((0.0304 * data0) - (0.062 * data0) * pow((data1/data0), 1.4));
  } else if((luxRatio > 0.50) && (luxRatio <= 0.61)) {
    sensorLux = ((0.0224 * data0) - (0.031 * data1));
  } else if((luxRatio > 0.61) && (luxRatio <= 0.80)) {
    sensorLux = ((0.0128 * data0) - (0.0153 * data1));
  } else if((luxRatio > 0.80) && (luxRatio <= 1.30)) {
    sensorLux = ((0.00146 * data0) - (0.00112 * data1));
  } else if(luxRatio > 1.30) {
    sensorLux = 0;
  }

  /* Return calculated value */
  *luxData = sensorLux;

  return EXIT_SUCCESS;
}

/*---------------------------------------------------------------------------------*/
int8_t apds9301_getControl(uint8_t file, Apds9301_PowerCtrl_e *control)
{
  uint8_t regValue;

  /* Validate inputs */
  if(control == NULL)
    return EXIT_FAILURE;

  /* Get Control register value */
  if(EXIT_FAILURE == apds9301_getReg(file, &regValue, APDS9301_CONTROL_REG))
    return EXIT_FAILURE;  

  /* Mask power bits for control reg */
  regValue &= APDS9301_CONTROL_MASK;

  /* Return current power state */
  *control = (Apds9301_PowerCtrl_e)regValue;

  return EXIT_SUCCESS;
}

/*---------------------------------------------------------------------------------*/
int8_t apds9301_getTimingIntegration(uint8_t file, Apds9301_TimingInt_e *integration)
{
  uint8_t regValue;

  /* Validate inputs */
  if(integration == NULL)
    return EXIT_FAILURE;

  /* Get Integration Timing register value */
  if(EXIT_FAILURE == apds9301_getReg(file, &regValue, APDS9301_TIMING_REG))
    return EXIT_FAILURE;  

  /* Mask Integration Timing bit and shift; set value to be returned */
  *integration = (Apds9301_TimingInt_e)((regValue & APDS9301_TIMING_INT_MASK) >> APDS9301_TIMING_INT_OFFSET);

  return EXIT_SUCCESS;
}

/*---------------------------------------------------------------------------------*/
int8_t apds9301_getTimingGain(uint8_t file, Apds9301_TimingGain_e *gain)
{
  uint8_t regValue;

  /* Validate inputs */
  if(gain == NULL)
    return EXIT_FAILURE;

  /* Get Timing Gain register value */
  if(EXIT_FAILURE == apds9301_getReg(file, &regValue, APDS9301_TIMING_REG))
    return EXIT_FAILURE;  

  /* Mask Timing gain bit and shift; set value to be returned */
  *gain = (Apds9301_TimingGain_e)((regValue & APDS9301_TIMING_GAIN_MASK) >> APDS9301_TIMING_GAIN_OFFSET);

  return EXIT_SUCCESS;
}

/*---------------------------------------------------------------------------------*/
int8_t apds9301_getDeviceId(uint8_t file, uint8_t *partNo, uint8_t *revNo)
{
  uint8_t regValue;

  /* Validate inputs */
  if((partNo == NULL) || (revNo == NULL))
    return EXIT_FAILURE;

  /* Get DeviceID register value */
  if(EXIT_FAILURE == apds9301_getReg(file, &regValue, APDS9301_ID_REG))
    return EXIT_FAILURE;  

  /* Set read values */

  *partNo = (uint8_t)((regValue & APDS9301_DEV_PARTNO_MASK) >> APDS9301_DEV_PARTNO_OFFSET);
  *revNo = (uint8_t)((regValue & APDS9301_DEV_REVNO_MASK) >> APDS9301_DEV_REVNO_OFFSET);

  return EXIT_SUCCESS;
}

/*---------------------------------------------------------------------------------*/
int8_t apds9301_getLowIntThreshold(uint8_t file, uint16_t *intThreshold)
{
  uint16_t wordValue;

  /* Validate inputs */
  if(intThreshold == NULL)
    return EXIT_FAILURE;

  /* Get low interrupt threshold register value */
  if(EXIT_FAILURE == apds9301_getWord(file, &wordValue, APDS9301_THRESHLOWLOW))
    return EXIT_FAILURE;  

  /* Set read value */
  *intThreshold = wordValue;

  return EXIT_SUCCESS;
}

/*---------------------------------------------------------------------------------*/
int8_t apds9301_getHighIntThreshold(uint8_t file, uint16_t *intThreshold)
{
  uint16_t wordValue;

  /* Validate inputs */
  if(intThreshold == NULL)
    return EXIT_FAILURE;

  /* Get high interrupt threshold register value */
  if(EXIT_FAILURE == apds9301_getWord(file, &wordValue, APDS9301_THRESHHIGHLOW))
    return EXIT_FAILURE;

  /* Set read value */
  *intThreshold = wordValue;

  return EXIT_SUCCESS;
}

/*---------------------------------------------------------------------------------*/
int8_t apds9301_getInterruptControl(uint8_t file, Apds9301_IntSelect_e *intSelect, Apds9301_IntPersist_e *intPersist)
{
  uint8_t regValue;

  /* Validate inputs */
  if((intSelect == NULL) || (intPersist == NULL))
    return EXIT_FAILURE;

  /* Get Interrupt Control register value */
  if(EXIT_FAILURE == apds9301_getReg(file, &regValue, APDS9301_INTERRUPT_CTRL_REG))
    return EXIT_FAILURE;  

  /* Mask and shift Interrupt Select bits and Persist bits for Int control reg */
  *intSelect = (Apds9301_IntSelect_e)((regValue & APDS9301_INT_CONTROL_SEL_MASK) >> APDS9301_INT_CONTROL_SEL_OFFSET);
  *intPersist = (Apds9301_IntPersist_e)((regValue & APDS9301_INT_CONTROL_PERSIST_MASK) >> APDS9301_INT_CONTROL_PERSIST_OFFSET);

  return EXIT_SUCCESS;
}

/*---------------------------------------------------------------------------------*/
int8_t apds9301_setControl(uint8_t file, Apds9301_PowerCtrl_e control)
{
  uint8_t reg;

  /* Read current register value */
  if(EXIT_FAILURE == apds9301_getReg(file, &reg, APDS9301_CONTROL_REG))
    return EXIT_FAILURE;

  /* Determine register value to set based on provided input */
  if(control == APDS9301_CTRL_POWERUP){
    reg |= APDS9301_CONTROL_MASK;
  } else if(control == APDS9301_CTRL_POWERDOWN) {
    reg &= ~(APDS9301_CONTROL_MASK);
  } else {
    ERROR_PRINT("apds9301_setControl() received an invalid input for setting the APDS9301 control reg - write ignored.\n");
    return EXIT_FAILURE;
  }

  /* Write to device */
  if(EXIT_FAILURE == setIicRegister(file, APDS9301_I2C_ADDR, APDS9301_CONTROL_REG, reg, APDS9301_REG_SIZE, APDS9301_ENDIANNESS))
    return EXIT_FAILURE;

  return EXIT_SUCCESS;
}

/*---------------------------------------------------------------------------------*/
int8_t apds9301_setTimingGain(uint8_t file, Apds9301_TimingGain_e gain)
{
  uint8_t reg;

  /* Read current register value */
  if(EXIT_FAILURE == apds9301_getReg(file, &reg, APDS9301_TIMING_REG))
    return EXIT_FAILURE;

  /* Determine register value to set based on provided input */
  if(gain == APDS9301_TIMING_GAIN_LOW) {
    reg &= ~(APDS9301_TIMING_GAIN_MASK);
  } else if(gain == APDS9301_TIMING_GAIN_HIGH){
    reg |= APDS9301_TIMING_GAIN_MASK;
  } else {
    ERROR_PRINT("apds9301_setTimingGain() received an invalid input for setting the APDS9301 Timing reg - write ignored.\n");
    return EXIT_FAILURE;
  }

  /* Write to device */
  if(EXIT_FAILURE == setIicRegister(file, APDS9301_I2C_ADDR, APDS9301_TIMING_REG, reg, APDS9301_REG_SIZE, APDS9301_ENDIANNESS))
    return EXIT_FAILURE;

  return EXIT_SUCCESS;
}

/*---------------------------------------------------------------------------------*/
int8_t apds9301_setTimingIntegration(uint8_t file, Apds9301_TimingInt_e integration)
{
  uint8_t reg;

  /* Read current register value */
  if(EXIT_FAILURE == apds9301_getReg(file, &reg, APDS9301_TIMING_REG))
    return EXIT_FAILURE;

  /* Determine register value to set based on provided input */
  if(integration == APDS9301_TIMING_INT_13P7){
    reg &= ~(APDS9301_TIMING_INT_MASK);
  } else if(integration == APDS9301_TIMING_INT_101) {
    reg &= ~(APDS9301_TIMING_INT_MASK);
    reg |= APDS9301_TIMING_INT_101;
  } else if(integration == APDS9301_TIMING_INT_402) {
    reg &= ~(APDS9301_TIMING_INT_MASK);
    reg |= APDS9301_TIMING_INT_402;
  } else {
    ERROR_PRINT("apds9301_setTimingIntegration() received an invalid input for setting the APDS9301 Timing reg - write ignored.\n");
    return EXIT_FAILURE;
  }

  /* Write to device */
  if(EXIT_FAILURE == setIicRegister(file, APDS9301_I2C_ADDR, APDS9301_TIMING_REG, reg, APDS9301_REG_SIZE, APDS9301_ENDIANNESS))
    return EXIT_FAILURE;

  return EXIT_SUCCESS;
}

/*---------------------------------------------------------------------------------*/
int8_t apds9301_setInterruptControl(uint8_t file, Apds9301_IntSelect_e intSelect, Apds9301_IntPersist_e persist)
{
  uint8_t reg;

  /* Read current register value */
  if(EXIT_FAILURE == apds9301_getReg(file, &reg, APDS9301_INTERRUPT_CTRL_REG))
    return EXIT_FAILURE;

  /* Determine Interrupt Control Select value to set based on input */
  if(intSelect == APDS9301_INT_SELECT_LEVEL_DISABLE){
    reg &= ~(APDS9301_INT_CONTROL_SEL_MASK);
  } else if (intSelect == APDS9301_INT_SELECT_LEVEL_ENABLE) {
    reg |= 0x10;
  } else {
    ERROR_PRINT("apds9301_setInterruptControl() received an invalid input for setting the APDS9301 "
                "Interrupt Control reg Select Field - write ignored.\n");
    return EXIT_FAILURE;
  }

  /* Determine Interrupt Control Persist value to set based on input */
  if(persist > APDS9301_INT_PERSIST_OUTSIDE_15P) {
    ERROR_PRINT("apds9301_setInterruptControl() received an invalid input for setting the APDS9301 "
                "Interrupt Control reg Presist Field - write ignored.\n");
    return EXIT_FAILURE;
  }
  reg &= ~(APDS9301_INT_CONTROL_PERSIST_MASK);
  reg |= persist;

  /* Write to device */
  if(EXIT_FAILURE == setIicRegister(file, APDS9301_I2C_ADDR, APDS9301_INTERRUPT_CTRL_REG, reg, APDS9301_REG_SIZE, APDS9301_ENDIANNESS))
    return EXIT_FAILURE;

  return EXIT_SUCCESS;
}

/*---------------------------------------------------------------------------------*/
int8_t apds9301_clearInterrupt(uint8_t file)
{
  uint8_t reg;

  /* Write existing value to Control Register with added Interrupt Clear bit set */
  /* Read current power state */
  if(EXIT_FAILURE == apds9301_getReg(file, &reg, APDS9301_CONTROL_REG))
    return EXIT_FAILURE;  

  /* Set value to write to register */
  reg |= APDS9301_CMD_INT_CLEAR_BIT;

  /* Write to clear bit field of command register to clear any pending interrupt */
  if(EXIT_FAILURE == setIicRegister(file, APDS9301_I2C_ADDR, APDS9301_CONTROL_REG, reg, APDS9301_REG_SIZE, APDS9301_ENDIANNESS))
    return EXIT_FAILURE;

  return EXIT_SUCCESS;
}

/*---------------------------------------------------------------------------------*/
int8_t apds9301_setLowIntThreshold(uint8_t file, uint16_t intThreshold)
{
  /* Write to device */
  if(EXIT_FAILURE == apds9301_writeWord(file, intThreshold, APDS9301_THRESHLOWLOW))
    return EXIT_FAILURE;

  return EXIT_SUCCESS;
}

/*---------------------------------------------------------------------------------*/
int8_t apds9301_setHighIntThreshold(uint8_t file, uint16_t intThreshold)
{
  /* Write to device */
  if(EXIT_FAILURE == apds9301_writeWord(file, intThreshold, APDS9301_THRESHHIGHLOW))
    return EXIT_FAILURE;

  return EXIT_SUCCESS;
}

/*---------------------------------------------------------------------------------*/
/* HELPER FUNCTIONS */
/*---------------------------------------------------------------------------------*/
/**
 * @brief Read 8-bit register using I2C getIicRegister() method
 *
 * @param file - FD for sensor device.
 * @param pReg - Pointer for return value to be pass by (8-bit value).
 * @param REG - Address on APDS-9301 device register to read from.
 *
 * @return int8_t status, EXIT_SUCCESS if succeeds
 */
int8_t apds9301_getReg(uint8_t file, uint8_t *pReg, uint8_t REG){
  /* Validate inputs */
  if(pReg == NULL)
    return EXIT_FAILURE;

  /* Read register value from device */
  uint32_t regValue = 0;
  if(EXIT_FAILURE == getIicRegister(file, APDS9301_I2C_ADDR, REG, &regValue, APDS9301_REG_SIZE, APDS9301_ENDIANNESS))
    return EXIT_FAILURE;

  *pReg = (uint8_t)(0xFF & regValue);
  return EXIT_SUCCESS;
}
/**
 * @brief Read 16-bit register using I2C getIicRegister() method
 *
 * @param file - FD for sensor device.
 * @param pWord - Pointer for return value to be pass by (16-bit value).
 * @param REG - Address on APDS-9301 device register to read from.
 *
 * @return int8_t status, EXIT_SUCCESS if succeeds
 */
/*---------------------------------------------------------------------------------*/
int8_t apds9301_getWord(uint8_t file, uint16_t *pWord, uint8_t REG)
{
  uint32_t word = 0;

  /* Validate inputs */
  if(pWord == NULL)
    return EXIT_FAILURE;

  /* Set Command Code for I2C Read protocol */
  REG |= APDS9301_CMD_WORD_BIT;

  /* Read 16-bit word from device */
  if(EXIT_FAILURE == getIicRegister(file, APDS9301_I2C_ADDR, REG, &word, APDS9301_WORD_SIZE, APDS9301_ENDIANNESS))
    return EXIT_FAILURE;

  *pWord = (uint16_t)(0xFFFF & word);
  return EXIT_SUCCESS;
}

/*---------------------------------------------------------------------------------*/
/**
 * @brief Write 16-bit register using I2C setIicRegister() method
 *
 * @param file - FD for sensor device.
 * @param word - 16-bit value to write.
 * @param REG - Address on APDS-9301 device register to write to.
 *
 * @return int8_t status, EXIT_SUCCESS if succeeds
 */
int8_t apds9301_writeWord(uint8_t file, uint16_t word, uint8_t REG)
{
  /* Set Command Code for I2C Write protocol */
  REG |= APDS9301_CMD_WORD_BIT;

  /* Write 16-bit word to device */
  if(EXIT_FAILURE == setIicRegister(file, APDS9301_I2C_ADDR, REG, word, APDS9301_WORD_SIZE, APDS9301_ENDIANNESS))
    return EXIT_FAILURE;

  return EXIT_SUCCESS;
}

