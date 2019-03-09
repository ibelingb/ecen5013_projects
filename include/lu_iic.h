/*
 * lu_iic.h
 *
 *  Created on: Mar 1, 2019
 *      Author: fpga1
 */

#ifndef SRC_LU_IIC_H_
#define SRC_LU_IIC_H_

#include <stdint.h>


int8_t setIicRegister(int IicFd, uint8_t slavAddr, uint8_t reg, uint8_t reg_value);
int8_t getIicRegister(int IicFd, uint8_t slavAddr, uint8_t reg, uint8_t *pReg_value);
int initIic(char *filename);

#endif /* SRC_LU_IIC_H_ */
