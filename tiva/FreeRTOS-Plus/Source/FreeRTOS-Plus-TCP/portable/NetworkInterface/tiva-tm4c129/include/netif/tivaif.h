/**
 * @file - tivaif.h
 * Prototypes for Stellaris Ethernet interface.
 *
 */

/*
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
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT
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

/*
 * Copyright (c) 2008 Texas Instruments Incorporated
 *
 */


#ifndef __TIVAIF_H__
#define __TIVAIF_H__

#include <stdint.h>
#include "FreeRTOS.h"
#include "FreeRTOSIPConfig.h"

/**
 * Set the physical address of the PHY we will be using if this is not
 * specified in lwipopts.h.  We assume 0 for the internal PHY.
 */
#ifndef PHY_PHYS_ADDR
#define PHY_PHYS_ADDR 0
#endif

/*------------------------------------------------------------------------------*/
/* from lwip's err.h */

typedef int8_t err_t;
/* Definitions for error constants. */

#define ERR_OK          0    /* No error, everything OK. */
#define ERR_MEM        -1    /* Out of memory error.     */
#define ERR_BUF        -2    /* Buffer error.            */
#define ERR_TIMEOUT    -3    /* Timeout.                 */
#define ERR_RTE        -4    /* Routing problem.         */
#define ERR_INPROGRESS -5    /* Operation in progress    */
#define ERR_VAL        -6    /* Illegal value.           */
#define ERR_WOULDBLOCK -7    /* Operation would block.   */
#define ERR_USE        -8    /* Address in use.          */
#define ERR_ISCONN     -9    /* Already connected.       */

#define ERR_IS_FATAL(e) ((e) < ERR_ISCONN)

#define ERR_ABRT       -10   /* Connection aborted.      */
#define ERR_RST        -11   /* Connection reset.        */
#define ERR_CLSD       -12   /* Connection closed.       */
#define ERR_CONN       -13   /* Not connected.           */

#define ERR_ARG        -14   /* Illegal argument.        */

#define ERR_IF         -15   /* Low-level netif error    */

/*------------------------------------------------------------------------------*/
/* from lwip's opt.h */

/** ETH_PAD_SIZE: number of bytes added before the ethernet header to ensure
 * alignment of payload after that header. Since the header is 14 bytes long,
 * without this padding e.g. addresses in the IP header will not be aligned
 * on a 32-bit boundary, so setting this to 2 can speed up 32-bit-platforms.
 */
#ifndef ETH_PAD_SIZE
#define ETH_PAD_SIZE                    0
#endif

/**
 * PBUF_POOL_BUFSIZE: the size of each pbuf in the pbuf pool. The default is
 * designed to accomodate single full size TCP frame in one pbuf, including
 * TCP_MSS, IP header, and link header.
 */
#ifndef PBUF_POOL_BUFSIZE
#define PBUF_POOL_BUFSIZE               LWIP_MEM_ALIGN_SIZE(TCP_MSS+40+PBUF_LINK_HLEN)
#endif

/**
 * PBUF_LINK_HLEN: the number of bytes that should be allocated for a
 * link level header. The default is 14, the standard value for
 * Ethernet.
 */
#ifndef PBUF_LINK_HLEN
#define PBUF_LINK_HLEN                  (16 + ETH_PAD_SIZE)
#endif

/**
 * TCP_MSS: TCP Maximum segment size. (default is 536, a conservative default,
 * you might want to increase this.)
 * For the receive side, this MSS is advertised to the remote side
 * when opening a connection. For the transmit size, this MSS sets
 * an upper limit on the MSS advertised by the remote host.
 */
#ifndef TCP_MSS
#define TCP_MSS                         536
#endif

/**
 * MEM_ALIGNMENT: should be set to the alignment of the CPU
 *    4 byte alignment -> #define MEM_ALIGNMENT 4
 *    2 byte alignment -> #define MEM_ALIGNMENT 2
 */
#ifndef MEM_ALIGNMENT
#define MEM_ALIGNMENT                   1
#endif

/** Calculate memory size for an aligned buffer - returns the next highest
 * multiple of MEM_ALIGNMENT (e.g. LWIP_MEM_ALIGN_SIZE(3) and
 * LWIP_MEM_ALIGN_SIZE(4) will both yield 4 for MEM_ALIGNMENT == 4).
 */
#ifndef LWIP_MEM_ALIGN_SIZE
#define LWIP_MEM_ALIGN_SIZE(size) (((size) + MEM_ALIGNMENT - 1) & ~(MEM_ALIGNMENT-1))
#endif

/*------------------------------------------------------------------------------------*/

/* From lwip's netif.h or opt.h or pbuf.h ------------------------------------------*/

/** Whether the network interface is 'up'. This is
 * a software flag used to control whether this network
 * interface is enabled and processes traffic.
 * It is set by the startup code (for static IP configuration) or
 * by dhcp/autoip when an address has been assigned.
 */
#define NETIF_FLAG_UP           0x01U
/** If set, the netif has broadcast capability.
 * Set by the netif driver in its init function. */
#define NETIF_FLAG_BROADCAST    0x02U
/** If set, the netif is one end of a point-to-point connection.
 * Set by the netif driver in its init function. */
#define NETIF_FLAG_POINTTOPOINT 0x04U
/** If set, the interface is configured using DHCP.
 * Set by the DHCP code when starting or stopping DHCP. */
#define NETIF_FLAG_DHCP         0x08U
/** If set, the interface has an active link
 *  (set by the network interface driver).
 * Either set by the netif driver in its init function (if the link
 * is up at that time) or at a later point once the link comes up
 * (if link detection is supported by the hardware). */
#define NETIF_FLAG_LINK_UP      0x10U
/** If set, the netif is an ethernet device using ARP.
 * Set by the netif driver in its init function.
 * Used to check input packet types and use of DHCP. */
#define NETIF_FLAG_ETHARP       0x20U
/** If set, the netif is an ethernet device. It might not use
 * ARP or TCP/IP if it is used for PPPoE only.
 */
#define NETIF_FLAG_ETHERNET     0x40U
/** If set, the netif has IGMP capability.
 * Set by the netif driver in its init function. */
#define NETIF_FLAG_IGMP         0x80U

#ifndef ETHARP_HWADDR_LEN
#define ETHARP_HWADDR_LEN     6
#endif

/* This is the aligned version of ip_addr_t,
   used as local variable, on the stack, etc. */
/** ip_addr_t uses a struct for convenience only, so that the same defines can
 * operate both on ip_addr_t as well as on ip_addr_p_t. */
typedef struct
{
  uint32_t addr;
} ip_addr_t;

struct pbuf
{
  /** next pbuf in singly linked pbuf chain */
  struct pbuf *next;
  void *payload;                        /** pointer to the actual data in the buffer */
  uint16_t tot_len;                        /** total length of this buffer and all next buffers in chain
                                         * belonging to the same packet.
                                         *
                                         * For non-queue packet chains this is the invariant:
                                         * p->tot_len == p->len + (p->next? p->next->tot_len: 0)
                                         */
  uint16_t len;                            /** length of this buffer */
  uint8_t type;                            /** pbuf_type as u8_t instead of enum to save space */
  uint8_t flags;                           /** misc flags */
  uint16_t ref;                            /**
                                         * the reference count always equals the number of pointers
                                         * that refer to this pbuf. This can be pointers from an application,
                                         * the stack itself, or pbuf->next pointers from a chain.
                                         */
};

/** Function prototype for netif->input functions. This function is saved as 'input'
 * callback function in the netif struct. Call it when a packet has been received.
 *
 * @param p The received packet, copied into a pbuf
 * @param inp The netif which received the packet
 */
typedef err_t (*netif_input_fn)(struct pbuf *p, struct netif *inp);
/** Function prototype for netif->output functions. Called by lwIP when a packet
 * shall be sent. For ethernet netif, set this to 'etharp_output' and set
 * 'linkoutput'.
 *
 * @param netif The netif which shall send a packet
 * @param p The packet to send (p->payload points to IP header)
 * @param ipaddr The IP address to which the packet shall be sent
 */
typedef err_t (*netif_output_fn)(struct netif *netif, struct pbuf *p, ip_addr_t *ipaddr);
/** Function prototype for netif->linkoutput functions. Only used for ethernet
 * netifs. This function is called by ARP when a packet shall be sent.
 *
 * @param netif The netif which shall send a packet
 * @param p The packet to send (raw ethernet packet)
 */
typedef err_t (*netif_linkoutput_fn)(struct netif *netif, struct pbuf *p);
/** Function prototype for netif status- or link-callback functions. */

/** must be the maximum of all used hardware address lengths
    across all types of interfaces in use */
#define NETIF_MAX_HWADDR_LEN 6U

/** Generic data structure used for all lwIP network interfaces.
 *  The following fields should be filled in by the initialization
 *  function for the device driver: hwaddr_len, hwaddr[], mtu, flags */
struct netif
{
  struct netif *next;                   /** pointer to next in linked list */
  ip_addr_t ip_addr;                    /** IP address configuration in network byte order */
  ip_addr_t netmask;
  ip_addr_t gw;
  netif_input_fn input;                 /** This function is called by the network device driver
                                         *  to pass a packet up the TCP/IP stack. */

  netif_output_fn output;               /** This function is called by the IP module when it wants
                                         *  to send a packet on the interface. This function typically
                                         *  first resolves the hardware address, then sends the packet. */

  netif_linkoutput_fn linkoutput;       /** This function is called by the ARP module when it wants
                                         *  to send a packet on the interface. This function outputs
                                         *  the pbuf as-is on the link medium. */

  void *state;                          /** This field can be set by the device driver and could point
                                         *  to state information for the device. */
  struct dhcp *dhcp;                    /** the DHCP client state information for this netif */
  struct autoip *autoip;                /** the AutoIP client state information for this netif */
  uint16_t mtu;                            /** maximum transfer unit (in bytes) */
  uint8_t hwaddr_len;                      /** number of bytes used in hwaddr */
  uint8_t hwaddr[NETIF_MAX_HWADDR_LEN];    /** link level hardware address of this interface */
  uint8_t flags;                           /** flags (see NETIF_FLAG_ above) */
  char name[2];                         /** descriptive abbreviation */
  uint8_t num;                             /** number of this interface */
};

/*------------------------------------------------------------------------------------------------*/

/**
 * If necessary, set the defaui32t number of transmit and receive DMA descriptors
 * used by the Ethernet MAC.
 *
 */
#ifndef NUM_RX_DESCRIPTORS
#define NUM_RX_DESCRIPTORS ipconfigNUM_NETWORK_BUFFER_DESCRIPTORS
#endif

#ifndef NUM_TX_DESCRIPTORS
#define NUM_TX_DESCRIPTORS ipconfigNUM_NETWORK_BUFFER_DESCRIPTORS
#endif

//extern int tivaif_input(struct netif *psNetif);
//extern err_t tivaif_init(struct netif *psNetif);
//extern void tivaif_interrupt(struct netif *netif, uint32_t ui32Status);

#if NETIF_DEBUG
void tivaif_debug_print(struct pbuf *psBuf);
#else
#define tivaif_debug_print(psBuf)
#endif /* NETIF_DEBUG */

#endif // __TIVAIF_H__
