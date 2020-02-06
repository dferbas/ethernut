/*
 * Copyright (C) 2012 by Rittal GmbH & Co. KG. All rights reserved.
 * Copyright (C) 2006 by egnite Software GmbH. All rights reserved.
 * Copyright (C) 2012-2016 Uwe Bonnes (bon@elektron.ikp.physik.tu-darmstadt.de)
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the copyright holders nor the names of
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF
 * THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * For additional information see http://www.ethernut.de/
 *
 */

#include <cfg/arch.h>
#include <cfg/os.h>
#include <cfg/clock.h>
#include <cfg/phycfg.h>
#include <cfg/dev.h>
#include <dev/board.h>

#include <string.h>

#include <sys/atom.h>
#include <sys/heap.h>
#include <sys/thread.h>
#include <sys/event.h>
#include <sys/timer.h>
#include <sys/confnet.h>
#include <sys/mutex.h>

#include <netinet/if_ether.h>
#include <net/ether.h>
#include <net/if_var.h>

#include <dev/irqreg.h>
#include <dev/gpio.h>
#include <dev/phy.h>
#include <dev/pins.h>

#include <arch/cm3/nxp/mk64f_clk.h>
#include <arch/cm3/nxp/vendor/fsl_gpio.h>
#include <arch/cm3/nxp/vendor/fsl_enet.h>
#include <arch/cm3/nxp/vendor/fsl_phy.h>

/* WARNING: Variadic macros are C99 and may fail with C89 compilers. */
#ifdef NUTDEBUG
#include <stdio.h>
#define EMPRINTF(args,...) printf(args,##__VA_ARGS__)
#else
#define EMPRINTF(args,...)
#endif

#ifndef NUT_THREAD_NICRXSTACK
#define NUT_THREAD_NICRXSTACK   1024
#endif


#define ETHMCUX_ENET ENET
#define ETHMCUX_PHY 0x01U
#define CORE_CLK_FREQ CLOCK_GetFreq(kCLOCK_CoreSysClk)
#define ETHMCUX_RXBD_NUM (4)
#define ETHMCUX_TXBD_NUM (4)
#define ETHMCUX_RXBUFF_SIZE (ENET_FRAME_MAX_FRAMELEN)
#define ETHMCUX_TXBUFF_SIZE (ENET_FRAME_MAX_FRAMELEN)
#define ETHMCUX_DATA_LENGTH (1000)
#define ETHMCUX_TRANSMIT_DATA_NUM (20)
#define ETHMCUX_BUFF_ALIGNMENT ENET_BUFF_ALIGNMENT

AT_NONCACHEABLE_SECTION_ALIGN(enet_rx_bd_struct_t g_rxBuffDescrip[ETHMCUX_RXBD_NUM], ENET_BUFF_ALIGNMENT);
AT_NONCACHEABLE_SECTION_ALIGN(enet_tx_bd_struct_t g_txBuffDescrip[ETHMCUX_TXBD_NUM], ENET_BUFF_ALIGNMENT);
/*! @brief The data buffers can be in cacheable region or in non-cacheable region.
 * If use cacheable region, the alignment size should be the maximum size of "CACHE LINE SIZE" and "ENET_BUFF_ALIGNMENT"
 * If use non-cache region, the alignment size is the "ENET_BUFF_ALIGNMENT".
 */
SDK_ALIGN(static uint8_t g_rxDataBuff[ETHMCUX_RXBD_NUM][SDK_SIZEALIGN(ETHMCUX_RXBUFF_SIZE, ETHMCUX_BUFF_ALIGNMENT)],
					ETHMCUX_BUFF_ALIGNMENT);
SDK_ALIGN(static uint8_t g_txDataBuff[ETHMCUX_TXBD_NUM][SDK_SIZEALIGN(ETHMCUX_TXBUFF_SIZE, ETHMCUX_BUFF_ALIGNMENT)],
					ETHMCUX_BUFF_ALIGNMENT);

static enet_handle_t g_handle;
static uint32_t g_testTxNum = 0;
static uint8_t m_frame[ENET_FRAME_MAX_FRAMELEN];


static unsigned int nic_phy_addr = NIC_PHY_ADDR;
 /*!
 * \brief Network interface controller information structure.
 */
struct _ETHMCUXINFO {
#ifdef NUT_PERFMON
		uint32_t ni_rx_packets;       /*!< Number of packets received. */
		uint32_t ni_tx_packets;       /*!< Number of packets sent. */
		uint32_t ni_overruns;         /*!< Number of packet overruns. */
		uint32_t ni_rx_frame_errors;  /*!< Number of frame errors. */
		uint32_t ni_rx_crc_errors;    /*!< Number of CRC errors. */
		uint32_t ni_rx_missed_errors; /*!< Number of missed packets. */
#endif
		HANDLE volatile ni_rx_rdy;  /*!< Receiver event queue. */
		HANDLE volatile ni_tx_rdy;  /*!< Transmitter event queue. */
		MUTEX ni_mutex;            /*!< Access mutex semaphore. */
		volatile int ni_tx_queued;  /*!< Number of packets in transmission queue. */
		volatile int ni_tx_quelen;  /*!< Number of bytes in transmission queue not sent. */
		volatile int ni_insane;     /*!< Set by error detection. */
		int ni_iomode;              /*!< 8 or 16 bit access. 32 bit is not supported. */
};

/*!
 * \brief Network interface controller information type.
 */
typedef struct _ETHMCUXINFO ETHMCUXINFO;

/*!
 * \brief changes the PHY address.
 *
 * \param new PHY address.
 *
 * \remark this must be done before calling the Emac Init Function.
 */
void EthMcux_SetPhyAddr(unsigned int new_phy_addr)
{
	nic_phy_addr = new_phy_addr;
}

/*! \fn EmacRxThread(void *arg)
 * \brief NIC receiver thread.
 *
 */
THREAD(EthMcuxRxThread, arg)
{
	NUTDEVICE *dev;
	IFNET *ifn;
	// ETHMCUXINFO *ni;
	NETBUF *nb;
	uint32_t length = 0;
	enet_data_error_stats_t eErrStatic;
	status_t status;
	int sleep_time;

	dev = arg;
	ifn = (IFNET *) dev->dev_icb;
	ETHMCUXINFO *ni = (ETHMCUXINFO *) dev->dev_dcb;
	// ni = (ETHMCUXINFO *) dev->dev_dcb;

	/* Run at high priority. */
	NutThreadSetPriority(9);

	/* Initialize the access mutex. */
//    NutEventPost(&ni->ni_mutex);
	for (;;) {
		sleep_time = 10;
		/* Get the Frame size */

		NutMutexLock(&ni->ni_mutex);

		status = ENET_GetRxFrameSize(&g_handle, &length);
		/* Call ENET_ReadFrame when there is a received frame. */
		if (length != 0) {
			nb = NutNetBufAlloc(0, NBAF_DATALINK, length);
			if (nb != NULL) {
				/* Received valid frame. Deliver the rx buffer with the size equal to length. */

				status = ENET_ReadFrame(ETHMCUX_ENET, &g_handle, nb->nb_dl.vp, length);
				if (status == kStatus_Success) {
					/* Discard short packets. */
//					if (nb->nb_dl.sz < 60) {
//						NutNetBufFree(nb);
//					} else {
						(*ifn->if_recv) (dev, nb);
						sleep_time = 0;
//					}
//
				} else {
					NutNetBufFree(nb);
				}
			}
		} else if (status == kStatus_ENET_RxFrameError) {
			/* Update the received buffer when error happened. */
			/* Get the error information of the received g_frame. */
			ENET_GetRxErrBeforeReadFrame(&g_handle, &eErrStatic);
			/* update the receive buffer. */
			ENET_ReadFrame(ETHMCUX_ENET, &g_handle, NULL, 0);
		}
		NutMutexUnlock(&ni->ni_mutex);

		if (sleep_time)
			NutSleep(sleep_time);
	}
}

/*!
 * \brief Send Ethernet packet.
 *
 * \todo This routine does not work.
 *
 * \param dev Identifies the device to use.
 * \param nb  Network buffer structure containing the packet to be sent.
 *            The structure must have been allocated by a previous
 *            call NutNetBufAlloc().
 *
 * \return 0 on success, -1 in case of any errors.
 */
int EthMcuxOutput(NUTDEVICE * dev, NETBUF * nb)
{
	int rc = -1;
	// ETHMCUXINFO *ni = (ETHMCUXINFO *) dev->dev_dcb;
	bool link = false;
	unsigned int sz;
	uint8_t *buf = m_frame;

	ETHMCUXINFO *ni = (ETHMCUXINFO *) dev->dev_dcb;

	if ((sz = nb->nb_nw.sz + nb->nb_tp.sz + nb->nb_ap.sz) > ETHERMTU) {
		return -1;
	}

	sz += nb->nb_dl.sz;

	memcpy(buf, nb->nb_dl.vp, nb->nb_dl.sz);
	/* ToDo: find if NUT can send data with MAC at this point already */
	//memcpy(&buf[6], g_macAddr, sizeof(g_macAddr));
	buf += nb->nb_dl.sz;
	memcpy(buf, nb->nb_nw.vp, nb->nb_nw.sz);
	buf += nb->nb_nw.sz; 
	memcpy(buf, nb->nb_tp.vp, nb->nb_tp.sz);
	buf += nb->nb_tp.sz;
	memcpy(buf, nb->nb_ap.vp, nb->nb_ap.sz);
  /* m_frame[12] = (sz >> 8) & 0xFFU;
   * m_frame[13] = sz & 0xFFU; */

	/*
	 * After initialization we are waiting for a long time to give
	 * the PHY a chance to establish an Ethernet link.
	 */

  NutMutexLock(&ni->ni_mutex);

	for (;;) {
		/* Send a multicast frame when the PHY is link up. */
		if (!PHY_GetLinkStatus(ETHMCUX_ENET, ETHMCUX_PHY, &link)) {
			if (link) {
				g_testTxNum++;
				rc = ENET_SendFrame(ETHMCUX_ENET, &g_handle, m_frame, sz);
				if (!rc)
					break;
			}
		}
		NutSleep(500);
	}

  NutMutexUnlock(&ni->ni_mutex);
	return rc;
}

static int EthMcuxIOCtl(NUTDEVICE * dev, int req, void *conf)
{
	int rc = 0;
	uint32_t *lvp = (uint32_t *)conf;
	IFNET *nif = (IFNET *)dev->dev_icb;
//	ETHMCUXINFO *ni = (ETHMCUXINFO *)dev->dev_dcb;

	switch (req)
	{
		case SIOCSIFFLAGS:
			break;

		case SIOCGIFFLAGS:
			*lvp = nif->if_flags;
			break;

		case SIOCSIFADDR:
			/* Set interface hardware address. */
			memcpy(nif->if_mac, conf, sizeof(nif->if_mac));
			ENET_SetMacAddr(ETHMCUX_ENET, nif->if_mac);
			break;

		case SIOCGIFADDR:
			/* Get interface hardware address. */
			memcpy(conf, nif->if_mac, sizeof(nif->if_mac));
			break;

		default:
			rc = -1;
			break;
	}
	return rc;
}


/*!
 * \brief Initialize Ethernet hardware.
 *
 * Applications should do not directly call this function. It is
 * automatically executed during during device registration by
 * NutRegisterDevice().
 *
 * \param dev Identifies the device to initialize.
 */
int EthMcuxInit(NUTDEVICE * dev)
{
	ETHMCUXINFO *ni = (ETHMCUXINFO *) dev->dev_dcb;
	IFNET *nif = (IFNET *)dev->dev_icb;
	enet_config_t config;
	uint32_t sysClock;
	bool link = false;
	phy_speed_t speed;
	phy_duplex_t duplex;
	status_t status;


	/* Clear ETHMCUXINFO structure. */
	memset(ni, 0, sizeof(ETHMCUXINFO));

	enet_buffer_config_t buffConfig[] = {{
		ETHMCUX_RXBD_NUM,
		ETHMCUX_TXBD_NUM,
		SDK_SIZEALIGN(ETHMCUX_RXBUFF_SIZE, ETHMCUX_BUFF_ALIGNMENT),
		SDK_SIZEALIGN(ETHMCUX_TXBUFF_SIZE, ETHMCUX_BUFF_ALIGNMENT),
		&g_rxBuffDescrip[0],
		&g_txBuffDescrip[0],
		&g_rxDataBuff[0][0],
		&g_txDataBuff[0][0],
	}};

	/* Get default configuration. */
	/*
	 * config.miiMode = kENET_RmiiMode;
	 * config.miiSpeed = kENET_MiiSpeed100M;
	 * config.miiDuplex = kENET_MiiFullDuplex;
	 * config.rxMaxFrameLen = ENET_FRAME_MAX_FRAMELEN;
	 */
	ENET_GetDefaultConfig(&config);
	config.miiMode = kENET_MiiMode;
	// config.interrupt |= kENET_RxFrameInterrupt;
	// config.interrupt |= kENET_TxFrameInterrupt;
	// config.interrupt |= kENET_MiiInterrupt;

	/* Set SMI to get PHY link status. */
	sysClock = CORE_CLK_FREQ;
	status   = PHY_Init(ETHMCUX_ENET, ETHMCUX_PHY, sysClock);
	while (status != kStatus_Success)
	{
		status = PHY_Init(ETHMCUX_ENET, ETHMCUX_PHY, sysClock);
	}

	PHY_GetLinkStatus(ETHMCUX_ENET, ETHMCUX_PHY, &link);
	if (link)
	{
		/* Get the actual PHY link speed. */
		PHY_GetLinkSpeedDuplex(ETHMCUX_ENET, ETHMCUX_PHY, &speed, &duplex);
		/* Change the MII speed and duplex for actual link status. */
		config.miiSpeed  = (enet_mii_speed_t)speed;
		config.miiDuplex = (enet_mii_duplex_t)duplex;
	}

	ENET_Init(ETHMCUX_ENET, &g_handle, &config, &buffConfig[0], nif->if_mac, sysClock);
	ENET_ActiveRead(ETHMCUX_ENET);
	/* Register interrupt handler. */
	/*
	if (NutRegisterIrqHandler(&sig_EMAC, EmacInterrupt, dev)) {
			return -1;
	}
	*/

	/* Start the receilver thread. */
	if (NutThreadCreate("emacrx", EthMcuxRxThread, dev,
		(NUT_THREAD_NICRXSTACK * NUT_THREAD_STACK_MULT) + NUT_THREAD_STACK_ADD) == NULL) {
		return -1;
	}

  NutMutexInit(&ni->ni_mutex);

	return 0;
}

static ETHMCUXINFO dcb_eth0;

/*!
 * \brief Network interface information structure.
 *
 * Used to call.
 */
static IFNET ifn_eth0 = {
		IFT_ETHER,                  /*!< \brief Interface type, if_type. */
		0,                          /*!< \brief Interface flags, if_flags. */
		{0, 0, 0, 0, 0, 0},         /*!< \brief Hardware net address, if_mac. */
		0,                          /*!< \brief IP address, if_local_ip. */
		0,                          /*!< \brief Remote IP address for point to point, if_remote_ip. */
		0,                          /*!< \brief IP network mask, if_mask. */
		ETHERMTU,                   /*!< \brief Maximum size of a transmission unit, if_mtu. */
		0,                          /*!< \brief Packet identifier, if_pkt_id. */
		0,                          /*!< \brief Linked list of arp entries, arpTable. */
		0,                          /*!< \brief Linked list of multicast address entries, if_mcast. */
		NutEtherInput,              /*!< \brief Routine to pass received data to, if_recv(). */
		EthMcuxOutput,                 /*!< \brief Driver output routine, if_send(). */
		NutEtherOutput,             /*!< \brief Media output routine, if_output(). */
		NULL                        /*!< \brief Interface specific control function, if_ioctl(). */
#ifdef NUT_PERFMON
		, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
#endif
};

/*!
 * \brief Device information structure.
 *
 * A pointer to this structure must be passed to NutRegisterDevice()
 * to bind this Ethernet device driver to the Nut/OS kernel.
 * An application may then call NutNetIfConfig() with the name \em eth0
 * of this driver to initialize the network interface.
 *
 */
NUTDEVICE devEthMcux = {
		0,                          /*!< \brief Pointer to next device. */
		{'e', 't', 'h', '0', 0, 0, 0, 0, 0},        /*!< \brief Unique device name. */
		IFTYP_NET,                  /*!< \brief Type of device. */
		0,                          /*!< \brief Base address. */
		0,                          /*!< \brief First interrupt number. */
		&ifn_eth0,                  /*!< \brief Interface control block. */
		&dcb_eth0,                  /*!< \brief Driver control block. */
		EthMcuxInit,                /*!< \brief Driver initialization routine. */
		EthMcuxIOCtl,               /*!< \brief Driver specific control function. */
		0,                          /*!< \brief Read from device. */
		0,                          /*!< \brief Write to device. */
#ifdef __HARVARD_ARCH__
		0,                          /*!< \brief Write from program space data to device. */
#endif
		0,                          /*!< \brief Open a device or file. */
		0,                          /*!< \brief Close a device or file. */
		0,                          /*!< \brief Request file size. */
		0,                          /*!< \brief Select function, optional, not yet implemented */
};

/*@}*/

