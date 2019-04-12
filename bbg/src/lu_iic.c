/***********************************************************************************
 * @author Josh Malburg
 * joshua.malburg@colorado.edu
 * Advanced Embedded Software Development
 * ECEN5013 - Rick Heidebrecht
 * @date March 8, 2019
 * arm-linux-gnueabi (Buildroot)
 * gcc (Ubuntu)
 ************************************************************************************
 *
 * @file lu_iic.c
 * @brief generic i2c-dev driver 
 *
 * https://www.linuxquestions.org/questions/programming-9/reading-data-via-i2c-dev-4175499069/
 * https://www.kernel.org/doc/Documentation/i2c/dev-interface
 * http://linux-sunxi.org/I2Cdev
 ************************************************************************************
 */

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <stdint.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>

#include "my_debug.h"


int8_t getIicRegister(int file, uint8_t slavAddr, uint8_t reg, uint32_t *pReg_value, uint8_t regSize, uint8_t regEndianness)
{
    unsigned char inbuf[regSize], outbuf;
    struct i2c_rdwr_ioctl_data packets;
    struct i2c_msg messages[2];
    uint32_t regValue;
    uint8_t ind;

    if((pReg_value == NULL) || (regSize > sizeof(uint32_t)))
    {
        ERROR_PRINT("getIicRegister - input error\n");
        return EXIT_FAILURE;
    }  

    /*
     * In order to read a register, we first do a "dummy write" by writing
     * 0 bytes to the register we want to read from.  */
    outbuf = reg;
    messages[0].addr  = slavAddr;
    messages[0].flags = 0;
    messages[0].len   = sizeof(outbuf);
    messages[0].buf   = &outbuf;

    /* The data will get returned in this structure */
    messages[1].addr  = slavAddr;
    messages[1].flags = I2C_M_RD/* | I2C_M_NOSTART */;
    messages[1].len   = sizeof(inbuf);
    messages[1].buf   = &inbuf[0];

    /* Send the request to the kernel and get the result back */
    packets.msgs      = messages;
    packets.nmsgs     = 2;
    if(ioctl(file, I2C_RDWR, &packets) < 0)
    {
        MUTED_PRINT("getIicRegister - IIC read failed, errno (%d): %s\n\r", errno, strerror(errno));
        return EXIT_FAILURE;
    }
    for(regValue = 0, ind = 0; ind < regSize; ++ind)
    {
        uint8_t byte = inbuf[ind];

        if(regEndianness)
            regValue += (byte << (8 * ind));
        else
            regValue += (byte << (8 * ((regSize - 1) - ind)));
    }
    *pReg_value = regValue;

	return EXIT_SUCCESS;
}

int8_t setIicRegister(int file, uint8_t slavAddr, uint8_t reg, uint32_t reg_value, uint8_t regSize, uint8_t regEndianness)
{
    if(regSize > sizeof(uint32_t))
        return EXIT_FAILURE;
    
    unsigned char outbuf[regSize + 1];
    struct i2c_rdwr_ioctl_data packets;
    struct i2c_msg messages[1];

    messages[0].addr  = slavAddr;
    messages[0].flags = 0;
    messages[0].len   = sizeof(outbuf);
    messages[0].buf   = outbuf;

    /* register to write to */
    outbuf[0] = reg;			

    /* values to write */
    uint32_t shifted;
    uint8_t masked;
    for(uint8_t ind = 0; ind < regSize; ++ind)
    {
        if(regEndianness)
        {
            shifted = reg_value >> (8 * ind);
            masked  = (uint8_t)(0xFF & shifted);
            outbuf[ind + 1] = masked;
        }
        else
        {
            shifted = reg_value >> (8 * ((regSize -1) - ind));
            masked  = (uint8_t)(0xFF & shifted);
            outbuf[ind + 1] = masked;
        }
    }

    /* Transfer the i2c packets to the kernel and verify it worked */
    packets.msgs  = messages;
    packets.nmsgs = 1;
    if(ioctl(file, I2C_RDWR, &packets) < 0)
    {
		MUTED_PRINT("setIicRegister - IIC write failed, errno (%d): %s\n\r", errno, strerror(errno));
		return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

int initIic(char *filename)
{
	int iic_fd;

	iic_fd = open(filename, O_RDWR | O_SYNC);
    if (iic_fd < 0) {
    	MUTED_PRINT("failed to open IIC device, errno (%d): %s\n\r", errno, strerror(errno));
		return -1;
    }
    return iic_fd;
}
