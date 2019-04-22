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
/* Helper struct holding a DMA descriptor and the pbuf it currently refers to */
typedef struct {
  tEMACDMADescriptor Desc;
  uint8_t *pBuf;
} tDescriptor;

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
#define RX_BUFFER_SIZE 1536
/*-------------------------------------------------------------------------------------------------*/
/* variables */
uint8_t g_ppui8RxBuffer[NUM_RX_DESCRIPTORS][RX_BUFFER_SIZE];
uint32_t g_ui32RxDescIndex;
uint32_t g_ui32TxDescIndex;



/*-------------------------------------------------------------------------------------------------*/
void InitDMADescriptors(void)
{
    uint32_t ui32Loop;

    /* Transmit list -  mark all descriptors as not owned by the hardware */
   for(ui32Loop = 0; ui32Loop < NUM_TX_DESCRIPTORS; ui32Loop++)
   {
       g_pTxDescriptors[ui32Loop].pBuf = (uint8_t *)0;
       g_pTxDescriptors[ui32Loop].Desc.ui32Count = 0;
       g_pTxDescriptors[ui32Loop].Desc.pvBuffer1 = 0;
       g_pTxDescriptors[ui32Loop].Desc.DES3.pLink = ((ui32Loop == (NUM_TX_DESCRIPTORS - 1)) ?
               &g_pTxDescriptors[0].Desc : &g_pTxDescriptors[ui32Loop + 1].Desc);

       //todo: are these flags right for simple tx?
       g_pTxDescriptors[ui32Loop].Desc.ui32CtrlStatus = DES0_TX_CTRL_INTERRUPT |
               DES0_TX_CTRL_LAST_SEG | DES0_TX_CTRL_FIRST_SEG |
               DES0_TX_CTRL_CHAINED | DES0_TX_CTRL_IP_ALL_CKHSUMS;

   }
   g_TxDescList.ui32Read = 0;
   g_TxDescList.ui32Write = 0;

   /* Receive list -  tag each descriptor with a pbuf and set all fields to
    * allow packets to be received */
  for(ui32Loop = 0; ui32Loop < NUM_RX_DESCRIPTORS; ui32Loop++)
  {
      g_pRxDescriptors[ui32Loop].pBuf = &g_ppui8RxBuffer[ui32Loop][0];
      g_pRxDescriptors[ui32Loop].Desc.ui32Count = DES1_RX_CTRL_CHAINED;


      if(g_pRxDescriptors[ui32Loop].pBuf)
      {
          /* Set the DMA to write directly into the pbuf payload */
          g_pRxDescriptors[ui32Loop].Desc.pvBuffer1 = g_pRxDescriptors[ui32Loop].pBuf;
          g_pRxDescriptors[ui32Loop].Desc.ui32Count |=  (RX_BUFFER_SIZE << DES1_RX_CTRL_BUFF1_SIZE_S);

          /* set ownership to MAC (not IP stack) */
          g_pRxDescriptors[ui32Loop].Desc.ui32CtrlStatus = DES0_RX_CTRL_OWN;
      }
      else
      {
          /* No pbuf available so leave the buffer pointer empty */
          g_pRxDescriptors[ui32Loop].Desc.pvBuffer1 = 0;
          g_pRxDescriptors[ui32Loop].Desc.ui32CtrlStatus = 0;
      }
      g_pRxDescriptors[ui32Loop].Desc.DES3.pLink = ((ui32Loop == (NUM_RX_DESCRIPTORS - 1)) ?
              &g_pRxDescriptors[0].Desc : &g_pRxDescriptors[ui32Loop + 1].Desc);
  }

  g_TxDescList.ui32Read = 0;
  g_TxDescList.ui32Write = 0;

  /* Set the descriptor pointers in the hardware */
  EMACRxDMADescriptorListSet(EMAC0_BASE, &g_pRxDescriptors[0].Desc);
  EMACTxDMADescriptorListSet(EMAC0_BASE, &g_pTxDescriptors[0].Desc);

  g_ui32RxDescIndex = 0;
  g_ui32TxDescIndex = NUM_TX_DESCRIPTORS - 1;
}

/**
 * This function should do the actual transmission of the packet. The packet is
 * contained in the pbuf that is passed to the function. This pbuf might be
 * chained.
 *
 * @param psNetif the lwip network interface structure for this ethernetif
 * @param p the MAC packet to send (e.g. IP packet including MAC addresses and type)
 * @return ERR_OK if the packet coui32d be sent
 *         an err_t value if the packet coui32dn't be sent
 */
#ifdef USE_LWIP_LIB
static err_t
tivaif_transmit(struct netif *psNetif, struct pbuf *p)
{
  tStellarisIF *pIF;
  tDescriptor *pDesc;
  struct pbuf *pBuf;
  uint32_t ui32NumChained, ui32NumDescs;
  bool bFirst;
  SYS_ARCH_DECL_PROTECT(lev);

  LWIP_DEBUGF(NETIF_DEBUG, ("tivaif_transmit 0x%08x, len %d\n", p,
              p->tot_len));

  /**
   * This entire function must run within a "critical section" to preserve
   * the integrity of the transmit pbuf queue.
   */
  SYS_ARCH_PROTECT(lev);

  /* Update our transmit attempt counter. */
  DRIVER_STATS_INC(TXCount);

  /**
   * Increase the reference count on the packet provided so that we can
   * hold on to it until we are finished transmitting its content.
   */
  pbuf_ref(p);

  /**
   * Determine whether all buffers passed are within SRAM and, if not, copy
   * the pbuf into SRAM-resident buffers so that the Ethernet DMA can access
   * the data.
   */
  p = tivaif_check_pbuf(p);

  /* Make sure we still have a valid buffer (it may have been copied) */
  if(!p)
  {
      LINK_STATS_INC(link.memerr);
      SYS_ARCH_UNPROTECT(lev);
      return(ERR_MEM);
  }

  /* Get our state data from the netif structure we were passed. */
  pIF = (tStellarisIF *)psNetif->state;

  /* Make sure that the transmit descriptors are not all in use */
  pDesc = &(pIF->pTxDescList->pDescriptors[pIF->pTxDescList->ui32Write]);
  if(pDesc->pBuf)
  {
      /**
       * The current write descriptor has a pbuf attached to it so this
       * implies that the ring is full. Reject this transmit request with a
       * memory error since we can't satisfy it just now.
       */
      pbuf_free(p);
      LINK_STATS_INC(link.memerr);
      DRIVER_STATS_INC(TXNoDescCount);
      SYS_ARCH_UNPROTECT(lev);
      return (ERR_MEM);
  }

  /* How many pbufs are in the chain passed? */
  ui32NumChained = (uint32_t)pbuf_clen(p);

  /* How many free transmit descriptors do we have? */
  ui32NumDescs = (pIF->pTxDescList->ui32Read > pIF->pTxDescList->ui32Write) ?
          (pIF->pTxDescList->ui32Read - pIF->pTxDescList->ui32Write) :
          ((NUM_TX_DESCRIPTORS - pIF->pTxDescList->ui32Write) +
           pIF->pTxDescList->ui32Read);

  /* Do we have enough free descriptors to send the whole packet? */
  if(ui32NumDescs < ui32NumChained)
  {
      /* No - we can't transmit this whole packet so return an error. */
      pbuf_free(p);
      LINK_STATS_INC(link.memerr);
      DRIVER_STATS_INC(TXNoDescCount);
      SYS_ARCH_UNPROTECT(lev);
      return (ERR_MEM);
  }

  /* Tag the first descriptor as the start of the packet. */
  bFirst = true;
  pDesc->Desc.ui32CtrlStatus = DES0_TX_CTRL_FIRST_SEG;

  /* Here, we know we can send the packet so write it to the descriptors */
  pBuf = p;

  while(ui32NumChained)
  {
      /* Get a pointer to the descriptor we will write next. */
      pDesc = &(pIF->pTxDescList->pDescriptors[pIF->pTxDescList->ui32Write]);

      /* Fill in the buffer pointer and length */
      pDesc->Desc.ui32Count = (uint32_t)pBuf->len;
      pDesc->Desc.pvBuffer1 = pBuf->payload;

      /* Tag the first descriptor as the start of the packet. */
      if(bFirst)
      {
          bFirst = false;
          pDesc->Desc.ui32CtrlStatus = DES0_TX_CTRL_FIRST_SEG;
      }
      else
      {
          pDesc->Desc.ui32CtrlStatus = 0;
      }

      pDesc->Desc.ui32CtrlStatus |= (DES0_TX_CTRL_IP_ALL_CKHSUMS |
                                     DES0_TX_CTRL_CHAINED);

      /* Decrement our descriptor counter, move on to the next buffer in the
       * pbuf chain. */
      ui32NumChained--;
      pBuf = pBuf->next;

      /* Update the descriptor list write index. */
      pIF->pTxDescList->ui32Write++;
      if(pIF->pTxDescList->ui32Write == NUM_TX_DESCRIPTORS)
      {
          pIF->pTxDescList->ui32Write = 0;
      }

      /* If this is the last descriptor, mark it as the end of the packet. */
      if(!ui32NumChained)
      {
          pDesc->Desc.ui32CtrlStatus |= (DES0_TX_CTRL_LAST_SEG |
                                         DES0_TX_CTRL_INTERRUPT);

          /* Tag the descriptor with the original pbuf pointer. */
          pDesc->pBuf = p;
      }
      else
      {
          /* Set the lsb of the pbuf pointer.  We use this as a signal that
           * we should not free the pbuf when we are walking the descriptor
           * list while processing the transmit interrupt.  We only free the
           * pbuf when processing the last descriptor used to transmit its
           * chain.
           */
          pDesc->pBuf = (struct pbuf *)((uint32_t)p + 1);
      }

      DRIVER_STATS_INC(TXBufQueuedCount);

      /* Hand the descriptor over to the hardware. */
      pDesc->Desc.ui32CtrlStatus |= DES0_TX_CTRL_OWN;
  }

  /* Tell the transmitter to start (in case it had stopped). */
  EMACTxDMAPollDemand(EMAC0_BASE);

  /* Update lwIP statistics */
  LINK_STATS_INC(link.xmit);

  SYS_ARCH_UNPROTECT(lev);
  return(ERR_OK);
}
#endif
/**
 * This function will process all transmit descriptors and free pbufs attached
 * to any that have been transmitted since we last checked.
 *
 * This function is called only from the Ethernet interrupt handler.
 *
 * @param netif the lwip network interface structure for this ethernetif
 * @return None.
 */
#ifdef USE_LWIP_LIB
static void tivaif_process_transmit(tStellarisIF *pIF)
{
    tDescriptorList *pDescList;
    uint32_t ui32NumDescs;

    /* Get a pointer to the transmit descriptor list. */
    pDescList = pIF->pTxDescList;

    /* Walk the list until we have checked all descriptors or we reach the
     * write pointer or find a descriptor that the hardware is still working
     * on.
     */
    for(ui32NumDescs = 0; ui32NumDescs < pDescList->ui32NumDescs; ui32NumDescs++)
    {
        /* Has the buffer attached to this descriptor been transmitted? */
        if(pDescList->pDescriptors[pDescList->ui32Read].Desc.ui32CtrlStatus &
           DES0_TX_CTRL_OWN)
        {
            /* No - we're finished. */
            break;
        }

        /* Does this descriptor have a buffer attached to it? */
        if(pDescList->pDescriptors[pDescList->ui32Read].pBuf)
        {
            /* Yes - free it if it's not marked as an intermediate pbuf */
            if(!((uint32_t)(pDescList->pDescriptors[pDescList->ui32Read].pBuf) & 1))
            {
                //TODO - hook up to FreeRTOS IP stack?
                //pbuf_free(pDescList->pDescriptors[pDescList->ui32Read].pBuf);
                DRIVER_STATS_INC(TXBufFreedCount);
            }
            pDescList->pDescriptors[pDescList->ui32Read].pBuf = NULL;
        }
        else
        {
            /* If the descriptor has no buffer, we are finished. */
            break;
        }

        /* Move on to the next descriptor. */
        pDescList->ui32Read++;
        if(pDescList->ui32Read == pDescList->ui32NumDescs)
        {
            pDescList->ui32Read = 0;
        }
    }
}
#endif
/**
 * This function will process all receive descriptors that contain newly read
 * data and pass complete frames up the lwIP stack as they are found.  The
 * timestamp of the packet will be placed into the pbuf structure if PTPD is
 * enabled.
 *
 * This function is called only from the Ethernet interrupt handler.
 *
 * @param psNetif the lwip network interface structure for this ethernetif
 * @return None.
 */
#ifdef USE_LWIP_LIB
static void tivaif_receive(struct netif *psNetif)
{

  tDescriptorList *pDescList;
  tStellarisIF *pIF;
  static struct pbuf *pBuf = NULL;
  uint32_t ui32DescEnd;

  /* Get a pointer to our state data */
  pIF = (tStellarisIF *)(psNetif->state);

  /* Get a pointer to the receive descriptor list. */
  pDescList = pIF->pRxDescList;

  /* Start with a NULL pbuf so that we don't try to link chain the first
   * time round.
   */
  //pBuf = NULL;

  /* Determine where we start and end our walk of the descriptor list */
  ui32DescEnd = pDescList->ui32Read ? (pDescList->ui32Read - 1) : (pDescList->ui32NumDescs - 1);

  /* Step through the descriptors that are marked for CPU attention. */
  while(pDescList->ui32Read != ui32DescEnd)
  {
      /* Does the current descriptor have a buffer attached to it? */
      if(pDescList->pDescriptors[pDescList->ui32Read].pBuf)
      {
          /* Yes - determine if the host has filled it yet. */
          if(pDescList->pDescriptors[pDescList->ui32Read].Desc.ui32CtrlStatus &
             DES0_RX_CTRL_OWN)
          {
              /* The DMA engine still owns the descriptor so we are finished */
              break;
          }

          DRIVER_STATS_INC(RXBufReadCount);

          /* If this descriptor contains the end of the packet, fix up the
           * buffer size accordingly.
           */
          if(pDescList->pDescriptors[pDescList->ui32Read].Desc.ui32CtrlStatus &
             DES0_RX_STAT_LAST_DESC)
          {
              /* This is the last descriptor for the frame so fix up the
               * length.  It is safe for us to modify the internal fields
               * directly here (rather than calling pbuf_realloc) since we
               * know each of these pbufs is never chained.
               */
              pDescList->pDescriptors[pDescList->ui32Read].pBuf->len =
                       (pDescList->pDescriptors[pDescList->ui32Read].Desc.ui32CtrlStatus &
                        DES0_RX_STAT_FRAME_LENGTH_M) >>
                        DES0_RX_STAT_FRAME_LENGTH_S;
              pDescList->pDescriptors[pDescList->ui32Read].pBuf->tot_len =
                        pDescList->pDescriptors[pDescList->ui32Read].pBuf->len;
          }

          if(pBuf)
          {
              /* Link this pbuf to the last one we looked at since this buffer
               * is a continuation of an existing frame (split across multiple
               * pbufs).  Note that we use pbuf_cat() here rather than
               * pbuf_chain() since we don't want to increase the reference
               * count of either pbuf - we only want to link them together.
               */
              pbuf_cat(pBuf, pDescList->pDescriptors[pDescList->ui32Read].pBuf);
              pDescList->pDescriptors[pDescList->ui32Read].pBuf = pBuf;
          }

          /* Remember the buffer associated with this descriptor. */
          pBuf = pDescList->pDescriptors[pDescList->ui32Read].pBuf;

          /* Is this the last descriptor for the current frame? */
          if(pDescList->pDescriptors[pDescList->ui32Read].Desc.ui32CtrlStatus &
             DES0_RX_STAT_LAST_DESC)
          {
              /* Yes - does the frame contain errors? */
              if(pDescList->pDescriptors[pDescList->ui32Read].Desc.ui32CtrlStatus &
                 DES0_RX_STAT_ERR)
              {
                  /* This is a bad frame so discard it and update the relevant
                   * statistics.
                   */
                  LWIP_DEBUGF(NETIF_DEBUG, ("tivaif_receive: packet error\n"));
                  pbuf_free(pBuf);
                  LINK_STATS_INC(link.drop);
                  DRIVER_STATS_INC(RXPacketErrCount);
                  pBuf = NULL;
              }
              else
              {
                  /* This is a good frame so pass it up the stack. */
                  LINK_STATS_INC(link.recv);
                  DRIVER_STATS_INC(RXPacketReadCount);

#if LWIP_PTPD
                  /* Place the timestamp in the PBUF if PTPD is enabled */
                  pBuf->time_s =
                       pDescList->pDescriptors[pDescList->ui32Read].Desc.ui32IEEE1588TimeHi;
                  pBuf->time_ns =
                       pDescList->pDescriptors[pDescList->ui32Read].Desc.ui32IEEE1588TimeLo;
#endif

#if NO_SYS
                  if(ethernet_input(pBuf, psNetif) != ERR_OK)
#else
                  if(tcpip_input(pBuf, psNetif) != ERR_OK)
#endif
                  {
                      /* drop the packet */
                      LWIP_DEBUGF(NETIF_DEBUG, ("tivaif_input: input error\n"));
                      pbuf_free(pBuf);

                      /* Adjust the link statistics */
                      LINK_STATS_INC(link.memerr);
                      LINK_STATS_INC(link.drop);
                      DRIVER_STATS_INC(RXPacketCBErrCount);
                  }

                  /* We're finished with this packet so make sure we don't try
                   * to link the next buffer to it.
                   */
                  pBuf = NULL;
              }
          }
      }

      /* Allocate a new buffer for this descriptor */
      pDescList->pDescriptors[pDescList->ui32Read].pBuf = pbuf_alloc(PBUF_RAW,
                                                        PBUF_POOL_BUFSIZE,
                                                        PBUF_POOL);
      pDescList->pDescriptors[pDescList->ui32Read].Desc.ui32Count =
                                                        DES1_RX_CTRL_CHAINED;
      if(pDescList->pDescriptors[pDescList->ui32Read].pBuf)
      {
          /* We got a buffer so fill in the payload pointer and size. */
          pDescList->pDescriptors[pDescList->ui32Read].Desc.pvBuffer1 =
                              pDescList->pDescriptors[pDescList->ui32Read].pBuf->payload;
          pDescList->pDescriptors[pDescList->ui32Read].Desc.ui32Count |=
                              (pDescList->pDescriptors[pDescList->ui32Read].pBuf->len <<
                               DES1_RX_CTRL_BUFF1_SIZE_S);

          /* Give this descriptor back to the hardware */
          pDescList->pDescriptors[pDescList->ui32Read].Desc.ui32CtrlStatus =
                              DES0_RX_CTRL_OWN;
      }
      else
      {
          LWIP_DEBUGF(NETIF_DEBUG, ("tivaif_receive: pbuf_alloc error\n"));

          pDescList->pDescriptors[pDescList->ui32Read].Desc.pvBuffer1 = 0;

          /* Update the stats to show we coui32dn't allocate a pbuf. */
          DRIVER_STATS_INC(RXNoBufCount);
          LINK_STATS_INC(link.memerr);

          /* Stop parsing here since we can't leave a broken descriptor in
           * the chain.
           */
          break;
      }

      /* Move on to the next descriptor in the chain, taking care to wrap. */
      pDescList->ui32Read++;
      if(pDescList->ui32Read == pDescList->ui32NumDescs)
      {
          pDescList->ui32Read = 0;
      }
  }

}
#endif

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
    /* should get here, FreeRTOS NetworkOuput function
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
#if(1)
                /* Allocate a new network buffer descriptor that references an Ethernet
                frame large enough to hold the maximum network packet size (as defined
                in the FreeRTOSIPConfig.h header file). */
                pxDescriptor = pxGetNetworkBufferWithDescriptor( ipTOTAL_ETHERNET_FRAME_SIZE, 0 );

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

//                *( ( NetworkBufferDescriptor_t ** )
//                   ( pxDMARxDescriptor->pvBuffer1 - ipBUFFER_PADDING ) ) = pxDMARxDescriptor;

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
#endif
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
int32_t PacketTransmit(uint8_t *pui8Buf, int32_t i32BufLen)
{
    /* Todo: find next available descriptor */
//    /* Walk the list until we have checked all descriptors or we reach the
//     * write pointer or find a descriptor that the hardware is still working
//     * on. */
//    for(ui32NumDescs = 0; ui32NumDescs < g_TxDescList->ui32NumDescs; ui32NumDescs++)
//    {
//        /* Has the buffer attached to this descriptor been transmitted? */
//        if(g_TxDescList->pDescriptors[g_TxDescList->ui32Read].Desc.ui32CtrlStatus &
//           DES0_TX_CTRL_OWN)
//        {
//            /* No - we're finished. */
//            break;
//        }
//
//        /* Does this descriptor have a buffer attached to it? */
//        if(g_TxDescList->pDescriptors[g_TxDescList->ui32Read].pBuf)
//        {
//            /* Yes - free it if it's not marked as an intermediate pbuf */
//            if(!((uint32_t)(g_TxDescList->pDescriptors[g_TxDescList->ui32Read].pBuf) & 1))
//            {
//                //TODO - hook up to FreeRTOS IP stack?
//                //pbuf_free(g_TxDescList->pDescriptors[g_TxDescList->ui32Read].pBuf);
//            }
//            g_TxDescList->pDescriptors[g_TxDescList->ui32Read].pBuf = NULL;
//        }
//        else {
//            /* If the descriptor has no buffer, we are finished. */
//            break;
//        }
//    }

    /* Wait for the transmit descriptor to free up */
    while(g_pTxDescriptors[g_ui32TxDescIndex].Desc.ui32CtrlStatus &  DES0_TX_CTRL_OWN)
    { /* Spin and waste time */ }

    /* Move to the next descriptor */
    g_ui32TxDescIndex++;
    if(g_ui32TxDescIndex == NUM_TX_DESCRIPTORS) {
        g_ui32TxDescIndex = 0;
    }

    /* Fill in the packet size and pointer, and tell the transmitter to start work */
    g_pTxDescriptors[g_ui32TxDescIndex].Desc.ui32Count = (uint32_t)i32BufLen;
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
    return(i32BufLen);
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
