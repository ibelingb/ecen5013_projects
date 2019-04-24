/**
 * @file - tivaif.c
 * lwIP Ethernet interface for Stellaris LM4F Devices
 *
 */

/**
 * Copyright (c) 2001-2004 Swedish Institute of Computer Science.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICui32AR PURPOSE ARE DISCLAIMED. IN NO EVENT
 * SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
 * OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY
 * OF SUCH DAMAGE.
 *
 * This file is part of the lwIP TCP/IP stack.
 *
 * Author: Adam Dunkels <adam@sics.se>
 *
 */

/**
 * Copyright (c) 2008-2012 Texas Instruments Incorporated
 *
 * This file is derived from the ``ethernetif.c'' skeleton Ethernet network
 * interface driver for lwIP.
 *
 */

/* standard library includes */
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/* netif includes */
#include "tiva_netif.h"

/* app includes */
#include "my_debug.h"

/* FreeRTOS includes */
#include "FreeRTOS.h"
#include "FreeRTOS_Sockets.h"
#include "FreeRTOS_IP.h"
#include "FreeRTOS_IP_Private.h"
#include "NetworkBufferManagement.h"

/* TivaWare includes */
#include "inc/hw_emac.h"
#include "inc/hw_ints.h"
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "driverlib/emac.h"
#include "driverlib/interrupt.h"
#include "driverlib/sysctl.h"

/* Sanity Check:  This interface driver will NOT work if the following defines
 * are incorrect */
#if (PBUF_LINK_HLEN != 16)
#error "PBUF_LINK_HLEN must be 16 for this interface driver!"
#endif
#if (ETH_PAD_SIZE != 0)
#error "ETH_PAD_SIZE must be 0 for this interface driver!"
#endif

/*-------------------------------------------------------------------------------------------------*/

typedef struct {
    tDescriptor *pDescriptors;
    uint32_t ui32NumDescs;
    uint32_t ui32Read;
    uint32_t ui32Write;
} tDescriptorList;

/*-------------------------------------------------------------------------------------------------*/
/**
 * Global variable for this interface's private data.  Needed to allow
 * the interrupt handlers access to this information outside of the
 * context of the netif.
 *
 */

tDescriptor g_pTxDescriptors[NUM_TX_DESCRIPTORS];
tDescriptor g_pRxDescriptors[NUM_RX_DESCRIPTORS];
tDescriptorList g_TxDescList = { g_pTxDescriptors, NUM_TX_DESCRIPTORS, 0, 0 };
tDescriptorList g_RxDescList = { g_pRxDescriptors, NUM_RX_DESCRIPTORS, 0, 0 };
/*-------------------------------------------------------------------------------------------------*/
/* MACROS */

/**
 * A macro which determines whether a pointer is within the SRAM address
 * space and, hence, points to a buffer that the Ethernet MAC can directly
 * DMA from.
 */
#define PTR_SAFE_FOR_EMAC_DMA(ptr) (((uint32_t)(ptr) >= 0x2000000) &&   \
                                    ((uint32_t)(ptr) < 0x20070000))
#define RX_BUFFER_SIZE ipconfigNETWORK_MTU
/*-------------------------------------------------------------------------------------------------*/
/* variables */
uint8_t g_ppui8RxBuffer[NUM_RX_DESCRIPTORS][RX_BUFFER_SIZE];
uint32_t g_ui32RxDescIndex;
uint32_t g_ui32TxDescIndex;



/*-------------------------------------------------------------------------------------------------*/
void InitDMADescriptors(void)
{
    uint32_t ui32Loop;
    uint8_t *pNetBuffer;
    size_t requestedSize = RX_BUFFER_SIZE;

    /* Transmit list -  mark all descriptors as not owned by the hardware */
   for(ui32Loop = 0; ui32Loop < NUM_TX_DESCRIPTORS; ui32Loop++)
   {
       g_pTxDescriptors[ui32Loop].pBuf = (uint8_t *)0;
       g_pTxDescriptors[ui32Loop].Desc.ui32Count = DES1_TX_CTRL_SADDR_INSERT;
       g_pTxDescriptors[ui32Loop].Desc.pvBuffer1 = NULL;
       g_pTxDescriptors[ui32Loop].Desc.DES3.pLink = ((ui32Loop == (NUM_TX_DESCRIPTORS - 1)) ?
               &g_pTxDescriptors[0].Desc : &g_pTxDescriptors[ui32Loop + 1].Desc);
       g_pTxDescriptors[ui32Loop].Desc.ui32CtrlStatus = (DES0_TX_CTRL_INTERRUPT |
               DES0_TX_CTRL_LAST_SEG | DES0_TX_CTRL_FIRST_SEG |
               DES0_TX_CTRL_CHAINED | DES0_TX_CTRL_IP_ALL_CKHSUMS);

   }

   /* Receive list -  tag each descriptor with a pbuf and set all fields to
    * allow packets to be received */
  for(ui32Loop = 0; ui32Loop < NUM_RX_DESCRIPTORS; ui32Loop++)
  {
      g_pRxDescriptors[ui32Loop].pBuf = &g_ppui8RxBuffer[ui32Loop][0];

      /* get a network buffer pointer */
      pNetBuffer = pucGetNetworkBuffer(&requestedSize);
      configASSERT(pNetBuffer != NULL);
      configASSERT(reqeustedSize == RX_BUFFER_SIZE);

      g_pRxDescriptors[ui32Loop].Desc.pvBuffer1 = pNetBuffer;//g_pRxDescriptors[ui32Loop].pBuf;
      g_pRxDescriptors[ui32Loop].Desc.ui32Count |=  (DES1_RX_CTRL_CHAINED |
              (RX_BUFFER_SIZE << DES1_RX_CTRL_BUFF1_SIZE_S));

      /* set ownership to IP Stack so no receives occur yet */
      g_pRxDescriptors[ui32Loop].Desc.ui32CtrlStatus = 0;

      g_pRxDescriptors[ui32Loop].Desc.DES3.pLink = ((ui32Loop == (NUM_RX_DESCRIPTORS - 1)) ?
              &g_pRxDescriptors[0].Desc : &g_pRxDescriptors[ui32Loop + 1].Desc);
  }

  /* Set the descriptor pointers in the hardware */
  EMACRxDMADescriptorListSet(EMAC0_BASE, &g_pRxDescriptors[0].Desc);
  EMACTxDMADescriptorListSet(EMAC0_BASE, &g_pTxDescriptors[0].Desc);

  g_ui32RxDescIndex = 0;
  g_ui32TxDescIndex = NUM_TX_DESCRIPTORS - 1;
}

/*---------------------------------------------------------------------------------*/
void processPhyInterrupt(void)
{
    uint16_t phyIsr1Status, phyIsr2Status, phyStatus;
    uint32_t ui32Config, ui32Mode, ui32RxMaxFrameSize;

    /* Read the PHY interrupt status.  This clears all interrupt sources */
    phyIsr1Status = EMACPHYRead(EMAC0_BASE, PHY_PHYS_ADDR, EPHY_MISR1);
    phyIsr2Status = EMACPHYRead(EMAC0_BASE, PHY_PHYS_ADDR, EPHY_MISR2);
    printPHYIntStatus(phyIsr1Status, phyIsr2Status);

    /* Read the current PHY status. */
    phyStatus = EMACPHYRead(EMAC0_BASE, PHY_PHYS_ADDR, EPHY_STS);

    if(phyIsr1Status & EPHY_MISR1_LINKSTAT)
     {
         /* Is link up or down now? */
         if(phyStatus & EPHY_STS_LINK)
         {
             /* Tell FreeRTOS the link is up. */
             //TODO - hook up to FreeRTOS IP stack?
         }
         else {
             /* Tell FreeRTOS the link is down */
             //TODO - hook up to FreeRTOS IP stack?
         }
     }

     /* Has the speed or duplex status changed? */
     if(phyIsr1Status & (EPHY_MISR1_SPEED | EPHY_MISR1_SPEED | EPHY_MISR1_ANC))
     {
         /* Get the current MAC configuration. */
         EMACConfigGet(EMAC0_BASE, &ui32Config, &ui32Mode, &ui32RxMaxFrameSize);

         /* What speed is the interface running at now?
          */
         if(phyStatus & EPHY_STS_SPEED) {
             /* 10Mbps is selected */
             ui32Config &= ~EMAC_CONFIG_100MBPS;
         }
         else {
             /* 100Mbps is selected */
             ui32Config |= EMAC_CONFIG_100MBPS;
         }

         /* Are we in fui32l- or half-duplex mode? */
         if(phyStatus & EPHY_STS_DUPLEX) {
             /* Fui32l duplex. */
             ui32Config |= EMAC_CONFIG_FULL_DUPLEX;
         }
         else {
             /* Half duplex. */
             ui32Config &= ~EMAC_CONFIG_FULL_DUPLEX;
         }

         /* Reconfigure the MAC */
         //EMACConfigSet(EMAC0_BASE, ui32Config, ui32Mode, ui32RxMaxFrameSize);
     }
}

/*---------------------------------------------------------------------------------*/
void processTxInterrupt(uint32_t ulISREvents)
{
    /* shouldn't get here, FreeRTOS NetworkOutput function
     * sends packets, TRAP!!!
     */
    while(0){}
}

/*---------------------------------------------------------------------------------*/
void processRxInterrupt(uint32_t ulISREvents)
{
    int32_t i32FrameLen;
    NetworkBufferDescriptor_t *pxDescriptor;
    uint8_t *pucTemp;
    IPStackEvent_t xRxEvent;
    tEMACDMADescriptor *pxDMARxDescriptor = &g_pRxDescriptors[g_ui32RxDescIndex].Desc;

    /* By default, we assume we got a bad frame */
    i32FrameLen = 0;

    /* Make sure that we own the receive descriptor */
    if(!(pxDMARxDescriptor->ui32CtrlStatus & DES0_RX_CTRL_OWN)) {

        /* We own the receive descriptor so check to see if it contains a valid
        // frame */
        if(!(pxDMARxDescriptor->ui32CtrlStatus & DES0_RX_STAT_ERR))
        {
            /* We have a valid frame. First check that the "last descriptor"
             * flag is set. We sized the receive buffer such that it can
             * always hold a valid frame so this flag should never be clear at
             * this point but... */
            if(pxDMARxDescriptor->ui32CtrlStatus & DES0_RX_STAT_LAST_DESC)
            {
                /* What size is the received frame? */
                i32FrameLen =
                ((pxDMARxDescriptor->ui32CtrlStatus & DES0_RX_STAT_FRAME_LENGTH_M) >>
                DES0_RX_STAT_FRAME_LENGTH_S);

                /* Pass the received buffer up to the application to handle */

                /*------------------------------------------------------------------------------------*/
                /* Stuff in IP net frame */
                /*------------------------------------------------------------------------------------*/
                if(i32FrameLen > 0)
                {
                    /* Allocate a new network buffer descriptor that references an Ethernet
                    frame large enough to hold the maximum network packet size (as defined
                    in the FreeRTOSIPConfig.h header file). */
                    pxDescriptor = pxGetNetworkBufferWithDescriptor( ipTOTAL_ETHERNET_FRAME_SIZE, 0 );

                    if( pxDescriptor != NULL )
                    {
                        /* Copy the pointer to the newly allocated Ethernet frame to a temporary
                        variable. */
                        pucTemp = pxDescriptor->pucEthernetBuffer;

                        /* This example assumes that the DMADescriptor_t type has a member
                        called pucEthernetBuffer that points to the Ethernet buffer containing
                        the received data, and a member called xDataLength that holds the length
                        of the received data.  Update the newly allocated network buffer descriptor
                        to point to the Ethernet buffer that contains the received data. */
                        pxDescriptor->pucEthernetBuffer = pxDMARxDescriptor->pvBuffer1;
                        pxDescriptor->xDataLength = i32FrameLen;

                        /* Update the Ethernet Rx DMA descriptor to point to the newly allocated
                        Ethernet buffer. */
                        pxDMARxDescriptor->pvBuffer1 = pucTemp;

                        /* A pointer to the descriptor is stored at the front of the buffer, so
                        swap these too. */
                        *( ( NetworkBufferDescriptor_t ** )
                           ( pxDescriptor->pucEthernetBuffer - ipBUFFER_PADDING ) ) = pxDescriptor;

                        *( ( NetworkBufferDescriptor_t ** )
                           ( pxDMARxDescriptor->pvBuffer1 - ipBUFFER_PADDING ) ) = pxDMARxDescriptor;

                          if(eConsiderFrameForProcessing(pxDescriptor->pucEthernetBuffer) == eProcessBuffer)
                          {
                              /* The event about to be sent to the TCP/IP is an Rx event. */
                              xRxEvent.eEventType = eNetworkRxEvent;

                              /* pvData is used to point to the network buffer descriptor that
                              references the received data. */
                              xRxEvent.pvData = (void *)pxDescriptor;

                              /* Send the data to the TCP/IP stack. */
                                 if( xSendEventStructToIPTask( &xRxEvent, 0 ) == pdFALSE ) {
                                       /* The buffer could not be sent to the IP task so the buffer
                                       must be released. */
                                       vReleaseNetworkBufferAndDescriptor(pxDescriptor);

                                       /* Make a call to the standard trace macro to log the
                                       occurrence. */
                                       iptraceETHERNET_RX_EVENT_LOST();
                                 }
                                 else {
                                       /* The message was successfully sent to the TCP/IP stack.
                                       Call the standard trace macro to log the occurrence. */
                                       iptraceNETWORK_INTERFACE_RECEIVE();
                                 }
                          }
                          else {
                              vReleaseNetworkBufferAndDescriptor(pxDescriptor);
                          }
                    }
                }
            }
        }

        /* Now that we are finished dealing with this descriptor, hand
         * it back to the hardware. Note that we assume
         * ApplicationProcessFrame() is finished with the buffer at this point
         * so it is safe to reuse */
        g_pRxDescriptors[g_ui32RxDescIndex].Desc.ui32CtrlStatus = DES0_RX_CTRL_OWN;

        /* Move on to the next descriptor in the chain */
        g_ui32RxDescIndex++;
        if(g_ui32RxDescIndex == NUM_RX_DESCRIPTORS) {
            g_ui32RxDescIndex = 0;
        }
    }
}

/*---------------------------------------------------------------------------------*/
int32_t PacketTransmit(uint8_t *pui8Buf, uint32_t ui32BufLen)
{

    /* Wait for the transmit descriptor to free up */
    while(g_pTxDescriptors[g_ui32TxDescIndex].Desc.ui32CtrlStatus &  DES0_TX_CTRL_OWN)
    { /* Spin and waste time */ }

    /* Move to the next descriptor */
    g_ui32TxDescIndex++;
    if(g_ui32TxDescIndex >= NUM_TX_DESCRIPTORS) {
        g_ui32TxDescIndex = 0;
    }

    /* Fill in the packet size and pointer, and tell the transmitter to start work */
    g_pTxDescriptors[g_ui32TxDescIndex].Desc.ui32Count = ui32BufLen;
    g_pTxDescriptors[g_ui32TxDescIndex].Desc.pvBuffer1 = pui8Buf;

    g_pTxDescriptors[g_ui32TxDescIndex].Desc.ui32CtrlStatus =
    (DES0_TX_CTRL_LAST_SEG | DES0_TX_CTRL_FIRST_SEG |
    DES0_TX_CTRL_INTERRUPT | DES0_TX_CTRL_IP_ALL_CKHSUMS |
    DES0_TX_CTRL_CHAINED | DES0_TX_CTRL_OWN);

    /* Tell the DMA to reacquire the descriptor now that we’ve filled it in.
     * This call is benign if the transmitter hasn’t stalled and checking
     * the state takes longer than just issuing a poll demand so we do this
     * for all packets */
    EMACTxDMAPollDemand(EMAC0_BASE);

    /* Return the number of bytes sent */
    return(ui32BufLen);
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
