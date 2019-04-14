/*
 * from: http://www.ti.com/lit/an/spma073/spma073.pdf
 * https://jspicer.net/2018/07/27/solution-for-i2c-busy-status-latency/
 * https://github.com/jspicer-ltu/Tiva-C-Embedded/tree/master/Experiment17-I2C
 */

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "hw_types.h"
#include "hw_ints.h"
#include "hw_memmap.h"
#include "driverlib/gpio.h"
#include "driverlib/i2c.h"
#include "driverlib/interrupt.h"
#include "driverlib/pin_map.h"
#include "driverlib/sysctl.h"
#include "driverlib/uart.h"
#include "uartstdio.h"

#include "my_debug.h"

//*****************************************************************************
//
// Define for I2C Module
//
//*****************************************************************************
#define SLAVE_ADDRESS_EXT 0x50
#define NUM_OF_I2CBYTES   255

//*****************************************************************************
//
// Enumerated Data Types for Master State Machine
//
//*****************************************************************************
enum I2C_MASTER_STATE
{
    I2C_OP_IDLE = 0,
    I2C_OP_TXADDR,
    I2C_OP_TXDATA,
    I2C_OP_RXDATA,
    I2C_OP_STOP,
    I2C_ERR_STATE
};

//*****************************************************************************
//
// Global variable for Delay Count
//
//*****************************************************************************
volatile uint16_t g_ui16SlaveWordAddress;
volatile uint8_t  g_ui8MasterTxData[NUM_OF_I2CBYTES];
volatile uint8_t  g_ui8MasterRxData[NUM_OF_I2CBYTES];
volatile uint8_t  g_ui8MasterCurrState;
volatile uint8_t  g_ui8MasterPrevState;
volatile bool     g_bI2CDirection;
volatile bool     g_bI2CRepeatedStart;
volatile uint8_t  g_ui8MasterBytes       = NUM_OF_I2CBYTES;
volatile uint8_t  g_ui8MasterBytesLength = NUM_OF_I2CBYTES;


int8_t initIicPins(void)
{
    /* Enable GPIO for Configuring the I2C Interface Pins */
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPION);

    /* Wait for the Peripheral to be ready for programming */
    while(!SysCtlPeripheralReady(SYSCTL_PERIPH_GPION));

    /* Configure Pins for I2C2 Master Interface */
    GPIOPinConfigure(GPIO_PN5_I2C2SCL);
    GPIOPinConfigure(GPIO_PN4_I2C2SDA);
    GPIOPinTypeI2C(GPIO_PORTN_BASE, GPIO_PIN_4);
    GPIOPinTypeI2CSCL(GPIO_PORTN_BASE, GPIO_PIN_5);

    return 0;
}

int8_t initIicController(uint32_t sysClock)
{
    /* Stop the Clock, Reset and Enable I2C Module in Master Function */
    SysCtlPeripheralDisable(SYSCTL_PERIPH_I2C2);
    SysCtlPeripheralReset(SYSCTL_PERIPH_I2C2);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_I2C2);

    /* Wait for the Peripheral to be ready for programming */
    while(!SysCtlPeripheralReady(SYSCTL_PERIPH_I2C2));

    /* Initialize and Configure the Master Module */
    I2CMasterInitExpClk(I2C2_BASE, sysClock, false);
    return 0;
}

int8_t initI2c(uint32_t sysClock)
{
    initIicPins();
    initIicController(sysClock);

    return 0;
}

int8_t sendIicByte(uint8_t slaveAddr, uint8_t reg, uint8_t *pData)
{
    /* Validate input */
    if(pData == NULL){
        ERROR_PRINT("TIVA I2C sendIic() received NULL pointer for data to write.\n");
        return -1;
    }

    /* set slave address */
    I2CMasterSlaveAddrSet(I2C2_BASE, slaveAddr, false);

    /* send reg address */
    I2CMasterDataPut(I2C2_BASE, reg);
    I2CMasterControl(I2C2_BASE, I2C_MASTER_CMD_SINGLE_SEND);

    /* Wait for I2C to become available */
    while(!I2CMasterBusy(I2C2_BASE));
    while(I2CMasterBusy(I2C2_BASE));

    /* Write data to I2C Device */
    I2CMasterDataPut(I2C2_BASE, pData[0]);
    I2CMasterControl(I2C2_BASE, I2C_MASTER_CMD_SINGLE_SEND);

    /* Wait for I2C to become available */
    while(!I2CMasterBusy(I2C2_BASE));
    while(I2CMasterBusy(I2C2_BASE));

    return 0;
}

int8_t sendIic2Bytes(uint8_t slaveAddr, uint8_t reg, uint8_t *pData)
{
    /* Validate input */
    if(pData == NULL){
        ERROR_PRINT("TIVA I2C sendIic() received NULL pointer for data to write.\n");
        return -1;
    }

    /* set slave address */
    I2CMasterSlaveAddrSet(I2C2_BASE, slaveAddr, false);

    /* send reg address */
    I2CMasterDataPut(I2C2_BASE, reg);
    I2CMasterControl(I2C2_BASE, I2C_MASTER_CMD_SINGLE_SEND);

    /* Wait for I2C to become available */
    while(!I2CMasterBusy(I2C2_BASE));
    while(I2CMasterBusy(I2C2_BASE));

    /* Write data[0] to I2C Device */
    I2CMasterDataPut(I2C2_BASE, pData[0]);
    I2CMasterControl(I2C2_BASE, I2C_MASTER_CMD_BURST_SEND_START);

    /* Wait for I2C to become available */
    while(!I2CMasterBusy(I2C2_BASE));
    while(I2CMasterBusy(I2C2_BASE));

    /* Write data[1] to I2C Device */
    I2CMasterDataPut(I2C2_BASE, pData[1]);
    I2CMasterControl(I2C2_BASE, I2C_MASTER_CMD_BURST_SEND_FINISH);

    /* Wait for I2C to become available */
    while(!I2CMasterBusy(I2C2_BASE));
    while(I2CMasterBusy(I2C2_BASE));

    return 0;
}

int8_t recvIic1Byte(uint8_t slaveAddr, uint8_t reg, uint8_t *pData)
{
    /* Validate input */
    if(pData == NULL){
        ERROR_PRINT("TIVA I2C recvIic1Byte() received NULL pointer for return data.\n");
        return -1;
    }

    /* set slave address */
    I2CMasterSlaveAddrSet(I2C2_BASE, slaveAddr, false);

    /* send reg address */
    I2CMasterDataPut(I2C2_BASE, reg);
    I2CMasterControl(I2C2_BASE, I2C_MASTER_CMD_SINGLE_SEND);

    /* Wait for I2C to become available */
    while(!I2CMasterBusy(I2C2_BASE));
    while(I2CMasterBusy(I2C2_BASE));

    /* set slave address */
    I2CMasterSlaveAddrSet(I2C2_BASE, slaveAddr, true);
    I2CMasterControl(I2C2_BASE, I2C_MASTER_CMD_SINGLE_RECEIVE);

    /* Wait for I2C to become available */
    while(!I2CMasterBusy(I2C2_BASE));
    while(I2CMasterBusy(I2C2_BASE));

    /* Read data returned from I2C bus */
    *pData = I2CMasterDataGet(I2C2_BASE);
    return 0;
}

int8_t recvIic2Bytes(uint8_t slaveAddr, uint8_t reg, uint8_t *pData)
{
    /* Validate input */
    if(pData == NULL) {
        ERROR_PRINT("TIVA I2C recvIic2Bytes() received NULL pointer for return data.\n");
        return -1;
    }

    /* set slave address */
    I2CMasterSlaveAddrSet(I2C2_BASE, slaveAddr, false);

    /* send reg address */
    I2CMasterDataPut(I2C2_BASE, reg);
    I2CMasterControl(I2C2_BASE, I2C_MASTER_CMD_SINGLE_SEND);

    /* Wait for I2C to become available */
    while(!I2CMasterBusy(I2C2_BASE));
    while(I2CMasterBusy(I2C2_BASE));

    /* set slave address */
    I2CMasterSlaveAddrSet(I2C2_BASE, slaveAddr, true);
    I2CMasterControl(I2C2_BASE, I2C_MASTER_CMD_BURST_RECEIVE_START);

    /* Wait for I2C to become available */
    while(!I2CMasterBusy(I2C2_BASE));
    while(I2CMasterBusy(I2C2_BASE));

    pData[0] = I2CMasterDataGet(I2C2_BASE);

    I2CMasterControl(I2C2_BASE, I2C_MASTER_CMD_BURST_RECEIVE_FINISH);
    while(!I2CMasterBusy(I2C2_BASE));
    while(I2CMasterBusy(I2C2_BASE));

    pData[1] = I2CMasterDataGet(I2C2_BASE);
    return 0;
}

