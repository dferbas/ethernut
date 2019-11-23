/*!
 * Copyright (C) 2001-2010 by egnite Software GmbH
 *
 * All rights reserved.
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


/*
 * $Log: usart.c,v $
 *
 */

#include <cfg/clock.h>

#include <sys/atom.h>
#include <sys/event.h>
#include <sys/timer.h>

#include <dev/irqreg.h>
#include <dev/gpio.h>
#include <dev/usartavr32.h>

#include <avr32/io.h>
#include <arch/avr32/gpio.h>

// Work around missing defines in some AVR32 models
#if !defined(AVR32_USART_CSR_MASK)
#define AVR32_USART_CSR_MASK 0xFFFFFFFF
#endif

#if !defined(AVR32_USART_IDR_MASK)
#define AVR32_USART_IDR_MASK 0xFFFFFFFF
#endif

/*!
 * \addtogroup xgNutArchAvr32Usart
 */
/*@{*/

/* \brief ASCII code for software flow control, starts transmitter. */
#define ASCII_XON   0x11
/* \brief ASCII code for software flow control, stops transmitter. */
#define ASCII_XOFF  0x13

/* \brief XON transmit pending flag. */
#define XON_PENDING     0x10
/* \brief XOFF transmit pending flag. */
#define XOFF_PENDING    0x20
/* \brief XOFF sent flag. */
#define XOFF_SENT       0x40
/* \brief XOFF received flag. */
#define XOFF_RCVD       0x80


/*!
 * \brief Receiver error flags.
 */
static ureg_t rx_errors;

/*!
 * \brief Enables software flow control if not equal zero.
 */
static uint_fast8_t flow_control;

/*!
 * \brief Transmit address frame, if not zero.
 */
static uint_fast8_t tx_aframe;

#ifdef UART_HDX_BIT
        /* define in cfg/modem.h */
#ifdef UART_HDX_FLIP_BIT        /* same as RTS toggle by Windows NT driver */
#define UART_HDX_TX             cbi
#define UART_HDX_RX             sbi
#else                           /* previous usage by Ethernut */
#define UART_HDX_TX             sbi
#define UART_HDX_RX             cbi
#endif
#endif


#ifdef UART_HDX_BIT
/*!
 * \brief Enables half duplex control if not equal zero.
 *
 * This variable exists only if the hardware configuration defines a
 * port bit to switch between receive and transmit mode.
 */
static uint_fast8_t hdx_control;
#endif

//#if defined(UART_RTS_BIT) || defined(US_MODE_HWHANDSHAKE)
/*!
 * \brief Enables RTS control if not equal zero.
 *
 * This variable exists only if the hardware configuration defines a
 * port bit to control the RTS signal.
 */
static uint_fast8_t rts_control;
//#endif

//#if defined(UART_CTS_BIT) || defined(US_MODE_HWHANDSHAKE)
/*!
 * \brief Enables CTS sense if not equal zero.
 *
 * This variable exists only if the hardware configuration defines a
 * port bit to sense the CTS signal.
 */
static uint_fast8_t cts_sense;
//#endif

#ifdef UART_CTS_BIT
/*!
 * \brief USARTn CTS sense interrupt handler.
 *
 * This interrupt routine is called when the CTS line level is low.
 * Typical line drivers negate the signal, thus driving our port
 * low when CTS is active.
 *
 * This routine exists only if the hardware configuration defines a
 * port bit to sense the CTS signal.
 */
static void Avr32UsartCts(void *arg)
{
    /* Enable transmit interrupt. */
	outr(USARTn_BASE + AVR32_USART_IER, AVR32_USART_IER_TXRDY_MASK);
    /* Disable CTS sense interrupt. */
    //cbi(EIMSK, UART_CTS_BIT);
}
#endif

#ifdef UART_HDX_BIT
/*
 * \brief USARTn transmitter empty interrupt handler.
 *
 * Used with half duplex communication to switch from tranmit to receive
 * mode after the last character has been transmitted.
 *
 * This routine exists only if the hardware configuration defines a
 * port bit to switch between receive and transmit mode.
 *
 * \param arg Pointer to the transmitter ring buffer.
 */
static void Avr32UsartTxEmpty(RINGBUF * rbf)
{
    /*
     * Check if half duplex mode has been enabled and if all characters
     * had been sent out.
     */
    if (hdx_control && rbf->rbf_cnt == 0) {
        /* Switch to receiver mode. */
        UART_HDX_RX(UART_HDX_PORT, UART_HDX_BIT);
    }
}
#endif

/*
 * \brief USARTn transmitter ready interrupt handler.
 *
 * \param rbf Pointer to the transmitter ring buffer.
 */
static void Avr32UsartTxReady(RINGBUF * rbf)
{
    register uint8_t *cp = rbf->rbf_tail;
	volatile avr32_usart_t* usart = (avr32_usart_t*)USARTn_BASE;

    /*
     * Process pending software flow controls first.
     */
    if (flow_control & (XON_PENDING | XOFF_PENDING)) {
        if (flow_control & XON_PENDING) {
			usart->thr = (ASCII_XOFF << AVR32_USART_THR_TXCHR_OFFSET) & AVR32_USART_THR_TXCHR_MASK;
            flow_control |= XOFF_SENT;
        } else {
			usart->thr = (ASCII_XON << AVR32_USART_THR_TXCHR_OFFSET) & AVR32_USART_THR_TXCHR_MASK;
            flow_control &= ~XOFF_SENT;
        }
        flow_control &= ~(XON_PENDING | XOFF_PENDING);
        return;
    }

    if (flow_control & XOFF_RCVD) {
        /*
         * If XOFF has been received, we disable the transmit interrupts
         * and return without sending anything.
         */
		usart->idr = AVR32_USART_IDR_TXRDY_MASK;
		usart->csr;
        return;
    }

    if (rbf->rbf_cnt) {

#ifdef UART_CTS_BIT
        /*
         * If CTS has been disabled, we disable the transmit interrupts
         * and return without sending anything.
         */
		if (cts_sense && GpioPinGet(UART_CTS_PORT, UART_CTS_BIT)) {
			usart->idr = AVR32_USART_IDR_TXRDY_MASK;
			usart->csr;
            return;
        }
#endif
        rbf->rbf_cnt--;

        /*
         * Send address in multidrop mode.
         */
        if (tx_aframe) {
			usart->cr |= AVR32_USART_CR_SENDA_MASK;
        }

        /*
         * Start transmission of the next character.
         */
		usart->thr = (*cp << AVR32_USART_THR_TXCHR_OFFSET) & AVR32_USART_THR_TXCHR_MASK;

        /*
         * Wrap around the buffer pointer if we reached its end.
         */
        if (++cp == rbf->rbf_last) {
            cp = rbf->rbf_start;
        }
        rbf->rbf_tail = cp;
        if (rbf->rbf_cnt == rbf->rbf_lwm) {
            NutEventPostFromIrq(&rbf->rbf_que);
			NutSelectWakeupFromIrq(rbf->wq_list, WQ_FLAG_WRITE);
        }
    }

    /*
     * Nothing left to transmit, disable interrupt.
     */
    else {
		usart->idr = AVR32_USART_IDR_TXRDY_MASK;
		usart->csr;
        rbf->rbf_cnt = 0;
        NutEventPostFromIrq(&rbf->rbf_que);
		NutSelectWakeupFromIrq(rbf->wq_list, WQ_FLAG_WRITE);
    }
}


/*
 * \brief USARTn receiver ready interrupt handler.
 *
 *
 * \param rbf Pointer to the receiver ring buffer.
 */
#define UART_READMULTIBYTE
static void Avr32UsartRxReady(RINGBUF * rbf)
{
    register size_t cnt;
    register uint8_t ch;
	volatile avr32_usart_t* usart = (avr32_usart_t*)USARTn_BASE;

#ifdef UART_READMULTIBYTE
    register uint8_t postEvent = 0;
    do {
#endif
    /*
     * We read the received character as early as possible to avoid overflows
     * caused by interrupt latency.
     */
    ch = (usart->rhr & AVR32_USART_RHR_RXCHR_MASK) >> AVR32_USART_RHR_RXCHR_OFFSET;

    /* Collect receiver errors. */
    rx_errors |= usart->csr & (AVR32_USART_CSR_OVRE_MASK | AVR32_USART_CSR_FRAME_MASK | AVR32_USART_CSR_PARE_MASK);

    /*
     * Handle software handshake. We have to do this before checking the
     * buffer, because flow control must work in write-only mode, where
     * there is no receive buffer.
     */
    if (flow_control) {
        /* XOFF character disables transmit interrupts. */
        if (ch == ASCII_XOFF) {
			usart->idr = AVR32_USART_IDR_TXRDY_MASK;
			usart->csr;
            flow_control |= XOFF_RCVD;
            return;
        }
        /* XON enables transmit interrupts. */
        else if (ch == ASCII_XON) {
			usart->ier = AVR32_USART_IER_TXRDY_MASK;
			usart->csr;
            flow_control &= ~XOFF_RCVD;
            return;
        }
    }

    /*
     * Check buffer overflow.
     */
    cnt = rbf->rbf_cnt;
    if (cnt >= rbf->rbf_siz) {
        rx_errors |= AVR32_USART_CSR_OVRE_MASK;
        return;
    }

    /* Wake up waiting threads if this is the first byte in the buffer. */
    if (cnt++ == 0) {
#ifdef UART_READMULTIBYTE
            // we do this later, to get the other bytes in time..
            postEvent = 1;
#else
            NutEventPostFromIrq(&rbf->rbf_que);
            NutSelectWakeupFromIrq(rbf->wq_list, WQ_FLAG_READ);
#endif    
    }

    /*
     * Check the high watermark for software handshake. If the number of
     * buffered bytes is above this mark, then send XOFF.
     */
    else if (flow_control) {
        if (cnt >= rbf->rbf_hwm) {
            if ((flow_control & XOFF_SENT) == 0) {
                if (usart->csr & AVR32_USART_CSR_TXRDY_MASK) {
					usart->thr = (ASCII_XOFF << AVR32_USART_THR_TXCHR_OFFSET) & AVR32_USART_THR_TXCHR_MASK;
                    flow_control |= XOFF_SENT;
                    flow_control &= ~XOFF_PENDING;
                } else {
                    flow_control |= XOFF_PENDING;
                }
            }
        }
    }
#ifdef UART_RTS_BIT
    /*
     * Check the high watermark for hardware handshake. If the number of
     * buffered bytes is above this mark, then disable RTS.
     */
    else if (rts_control && cnt >= rbf->rbf_hwm) {
		GpioPinSetLow(UART_RTS_PORT, UART_RTS_BIT);
    }
#endif

    /*
     * Store the character and increment and the ring buffer pointer.
     */
    *rbf->rbf_head++ = ch;
    if (rbf->rbf_head == rbf->rbf_last) {
        rbf->rbf_head = rbf->rbf_start;
    }

    /* Update the ring buffer counter. */
    rbf->rbf_cnt = cnt;

#ifdef UART_READMULTIBYTE
    } while ( (usart->csr & AVR32_USART_CSR_RXRDY_MASK) != 0 ); // byte in buffer?

    // Eventually post event to wake thread
    if (postEvent) {
        NutEventPostFromIrq(&rbf->rbf_que);
        NutSelectWakeupFromIrq(rbf->wq_list, WQ_FLAG_READ);
    }
#endif

}

/*!
 * \brief USART interrupt handler.
 *
 * \param arg Pointer to the device specific control block.
 */
static void Avr32UsartInterrupt(void *arg)
{
    USARTDCB *dcb = (USARTDCB *) arg;
    ureg_t csr = inr(USARTn_BASE + AVR32_USART_CSR) & AVR32_USART_CSR_MASK;

    if (csr & AVR32_USART_CSR_RXRDY_MASK) {
        Avr32UsartRxReady(&dcb->dcb_rx_rbf);
    }
    if (csr & AVR32_USART_CSR_TXRDY_MASK) {
        Avr32UsartTxReady(&dcb->dcb_tx_rbf);
    }
#ifdef UART_HDX_BIT
    if (csr & AVR32_USART_CSR_TXEMPTY_MASK) {
        Avr32UsartTxEmpty(&dcb->dcb_tx_rbf);
    }
#endif /*  UART_HDX_BIT */
}

/*!
 * \brief Carefully enable USART hardware functions.
 *
 * Always enable transmitter and receiver, even on read-only or
 * write-only mode. So we can support software flow control.
 */
static void Avr32UsartEnable(void)
{
	volatile avr32_usart_t* usart = (avr32_usart_t*)USARTn_BASE;

    NutEnterCritical();

    /* Enable UART receiver and transmitter. */
	usart->cr |= AVR32_USART_CR_RXEN_MASK | AVR32_USART_CR_TXEN_MASK;

    /* Enable UART receiver and transmitter interrupts. */
	usart->ier = AVR32_USART_IER_RXRDY_MASK | AVR32_USART_IER_TXRDY_MASK;

#ifdef UART_HDX_BIT
    if (hdx_control) {
        /* Enable transmit complete interrupt. */
		usart->ier = AVR32_USART_IER_TXEMPTY_MASK;
    }
#endif
	
	usart->csr;
    NutExitCritical();
}

/*!
 * \brief Carefully disable USART hardware functions.
 */
static void Avr32UsartDisable(void)
{
	volatile avr32_usart_t* usart = (avr32_usart_t*)USARTn_BASE;

    /*
     * Disable USART interrupts.
     */
    NutEnterCritical();
	usart->idr = AVR32_USART_IDR_MASK;
	usart->csr;
    NutExitCritical();

    /*
     * Allow incoming or outgoing character to finish.
     */
    NutDelay(10);

    /*
     * Disable USART transmit and receive.
     */
	usart->cr = AVR32_USART_CR_RXDIS_MASK | AVR32_USART_CR_TXDIS_MASK;
}

/*!
 * \brief Query the USART hardware for the selected speed.
 *
 * This function is called by ioctl function of the upper level USART
 * driver through the USARTDCB jump table.
 *
 * \return The currently selected baudrate.
 */
static uint32_t Avr32UsartGetSpeed(void)
{
	volatile avr32_usart_t* usart = (avr32_usart_t*)USARTn_BASE;
    uint32_t clk = NutClockGet(NUT_HWCLK_PERIPHERAL_A);
    uint32_t cd = usart->brgr & AVR32_USART_BRGR_CD_MASK >> AVR32_USART_BRGR_CD_OFFSET;

    if (usart->brgr & AVR32_USART_BRGR_FP_MASK) {
        cd += ((usart->brgr & AVR32_USART_BRGR_FP_MASK) >> AVR32_USART_BRGR_FP_OFFSET) / 8;
    }

    return clk / (8UL * (2 - ((usart->mr & AVR32_USART_MR_OVER_MASK) >> AVR32_USART_MR_OVER_OFFSET)) * cd);
}

/*!
 * \brief Set the USART hardware bit rate.
 *
 * This function is called by ioctl function of the upper level USART
 * driver through the USARTDCB jump table.
 *
 * \param rate Number of bits per second.
 *
 * \return 0 on success, -1 otherwise.
 */
static int Avr32UsartSetSpeed(uint32_t rate)
{
	volatile avr32_usart_t* usart = (avr32_usart_t*)USARTn_BASE;
    const unsigned long pba_hz = NutClockGet(NUT_HWCLK_PERIPHERAL_A);
    const unsigned int over = (pba_hz >= 16 * rate) ? 16 : 8;
    const unsigned int cd_fp = ((1 << AVR32_USART_BRGR_FP_SIZE) * pba_hz + (over * rate) / 2) / (over * rate);
    const unsigned int cd = cd_fp >> AVR32_USART_BRGR_FP_SIZE;
    const unsigned int fp = cd_fp & ((1 << AVR32_USART_BRGR_FP_SIZE) - 1);

    if (cd < 1 || cd > (1 << AVR32_USART_BRGR_CD_SIZE) - 1)
        return -1;

    Avr32UsartDisable();
	usart->mr = (usart->mr & ~(AVR32_USART_MR_USCLKS_MASK | AVR32_USART_MR_SYNC_MASK | AVR32_USART_MR_OVER_MASK)) |
	                           AVR32_USART_MR_USCLKS_MCK << AVR32_USART_MR_USCLKS_OFFSET |
	                           ((over == 16) ? AVR32_USART_MR_OVER_X16 : AVR32_USART_MR_OVER_X8) << AVR32_USART_MR_OVER_OFFSET;

    usart->brgr = cd << AVR32_USART_BRGR_CD_OFFSET | fp << AVR32_USART_BRGR_FP_OFFSET;
	
    Avr32UsartEnable();

    return 0;
}

/*!
 * \brief Query the USART hardware for the number of data bits.
 *
 * This function is called by ioctl function of the upper level USART
 * driver through the USARTDCB jump table.
 *
 * \return The number of data bits set.
 */
static uint8_t Avr32UsartGetDataBits(void)
{
    uint8_t val;
	volatile avr32_usart_t* usart = (avr32_usart_t*)USARTn_BASE;

    if (usart->mr & AVR32_USART_MR_MODE9_MASK) {
        val = 9;
    } else {
        val = ((usart->mr & AVR32_USART_MR_CHRL_MASK) >> AVR32_USART_MR_CHRL_OFFSET) + 5;
    }
    return (uint8_t) val;
}

/*!
 * \brief Set the USART hardware to the number of data bits.
 *
 * This function is called by ioctl function of the upper level USART
 * driver through the USARTDCB jump table.
 *
 * \return 0 on success, -1 otherwise.
 */
static int Avr32UsartSetDataBits(uint8_t bits)
{
	volatile avr32_usart_t* usart = (avr32_usart_t*)USARTn_BASE;
	uint32_t mr = usart->mr;

    if (bits == 9) {
        /* Character length set to 9 bits. MODE9 dominates CHRL. */
        mr |= AVR32_USART_MR_MODE9_MASK;
    } else {
		mr &= ~AVR32_USART_MR_MODE9_MASK;
		mr = (mr & ~AVR32_USART_MR_CHRL_MASK) | ((bits - 5) << AVR32_USART_MR_CHRL_OFFSET);
    }
	Avr32UsartDisable();
	usart->mr = mr;
	Avr32UsartEnable();

    /*
     * Verify the result.
     */
    if (Avr32UsartGetDataBits() != bits) {
        return -1;
    }
    return 0;
}

/*!
 * \brief Query the USART hardware for the parity mode.
 *
 * This routine is called by ioctl function of the upper level USART
 * driver through the USARTDCB jump table.
 *
 * \return Parity mode, either 0 (disabled), 1 (odd), 2 (even) or 9 (multidrop).
 */
static uint8_t Avr32UsartGetParity(void)
{
	volatile avr32_usart_t* usart = (avr32_usart_t*)USARTn_BASE;
    uint8_t val;

    if (usart->mr & AVR32_USART_MR_MODE9_MASK) {
        val = 9;
    } else {
#if AVR32_USART_FEATURES == 0x000000f8
		switch (usart->MR.usart_mode.par) {
#else
		switch (usart->MR.par) {
#endif
		case AVR32_USART_MR_PAR_ODD:
			val = 1;
			break;
		case AVR32_USART_MR_PAR_EVEN:
			val = 2;
			break;
		default:
			val = 0;
			break;
		}
    }
    return val;
}

/*!
 * \brief Set the USART hardware to the specified parity mode.
 *
 * This routine is called by ioctl function of the upper level USART
 * driver through the USARTDCB jump table.
 *
 * \param mode 0 (disabled), 1 (odd) or 2 (even)
 *
 * \return 0 on success, -1 otherwise.
 */
static int Avr32UsartSetParity(uint8_t mode)
{
	volatile avr32_usart_t* usart = (avr32_usart_t*)USARTn_BASE;

    Avr32UsartDisable();
    switch (mode) {
#if AVR32_USART_FEATURES == 0x000000f8
	case 0:		usart->MR.usart_mode.par = AVR32_USART_MR_PAR_NONE;        break;
	case 1:		usart->MR.usart_mode.par = AVR32_USART_MR_PAR_ODD;         break;
	case 2:		usart->MR.usart_mode.par = AVR32_USART_MR_PAR_EVEN;        break;
#else
	case 0:		usart->MR.par = AVR32_USART_MR_PAR_NONE;        break;
	case 1:		usart->MR.par = AVR32_USART_MR_PAR_ODD;         break;
	case 2:		usart->MR.par = AVR32_USART_MR_PAR_EVEN;        break;
#endif
    }
    Avr32UsartEnable();

    /*
     * Verify the result.
     */
    if (Avr32UsartGetParity() != mode) {
        return -1;
    }
    return 0;
}

/*!
 * \brief Query the USART hardware for the number of stop bits.
 *
 * This routine is called by ioctl function of the upper level USART
 * driver through the USARTDCB jump table.
 *
 * \return The number of stop bits set, either 1, 2 or 3 (1.5 bits).
 */
static uint8_t Avr32UsartGetStopBits(void)
{
	volatile avr32_usart_t* usart = (avr32_usart_t*)USARTn_BASE;

    switch (usart->mr & AVR32_USART_MR_NBSTOP_MASK >> AVR32_USART_MR_NBSTOP_OFFSET) {
    case AVR32_USART_MR_NBSTOP_1:
        return 1;
    case AVR32_USART_MR_NBSTOP_2:
        return 2;
    case AVR32_USART_MR_NBSTOP_1_5:
        return 3;
    }
    return 0;
}

/*!
 * \brief Set the USART hardware to the number of stop bits.
 *
 * This routine is called by ioctl function of the upper level USART
 * driver through the USARTDCB jump table.
 *
 * \return 0 on success, -1 otherwise.
 */
static int Avr32UsartSetStopBits(uint8_t bits)
{
	volatile avr32_usart_t* usart = (avr32_usart_t*)USARTn_BASE;

    Avr32UsartDisable();
    switch (bits) {
#if AVR32_USART_FEATURES == 0x000000f8
	case 1:		usart->MR.usart_mode.nbstop = AVR32_USART_MR_NBSTOP_1;        break;
	case 2:		usart->MR.usart_mode.nbstop = AVR32_USART_MR_NBSTOP_2;        break;
	case 3:		usart->MR.usart_mode.nbstop = AVR32_USART_MR_NBSTOP_1_5;      break;
#else
	case 1:		usart->MR.nbstop = AVR32_USART_MR_NBSTOP_1;        break;
    case 2:		usart->MR.nbstop = AVR32_USART_MR_NBSTOP_2;        break;
    case 3:		usart->MR.nbstop = AVR32_USART_MR_NBSTOP_1_5;      break;
#endif
    }
    Avr32UsartEnable();

    /*
     * Verify the result.
     */
    if (Avr32UsartGetStopBits() != bits) {
        return -1;
    }
    return 0;
}

/*!
 * \brief Query the USART hardware status.
 *
 * \return Status flags.
 */
static uint32_t Avr32UsartGetStatus(void)
{
    uint32_t rc = 0;
#if defined(US_MODE_HWHANDSHAKE)
    uint32_t csr = inr(USARTn_BASE + AVR32_USART_CSR);
#endif

    /*
     * Set receiver error flags.
     */
    if ((rx_errors & AVR32_USART_CSR_FRAME_MASK) != 0) {
        rc |= UART_FRAMINGERROR;
    }
    if ((rx_errors & AVR32_USART_CSR_OVRE_MASK) != 0) {
        rc |= UART_OVERRUNERROR;
    }
    if ((rx_errors & AVR32_USART_CSR_PARE_MASK) != 0) {
        rc |= UART_PARITYERROR;
    }

    /*
     * Determine software handshake status. The flow control status may
     * change during interrupt, but this doesn't really hurt us.
     */
    if (flow_control) {
        if (flow_control & XOFF_SENT) {
            rc |= UART_RXDISABLED;
        }
        if (flow_control & XOFF_RCVD) {
            rc |= UART_TXDISABLED;
        }
    }

    /*
     * Determine hardware handshake control status.
     */
#if defined(UART_RTS_BIT)
    if (GpioPinGet(UART_RTS_PORT, UART_RTS_BIT)) {
        rc |= UART_RTSDISABLED;
        if (rts_control) {
            rc |= UART_RXDISABLED;
        }
    } else {
        rc |= UART_RTSENABLED;
    }
#elif defined(US_MODE_HWHANDSHAKE)
    /* How to find out? */
#endif

    /*
     * Determine hardware handshake sense status.
     */
#ifdef UART_CTS_BIT
	if (GpioPinGet(UART_CTS_PORT, UART_CTS_BIT)) {
        rc |= UART_CTSDISABLED;
        if (cts_sense) {
            rc |= UART_RXDISABLED;
        }
    } else {
        rc |= UART_CTSENABLED;
    }
#elif defined(US_MODE_HWHANDSHAKE)
    if (csr & AVR32_USART_CSR_CTS_MASK) {
        rc |= UART_CTSDISABLED;
        if (cts_sense) {
            rc |= UART_RXDISABLED;
        }
    } else {
        rc |= UART_CTSENABLED;
    }
#endif

    /*
     * Determine hardware modem sense status.
     */
#if defined(US_MODE_HWHANDSHAKE) && 0
    /* I'm confused. Awful flag mismatch? Why do we have uart.h and usart.h? */
    if (csr & AVR32_USART_CSR_RI_MASK) {
        rc |= UART_RIDISABLED;
    } else {
        rc |= UART_RIENABLED;
    }
    if (csr & AVR32_USART_CSR_DSR_MASK) {
        rc |= UART_DSRDISABLED;
    } else {
        rc |= UART_DSRENABLED;
    }
    if (csr & AVR32_USART_CSR_DCD_MASK) {
        rc |= UART_DCDDISABLED;
    } else {
        rc |= UART_DCDENABLED;
    }
#endif

    /*
     * If transmitter and receiver haven't been detected disabled by any
     * of the checks above, then they are probably enabled.
     */
    if ((rc & UART_RXDISABLED) == 0) {
        rc |= UART_RXENABLED;
    }
    if ((rc & UART_TXDISABLED) == 0) {
        rc |= UART_TXENABLED;
    }

    /*
     * Process multidrop setting.
     */
    if (tx_aframe) {
        rc |= UART_TXADDRFRAME;
    } else {
        rc |= UART_TXNORMFRAME;
    }
    return rc;
}

/*!
 * \brief Set the USART hardware status.
 *
 * \param flags Status flags.
 *
 * \return 0 on success, -1 otherwise.
 */
static int Avr32UsartSetStatus(uint32_t flags)
{
    /*
     * Process software handshake control.
     */
    if (flow_control) {

        /* Access to the flow control status must be atomic. */
        NutEnterCritical();

        /*
         * Enabling or disabling the receiver means to behave like
         * having sent a XON or XOFF character resp.
         */
        if (flags & UART_RXENABLED) {
            flow_control &= ~XOFF_SENT;
        } else if (flags & UART_RXDISABLED) {
            flow_control |= XOFF_SENT;
        }

        /*
         * Enabling or disabling the transmitter means to behave like
         * having received a XON or XOFF character resp.
         */
        if (flags & UART_TXENABLED) {
            flow_control &= ~XOFF_RCVD;
        } else if (flags & UART_TXDISABLED) {
            flow_control |= XOFF_RCVD;
        }
        NutExitCritical();
    }

    /*
     * Process hardware handshake control.
     */
#if defined(UART_RTS_BIT)
    /* Manually controlled via GPIO. */
    if (rts_control) {
        if (flags & UART_RXDISABLED) {
			GpioPinSetHigh(UART_RTS_PORT, UART_RTS_BIT);
        }
        if (flags & UART_RXENABLED) {
			GpioPinSetLow(UART_RTS_PORT, UART_RTS_BIT);
        }
    }
    if (flags & UART_RTSDISABLED) {
		GpioPinSetHigh(UART_RTS_PORT, UART_RTS_BIT);
    }
    if (flags & UART_RTSENABLED) {
		GpioPinSetLow(UART_RTS_PORT, UART_RTS_BIT);
    }
#elif defined(US_MODE_HWHANDSHAKE)
    /* Build in hardware. */
    if (rts_control) {
        if (flags & UART_RXDISABLED) {
            USARTn_BASE.CR.rtsdis = 1;
        }
        if (flags & UART_RXENABLED) {
            USARTn_BASE.CR.rtsen = 1;
        }
    }
    if (flags & UART_RTSDISABLED) {
        USARTn_BASE.CR.rtsdis = 1;
    }
    if (flags & UART_RTSENABLED) {
        USARTn_BASE.CR.rtsen = 1;
    }
#endif

    /*
     * Process hardware modem control.
     */
#if defined(UART_DTR_BIT)
    /* Manually controlled via GPIO. */
    if (flags & UART_DTRDISABLED) {
        sbi(UART_DTR_PORT, UART_DTR_BIT);
    }
    if (flags & UART_DTRENABLED) {
        cbi(UART_DTR_PORT, UART_DTR_BIT);
    }
#elif defined(US_MODE_HWHANDSHAKE)
    /* Build in hardware. */
    if (flags & UART_DTRDISABLED) {
        USARTn_BASE.CR.dtrdis = 1;
    }
    if (flags & UART_DTRENABLED) {
        USARTn_BASE.CR.dtren = 1;
    }
#endif

    /*
     * Process multidrop setting.
     */
    if (flags & UART_TXADDRFRAME) {
        tx_aframe = 1;
    }
    if (flags & UART_TXNORMFRAME) {
        tx_aframe = 0;
    }

    /*
     * Clear UART receive errors.
     */
    if (flags & UART_ERRORS) {
        outr(USARTn_BASE + AVR32_USART_CR, inr(USARTn_BASE + AVR32_USART_CR) | AVR32_USART_CR_RSTSTA_MASK);
    }

    /*
     * Verify the result.
     */
    if ((Avr32UsartGetStatus() & ~UART_ERRORS) != flags) {
        return -1;
    }
    return 0;
}

/*!
 * \brief Query the USART hardware for synchronous mode.
 *
 * This function is called by ioctl function of the upper level USART
 * driver through the USARTDCB jump table.
 *
 * \return Or-ed combination of \ref UART_SYNC, \ref UART_MASTER,
 *         \ref UART_NCLOCK and \ref UART_HIGHSPEED.
 */
static uint8_t Avr32UsartGetClockMode(void)
{
    uint8_t rc = 0;

    return rc;
}

/*!
 * \brief Set asynchronous or synchronous mode.
 *
 * This function is called by ioctl function of the upper level USART
 * driver through the USARTDCB jump table.
 *
 * \param mode Must be an or-ed combination of USART_SYNC, USART_MASTER,
 *             USART_NCLOCK and USART_HIGHSPEED.
 *
 * \return 0 on success, -1 otherwise.
 */
static int Avr32UsartSetClockMode(uint8_t mode)
{
    /*
     * Verify the result.
     */
    if (Avr32UsartGetClockMode() != mode) {
        return -1;
    }
    return 0;
}

/*!
 * \brief Query flow control mode.
 *
 * This routine is called by ioctl function of the upper level USART
 * driver through the USARTDCB jump table.
 *
 * \return See UsartIOCtl().
 */
static uint32_t Avr32UsartGetFlowControl(void)
{
    uint32_t rc = 0;

    if (flow_control) {
        rc |= USART_MF_XONXOFF;
    } else {
        rc &= ~USART_MF_XONXOFF;
    }

#ifdef UART_RTS_BIT
    if (rts_control) {
        rc |= USART_MF_RTSCONTROL;
    } else {
        rc &= ~USART_MF_RTSCONTROL;
    }
#endif

#ifdef UART_CTS_BIT
    if (cts_sense) {
        rc |= USART_MF_CTSSENSE;
    } else {
        rc &= ~USART_MF_CTSSENSE;
    }
#endif

#ifdef UART_HDX_BIT
    if (hdx_control) {
        rc |= USART_MF_HALFDUPLEX;
    } else {
        rc &= ~USART_MF_HALFDUPLEX;
    }
#endif

    return rc;
}

/*!
 * \brief Set flow control mode.
 *
 * This function is called by ioctl function of the upper level USART
 * driver through the USARTDCB jump table.
 *
 * \param flags See UsartIOCtl().
 *
 * \return 0 on success, -1 otherwise.
 */
static int Avr32UsartSetFlowControl(uint32_t flags)
{
    /*
     * Set software handshake mode.
     */
    if (flags & USART_MF_XONXOFF) {
        if (flow_control == 0) {
            NutEnterCritical();
            flow_control = 1 | XOFF_SENT;       /* force XON to be sent on next read */
            NutExitCritical();
        }
    } else {
        NutEnterCritical();
        flow_control = 0;
        NutExitCritical();
    }

    /*
     * Set RTS control mode.
     */
    if (flags & USART_MF_RTSCONTROL) {
#if defined(UART_RTS_BIT)
		GpioPinSetLow(UART_RTS_PORT, UART_RTS_BIT);
        rts_control = 1;
#endif
	}
	else if (rts_control) {
        rts_control = 0;
#if defined(UART_RTS_BIT)
		GpioPinSetLow(UART_RTS_PORT, UART_RTS_BIT);
#endif
    }

/*
 * Set CTS sense mode.
 */
    if (flags & USART_MF_CTSSENSE) {
#if defined(UART_CTS_BIT)
        /* Register CTS sense interrupt. */
		GpioPinConfigSet(UART_CTS_PORT, UART_CTS_BIT, GPIO_CFG_PULLUP);
		if (GpioRegisterIrqHandler(&UART_CTS_SIGNAL, UART_CTS_BIT, Avr32UsartCts, 0)) {
            return -1;
        }
        cts_sense = 1;
#elif defined(US_MODE_HWHANDSHAKE)
        outr(USARTn_BASE + AVR32_USART_MR, inr(USARTn_BASE + AVR32_USART_MR) | (AVR32_USART_MR_MODE_HARDWARE << AVR32_USART_MR_MODE_OFFSET) );
        cts_sense = 1;
        rts_control = 1;
#endif
    } else if (cts_sense) {
#if defined(UART_CTS_BIT)
        /* Deregister CTS sense interrupt. */
		GpioRegisterIrqHandler(&UART_CTS_SIGNAL, UART_CTS_BIT, NULL, 0);
#elif defined(US_MODE_HWHANDSHAKE)
        outr(USARTn_BASE + AVR32_USART_MR, inr(USARTn_BASE + AVR32_USART_MR) | (AVR32_USART_MR_MODE_NORMAL << AVR32_USART_MR_MODE_OFFSET) );
        rts_control = 0;
#endif
        cts_sense = 0;
    }
#ifdef UART_HDX_BIT
    /*
     * Set half duplex mode.
     */
    if (flags & USART_MF_HALFDUPLEX) {
        /* Register transmit complete interrupt. */
        if (NutRegisterIrqHandler(&sig_UART_TRANS, Avr32UsartTxComplete, &dcb_usart.dcb_rx_rbf)) {
            return -1;
        }
        /* Initially enable the receiver. */
        UART_HDX_RX(UART_HDX_PORT, UART_HDX_BIT);
        sbi(UART_HDX_DDR, UART_HDX_BIT);
        hdx_control = 1;
        /* Enable transmit complete interrupt. */
        sbi(UCSRnB, TXCIE);
    } else if (hdx_control) {
        hdx_control = 0;
        /* disable transmit complete interrupt */
        cbi(UCSRnB, TXCIE);
        /* Deregister transmit complete interrupt. */
        NutRegisterIrqHandler(&sig_UART_TRANS, 0, 0);
        cbi(UART_HDX_DDR, UART_HDX_BIT);
    }
#endif

    /*
     * Verify the result.
     */
    if (Avr32UsartGetFlowControl() != flags) {
        return -1;
    }
    return 0;
}

/*!
 * \brief Start the USART transmitter hardware.
 *
 * The upper level USART driver will call this function through the
 * USARTDCB jump table each time it added one or more bytes to the
 * transmit buffer.
 */
static void Avr32UsartTxStart(void)
{
#ifdef UART_HDX_BIT
    if (hdx_control) {
        /* Enable half duplex transmitter. */
        UART_HDX_TX(UART_HDX_PORT, UART_HDX_BIT);
    }
#endif
    /* Enable transmit interrupts. */
    outr(USARTn_BASE + AVR32_USART_IER, AVR32_USART_IER_TXRDY_MASK);
}

/*!
 * \brief Start the USART receiver hardware.
 *
 * The upper level USART driver will call this function through the
 * USARTDCB jump table each time it removed enough bytes from the
 * receive buffer. Enough means, that the number of bytes left in
 * the buffer is below the low watermark.
 */
static void Avr32UsartRxStart(void)
{
    /*
     * Do any required software flow control.
     */
    if (flow_control && (flow_control & XOFF_SENT) != 0) {
        NutEnterCritical();
        if (inr(USARTn_BASE + AVR32_USART_CSR) & AVR32_USART_CSR_TXRDY) {
            outr(USARTn_BASE + AVR32_USART_THR, (ASCII_XON << AVR32_USART_THR_TXCHR_OFFSET) & AVR32_USART_THR_TXCHR_MASK );
            flow_control &= ~XON_PENDING;
        } else {
            flow_control |= XON_PENDING;
        }
        flow_control &= ~(XOFF_SENT | XOFF_PENDING);
        NutExitCritical();
    }
#ifdef UART_RTS_BIT
    if (rts_control) {
        /* Enable RTS. */
		GpioPinSetLow(UART_RTS_PORT, UART_RTS_BIT);
    }
#endif
}

/*
 * \brief Initialize the USART hardware driver.
 *
 * This function is called during device registration by the upper level
 * USART driver through the USARTDCB jump table.
 *
 * \return 0 on success, -1 otherwise.
 */
static int Avr32UsartInit(void)
{
	volatile avr32_usart_t* usart = (avr32_usart_t*)USARTn_BASE;

    /*
     * Register receive and transmit interrupts.
     */
    if (NutRegisterIrqHandler(&SIG_UART, Avr32UsartInterrupt, &dcb_usart)) {
        return -1;
    }

    /* Disable GPIO on UART tx/rx pins. */
    gpio_enable_module_pin(USART_RX_PIN, USART_RX_FUNCTION);
    gpio_enable_module_pin(USART_TX_PIN, USART_TX_FUNCTION);

    /* Disable all USART interrupts.
     ** Interrupts needed should be set explicitly on every reset. */
	usart->idr = AVR32_USART_IDR_MASK;
	usart->csr;

    /* Reset mode and other registers that could cause unpredictable behavior after reset. */
	usart->mr = 0;
	usart->rtor = 0;
	usart->ttgr = 0;

    /* Shutdown TX and RX (will be re-enabled when setup has successfully completed),
     ** reset status bits and turn off DTR and RTS. */
	  usart->cr = AVR32_USART_CR_RSTRX_MASK   |
                  AVR32_USART_CR_RSTTX_MASK   |
                  AVR32_USART_CR_RSTSTA_MASK  |
                  AVR32_USART_CR_RSTIT_MASK   |
                  AVR32_USART_CR_RSTNACK_MASK |
#ifdef AVR32_USART_CR_DTRDIS_MASK
                  AVR32_USART_CR_DTRDIS_MASK  |
#endif
                  AVR32_USART_CR_RTSDIS_MASK;

	usart->mr |= (8 - 5) << AVR32_USART_MR_CHRL_OFFSET;  // 8 bits character length
	usart->mr |= AVR32_USART_MR_PAR_NONE << AVR32_USART_MR_PAR_OFFSET; // No parity.
	usart->mr |= AVR32_USART_MR_CHMODE_NORMAL << AVR32_USART_MR_CHMODE_OFFSET; // Normal Mode
	usart->mr |= AVR32_USART_MR_NBSTOP_1 << AVR32_USART_MR_NBSTOP_OFFSET; // 1 Stop bit


#ifdef UART_RTS_BIT
	GpioPinConfigSet(UART_RTS_PORT, UART_RTS_BIT, GPIO_CFG_OUTPUT);
	GpioPinSetLow(UART_RTS_PORT, UART_RTS_BIT);
#endif

    /* Setup baudrate and enable input and output. */

    return Avr32UsartSetSpeed( USART_INITSPEED );
}

/*
 * \brief Deinitialize the USART hardware driver.
 *
 * This function is called during device deregistration by the upper
 * level USART driver through the USARTDCB jump table.
 *
 * \return 0 on success, -1 otherwise.
 */
static int Avr32UsartDeinit(void)
{
    /* Deregister receive and transmit interrupts. */
    NutRegisterIrqHandler(&SIG_UART, 0, 0);

    /* Shutdown TX and RX (will be re-enabled when setup has successfully completed),
     ** reset status bits and turn off DTR and RTS. */
    outr(USARTn_BASE + AVR32_USART_CR, AVR32_USART_CR_RSTRX_MASK |
        AVR32_USART_CR_RSTTX_MASK | AVR32_USART_CR_RSTSTA_MASK |
        AVR32_USART_CR_RSTIT_MASK | AVR32_USART_CR_RSTNACK_MASK |
#ifdef AVR32_USART_CR_DTRDIS_MASK
        AVR32_USART_CR_DTRDIS_MASK |
#endif
        AVR32_USART_CR_RTSDIS_MASK);

    /* Disable all UART interrupts. */
    outr(USARTn_BASE + AVR32_USART_IDR, AVR32_USART_IDR_MASK);
    inr(USARTn_BASE + AVR32_USART_CSR);

    /* Disable UART clock. */

    /* Enable GPIO on UART tx/rx pins. */

    /*
     * Disabling flow control shouldn't be required here, because it's up
     * to the upper level to do this on the last close or during
     * deregistration.
     */
#ifdef UART_HDX_BIT
    /* Deregister transmit complete interrupt. */
    if (hdx_control) {
        hdx_control = 0;
        NutRegisterIrqHandler(&sig_UART_TRANS, 0, 0);
    }
#endif

#ifdef UART_CTS_BIT
    if (cts_sense) {
        cts_sense = 0;
        /* Deregister CTS sense interrupt. */
		GpioRegisterIrqHandler(&UART_CTS_SIGNAL, UART_CTS_BIT, NULL, 0);
    }
#endif

#ifdef UART_RTS_BIT
    if (rts_control) {
        rts_control = 0;
    }
#endif

    return 0;
}

/*@}*/
