/*
 * references:
 * https://www.linuxquestions.org/questions/programming-9/reading-data-via-i2c-dev-4175499069/
 * https://www.kernel.org/doc/Documentation/i2c/dev-interface
 * http://linux-sunxi.org/I2Cdev
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


int8_t getIicRegister(int file, uint8_t slavAddr, uint8_t reg, uint8_t *pReg_value)
{
    unsigned char inbuf, outbuf;
    struct i2c_rdwr_ioctl_data packets;
    struct i2c_msg messages[2];

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
    messages[1].buf   = &inbuf;

    /* Send the request to the kernel and get the result back */
    packets.msgs      = messages;
    packets.nmsgs     = 2;
    if(ioctl(file, I2C_RDWR, &packets) < 0)
    {
        printf("getIicRegister - IIC read failed, errno (%d): %s\n\r", errno, strerror(errno));
        return 1;
    }
    *pReg_value = inbuf;

	return EXIT_SUCCESS;

//	static uint8_t buf[0xff];
//	buf[0] = reg;
//
//    /* set the I2C slave address; try production then prototype hardware */
//    if(ioctl(IicFd, I2C_SLAVE_FORCE, slavAddr) < 0)
//    {
//    	printf("failed to set slave addr, errno (%d): %s\n\r", errno, strerror(errno));
//    	return -1;
//    }
//
//    if(slavAddr != 0x70)
//    {
//    	/* read back value */
//    	size_t readSize = sizeof(buf) / sizeof(buf[0]);
//    	if (read(IicFd, buf, readSize) != readSize) {
//    		printf("getIicRegister - IIC read failed, errno (%d): %s\n\r", errno, strerror(errno));
//    		return EXIT_FAILURE;
//    	}
//
//    	/* return read reg */
//    	*pReg_value = buf[reg];
//    }
//    else
//    {
//    	/* tell chip what register to get */
//        size_t writeSize = sizeof(buf[0]) / sizeof(buf[0]);
//    	if (write(IicFd, buf, writeSize) != writeSize)
//    	{
//    		printf("getIicRegister - IIC read failed, errno (%d): %s\n\r", errno, strerror(errno));
//    		return EXIT_FAILURE;
//    	}
//    	/* read back value */
//    	if (read(IicFd, buf, 1) != 1) {
//    		printf("getIicRegister - IIC read failed, errno (%d): %s\n\r", errno, strerror(errno));
//    		return EXIT_FAILURE;
//    	}
//
//    	/* return read reg */
//    	*pReg_value = buf[0];
//    }

	return EXIT_SUCCESS;
}

int8_t setIicRegister(int file, uint8_t slavAddr, uint8_t reg, uint8_t reg_value)
{


    unsigned char outbuf[2];
    struct i2c_rdwr_ioctl_data packets;
    struct i2c_msg messages[1];

    messages[0].addr  = slavAddr;
    messages[0].flags = 0;
    messages[0].len   = sizeof(outbuf);
    messages[0].buf   = outbuf;

    outbuf[0] = reg;			/* register to write to */
    outbuf[1] = reg_value;		/* write value */

    /* Transfer the i2c packets to the kernel and verify it worked */
    packets.msgs  = messages;
    packets.nmsgs = 1;
    if(ioctl(file, I2C_RDWR, &packets) < 0)
    {
		printf("setIicRegister - IIC write failed, errno (%d): %s\n\r", errno, strerror(errno));
		return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;

//	uint8_t pTxData[2];
//	uint8_t bytesSent;
//	uint8_t readValue;
//
//    /* set the I2C slave address; try production then prototype hardware */
//    if(ioctl(IicFd, I2C_SLAVE_FORCE, slavAddr) < 0)
//    {
//    	printf("failed to set slave addr, errno (%d): %s\n\r", errno, strerror(errno));
//    	return -1;
//    }
//
//	pTxData[0] = reg;
//	pTxData[1] = reg_value;
//
//	/* write addr and reg to slave */
//	bytesSent = write(IicFd, &pTxData, sizeof(pTxData));
//	if (bytesSent != sizeof(pTxData)) {
//		printf("setIicRegister - IIC write failed, errno (%d): %s\n\r", errno, strerror(errno));
//		return EXIT_FAILURE;
//	}
//	getIicRegister(IicFd, slavAddr, reg, &readValue);
//	if(readValue != reg_value)
//		printf("written value didn't take, slavAddr: %x, reg: %x, reg_value: %x, readValue: %x\n", slavAddr, reg, reg_value, readValue);
//
//	return EXIT_SUCCESS;
}

int initIic(char *filename)
{
	int iic_fd;

	iic_fd = open(filename, O_RDWR | O_SYNC);
    if (iic_fd < 0) {
    	printf("failed to open IIC device, errno (%d): %s\n\r", errno, strerror(errno));
		return -1;
    }
    return iic_fd;
}
