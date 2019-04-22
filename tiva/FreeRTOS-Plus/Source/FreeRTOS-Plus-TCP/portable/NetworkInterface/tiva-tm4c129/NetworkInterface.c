/***********************************************************************************
 * @author Joshua Malburg
 * joshua.malburg@colorado.edu
 * Advanced Embedded Software Development
 * ECEN5013 - Rick Feidebrecht
 * @date April 21, 2019
 * CCS  Version: 8.3.0.00009
 ************************************************************************************
 *
 * @file NetworkInterface.c
 * @brief
 *
 * References:
 * https://www.freertos.org/FreeRTOS-Plus/FreeRTOS_Plus_TCP/
 * TCP_IP_Configuration.html#ipconfigNUM_NETWORK_BUFFER_DESCRIPTORS
 * https://www.freertos.org/FreeRTOS-Plus/FreeRTOS_Plus_TCP/
 * Embedded_Ethernet_Porting.html#creating_a_simple_network_interface_port_layer
 *
 * https://www.freertos.org/Debugging-Hard-Faults-On-Cortex-M-Microcontrollers.html
 *
 ************************************************************************************
 */

/* Standard includes */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdbool.h>

/* FreeRTOS includes */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"

/* FreeRTOS+TCP includes */
#include "FreeRTOS_IP.h"
#include "FreeRTOS_Sockets.h"
#include "FreeRTOS_IP_Private.h"
#include "NetworkBufferManagement.h"
#include "NetworkInterface.h"

/* TivaWare includes */
#include "driverlib/emac.h"         /* for ip stack init */
#include "driverlib/interrupt.h"    /* for ip stack init */
#include "inc/hw_ints.h"            /* for ip stack init */
#include "driverlib/flash.h"        /* for ip stack init */
#include "driverlib/sysctl.h"       /* for clk */
#include "inc/hw_memmap.h"
#include "inc/hw_emac.h"
#include "driverlib/debug.h"

/* my includes */
#include "tiva_netif.h"
#include "my_debug.h"

/*-----------------------------------------------------------*/
/* MACROS */
/*-----------------------------------------------------------*/

/* define PHY configuration */
#define EMAC_PHY_CONFIG (EMAC_PHY_TYPE_INTERNAL | EMAC_PHY_INT_MDIX_EN | EMAC_PHY_AN_100B_T_FULL_DUPLEX)

/* IP Address Acquisition Modes */
#define IPADDR_USE_STATIC       0
#define IPADDR_USE_DHCP         1
#define IPADDR_USE_AUTOIP       2


/* Netif interrupt handling task */
#ifndef configEMAC_TASK_STACK_SIZE
#define configEMAC_TASK_STACK_SIZE (2 * configMINIMAL_STACK_SIZE)
#endif

#if (ipconfigZERO_COPY_TX_DRIVER == 1)
    #ifndef EMAC_MAX_BLOCK_TIME_MS
        #define EMAC_MAX_BLOCK_TIME_MS  100ul
    #endif


    #ifndef PHY_LS_HIGH_CHECK_TIME_MS
        /* Check if the LinkSStatus in the PHY is still high after 15 seconds of not
        receiving packets. */
        #define PHY_LS_HIGH_CHECK_TIME_MS   15000
    #endif

    #ifndef PHY_LS_LOW_CHECK_TIME_MS
        /* Check if the LinkSStatus in the PHY is still low every second. */
        #define PHY_LS_LOW_CHECK_TIME_MS    1000
    #endif

    /* Interrupt events to process */
    #define EMAC_IF_RX_EVENT        1UL
    #define EMAC_IF_TX_EVENT        2UL
    #define EMAC_IF_ERR_EVENT       4UL
    #define EMAC_IF_ALL_EVENT       ( EMAC_IF_RX_EVENT | EMAC_IF_TX_EVENT | EMAC_IF_ERR_EVENT )
#endif

/*-----------------------------------------------------------*/
extern uint32_t g_sysClk;


    /* Holds the handle of the task used as a deferred interrupt processor.  The
    handle is used so direct notifications can be sent to the task for all EMAC/DMA
    related interrupts. */
    TaskHandle_t xEMACTaskHandle = NULL;
    static xQueueHandle g_pInterrupt;

/*-----------------------------------------------------------*/
BaseType_t InitMACPHY(uint32_t sysClk);
void prvEMACHandlerTask(void *pvParameters);

#if (ipconfigZERO_COPY_TX_DRIVER == 1)

    void EthernetHandler(void);
#endif

/*-----------------------------------------------------------*/
BaseType_t xNetworkInterfaceInitialise( void )
{
    if(InitMACPHY(g_sysClk) != pdTRUE) {
        return pdFAIL;
    }

    /* create interrupt queue */
    g_pInterrupt = xQueueCreate(ipconfigEVENT_QUEUE_LENGTH, sizeof(uint32_t));
    if(g_pInterrupt == 0) {
        return pdFAIL;
    }

    /* The handler task is created at the highest possible priority to
    ensure the interrupt handler can return directly to it. */
    xTaskCreate( prvEMACHandlerTask, "emacEvent", configEMAC_TASK_STACK_SIZE, NULL, configMAX_PRIORITIES - 1, &xEMACTaskHandle );
    configASSERT( xEMACTaskHandle );

	return pdPASS;
}
/*-----------------------------------------------------------*/

BaseType_t xNetworkInterfaceOutput( NetworkBufferDescriptor_t * const pxNetworkBuffer, BaseType_t bReleaseAfterSend )
{
    uint32_t errCode = ERR_OK;
    uint32_t ulTransmitSize;
    struct pbuf macBuff;

    /* Simple network interfaces (as opposed to more efficient zero copy network
    interfaces) just use Ethernet peripheral driver library functions to copy
    data from the FreeRTOS+TCP buffer into the peripheral driver's own buffer.
    This example assumes SendData() is a peripheral driver library function that
    takes a pointer to the start of the data to be sent and the length of the
    data to be sent as two separate parameters.  The start of the data is located
    by pxDescriptor->pucEthernetBuffer.  The length of the data is located
    by pxDescriptor->xDataLength. */
    macBuff.payload = pxNetworkBuffer->pucEthernetBuffer;
    macBuff.len = pxNetworkBuffer->xDataLength;
    macBuff.tot_len = pxNetworkBuffer->xDataLength;

    /* Get bytes in current buffer  */
    ulTransmitSize = pxNetworkBuffer->xDataLength;

    if(ulTransmitSize > PBUF_POOL_BUFSIZE)
    {
        ulTransmitSize = PBUF_POOL_BUFSIZE;
    }

    #if( ipconfigZERO_COPY_TX_DRIVER == 0 )
    {
        /* Copy the bytes. */
        //memcpy( ( void * ) pxDmaTxDesc->Buffer1Addr, pxNetworkBuffer->pucEthernetBuffer, ulTransmitSize);
    }
    #else
    {
        /* Move the buffer. */
        pxDmaTxDesc->Buffer1Addr = ( uint32_t )pxNetworkBuffer->pucEthernetBuffer;
        /* The Network Buffer has been passed to DMA, no need to release it. */
        bReleaseAfterSend = pdFALSE_UNSIGNED;
    }
    #endif /* ipconfigZERO_COPY_TX_DRIVER */
    //todo: errCode = gNetif.linkoutput(&gNetif, &macBuff);
    if(errCode != ERR_OK) {
        return pdFAIL;
    }

    /* Call the standard trace macro to log the send event. */
    iptraceNETWORK_INTERFACE_TRANSMIT();

    if( bReleaseAfterSend != pdFALSE )
    {
        /* It is assumed SendData() copies the data out of the FreeRTOS+TCP Ethernet
        buffer.  The Ethernet buffer is therefore no longer needed, and must be
        freed for re-use. */
        vReleaseNetworkBufferAndDescriptor( pxNetworkBuffer );
    }
    return pdTRUE;
}

/*-----------------------------------------------------------*/

BaseType_t InitMACPHY(uint32_t sysClk)
{
    uint32_t ui32User0, ui32User1;
    uint8_t pui8MAC[6];

    /* Get the MAC address from the user registers */
    FlashUserGet(&ui32User0, &ui32User1);
    if((ui32User0 == 0xffffffff) || (ui32User1 == 0xffffffff))
    {
        return pdFALSE;
    }

    /* Convert the 24/24 split MAC address from NV ram into a 32/16 split MAC
     * address needed to program the hardware registers, then program the MAC
     * address into the Ethernet Controller registers */
    pui8MAC[0] = ((ui32User0 >>  0) & 0xff);
    pui8MAC[1] = ((ui32User0 >>  8) & 0xff);
    pui8MAC[2] = ((ui32User0 >> 16) & 0xff);
    pui8MAC[3] = ((ui32User1 >>  0) & 0xff);
    pui8MAC[4] = ((ui32User1 >>  8) & 0xff);
    pui8MAC[5] = ((ui32User1 >> 16) & 0xff);


#if (ipconfigZERO_COPY_TX_DRIVER == 1)
    /* Lower the priority of the Ethernet interrupt handler.  This is required
     * so that the interrupt handler can safely call the interrupt-safe
     * FreeRTOS functions (specifically to send messages to the queue) */
    IntPrioritySet(INT_EMAC0_TM4C129, 0xC0);
#endif
    /*----------------------------------------------------------------------------*
     * Initialize EMAC & PHY
     *----------------------------------------------------------------------------*/

    /* Enable the ethernet peripheral */
    SysCtlPeripheralEnable(SYSCTL_PERIPH_EMAC0);
    SysCtlPeripheralReset(SYSCTL_PERIPH_EMAC0);

    /* Enable the internal PHY if it's present and we're being asked to use it. */
    if((EMAC_PHY_CONFIG & EMAC_PHY_TYPE_MASK) == EMAC_PHY_TYPE_INTERNAL)
    {
        /* We've been asked to configure for use with the internal
         * PHY.  Is it present? */
        if(SysCtlPeripheralPresent(SYSCTL_PERIPH_EPHY0))
        {
            /* Yes - enable and reset it. */
            SysCtlPeripheralEnable(SYSCTL_PERIPH_EPHY0);
            SysCtlPeripheralReset(SYSCTL_PERIPH_EPHY0);
        }
        else
        {
            /* Internal PHY is not present on this part so hang here. */
            while(1) {}
        }
    }

    /* Wait for the MAC to come out of reset */
    while(!SysCtlPeripheralReady(SYSCTL_PERIPH_EMAC0)) {}

    /* Configure for use with whichever PHY the user requires */
    EMACPHYConfigSet(EMAC0_BASE, EMAC_PHY_CONFIG);

    /* Initialize the MAC and set the DMA mode */
    EMACInit(EMAC0_BASE, sysClk,
                 EMAC_BCONFIG_MIXED_BURST | EMAC_BCONFIG_PRIORITY_FIXED,
                 4, 4, 0);

    /* Set MAC configuration options */
    EMACConfigSet(EMAC0_BASE, (EMAC_CONFIG_FULL_DUPLEX | EMAC_CONFIG_CHECKSUM_OFFLOAD |
                                   EMAC_CONFIG_7BYTE_PREAMBLE | EMAC_CONFIG_IF_GAP_96BITS |
                                   EMAC_CONFIG_USE_MACADDR0 | EMAC_CONFIG_SA_FROM_DESCRIPTOR |
                                   EMAC_CONFIG_BO_LIMIT_1024),
                      (EMAC_MODE_RX_STORE_FORWARD | EMAC_MODE_TX_STORE_FORWARD |
                       EMAC_MODE_TX_THRESHOLD_64_BYTES | EMAC_MODE_RX_THRESHOLD_64_BYTES), 0);

    /* Program the hardware with its MAC address (for filtering) */
    EMACAddrSet(EMAC0_BASE, 0, (uint8_t *)pui8MAC);

    /*------------------------------------------------------------------------------------------*/
    /* from tivaif_init (tiva-tm4c129.c) */
    /*------------------------------------------------------------------------------------------*/
    uint16_t ui16Val;

    /* Initialize the DMA descriptors. */
    InitDMADescriptors();

    /* Clear any stray PHY interrupts that may be set. */
    ui16Val = EMACPHYRead(EMAC0_BASE, PHY_PHYS_ADDR, EPHY_MISR1);
    ui16Val = EMACPHYRead(EMAC0_BASE, PHY_PHYS_ADDR, EPHY_MISR2);

    /* Configure and enable the link status change interrupt in the PHY. */
    ui16Val = EMACPHYRead(EMAC0_BASE, PHY_PHYS_ADDR, EPHY_SCR);
    ui16Val |= (EPHY_SCR_INTEN_EXT | EPHY_SCR_INTOE_EXT);
    EMACPHYWrite(EMAC0_BASE, PHY_PHYS_ADDR, EPHY_SCR, ui16Val);
    EMACPHYWrite(EMAC0_BASE, PHY_PHYS_ADDR, EPHY_MISR1, (EPHY_MISR1_LINKSTATEN |
                 EPHY_MISR1_SPEEDEN | EPHY_MISR1_DUPLEXMEN | EPHY_MISR1_ANCEN));

    /* Read the PHY interrupt status to clear any stray events. */
    ui16Val = EMACPHYRead(EMAC0_BASE, PHY_PHYS_ADDR, EPHY_MISR1);

    /**
     * Set MAC filtering options.  We receive all broadcast and mui32ticast
     * packets along with those addressed specifically for us.
     */
    EMACFrameFilterSet(EMAC0_BASE, (EMAC_FRMFILTER_HASH_AND_PERFECT |
                       EMAC_FRMFILTER_PASS_MULTICAST));

    /* Clear any pending MAC interrupts. */
    EMACIntClear(EMAC0_BASE, EMACIntStatus(EMAC0_BASE, false));

    /* Enable the Ethernet MAC transmitter and receiver. */
    EMACTxEnable(EMAC0_BASE);
    EMACRxEnable(EMAC0_BASE);

    /* Enable the Ethernet RX and TX interrupt source. */
    EMACIntEnable(EMAC0_BASE, (EMAC_INT_RECEIVE | EMAC_INT_TRANSMIT |
                  EMAC_INT_TX_STOPPED | EMAC_INT_RX_NO_BUFFER |
                  EMAC_INT_RX_STOPPED | EMAC_INT_PHY));


    /* Enable the Ethernet interrupt. */
    IntEnable(INT_EMAC0);
#if (ipconfigZERO_COPY_TX_DRIVER == 1)
    /* Enable all processor interrupts. */
    IntMasterEnable();
#endif

    /* Tell the PHY to start an auto-negotiation cycle. */
    EMACPHYWrite(EMAC0_BASE, PHY_PHYS_ADDR, EPHY_BMCR, (EPHY_BMCR_ANEN |
                 EPHY_BMCR_RESTARTAN));

    printPhyStatus();

    return pdTRUE;
}

/*-----------------------------------------------------------*/
void printPhyStatus(void)
{
    uint16_t phyStatus;

    /* Read the current PHY status. */
    phyStatus = EMACPHYRead(EMAC0_BASE, PHY_PHYS_ADDR, EPHY_STS);
    INFO_PRINT("PHY status: %d", phyStatus);

    if(phyStatus & EPHY_STS_LINK) {
        INFO_PRINT(", link up");
    }
    else {
        INFO_PRINT(", link down");
    }

    /* What speed is the interface running at now?
     */
    if(phyStatus & EPHY_STS_SPEED) {
        /* 10Mbps is selected */
        INFO_PRINT(", 10 Mbps speed");
    }
    else {
        /* 100Mbps is selected */
        INFO_PRINT(", 100 Mbps speed");
    }

    /* Are we in fui32l- or half-duplex mode? */
    if(phyStatus & EPHY_STS_DUPLEX) {
        /* Full duplex. */
        INFO_PRINT(", full duplex\n");
    }
    else {
        /* Half duplex. */
        INFO_PRINT(", half duplex\n");
    }
}

/*-----------------------------------------------------------*/
void printMACIntStatus(uint32_t intStatus)
{
    INFO_PRINT("MAC INT status: %d", intStatus);

    if(intStatus & EMAC_INT_PHY) {
        INFO_PRINT(", EMAC_INT_PHY");
    }
    if(intStatus & EMAC_INT_EARLY_RECEIVE) {
        INFO_PRINT(", EMAC_INT_EARLY_RECEIVE");
    }
    if(intStatus & EPHY_STS_DUPLEX) {
        INFO_PRINT(", EMAC_INT_BUS_ERROR");
    }
    if(intStatus & EMAC_INT_EARLY_TRANSMIT) {
        INFO_PRINT(", EMAC_INT_EARLY_TRANSMIT");
    }
    if(intStatus & EMAC_INT_RX_WATCHDOG) {
        INFO_PRINT(", EMAC_INT_RX_WATCHDOG");
    }
    if(intStatus & EMAC_INT_RX_STOPPED) {
        INFO_PRINT(", EMAC_INT_RX_STOPPED");
    }
    if(intStatus & EMAC_INT_RX_NO_BUFFER) {
        INFO_PRINT(", EMAC_INT_RX_NO_BUFFER");
    }
    if(intStatus & EMAC_INT_RECEIVE) {
        INFO_PRINT(", EMAC_INT_RECEIVE");
    }
    if(intStatus & EMAC_INT_TX_UNDERFLOW) {
        INFO_PRINT(", EMAC_INT_TX_UNDERFLOW");
    }
    if(intStatus & EMAC_INT_RX_OVERFLOW) {
        INFO_PRINT(", EMAC_INT_RX_OVERFLOW");
    }
    if(intStatus & EMAC_INT_TX_JABBER) {
        INFO_PRINT(", EMAC_INT_TX_JABBER");
    }
    if(intStatus & EMAC_INT_TX_NO_BUFFER) {
        INFO_PRINT(", EMAC_INT_TX_NO_BUFFER");
    }
    if(intStatus & EMAC_INT_TRANSMIT) {
        INFO_PRINT(", EMAC_INT_TRANSMIT");
    }
    INFO_PRINT("\n");
}

/*-----------------------------------------------------------*/
void printPHYIntStatus(uint32_t phyIsr1Status, uint32_t phyIsr2Status)
{
    INFO_PRINT("PHY ISR1 status: %d", phyIsr1Status);

    if(phyIsr1Status & EPHY_MISR1_LINKSTAT) {
        INFO_PRINT(", EPHY_MISR1_LINKSTAT");
    }
    if(phyIsr1Status & EPHY_MISR1_SPEED) {
        INFO_PRINT(", EPHY_MISR1_SPEED");
    }
    if(phyIsr1Status & EPHY_MISR1_DUPLEXM) {
        INFO_PRINT(", EPHY_MISR1_DUPLEXM");
    }
    if(phyIsr1Status & EPHY_MISR1_ANC) {
        INFO_PRINT(", EPHY_MISR1_ANC");
    }
    if(phyIsr1Status & EPHY_MISR1_FCHF) {
        INFO_PRINT(", EPHY_MISR1_FCHF");
    }
    if(phyIsr1Status & EPHY_MISR1_RXHF) {
        INFO_PRINT(", EPHY_MISR1_RXHF");
    }
    if(phyIsr1Status & EPHY_MISR1_LINKSTATEN) {
        INFO_PRINT(", EPHY_MISR1_LINKSTATEN");
    }
    if(phyIsr1Status & EPHY_MISR1_SPEEDEN) {
        INFO_PRINT(", EPHY_MISR1_SPEEDEN");
    }
    if(phyIsr1Status & EPHY_MISR1_DUPLEXMEN) {
        INFO_PRINT(", EPHY_MISR1_DUPLEXMEN");
    }
    if(phyIsr1Status & EPHY_MISR1_ANCEN) {
        INFO_PRINT(", EPHY_MISR1_ANCEN");
    }
    if(phyIsr1Status & EPHY_MISR1_FCHFEN) {
        INFO_PRINT(", EPHY_MISR1_FCHFEN");
    }
    if(phyIsr1Status & EPHY_MISR1_RXHFEN) {
        INFO_PRINT(", EPHY_MISR1_RXHFEN");
    }
    INFO_PRINT("\n");
    INFO_PRINT("PHY ISR2 status: %d", phyIsr2Status);

    /* IRS2 */
    if(phyIsr2Status & EPHY_MISR2_ANERR) {
        INFO_PRINT(", EPHY_MISR2_ANERR");
    }
    if(phyIsr2Status & EPHY_MISR2_PAGERX) {
        INFO_PRINT(", EPHY_MISR2_PAGERX");
    }
    if(phyIsr2Status & EPHY_MISR2_LBFIFO) {
        INFO_PRINT(", EPHY_MISR2_LBFIFO");
    }
    if(phyIsr2Status & EPHY_MISR2_MDICO) {
        INFO_PRINT(", EPHY_MISR2_MDICO");
    }
    if(phyIsr2Status & EPHY_MISR2_SLEEP) {
        INFO_PRINT(", EPHY_MISR2_SLEEP");
    }
    if(phyIsr2Status & EPHY_MISR2_POLINT) {
        INFO_PRINT(", EPHY_MISR2_POLINT");
    }
    if(phyIsr2Status & EPHY_MISR2_JABBER) {
        INFO_PRINT(", EPHY_MISR2_JABBER");
    }
    if(phyIsr2Status & EPHY_MISR2_ANERREN) {
        INFO_PRINT(", EPHY_MISR2_ANERREN");
    }
    if(phyIsr2Status & EPHY_MISR2_PAGERXEN) {
        INFO_PRINT(", EPHY_MISR2_PAGERXEN");
    }
    if(phyIsr2Status & EPHY_MISR2_LBFIFOEN) {
        INFO_PRINT(", EPHY_MISR2_LBFIFOEN");
    }
    if(phyIsr2Status & EPHY_MISR2_MDICOEN) {
        INFO_PRINT(", EPHY_MISR2_MDICOEN");
    }
    if(phyIsr2Status & EPHY_MISR2_SLEEPEN) {
        INFO_PRINT(", EPHY_MISR2_SLEEPEN");
    }
    if(phyIsr2Status & EPHY_MISR2_POLINTEN) {
        INFO_PRINT(", EPHY_MISR2_POLINTEN");
    }
    if(phyIsr2Status & EPHY_MISR2_JABBEREN) {
        INFO_PRINT(", EPHY_MISR2_JABBEREN");
    }
}
/*------------------------------------------------------------------------*/
/**
 * @brief this is the deferred ethernet interrupt handler task
 */
void prvEMACHandlerTask( void *pvParameters )
{

    TimeOut_t xPhyTime;
    TickType_t xPhyRemTime;
    UBaseType_t uxCount;
    #if( ipconfigZERO_COPY_TX_DRIVER != 0 )
        NetworkBufferDescriptor_t *pxBuffer;
        const TickType_t ulMaxBlockTime = pdMS_TO_TICKS( EMAC_MAX_BLOCK_TIME_MS );
    #endif
    uint8_t *pucBuffer;
    BaseType_t xResult = 0;
    uint32_t xStatus;
    const TickType_t xDelay = (1000  / portTICK_PERIOD_MS);
    uint32_t ulISREvents;
    uint16_t phyIsr1Status, phyIsr2Status;

    configASSERT( xEMACTaskHandle );

    for(;;)
    {
        /* get interrupt msg */
        if(xQueueReceive(g_pInterrupt, (void *)&ulISREvents, xDelay) != pdFALSE) {
            printMACIntStatus(ulISREvents);

            if(ulISREvents & EMAC_INT_PHY) {
                /* Read the PHY interrupt status.  This clears all interrupt sources.
                 * Note that we are only enabling sources in EPHY_MISR1 so we don't
                 * read EPHY_MISR2.
                 */
                phyIsr1Status = EMACPHYRead(EMAC0_BASE, PHY_PHYS_ADDR, EPHY_MISR1);
                phyIsr2Status = EMACPHYRead(EMAC0_BASE, PHY_PHYS_ADDR, EPHY_MISR2);
                printPHYIntStatus(phyIsr1Status, phyIsr2Status);
            }
        }
        /* Enable the Ethernet RX and TX interrupt source. */
        EMACIntEnable(EMAC0_BASE, (EMAC_INT_RECEIVE | EMAC_INT_TRANSMIT |
                      EMAC_INT_TX_STOPPED | EMAC_INT_RX_NO_BUFFER |
                      EMAC_INT_RX_STOPPED | EMAC_INT_PHY));
    }
#if (ipconfigZERO_COPY_TX_DRIVER == 1)

    vTaskSetTimeOutState( &xPhyTime );
    xPhyRemTime = pdMS_TO_TICKS( PHY_LS_LOW_CHECK_TIME_MS );

    for( ;; )
    {
        vCheckBuffersAndQueue();

        if( ( ulISREvents & EMAC_IF_ALL_EVENT ) == 0 )
        {
            /* No events to process now, wait for the next. */
            ulTaskNotifyTake( pdFALSE, ulMaxBlockTime );
        }

        if( ( ulISREvents & EMAC_IF_RX_EVENT ) != 0 )
        {
            ulISREvents &= ~EMAC_IF_RX_EVENT;

            /* Wait for the EMAC interrupt to indicate that another packet has been
            received. */
            xResult = prvEMACRxPoll();
        }

        if( ( ulISREvents & EMAC_IF_TX_EVENT ) != 0 )
        {
            /* Future extension: code to release TX buffers if zero-copy is used. */
            ulISREvents &= ~EMAC_IF_TX_EVENT;
            while( xQueueReceive( xTxBufferQueue, &pucBuffer, 0 ) != pdFALSE )
            {
                #if( ipconfigZERO_COPY_TX_DRIVER != 0 )
                {
                    pxBuffer = pxPacketBuffer_to_NetworkBuffer( pucBuffer );
                    if( pxBuffer != NULL )
                    {
                        vReleaseNetworkBufferAndDescriptor( pxBuffer );
                        tx_release_count[ 0 ]++;
                    }
                    else
                    {
                        tx_release_count[ 1 ]++;
                    }
                }
                #else
                {
                    tx_release_count[ 0 ]++;
                }
                #endif
                uxCount = uxQueueMessagesWaiting( ( QueueHandle_t ) xTXDescriptorSemaphore );
                if( uxCount < NUM_TX_DESCRIPTORS )
                {
                    /* Tell the counting semaphore that one more TX descriptor is available. */
                    xSemaphoreGive( xTXDescriptorSemaphore );
                }
            }
        }

        if( ( ulISREvents & EMAC_IF_ERR_EVENT ) != 0 )
        {
            /* Future extension: logging about errors that occurred. */
            ulISREvents &= ~EMAC_IF_ERR_EVENT;
        }

        if( xResult > 0 )
        {
            /* A packet was received. No need to check for the PHY status now,
            but set a timer to check it later on. */
            vTaskSetTimeOutState( &xPhyTime );
            xPhyRemTime = pdMS_TO_TICKS( PHY_LS_HIGH_CHECK_TIME_MS );
            xResult = 0;
        }
        else if( xTaskCheckForTimeOut( &xPhyTime, &xPhyRemTime ) != pdFALSE )
        {
            /* Check the link status again. */
            xStatus = ulReadMDIO( PHY_REG_01_BMSR );

            if( ( ulPHYLinkStatus & BMSR_LINK_STATUS ) != ( xStatus & BMSR_LINK_STATUS ) )
            {
                ulPHYLinkStatus = xStatus;
                FreeRTOS_printf( ( "prvEMACHandlerTask: PHY LS now %d\n", ( ulPHYLinkStatus & BMSR_LINK_STATUS ) != 0 ) );
            }

            vTaskSetTimeOutState( &xPhyTime );
            if( ( ulPHYLinkStatus & BMSR_LINK_STATUS ) != 0 )
            {
                xPhyRemTime = pdMS_TO_TICKS( PHY_LS_HIGH_CHECK_TIME_MS );
            }
            else
            {
                xPhyRemTime = pdMS_TO_TICKS( PHY_LS_LOW_CHECK_TIME_MS );
            }
        }
    }
#endif
}
/*-------------------------------------------------------------------------------------------------------------------------*/
//*****************************************************************************
//
// This is the code that gets called when the processor receives an ethernet
// interrupt.  This simply enters an infinite loop, preserving the system state
// for examination by a debugger.
//
//*****************************************************************************
void xEthernetHandler(void)
{
    uint32_t ui32Status;
    uint32_t ui32TimerStatus;
    portBASE_TYPE xWake = pdFALSE;

    /* Read and Clear the interrupt. */
    ui32Status = EMACIntStatus(EMAC0_BASE, true);

    /* If the PMT mode exit status bit is set then enable the MAC transmit
     * and receive paths, read the PMT status to clear the interrupt and
     * clear the interrupt flag */
    if(ui32Status & EMAC_INT_POWER_MGMNT)
    {
        EMACTxEnable(EMAC0_BASE);
        EMACRxEnable(EMAC0_BASE);
        EMACPowerManagementStatusGet(EMAC0_BASE);
        ui32Status &= ~(EMAC_INT_POWER_MGMNT);
    }

    /*If the interrupt really came from the Ethernet and not our timer, clear it */
    if(ui32Status) {
        EMACIntClear(EMAC0_BASE, ui32Status);
    }

    /* Check to see whether a hardware timer interrupt has been reported */
    if(ui32Status & EMAC_INT_TIMESTAMP) {
        /* Yes - read and clear the timestamp interrupt status */
        ui32TimerStatus = EMACTimestampIntStatus(EMAC0_BASE);
    }

    /* Signal the deferred Ethernet interrupt task */
    xQueueSendFromISR(g_pInterrupt, (void *)&ui32Status, &xWake);
    //
    // Disable the Ethernet interrupts.  Since the interrupts have not been
    // handled, they are not asserted.  Once they are handled by the deferrerd Ethernet
    // interrupt task, it will re-enable the interrupts.
    //
    EMACIntDisable(EMAC0_BASE, (EMAC_INT_RECEIVE | EMAC_INT_TRANSMIT |
                                    EMAC_INT_TX_STOPPED | EMAC_INT_RX_NO_BUFFER |
                                    EMAC_INT_RX_STOPPED | EMAC_INT_PHY));

#if (ipconfigZERO_COPY_TX_DRIVER == 1)
    /* Potentially task switch as a result of the above queue write */
    if(xWake == pdTRUE) {
        portYIELD_FROM_ISR(true);
    }
#endif
}
