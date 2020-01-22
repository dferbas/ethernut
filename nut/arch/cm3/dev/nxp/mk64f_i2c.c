/*
 * Copyright 2012-2016 by Embedded Technologies s.r.o. All rights reserved.
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
 */

#include <string.h>

#include <dev/irqreg.h>

#include <sys/event.h>
#include <sys/atom.h>
#include <sys/timer.h>
#include <sys/thread.h>
#include <sys/heap.h>
#include <stdio.h>
#include <dev/twif.h>

#include <arch/cm3/nxp/vendor/fsl_i2c.h>

/*!
 * \addtogroup xgMcf5225
 */
/*@{*/

#define NUT_THREAD_TWI_TRANSACT_STACK   	1000			// TODO .. tune it
#define TWI_MASTER_CLK_SRC I2C0_CLK_SRC
#define TWI_MASTER_CLK_FREQ CLOCK_GetFreq(I2C0_CLK_SRC)
#define TWI_SLAVE_CLK_SRC I2C1_CLK_SRC
#define TWI_SLAVE_CLK_FREQ CLOCK_GetFreq(I2C1_CLK_SRC)
#define TWI_MASTER_IRQ I2C0_IRQn
#define TWI_SLAVE_IRQ I2C1_IRQn
#define TWI_MASTER_IRQHandler I2C0_IRQHandler
#define TWI_SLAVE_IRQHandler I2C1_IRQHandler
#define TWI_MASTER_SLAVE_ADDR_7BIT 0x7EU
#define TWI_DEFAULT_BAUDRATE 400000U

#define MODE_READ       kI2C_Read       /* Work as Receiver */
#define MODE_WRITE      kI2C_Write       /* Work as Transmitter */

enum _i2c_flag_constants
{
/*! All flags which are cleared by the driver upon starting a transfer. */
    kClearFlags = kI2C_ArbitrationLostFlag | kI2C_IntPendingFlag | kI2C_StartDetectFlag | kI2C_StopDetectFlag,
    kIrqFlags   = kI2C_GlobalInterruptEnable | kI2C_StartStopDetectInterruptEnable,
};

static void TwInterrupt(void *arg)
{
	NUTTWIBUS *bus = (NUTTWIBUS *) arg;
	NUTTWIICB *icb = bus->bus_icb;
	I2C_Type *i2c = (I2C_Type *)bus->bus_base;
	uint32_t status_reg;

	/* Clear pending flag. */
	i2c->S = kI2C_IntPendingFlag;

	status_reg = i2c->S;

	if (status_reg & (uint32_t)kI2C_ArbitrationLostFlag) {
		/* Clear arbitration lost flag. */
		i2c->S = kI2C_ArbitrationLostFlag; 
		icb->tw_mm_err = kStatus_I2C_ArbitrationLost;
		goto stop;
	} else if (status_reg & (uint32_t)kI2C_ReceiveNakFlag) {
		if (icb->tw_mm_rxlen != 1) {
			icb->tw_mm_err = kStatus_I2C_Nak;
			goto stop;
		}
	} else if (icb->tw_mm_err) {
		goto finally;
	}


	if (icb->tw_mm_iadrlen) {
		icb->tw_mm_iadrlen--;

		dummy_debug(0xAA, *(icb->tw_mm_iadr));
		i2c->D = *(icb->tw_mm_iadr)++;
		return;
	}

	if (icb->tw_mm_txlen) {
		/* Or transmit data byte if any */
		i2c->D = *(icb->tw_mm_txbuf)++;
		icb->tw_mm_txlen--;
		return;
	}

	if (icb->tw_mm_rxlen) {
		if (icb->tw_mm_dir == MODE_WRITE) {
			icb->tw_mm_err = I2C_MasterRepeatedStart(i2c, icb->tw_mm_sla, kI2C_Read);
			/* Return if error. */
			if (icb->tw_mm_err) {
				goto stop;
			}
			/* Read dummy to free the bus. */
			icb->tw_mm_dir = MODE_READ;
			icb->tw_mm_rxbegin = 0;
			return;
		}

		if (!icb->tw_mm_rxbegin) {
			int dummy;
			/* Setup the I2C peripheral to receive data. */
			i2c->C1 &= ~(uint8_t)(I2C_C1_TX_MASK | I2C_C1_TXAK_MASK);
			/* Do dummy read. */
			dummy = i2c->D;
			icb->tw_mm_rxbegin = 1;

			if (icb->tw_mm_rxlen == 1U) {
				/* Issue NACK on read. */
				i2c->C1 |= I2C_C1_TXAK_MASK;
			}
			return;
		}

		if (icb->tw_mm_rxlen == 1U) {
			i2c->C1 &= ~(uint8_t)(I2C_C1_MST_MASK | I2C_C1_TX_MASK | I2C_C1_TXAK_MASK);
		}

		/* Receive character */
		*(icb->tw_mm_rxbuf) = i2c->D;
		icb->tw_mm_rxbuf++;
		icb->tw_mm_rxlen--;

		if (icb->tw_mm_rxlen == 1U) {
			/* Suppress ACK signal in response if only one char to receive */
			i2c->C1 |= I2C_C1_TXAK_MASK;
		}

		if (!icb->tw_mm_rxlen)
			goto finally;

		return;
	}

stop:
	if ((i2c->C1 & I2C_C1_MST_MASK)) {
		i2c->C1 &= ~(uint8_t)(I2C_C1_MST_MASK | I2C_C1_TX_MASK | I2C_C1_TXAK_MASK);
	}

finally:
	NutEventPostFromIrq(&icb->tw_mm_mtx);
}

int TwiMasterLow(NUTTWIBUS *bus, uint8_t sla, uint32_t iadr
		, uint8_t iadrlen, CONST void *txdata, uint16_t txlen, void *rxdata
		, uint16_t rxsiz, uint32_t tmo)
{
	I2C_Type *i2c = (I2C_Type *)bus->bus_base;
	int rc = -1;
	NUTTWIICB *icb = bus->bus_icb;

	/* Quit if nothing to do */
	if ((txlen == 0) && (rxsiz == 0)) {
		return 0;
	}

	/* This routine is marked reentrant, so lock the interface. */
	if (NutEventWait(&bus->bus_mutex, tmo)) {
		icb->tw_mm_error = TWERR_IF_LOCKED;
		return rc;
	}

	/* Fetch transfer parameters for current transaction */
	icb->tw_mm_sla = sla;
	icb->tw_mm_iadrlen = iadrlen;
	if (iadrlen) {
		iadr = __builtin_bswap32(iadr);
		icb->tw_mm_iadr = ((uint8_t *)&iadr) + 4 - iadrlen;
		dummy_debug(0xB1, icb->tw_mm_iadr);
	}
	icb->tw_mm_txbuf = (uint8_t*) txdata;
	icb->tw_mm_txlen = txlen;
	icb->tw_mm_rxbuf = rxdata;
	icb->tw_mm_rxlen = rxsiz;
	icb->tw_mm_err = TWERR_OK;
	icb->tw_mm_rxbegin = 0;

	if (icb->tw_mm_iadrlen | icb->tw_mm_txlen) {
		icb->tw_mm_dir = MODE_WRITE;
	} else {
		icb->tw_mm_dir = MODE_READ;
	}

	I2C_MasterClearStatusFlags(i2c, (uint32_t)kClearFlags);


  /* Wait until the data register is ready for transmit. */
  while (0U == (i2c->S & (uint8_t)kI2C_TransferCompleteFlag)) {
  }
	/* Issue start and wait till transmission completed */

	if (!I2C_MasterStart(i2c, sla, icb->tw_mm_dir)) {
		/* Enable module interrupt. */
		NutIrqEnable(bus->bus_sig_ev);
		I2C_EnableInterrupts(i2c, kI2C_GlobalInterruptEnable);

		/* Wait for transfer complete. */
		if (NutEventWait(&icb->tw_mm_mtx, tmo)) {
			icb->tw_mm_error = TWERR_TIMEOUT;
		}
		I2C_DisableInterrupts(i2c, kI2C_GlobalInterruptEnable);
		NutIrqDisable(bus->bus_sig_ev);

		/* Check for errors that may have been detected by the interrupt routine. */
		if (icb->tw_mm_err) {
			icb->tw_mm_error = icb->tw_mm_err;
		} else {
			if (rxsiz)
				rc = (int) (rxsiz - icb->tw_mm_rxlen);
			else
				rc = (int) (txlen - icb->tw_mm_txlen);
		}
	}
	

	/* Release the interface. */
	NutEventPost(&bus->bus_mutex);

	return rc;
}




/*!
 * \brief Transmit and/or receive data as a master.
 *
 * The two-wire serial interface must have been initialized by calling
 * TwInit() before this function can be used.
 *
 * \param bus     Pointer to the \ref NUTTWIBUS structure, which is provided
 *                by the bus controller driver.
 * \param sla     Slave address of the destination. This slave address
 *                must be specified as a 7-bit address. For example, the
 *                PCF8574A may be configured to slave addresses from 0x38
 *                to 0x3F.
 * \param txdata  Points to the data to transmit. Ignored, if the number
 *                of data bytes to transmit is zero.
 * \param txlen   Number of data bytes to transmit. If zero, then the
 *                interface will not send any data to the slave device
 *                and will directly enter the master receive mode.
 * \param rxdata  Points to a buffer, where the received data will be
 *                stored. Ignored, if the maximum number of bytes to
 *                receive is zero.
 * \param rxsiz   Maximum number of bytes to receive. Set to zero, if
 *                no bytes are expected from the slave device.
 * \param tmo     Timeout in milliseconds. To disable timeout, set this
 *                parameter to NUT_WAIT_INFINITE.
 *
 * \return The number of bytes received, -1 in case of an error or timeout.
 */
int NutTwiMasterTranceive(NUTTWIBUS *bus, uint8_t sla, CONST void *txdata, uint16_t txlen, void *rxdata, uint16_t rxsiz,
				uint32_t tmo)
{
		return TwiMasterLow(bus, sla, 0, 0, txdata, txlen, rxdata, rxsiz, tmo);
}

/*!
 * \brief Receive data as a master from a device having internal addressable registers
 *
 * The two-wire serial interface must have been initialized by calling
 * TwInit() before this function can be used.
 *
 * \param bus     Pointer to the \ref NUTTWIBUS structure, which is provided
 *                by the bus controller driver.
 * \param sla     Slave address of the destination. This slave address
 *                must be specified as a 7-bit address. For example, the
 *                PCF8574A may be configured to slave addresses from 0x38
 *                to 0x3F.
 * \param iadr    Address send to the device to access certain registers
 *                or memory addresses of it.
 * \param iadrlen Number of bytes to send as address, maximum 3 bytes are
 *                supported from AT91SAM7
 * \param rxdata  Points to a buffer, where the received data will be
 *                stored.
 * \param rxsiz   Maximum number of bytes to receive.
 * \param tmo     Timeout in milliseconds. To disable timeout, set this
 *                parameter to NUT_WAIT_INFINITE.
 *
 * \return The number of bytes received, -1 in case of an error or timeout.
 */
int NutTwiMasterRegRead(NUTTWIBUS *bus, uint8_t sla, uint32_t iadr, uint8_t iadrlen, void *rxdata, uint16_t rxsiz, uint32_t tmo)
{
		return TwiMasterLow(bus, sla, iadr, iadrlen, NULL, 0, rxdata, rxsiz, tmo);
}

/*!
 * \brief Transmit data as a master to a device having internal addressable registers
 *
 * The two-wire serial interface must have been initialized by calling
 * TwInit() before this function can be used.
 *
 * \param bus     Pointer to the \ref NUTTWIBUS structure, which is provided
 *                by the bus controller driver.
 * \param sla     Slave address of the destination. This slave address
 *                must be specified as a 7-bit address. For example, the
 *                PCF8574A may be configured to slave addresses from 0x38
 *                to 0x3F.
 * \param iadr    Address send to the device to access certain registers
 *                or memory addresses of it.
 * \param iadrlen Number of bytes to send as address, maximum 3 bytes are
 *                supported from AT91SAM7
 * \param txdata  Points to a buffer, where the data to transmit will be
 *                stored.
 * \param txsiz   Maximum number of bytes to transmit.
 * \param tmo     Timeout in milliseconds. To disable timeout, set this
 *                parameter to NUT_WAIT_INFINITE.
 *
 * \return        The number of bytes transmitted, -1 in case of an error
 *                or timeout. Number could be less if slave end transmission
 *                with NAK.
 */

int NutTwiMasterRegWrite(NUTTWIBUS *bus, uint8_t sla, uint32_t iadr, uint8_t iadrlen, CONST void *txdata, uint16_t txsiz,
				uint32_t tmo)
{
		return TwiMasterLow(bus, sla, iadr, iadrlen, txdata, txsiz, NULL, 0, tmo);
}



/*!
 * \brief Get last master mode error only from I2C0!!!.
 *
 * You may call this function to determine the specific cause
 * of an error after TwMasterTransact() failed.
 *
 * \note This function is only available on ATmega128 systems.
 *
 */
int NutTwiMasterError(NUTTWIBUS *bus)
{
	int rc = (int) bus->bus_icb->tw_mm_error;
	return rc;
}

/*!
 * \brief Perform TWI control functions.
 *
 * This function is only available on mcf5xxxx systems.
 *
 * \param req  Requested control function. May be set to one of the
 *	       following constants:
 *	       - TWI_SETSPEED + DCB_BASE_MASK_TWIn, if conf points to an uint32_t value speed in Hz.
 *	       - TWI_GETSPEED + DCB_BASE_MASK_TWIn, if conf points to an uint32_t value receiving the current speed in Hz.
 * \param conf Points to a buffer that contains any data required for
 *	       the given control function or receives data from that
 *	       function.
 * \return 0 on success, -1 otherwise.
 *
 * \note Timeout is limited to the granularity of the system timer.
 *
 */
int NutTwiIOCtl(NUTTWIBUS *bus, int req, void *conf)
{
	return -1;
}

int NutRegisterTwiBus(NUTTWIBUS *bus, uint8_t sla)
{
	I2C_Type *i2c = (I2C_Type *)bus->bus_base;
	i2c_master_config_t masterConfig;
	int rc = -1;
	NUTTWIICB *icb = NULL;

	/* Check if bus was already registered */
	if (bus->bus_icb) {
		return 0;
	}

	/* Allocate ICB for this bus */
	if ((icb = NutHeapAllocClear(sizeof(NUTTWIICB))) == NULL) {
		return rc;
	}

	/* Link bus and ICB */
	bus->bus_icb = icb;

	if (NutRegisterIrqHandler(bus->bus_sig_ev, TwInterrupt, bus)) {
		goto err;
	}

	/* Initialize GPIO Hardware */
	if ((bus->bus_initbus != NULL) && ((rc = bus->bus_initbus()))) {
		goto err;
	}

	I2C_MasterGetDefaultConfig(&masterConfig);
	masterConfig.baudRate_Bps = TWI_DEFAULT_BAUDRATE;

	I2C_MasterInit(i2c, &masterConfig, TWI_MASTER_CLK_FREQ);

  NutEventPost(&bus->bus_mutex);
	return 0;

err:
	if (icb) {
		NutHeapFree(icb);
	}

	return rc;
}

/*@}*/
