
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
#include "driverlib/debug.h"

/* netif includes */
#include "netif/tivaif.h"
#include "lwip/err.h"

/*-----------------------------------------------------------*/
#define EMAC_PHY_CONFIG (EMAC_PHY_TYPE_INTERNAL | EMAC_PHY_INT_MDIX_EN | EMAC_PHY_AN_100B_T_FULL_DUPLEX)

// IP Address Acquisition Modes
#define IPADDR_USE_STATIC       0
#define IPADDR_USE_DHCP         1
#define IPADDR_USE_AUTOIP       2

/*-----------------------------------------------------------*/
static struct netif gNetif = {{0}};
extern uint32_t g_sysClk;

/*-----------------------------------------------------------*/
BaseType_t IPInit(uint32_t sysClk);


/*-----------------------------------------------------------*/
BaseType_t xNetworkInterfaceInitialise( void )
{
    if(IPInit(g_sysClk) != ERR_OK) {
        return pdFAIL;
    }
	return pdPASS;
}
/*-----------------------------------------------------------*/

BaseType_t xGetPhyLinkStatus( void )
{
	return pdPASS;
}
/*-----------------------------------------------------------*/

BaseType_t xNetworkInterfaceOutput( NetworkBufferDescriptor_t * const pxNetworkBuffer, BaseType_t bReleaseAfterSend )
{
    uint32_t errCode;
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

    errCode = gNetif.linkoutput(&gNetif, &macBuff);
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

BaseType_t IPInit(uint32_t sysClk)
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

    /* Lower the priority of the Ethernet interrupt handler.  This is required
     * so that the interrupt handler can safely call the interrupt-safe
     * FreeRTOS functions (specifically to send messages to the queue) */
    IntPrioritySet(INT_EMAC0, 0xC0);

    /*----------------------------------------------------------------------------*
     * Initialize EMAC
     *----------------------------------------------------------------------------*/

    /* Enable the ethernet peripheral */
    SysCtlPeripheralEnable(SYSCTL_PERIPH_EMAC0);
    SysCtlPeripheralReset(SYSCTL_PERIPH_EMAC0);

    /* Enable the internal PHY if it's present and we're being
     * asked to use it. */
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
            while(1)
            {
            }
        }
    }

    /* Wait for the MAC to come out of reset */
    while(!SysCtlPeripheralReady(SYSCTL_PERIPH_EMAC0))
    {
    }

    /* Configure for use with whichever PHY the user requires */
    EMACPHYConfigSet(EMAC0_BASE, EMAC_PHY_CONFIG);

    /* Initialize the MAC and set the DMA mode */
    EMACInit(EMAC0_BASE, sysClk,
                 EMAC_BCONFIG_MIXED_BURST | EMAC_BCONFIG_PRIORITY_FIXED,
                 4, 4, 0);

    /* Set MAC configuration options */
    EMACConfigSet(EMAC0_BASE, (EMAC_CONFIG_FULL_DUPLEX |
                                   EMAC_CONFIG_CHECKSUM_OFFLOAD |
                                   EMAC_CONFIG_7BYTE_PREAMBLE |
                                   EMAC_CONFIG_IF_GAP_96BITS |
                                   EMAC_CONFIG_USE_MACADDR0 |
                                   EMAC_CONFIG_SA_FROM_DESCRIPTOR |
                                   EMAC_CONFIG_BO_LIMIT_1024),
                      (EMAC_MODE_RX_STORE_FORWARD |
                       EMAC_MODE_TX_STORE_FORWARD |
                       EMAC_MODE_TX_THRESHOLD_64_BYTES |
                       EMAC_MODE_RX_THRESHOLD_64_BYTES), 0);

    /* Program the hardware with its MAC address (for filtering) */
    EMACAddrSet(EMAC0_BASE, 0, (uint8_t *)pui8MAC);


    if(tivaif_init(&gNetif) != ERR_OK) {
        return pdFALSE;
    }
    return pdTRUE;
}
/*-----------------------------------------------------------*/
