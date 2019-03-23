/***********************************************************************************
 * @author Joshua Malburg
 * joshua.malburg@colorad.edu
 * Advanced Embedded Software Development
 * ECEN5013 - Rick Heidebrecht
 * @date March 8, 2019
 * arm-linux-gnueabi (Buildroot)
 * gcc (Ubuntu)
 ************************************************************************************
 *
 * @file lu_iic.h
 * @brief generic i2c-dev driver wrapper
 *
 * https://www.linuxquestions.org/questions/programming-9/reading-data-via-i2c-dev-4175499069/
 * https://www.kernel.org/doc/Documentation/i2c/dev-interface
 * http://linux-sunxi.org/I2Cdev
 ************************************************************************************
 */

#ifndef SRC_LU_IIC_H_
#define SRC_LU_IIC_H_

#include <stdint.h>

int8_t setIicRegister(int IicFd, uint8_t slavAddr, uint8_t reg, uint32_t reg_value, uint8_t regSize, uint8_t regEndianness);
int8_t getIicRegister(int IicFd, uint8_t slavAddr, uint8_t reg, uint32_t *pReg_value, uint8_t regSize, uint8_t regEndianness);
int initIic(char *filename);

#endif /* SRC_LU_IIC_H_ */
