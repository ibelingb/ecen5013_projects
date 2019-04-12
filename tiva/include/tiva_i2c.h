/*
 * tiva_i2c.h
 *
 *  Created on: Apr 8, 2019
 *      Author: malbu
 */

#ifndef TIVA_I2C_H_
#define TIVA_I2C_H_

#include <stdint.h>

int8_t initI2c(uint32_t sysClock);
int8_t sendIic(void);
int8_t recvIic1Bytes(uint8_t slaveAddr, uint8_t reg, uint8_t *pData);
int8_t recvIic2Bytes(uint8_t slaveAddr, uint8_t reg, uint8_t *pData);

#endif /* TIVA_I2C_H_ */
