
#ifdef NUTDEBUG
	#include <net/netdebug.h>
#endif


//#define UART_SW_FLOWCONTROL

/* IOCtrl Halfduplex flag, ktery urci komunkaci pres porty AB, XY */
#define USART_MF_HALFDUPLEX_YZ	0x2000
#define USART_MF_FULLDUPLEX_232	0x4000
#define USART_MF_LOOPBACK_AB	0x8000
#define USART_MF_LOOPBACK_YZ	0x4000		/* same as for 232 if piggy-232 board present */

/* Attribute, which selects AB or XY uart0's transceiver. */
#define HDX_CONTROL_SHIFT			8
#define HDX_CONTROL_MASK			((USART_MF_HALFDUPLEX | USART_MF_HALFDUPLEX_YZ) >> HDX_CONTROL_SHIFT)			//ignore other bits
#define HDX_CONTROL_HALFDUPLEX		 (USART_MF_HALFDUPLEX    >> HDX_CONTROL_SHIFT)
#define HDX_CONTROL_HALFDUPLEX_YZ	 (USART_MF_HALFDUPLEX_YZ >> HDX_CONTROL_SHIFT)

#ifdef NUTTRACER
#include <sys/tracer.h>
#endif

/* Enable Transmit Ready Interrupt. */
#define SET_TXRDY_INTERRUPT() \
		{   \
			UART_EnableInterrupts(MK64F_USART_BASE, kUART_TxDataRegEmptyInterruptEnable); \
		}

/* Disable Transmit Ready Interrupt. */
#define CLR_TXRDY_INTERRUPT() \
		{   \
			UART_DisableInterrupts(MK64F_USART_BASE, kUART_TxDataRegEmptyInterruptEnable); \
		}

/* Enable Receive Ready Interrupt. */
#define SET_RXRDY_INTERRUPT() \
		{   \
			UART_EnableInterrupts(MK64F_USART_BASE, kUART_RxDataRegFullInterruptEnable); \
		}

/* Enable Transmit & Receive Ready Interrupts. */
#define SET_TXRXRDY_INTERRUPTS() \
		{   \
			UART_DisableInterrupts(MK64F_USART_BASE, kUART_TxDataRegEmptyInterruptEnable | kUART_RxDataRegFullInterruptEnable); \
		}

/* Disable All Interrupts. */
#define CLR_ALL_INTERRUPTS() \
		{   \
			UART_DisableInterrupts(MK64F_USART_BASE, kUART_TxDataRegEmptyInterruptEnable | kUART_RxDataRegFullInterruptEnable); \
		}

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
 * \brief Receiver error flags.
 */
#define UART_RECEIVEBREAK		0x08
// #define UART_FLAGS_CLEAR_ALL	(UART_ERRORS | UART_RECEIVEBREAK | MCF_UART_UCR_RESET_RX | MCF_UART_UCR_RESET_TX)

/*!
 * \brief Enables software flow control if not equal zero.
 */
#ifdef UART_SW_FLOWCONTROL
static ureg_t flow_control;
#endif

#if defined(UART_HDX_BIT) || defined(UART_HDB_FDX_BIT)
/* define in cfg/modem.h */
#ifdef UART_HDX_FLIP_BIT    /* same as RTS toggle by Windows NT driver */
#define UART_HDX_TX     cbi
#define UART_HDX_RX     sbi
#else                       /* previous usage by Ethernut */
#define UART_HDX_TX     sbi
#define UART_HDX_RX     cbi
#endif
#endif

#if defined(UART_HDX_BIT) || defined(UART_HDB_FDX_BIT)
/*!
 * \brief Enables half duplex control if not equal zero.
 *
 * This variable exists only if the hardware configuration defines a
 * port bit to switch between receive and transmit mode.
 */
static ureg_t hdx_control;
#endif

#ifdef UART_RTS_BIT
/*!
 * \brief Enables RTS control if not equal zero.
 *
 * This variable exists only if the hardware configuration defines a
 * port bit to control the RTS signal.
 */
static ureg_t rts_control;
#endif

#ifdef UART_CTS_BIT
/*!
 * \brief Enables CTS sense if not equal zero.
 *
 * This variable exists only if the hardware configuration defines a
 * port bit to sense the CTS signal.
 */
static ureg_t cts_sense;
#endif


static uint32_t m_parity;
static uint32_t m_stop_bits;
static uint32_t m_baud_rate;
/*!
 * \addtogroup xgMcf5225
 */
/*@{*/

#undef NUTTRACER    // TODO .. not ported yet



static void Mk64fUsartTxReady(void *arg)
{
	register RINGBUF *rbf = (RINGBUF *) arg;

	register uint8_t *cp = rbf->rbf_tail;
	uint8_t postEvent = 0;

#ifdef NUTTRACER
	TRACE_ADD_ITEM(TRACE_TAG_INTERRUPT_ENTER,TRACE_INT_UART_TXEMPTY);
#endif

#ifdef UART_SW_FLOWCONTROL
	/*
	 * Process pending software flow controls first.
	 */
	if (flow_control & (XON_PENDING | XOFF_PENDING)) {
		if (flow_control & XOFF_PENDING) {
			UART_WriteByte(MK64F_USART_BASE, ASCII_XOFF);
			flow_control |= XOFF_SENT;
		} else {
			flow_control &= ~XOFF_SENT;
		}
		flow_control &= ~(XON_PENDING | XOFF_PENDING);
#ifdef NUTTRACER
		TRACE_ADD_ITEM(TRACE_TAG_INTERRUPT_EXIT,TRACE_INT_UART_TXEMPTY);
#endif
		return;
	}

	if (flow_control & XOFF_RCVD) {
		/*
		 * If XOFF has been received, we disable the transmit interrupts
		 * and return without sending anything.
		 */
		CLR_TXRDY_INTERRUPT();

#ifdef NUTTRACER
		TRACE_ADD_ITEM(TRACE_TAG_INTERRUPT_EXIT,TRACE_INT_UART_TXEMPTY);
#endif
		return;
	}
#endif /* UART_SW_FLOWCONTROL */

	if (rbf->rbf_cnt) {
		/* rbf_cnt != 0, test before call this function */
		rbf->rbf_cnt--;
		/*
		 * Start transmission of the next character and clear TXRDY bit
		 * in USR register.
		 */
		UART_WriteByte(MK64F_USART_BASE, *cp);
		/*
		 * Wrap around the buffer pointer if we reached its end.
		 */
		if (++cp == rbf->rbf_last)
		{
			cp = rbf->rbf_start;
		}
		rbf->rbf_tail = cp;
		if (rbf->rbf_cnt == rbf->rbf_lwm)
		{
			postEvent = 1;
		}
	}

	if (rbf->rbf_cnt == 0) {

#if defined(UART_HDX_BIT) || defined(UART_HDB_FDX_BIT)
		if (!(hdx_control & HDX_CONTROL_MASK))
#endif
		{
			/* Disable USART transmit interrupt. Send last byte in shift register. */
			CLR_TXRDY_INTERRUPT();
		}
		postEvent = 1;
	}
	if (postEvent)
		NutEventPostFromIrq(&rbf->rbf_que);
#ifdef NUTTRACER
	TRACE_ADD_ITEM(TRACE_TAG_INTERRUPT_EXIT,TRACE_INT_UART_TXEMPTY);
#endif
}


static void Mk64fUsartRxComplete(void *arg)
{
	register RINGBUF *rbf = (RINGBUF *) arg;

	register size_t cnt;
	register uint8_t ch;

#ifdef NUTTRACER
	TRACE_ADD_ITEM(TRACE_TAG_INTERRUPT_ENTER,TRACE_INT_UART_RXCOMPL);
#endif

	register uint8_t postEvent = 0;
	do {
		ch = UART_ReadByte(MK64F_USART_BASE);

#ifdef UART_SW_FLOWCONTROL
		/*
		 * Handle software handshake. We have to do this before checking the
		 * buffer, because flow control must work in write-only mode, where
		 * there is no receive buffer.
		 */
		if (flow_control) {
			/* XOFF character disables transmit interrupts. */
			if (ch == ASCII_XOFF) {
				CLR_TXRDY_INTERRUPT();
				flow_control |= XOFF_RCVD;
#ifdef NUTTRACER
				TRACE_ADD_ITEM(TRACE_TAG_INTERRUPT_EXIT,TRACE_INT_UART_RXCOMPL);
#endif
				return;
			} else if (ch == ASCII_XON) {
			/* XON enables transmit interrupts. */
				SET_TXRDY_INTERRUPT();
				flow_control &= ~XOFF_RCVD;
#ifdef NUTTRACER
				TRACE_ADD_ITEM(TRACE_TAG_INTERRUPT_EXIT,TRACE_INT_UART_RXCOMPL);
#endif
				return;
			}
		}
#endif

		/*
		 * Check buffer overflow.
		 */
		cnt = rbf->rbf_cnt;
		if (cnt >= rbf->rbf_siz)
		{
			rx_errors |= kUART_RxOverrunFlag; // same flag as FIFO Overrun
#ifdef NUTTRACER
					TRACE_ADD_ITEM(TRACE_TAG_INTERRUPT_EXIT,TRACE_INT_UART_RXCOMPL);
#endif
			return;
		}

		/* Wake up waiting threads if this is the first byte in the buffer. */
		if (cnt++ == 0)
		{
			// we do this later, to get the other bytes in time..
			postEvent = 1;
		}
#ifdef UART_SW_FLOWCONTROL
		else if (flow_control)
		{
			/*
			 * Check the high watermark for software handshake. If the number of
			 * buffered bytes is above this mark, then send XOFF.
			 */
			if (cnt >= rbf->rbf_hwm) {
				if ((flow_control & XOFF_SENT) == 0) {
					if (kUART_TxDataRegEmptyFlag & UART_GetStatusFlags(MK64F_USART_BASE)) {
						UART_WriteByte(MK64F_USART_BASE, ASCII_XOFF);
						flow_control |= XOFF_SENT;
						flow_control &= ~XOFF_PENDING;
					} else {
						flow_control |= XOFF_PENDING;
					}
				}
			}
		}
#endif

#ifdef UART_RTS_BIT
		/*
		 * Check the high watermark for hardware handshake. If the number of
		 * buffered bytes is above this mark, then disable RTS.
		 */
		else if (rts_control && cnt >= rbf->rbf_hwm) {
			// GPIO_PinWrite(MK64F_USART_BASE, MK64F_USART_PIN_RTS, 1);
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
	} while (kUART_RxFifoEmptyFlag & UART_GetStatusFlags(MK64F_USART_BASE)); // byte in buffer?

	// Eventually post event to wake thread
	if (postEvent)
		NutEventPostFromIrq(&rbf->rbf_que);

#ifdef NUTTRACER
	TRACE_ADD_ITEM(TRACE_TAG_INTERRUPT_EXIT,TRACE_INT_UART_RXCOMPL);
#endif

}

#if defined(UART_HDX_BIT) || defined(UART_HDB_FDX_BIT)
/*!
 * \brief USARTn transmit complete interrupt handler.
 *
 * Used with half duplex communication to switch from tranmit to receive
 * mode after the last character has been transmitted.
 *
 * This routine exists only if the hardware configuration defines a
 * port bit to switch between receive and transmit mode.
 *
 * \param *arg Pointer to the transmitter ring buffer.
 */
static void Mk64fUsartTxEmpty(void *arg)
{
	/* Last byte from shift register was sent. */

#if (((PLATFORM_SUB == REV_D) || (PLATFORM_SUB == REV_F)) && defined(UART_HDB_FDX_BIT))
	if ((hdx_control & HDX_CONTROL_MASK) & HDX_CONTROL_HALFDUPLEX_YZ)
	{
		/* Set Half duplex on second chip*/
		/* RE2 = 0 Enable Receiver, DE2 = 0 Disable Transmitter */
		GPIO_PinWrite(MK64F_USART_GPIO_RE2, MK64F_USART_PIN_RE2, 0);
		GPIO_PinWrite(MK64F_USART_GPIO_DE2, MK64F_USART_PIN_DE2, 0);
	}
	else
#endif
	{
		/* RE1 = 0 Enable Receiver, DE1 = 0 Disable Transmitter */
		GPIO_PinWrite(MK64F_USART_GPIO_RE1, MK64F_USART_PIN_RE1, 0);
		GPIO_PinWrite(MK64F_USART_GPIO_DE1, MK64F_USART_PIN_DE1, 0);
	}

	/* Disable USART transmit interrupt.  */
	CLR_TXRDY_INTERRUPT();
}
#endif



static void Mk64fUsartInterrupts(void *arg)
{
	register USARTDCB *p_dcb = (USARTDCB *) arg;

	if (kUART_RxDataRegFullFlag & UART_GetStatusFlags(MK64F_USART_BASE))
	{
		Mk64fUsartRxComplete(&p_dcb->dcb_rx_rbf);
	}

	if (kUART_TxDataRegEmptyFlag & UART_GetStatusFlags(MK64F_USART_BASE))
	{
		Mk64fUsartTxReady(&p_dcb->dcb_tx_rbf);
	}

#if defined(UART_HDX_BIT) || defined(UART_HDB_FDX_BIT)
	if (kUART_TxDataRegEmptyFlag & UART_GetStatusFlags(MK64F_USART_BASE)) {
		if ((hdx_control & HDX_CONTROL_MASK) && (p_dcb->dcb_tx_rbf.rbf_cnt == 0)) {
			Mk64fUsartTxEmpty(&p_dcb->dcb_tx_rbf);
		}
	}
#endif
}

static int Mk64fUsartInit(void)
{
	status_t status;
	uart_config_t config;
	/*
	 * config.baudRate_Bps = 115200U;
	 * config.parityMode = kUART_ParityDisabled;
	 * config.stopBitCount = kUART_OneStopBit;
	 * config.txFifoWatermark = 0;
	 * config.rxFifoWatermark = 1;
	 * config.enableTx = false;
	 * config.enableRx = false;
	 */

	UART_GetDefaultConfig(&config);
	config.baudRate_Bps = MK64F_USART_BAUDRATE;
	config.parityMode = MK64F_USART_PARITY;
	config.stopBitCount = MK64F_USART_STOP_BIT;
	config.enableTx     = true;
	config.enableRx     = true;

	m_parity = 0;
	m_stop_bits = 1;
	m_baud_rate = MK64F_USART_BAUDRATE;

	status = UART_Init(MK64F_USART_BASE, &config, MK64F_USART_CLOCK_FREQ);

	/* Half duplex AB default:
	 *  Chip 1 - Rx enable, Tx disable
	 *  Chip 2 - both Rx, Tx disabled
	 */
#if defined(UART_HDX_BIT) || defined(UART_HDB_FDX_BIT)
	/* Enable Receiver on AB */
	/* RE1 = 0 Enable Receiver, DE1 = 0 Disable Transmitter */
	GPIO_PinWrite(MK64F_USART_GPIO_RE1, MK64F_USART_PIN_RE1, 0);
	GPIO_PinWrite(MK64F_USART_GPIO_DE1, MK64F_USART_PIN_DE1, 0);
#endif

#ifdef UART_HDX_BIT
	/* Half duplex default */
	#ifndef UART_HDB_FDX_BIT
	hdx_control = HDX_CONTROL_HALFDUPLEX;
	#endif
#endif

#ifdef UART_HDB_FDX_BIT
	/* YZ disabled */
	/* RE2 = 1 Disable Receiver, DE2 = 0 Disable Transmitter*/
	GPIO_PinWrite(MK64F_USART_GPIO_RE2, MK64F_USART_PIN_RE2, 0);
	GPIO_PinWrite(MK64F_USART_GPIO_DE2, MK64F_USART_PIN_DE2, 0);
#endif

	if (status != kStatus_Success) {
		return -1;
	}

	if (NutRegisterIrqHandler(&sig_USART, Mk64fUsartInterrupts, &dcb_usart)
     || NutIrqEnable(&sig_USART)) {
		return -1;
	}
	return 0;
}

/*!
 * \brief Deinitialize the USART hardware driver.
 *
 * This function is called during device deregistration by the upper
 * level USART driver through the USARTDCB jump table.
 *
 * \return 0 on success, -1 otherwise.
 */
static int Mk64fUsartDeinit(void)
{
	UART_Deinit(MK64F_USART_BASE);

#ifdef UART_HDB_FDX_BIT
	/*
	 * Disabling flow control shouldn't be required here, because it's up
	 * to the upper level to do this on the last close or during
	 * deregistration.
	 */
	if (hdx_control) {
		hdx_control = 0;
	}
#endif

#ifdef UART_CTS_BIT
		/* Deregister CTS sense interrupt. */
	if (cts_sense) {
		cts_sense = 0;
	}
#endif

#ifdef UART_RTS_BIT
	if (rts_control) {
		rts_control = 0;
	}
#endif

	return 0;
}

static void Mk64fUsartTxStart(void)
{
#if defined(UART_HDX_BIT) || defined(UART_HDB_FDX_BIT)
	if (hdx_control & HDX_CONTROL_MASK)
	{
	#if (defined(UART_HDB_FDX_BIT))
		if (hdx_control & HDX_CONTROL_HALFDUPLEX_YZ)
		{
			/* Enable xmitter on YZ */
			/* RE2 = 1 Disable Receiver, DE2 = 1 Enable Transmitter */
			GPIO_PinWrite(MK64F_USART_GPIO_DE2, MK64F_USART_PIN_RE2, 1);
			GPIO_PinWrite(MK64F_USART_GPIO_DE2, MK64F_USART_PIN_DE2, 1);
		}
		else
	#endif
		{
			/* Enable xmitter on AB */
			/* RE1 = 1 Disable Receiver, DE1 = 1 Enable Transmitter */
			GPIO_PinWrite(MK64F_USART_GPIO_DE1, MK64F_USART_PIN_RE1, 1);
			GPIO_PinWrite(MK64F_USART_GPIO_DE1, MK64F_USART_PIN_DE1, 1);
		}
	}
#endif
	/* Enable Transmitter Ready Interrupt */
	UART_EnableInterrupts(MK64F_USART_BASE,
			kUART_TxDataRegEmptyInterruptEnable | kUART_TransmissionCompleteInterruptEnable);
}

static void Mk64fUsartRxStart(void)
{
	/* Enable Receive Ready Interrupt */
	UART_EnableInterrupts(MK64F_USART_BASE, kUART_RxDataRegFullInterruptEnable);

	/*
	 * Do any required software flow control.
	 */
#ifdef UART_SW_FLOWCONTROL
	if (flow_control & XOFF_SENT)
	{
		NutUseCritical();
		NutEnterCriticalLevel(IH_USART_LEVEL);

		if (kUART_TxDataRegEmptyFlag & UART_GetStatusFlags(MK64F_USART_BASE)) {
			flow_control &= ~XON_PENDING;
		} else {
			flow_control &= ~(XOFF_SENT | XOFF_PENDING);
		}

		NutExitCritical();
	}
#endif

#ifdef UART_RTS_BIT
	if (rts_control) {
		/* Enable RTS. Manually asserted first time, automatically negated if overrun occurs */
		// GPIO_PinWrite(MK64F_USART_BASE, MK64F_USART_PIN_RTS, 0);
	}
#endif
}


static int Mk64fUsartSetFlowControl(uint32_t flags)
{
#ifdef UART_SW_FLOWCONTROL
	NutUseCritical();
	/*
	 * Set software handshake mode.
	 */
	if (flags & USART_MF_XONXOFF) {
		if (flow_control == 0) {
			NutEnterCriticalLevel(IH_USART_LEVEL);
			flow_control = 1 | XOFF_SENT; /* force XON to be sent on next read */
			NutExitCritical();
		}
	} else {
		NutEnterCriticalLevel(IH_USART_LEVEL);
		flow_control = 0;
		NutExitCritical();
	}
#endif

#ifdef UART_RTS_BIT
	/*
	 * Set RTS control mode.
	 */
	if (flags & USART_MF_RTSCONTROL) {
		/* Set GPIO function for RTS - PUC2 */
		// MCF_GPIO_RCTS_PAR &= ~MCF_GPIO_RCTS_PAR_RTS(0x03);

		/* Set as an output */
		// MCF_GPIO_RCTS_DDR |= MCF_GPIO_RCTS_DDR_RTS;

		/* Set output enable RTS */
		//MCF_GPIO_RCTS_CLR = ~MCF_GPIO_RCTS_PORT_RTS;

		rts_control = 1;
	} else if (rts_control) {
		rts_control = 0;
	}
#endif

#ifdef UART_CTS_BIT
	/*
	 * Set CTS sense mode.
	 */
	if (flags & USART_MF_CTSSENSE) {
		/* Set primary function (usart) for CTS */
		// MCF_GPIO_RCTS_PAR |= MCF_GPIO_RCTS_PAR_CTS;
		/* Set CTS hardware flow control, pointer points after read to UMR2 register after init */
		// usartControlRegister.umr2 |= MCF_UART_UMR_TXCTS;
		// MCF_UARTn_UMR2 = usartControlRegister.umr2;

		cts_sense = 1;
	}
	else if (cts_sense)
	{
		/* Clear CTS hardware flow control, pointer points after read to UMR2 register after init */
		// usartControlRegister.umr2 &= ~MCF_UART_UMR_TXCTS;
		// MCF_UARTn_UMR2 = usartControlRegister.umr2;
		cts_sense = 0;
	}
#endif

#ifndef UART_HDB_FDX_BIT
#ifdef UART_HDX_BIT
	/* Set loopback mode on AB transceiver - data Tx are echoed as Rx */
	if (flags & USART_MF_LOOPBACK_AB) {
		//Configure loopback on AB, this interface does not have YZ
		// Enable both Tx, Rx on first chip
		// MCF_GPIO_CLR_CHIP1 = ~MCF_GPIO_PORT_RE1;	/* RE1 = 0 Enable Receiver */
		// MCF_GPIO_SET_CHIP1 = MCF_GPIO_PORT_DE1;		/* DE1 = 1 Enable Transmitter */

		GPIO_PinWrite(MK64F_USART_GPIO_RE1, MK64F_USART_PIN_RE1, 0);
		GPIO_PinWrite(MK64F_USART_GPIO_DE1, MK64F_USART_PIN_DE1, 1);
		hdx_control = USART_MF_LOOPBACK_AB >> HDX_CONTROL_SHIFT;
	} else {
		/* Enable receiver on AB (force if previously loopback was set) */
		/* RE1 = 0 Enable Receiver, DE1 = 0 Disable Transmitter */
		// MCF_GPIO_CLR_CHIP1 = ~(MCF_GPIO_PORT_RE1 | MCF_GPIO_PORT_DE1);
		GPIO_PinWrite(MK64F_USART_GPIO_RE1, MK64F_USART_PIN_RE1, 0);
		GPIO_PinWrite(MK64F_USART_GPIO_DE1, MK64F_USART_PIN_DE1, 0);

		hdx_control = USART_MF_HALFDUPLEX >> HDX_CONTROL_SHIFT;
	}
#endif
#else	/* UART_HDB_FDX_BIT */

	/* Set RS232 mode - Komunikace pres porty YZ */
	if (flags & USART_MF_FULLDUPLEX_232)	/* same functionality as USART_MF_LOOPBACK_YZ (i.e. loopback for RS485, 232 for piggy-232) */
	{
		//Disable RS485 on AB, enable RS232 on YZ (same as Disable AB, configure loopback on YZ)
		// Disable 1st chip
		// MCF_GPIO_CLR_CHIP1 = ~MCF_GPIO_PORT_DE1;	/* DE1 = 0 Disable Transmitter */
		// MCF_GPIO_SET_CHIP1 = MCF_GPIO_PORT_RE1;		/* RE1 = 1 Disable Receiver */
		GPIO_PinWrite(MK64F_USART_GPIO_DE1, MK64F_USART_PIN_DE1, 0);
		GPIO_PinWrite(MK64F_USART_GPIO_RE1, MK64F_USART_PIN_RE1, 1);
		// Configure RS232 on second chip (Intersil ICL3221)
		// MCF_GPIO_CLR_CHIP2 = ~MCF_GPIO_PORT_RE2;	/* RE2 = 0 Enable Receiver, in this case EN_IN (enable input)*/
		// MCF_GPIO_SET_CHIP2 = MCF_GPIO_PORT_DE2;	/* DE2 = 1 Enable Transmitter, in this case F_OFF- (do not force off)*/
		GPIO_PinWrite(MK64F_USART_GPIO_RE2, MK64F_USART_PIN_RE2, 0);
		GPIO_PinWrite(MK64F_USART_GPIO_DE2, MK64F_USART_PIN_DE2, 1);
		//no switching Tx / Rx on transceiver chip
		hdx_control = USART_MF_FULLDUPLEX_232 >> HDX_CONTROL_SHIFT;
	} else if (flags & USART_MF_LOOPBACK_AB) {
	/* Set loopback mode on AB transceiver - data Tx are echoed as Rx */
		//Configure loopback on AB, disable YZ
		// Disable 2nd chip
		// MCF_GPIO_CLR_CHIP2 = ~MCF_GPIO_PORT_DE2;	/* DE2 = 0 Disable Transmitter */
		// MCF_GPIO_SET_CHIP2 = MCF_GPIO_PORT_RE2;		/* RE2 = 1 Disable Receiver */
		GPIO_PinWrite(MK64F_USART_GPIO_DE2, MK64F_USART_PIN_DE2, 0);
		GPIO_PinWrite(MK64F_USART_GPIO_RE2, MK64F_USART_PIN_RE2, 1);

		// Enable both Tx, Rx on first chip
		// MCF_GPIO_CLR_CHIP1 = ~MCF_GPIO_PORT_RE1;	/* RE1 = 0 Enable Receiver */
		// MCF_GPIO_SET_CHIP1 = MCF_GPIO_PORT_DE1;		/* DE1 = 1 Enable Transmitter */
		GPIO_PinWrite(MK64F_USART_GPIO_RE1, MK64F_USART_PIN_RE1, 0);
		GPIO_PinWrite(MK64F_USART_GPIO_DE1, MK64F_USART_PIN_DE1, 1);

		hdx_control = USART_MF_LOOPBACK_AB >> HDX_CONTROL_SHIFT;
	} else if (flags & USART_MF_HALFDUPLEX) {
	/*
	 * Set half duplex mode.
	 */
		/* Half duplex, enable RS485 on YZ */
		if (flags & USART_MF_HALFDUPLEX_YZ) {
			// Disable AB
			// MCF_GPIO_CLR_CHIP1 = ~MCF_GPIO_PORT_DE1;	/* DE1 = 0 Disable Transmitter */
			// MCF_GPIO_SET_CHIP1 = MCF_GPIO_PORT_RE1;		/* RE1 = 1 Disable Receiver */
			GPIO_PinWrite(MK64F_USART_GPIO_DE1, MK64F_USART_PIN_DE1, 0);
			GPIO_PinWrite(MK64F_USART_GPIO_RE1, MK64F_USART_PIN_RE1, 1);
			//TODO: conditionally not compile?
			/*
			 * If this mode is selected on REV_D boards with RS232 piggy-back,
			 * it enables RS232 transceiver to Rx from YZ which will interfere with Rx from AB!!!
			 */

			/* Enable half duplex on YZ */
			/* RE2 = 0 Enable Receiver, DE2 = 0 Disable Transmitter */
			// MCF_GPIO_CLR_CHIP2 = ~(MCF_GPIO_PORT_RE2 | MCF_GPIO_PORT_DE2);
			GPIO_PinWrite(MK64F_USART_GPIO_DE2, MK64F_USART_PIN_DE2, 0);
			GPIO_PinWrite(MK64F_USART_GPIO_RE2, MK64F_USART_PIN_RE2, 0);
			hdx_control = (USART_MF_HALFDUPLEX | USART_MF_HALFDUPLEX_YZ) >> HDX_CONTROL_SHIFT;
		} else {
		/* Half duplex, enable RS485 on AB */
			/* Disable YZ */
			// MCF_GPIO_CLR_CHIP2 = ~MCF_GPIO_PORT_DE2;	/* DE2 = 0 Disable Transmitter*/
			// MCF_GPIO_SET_CHIP2 = MCF_GPIO_PORT_RE2;		/* RE2 = 1 Disable Receiver */
			GPIO_PinWrite(MK64F_USART_GPIO_DE2, MK64F_USART_PIN_DE2, 0);
			GPIO_PinWrite(MK64F_USART_GPIO_RE2, MK64F_USART_PIN_RE2, 1);
			/* Enable half duplex on AB */
			/* RE1 = 0 Enable Receiver, DE1 = 0 Disable Transmitter */
			// MCF_GPIO_CLR_CHIP1 = ~(MCF_GPIO_PORT_RE1 | MCF_GPIO_PORT_DE1);
			GPIO_PinWrite(MK64F_USART_GPIO_DE1, MK64F_USART_PIN_DE1, 0);
			GPIO_PinWrite(MK64F_USART_GPIO_RE1, MK64F_USART_PIN_RE1, 0);

			hdx_control = USART_MF_HALFDUPLEX >> HDX_CONTROL_SHIFT;
		}
	} else {
	/*
	 * Set full duplex mode
	 */
		if (hdx_control) {
			//TODO: conditionally not compile?
			/*
			 * If this mode is selected on REV_D boards with RS232 piggy-back,
			 * it enables RS232 transceiver to Tx to YZ which can be of no care?
			 */
			hdx_control = 0;
		}

		/* RE1 = 0 Enable Receiver, DE1 = 0 Disable Transmitter */
		// MCF_GPIO_CLR_CHIP1 = ~(MCF_GPIO_PORT_RE1 | MCF_GPIO_PORT_DE1);
		GPIO_PinWrite(MK64F_USART_GPIO_DE1, MK64F_USART_PIN_DE1, 0);
		GPIO_PinWrite(MK64F_USART_GPIO_RE1, MK64F_USART_PIN_RE1, 0);

		/* RE2 = 1 Disable Receiver, DE2 = 1 Enable Transmitter */
		// MCF_GPIO_SET_CHIP2 = MCF_GPIO_PORT_RE2 | MCF_GPIO_PORT_DE2;
		GPIO_PinWrite(MK64F_USART_GPIO_DE2, MK64F_USART_PIN_DE2, 1);
		GPIO_PinWrite(MK64F_USART_GPIO_RE2, MK64F_USART_PIN_RE2, 1);
	}

#endif	/* UART_HDB_FDX_BIT */

	/*
	 * Verify the result.
	 */
	if (Mk64fUsartGetFlowControl() != flags)
	{
		return -1;
	}
	return 0;
}

static uint32_t Mk64fUsartGetFlowControl(void)
{
	uint32_t rc = 0;

#ifdef UART_SW_FLOWCONTROL
	if (flow_control) {
		rc |= USART_MF_XONXOFF;
	} else {
		rc &= ~USART_MF_XONXOFF;
	}
#endif

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

#if defined(UART_HDX_BIT) || defined(UART_HDB_FDX_BIT)
	rc |= hdx_control << HDX_CONTROL_SHIFT;
#endif
	return rc;
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
static int Mk64fUsartSetSpeed(uint32_t rate)
{
	status_t status;
	uint32_t srcClock_Hz = NutGetCpuClock();

	status = UART_SetBaudRate(MK64F_USART_BASE, rate, srcClock_Hz);

	if (status != kStatus_Success)
		return -1;

	m_baud_rate = rate;
	return 0;
}

static uint32_t Mk64fUsartGetSpeed(void)
{
	return m_baud_rate;
}

/*!
 * \brief Set the USART hardware to the number of data bits.
 *
 * This function is called by ioctl function of the upper level USART
 * driver through the USARTDCB jump table.
 *
 * \return 0 on success, -1 otherwise.
 */
static int Mk64fUsartSetDataBits(uint8_t bits)
{
	/* Clear register data bits. */

	switch (bits) {
	case 8:
		return 0;
	case 5:
	case 6:
	case 7:
	default:
		break;
	}
	return -1;
}

static uint8_t Mk64fUsartGetDataBits(void)
{
	return 8;
}

static int Mk64fUsartSetParity(uint8_t mode)
{
	uint8_t temp;
	uart_parity_mode_t parityMode;

	switch (mode)
	{
	case 1:
		parityMode = kUART_ParityOdd;
		break;
	case 2:
		parityMode = kUART_ParityEven;
		break;
	case 0:
		parityMode = kUART_ParityDisabled;
	default:
		return -1;
	}

	temp = MK64F_USART_BASE->C1 & ~(UART_C1_PE_MASK | UART_C1_PT_MASK | UART_C1_M_MASK | UART_C1_ILT_MASK);

	temp |= UART_C1_ILT(kUART_IdleTypeStartBit);

	if (kUART_ParityDisabled != parityMode) {
		temp |= (UART_C1_M_MASK | (uint8_t)parityMode);
	}

	MK64F_USART_BASE->C1 = temp;
	
	m_parity = mode;

	return 0;
}

static uint8_t Mk64fUsartGetParity(void)
{
	return m_parity;
}




static int Mk64fUsartSetStopBits(uint8_t bits)
{
	uart_stop_bit_count_t stopBitCount;

	switch (bits) {
	case 1:
		stopBitCount = kUART_OneStopBit;
		break;
	case 2:
		stopBitCount = kUART_TwoStopBit;
		break;
	default :
		return -1;
	}

	MK64F_USART_BASE->BDH = (MK64F_USART_BASE->BDH & ~UART_BDH_SBNS_MASK) | UART_BDH_SBNS((uint8_t)stopBitCount);

	m_stop_bits = bits;

	return 0;
}

static uint8_t Mk64fUsartGetStopBits(void)
{
	return m_stop_bits;
}

static int Mk64fUsartSetStatus(uint32_t flags)
{
	/*
	 * Process software handshake control.
	 */
#ifdef UART_SW_FLOWCONTROL
	if (flow_control) {
		NutUseCritical();

		/* Access to the flow control status must be atomic. */
		NutEnterCriticalLevel(IH_USART_LEVEL);

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
#endif
	// todo: RTS, CTS, ...
	/*
	 * Reset UART receiver and/or transmitter.
	 *
	 * Note: receiver/transmitter will be also disabled - see chapter 28.3.5/p. 28-9 in MCF52259 reference manual.
	 * (It can be reenabled by setting speed, parity, data bits or stop bits.)
	 */
	/*
	if (flags & UART_RESETRX) {
		#warning "USART RESETRX is not implemented"
	}
	if (flags & UART_RESETTX) {
		#warning "USART RESETRX is not implemented"
	}
	*/

	/*
	 * Clear UART receive errors.
	 */
	if (flags & UART_FRAMINGERROR) {
		// rx_errors &= ~MCF_UART_USR_FE;
	}
	if (flags & UART_OVERRUNERROR) {
		// MCF_UARTn_UCR = MCF_UART_UCR_RESET_ERROR; //reset all errors
		// rx_errors &= ~MCF_UART_USR_OE;
	}
	if (flags & UART_PARITYERROR) {
		// rx_errors &= ~MCF_UART_USR_PE;
	}
	if (flags & UART_RECEIVEBREAK) {
		// rx_errors &= ~MCF_UART_USR_RB;
	}

	return 0;
}

static uint32_t Mk64fUsartGetStatus(void)
{
	uint32_t rc = 0;

	rc = UART_GetStatusFlags(MK64F_USART_BASE);


	/*
	 * Determine software handshake status. The flow control status may
	 * change during interrupt, but this doesn't really hurt us.
	 */
#ifdef UART_SW_FLOWCONTROL
	if (flow_control) {
		if (flow_control & XOFF_SENT) {
			rc |= UART_RXDISABLED;
		}
		if (flow_control & XOFF_RCVD) {
			rc |= UART_TXDISABLED;
		}
	}
#endif
	// todo: RTS, CTS, ...

	return rc;
}


