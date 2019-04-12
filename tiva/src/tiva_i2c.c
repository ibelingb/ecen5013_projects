/*
 * from: http://www.ti.com/lit/an/spma073/spma073.pdf
 * https://jspicer.net/2018/07/27/solution-for-i2c-busy-status-latency/
 * https://github.com/jspicer-ltu/Tiva-C-Embedded/tree/master/Experiment17-I2C
 */

#include <stdint.h>
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

int8_t sendIic(uint8_t slaveAddr, uint8_t reg, uint8_t *pData, uint8_t regSize, uint8_t len)
{
    uint8_t ind;

    /* set slave address */
    I2CMasterSlaveAddrSet(I2C2_BASE, slaveAddr, false);

    /* send reg address */
    I2CMasterDataPut(I2C2_BASE, reg);
    I2CMasterControl(I2C2_BASE, I2C_MASTER_CMD_SINGLE_SEND);

//    int error = 0;
//
//    // Clear the RIS bit (master interrupt)
//    i2c->MICR |= I2C_MICR_IC;
//
//    // Set the control flags.
//    i2c->MCS = 0x3;
//
//    // Wait until the RIS bit is set, which indicates the next byte to transfer is being requested.
//    while (!(i2c->MRIS & I2C_MRIS_RIS));

//    // Check the error status.
//    uint32_t mcs = i2c->MCS;
//    error |= mcs & (I2C_MCS_ARBLST | I2C_MCS_DATACK | I2C_MCS_ADRACK | I2C_MCS_ERROR);

    while(!I2CMasterBusy(I2C2_BASE));
    while(I2CMasterBusy(I2C2_BASE));

    /* send reg values */
    for(ind = 0; ind < len; ++ind)
    {
        I2CMasterDataPut(I2C2_BASE, pData[ind]);
        I2CMasterControl(I2C2_BASE, I2C_MASTER_CMD_SINGLE_SEND);

        while(!I2CMasterBusy(I2C2_BASE));
        while(I2CMasterBusy(I2C2_BASE));
    }
    return 0;
}

int8_t recvIic1Byte(uint8_t slaveAddr, uint8_t reg, uint8_t *pData)
{
    /* set slave address */
    I2CMasterSlaveAddrSet(I2C2_BASE, slaveAddr, false);

    /* send reg address */
    I2CMasterDataPut(I2C2_BASE, reg);
    I2CMasterControl(I2C2_BASE, I2C_MASTER_CMD_SINGLE_SEND);

    while(!I2CMasterBusy(I2C2_BASE));
    while(I2CMasterBusy(I2C2_BASE));

    /* set slave address */
    I2CMasterSlaveAddrSet(I2C2_BASE, slaveAddr, true);


    I2CMasterControl(I2C2_BASE, I2C_MASTER_CMD_SINGLE_RECEIVE);
    while(!I2CMasterBusy(I2C2_BASE));
    while(I2CMasterBusy(I2C2_BASE));

    *pData = I2CMasterDataGet(I2C2_BASE);
    return 0;
}

int8_t recvIic2Bytes(uint8_t slaveAddr, uint8_t reg, uint8_t *pData)
{
    /* set slave address */
    I2CMasterSlaveAddrSet(I2C2_BASE, slaveAddr, false);

    /* send reg address */
    I2CMasterDataPut(I2C2_BASE, reg);
    I2CMasterControl(I2C2_BASE, I2C_MASTER_CMD_SINGLE_SEND);

    while(!I2CMasterBusy(I2C2_BASE));
    while(I2CMasterBusy(I2C2_BASE));

    /* set slave address */
    I2CMasterSlaveAddrSet(I2C2_BASE, slaveAddr, true);

    I2CMasterBurstLengthSet(I2C2_BASE, 2);

    I2CMasterControl(I2C2_BASE, I2C_MASTER_CMD_BURST_RECEIVE_START);
    while(!I2CMasterBusy(I2C2_BASE));
    while(I2CMasterBusy(I2C2_BASE));

    pData[0] = I2CMasterDataGet(I2C2_BASE);

    I2CMasterControl(I2C2_BASE, I2C_MASTER_CMD_BURST_RECEIVE_FINISH);
    while(!I2CMasterBusy(I2C2_BASE));
    while(I2CMasterBusy(I2C2_BASE));

    pData[1] = I2CMasterDataGet(I2C2_BASE);
    return 0;
}

