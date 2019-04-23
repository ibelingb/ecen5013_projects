
#ifndef __TIVAIF_H__
#define __TIVAIF_H__

#include <stdint.h>
#include "FreeRTOS.h"
#include "FreeRTOSIPConfig.h"

/* TivaWare includes */
#include "driverlib/emac.h"

/**
 * Set the physical address of the PHY we will be using if this is not
 * specified in lwipopts.h.  We assume 0 for the internal PHY.
 */
#ifndef PHY_PHYS_ADDR
#define PHY_PHYS_ADDR 0
#endif

/*------------------------------------------------------------------------------*/

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
#define PBUF_POOL_BUFSIZE               LWIP_MEM_ALIGN_SIZE(ipconfigNETWORK_MTU)
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
#define TCP_MSS                         ipconfigTCP_MSS
#endif

/**
 * MEM_ALIGNMENT: should be set to the alignment of the CPU
 *    4 byte alignment -> #define MEM_ALIGNMENT 4
 *    2 byte alignment -> #define MEM_ALIGNMENT 2
 */
#ifndef MEM_ALIGNMENT
#define MEM_ALIGNMENT                   8
#endif

/** Calculate memory size for an aligned buffer - returns the next highest
 * multiple of MEM_ALIGNMENT (e.g. LWIP_MEM_ALIGN_SIZE(3) and
 * LWIP_MEM_ALIGN_SIZE(4) will both yield 4 for MEM_ALIGNMENT == 4).
 */
#ifndef LWIP_MEM_ALIGN_SIZE
#define LWIP_MEM_ALIGN_SIZE(size) (((size) + MEM_ALIGNMENT - 1) & ~(MEM_ALIGNMENT-1))
#endif

/*------------------------------------------------------------------------------------------------*/

#ifndef ipconfigNUM_NETWORK_BUFFER_DESCRIPTORS
    #error please define ipconfigNUM_NETWORK_BUFFER_DESCRIPTORS in FreeRTOSIPConfig.h
#endif

#ifndef NUM_RX_DESCRIPTORS
#define NUM_RX_DESCRIPTORS      (ipconfigNUM_NETWORK_BUFFER_DESCRIPTORS - NUM_TX_DESCRIPTORS)
#endif

#ifndef NUM_TX_DESCRIPTORS
#define NUM_TX_DESCRIPTORS      (ipconfigNUM_NETWORK_BUFFER_DESCRIPTORS / 4)
#endif

#if (ipconfigNUM_NETWORK_BUFFER_DESCRIPTORS < (NUM_RX_DESCRIPTORS + NUM_TX_DESCRIPTORS))
    #error n0t enough desciptors
#endif

/*------------------------------------------------------------------------------------------------*/

typedef struct {
  tEMACDMADescriptor Desc;
  uint8_t *pBuf;
} tDescriptor;
extern tDescriptor g_pTxDescriptors[NUM_TX_DESCRIPTORS];
extern tDescriptor g_pRxDescriptors[NUM_RX_DESCRIPTORS];

/*------------------------------------------------------------------------------------------------*/

void InitDMADescriptors(void);
int32_t PacketTransmit(uint8_t *pui8Buf, int32_t i32BufLen);

void processPhyInterrupt(void);
void processTxInterrupt(uint32_t ulISREvents);
void processRxInterrupt(uint32_t ulISREvents);

void printPhyStatus(void);
void printPHYIntStatus(uint32_t phyIsr1Status, uint32_t phyIsr2Status);

#endif // __TIVAIF_H__
