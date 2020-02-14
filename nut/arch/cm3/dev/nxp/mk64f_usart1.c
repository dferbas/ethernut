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


#include <cfg/arch.h>
#include <dev/usart.h>
#include <dev/gpio.h>
#include <dev/board.h>
#include <sys/timer.h>

#include <arch/cm3/nxp/mk64f_usart.h>

static uint32_t Mk64fUsartGetSpeed(void);
static int Mk64fUsartSetSpeed(uint32_t rate);
static uint8_t Mk64fUsartGetDataBits(void);
static int Mk64fUsartSetDataBits(uint8_t bits);
static uint8_t Mk64fUsartGetParity(void);
static int Mk64fUsartSetParity(uint8_t mode);
static uint8_t Mk64fUsartGetStopBits(void);
static int Mk64fUsartSetStopBits(uint8_t bits);
static uint32_t Mk64fUsartGetStatus(void);
static int Mk64fUsartSetStatus(uint32_t flags);
static uint32_t Mk64fUsartGetFlowControl(void);
static int Mk64fUsartSetFlowControl(uint32_t flags);
static void Mk64fUsartTxStart(void);
static void Mk64fUsartRxStart(void);
static int Mk64fUsartInit(void);
static int Mk64fUsartDeinit(void);



/*!
 * \brief USART1 device control block structure.
 */
static USARTDCB dcb_usart1 = {
    0,                          /* dcb_modeflags */
    0,                          /* dcb_statusflags */
    0,                          /* dcb_rtimeout */
    0,                          /* dcb_wtimeout */
    {0, 0, 0, 0, 0, 0, 0, 0},   /* dcb_tx_rbf */
    {0, 0, 0, 0, 0, 0, 0, 0},   /* dcb_rx_rbf */
    0,                          /* dbc_last_eol */
    Mk64fUsartInit,               /* dcb_init */
    Mk64fUsartDeinit,             /* dcb_deinit */
    Mk64fUsartTxStart,            /* dcb_tx_start */
    Mk64fUsartRxStart,            /* dcb_rx_start */
    Mk64fUsartSetFlowControl,     /* dcb_set_flow_control */
    Mk64fUsartGetFlowControl,     /* dcb_get_flow_control */
    Mk64fUsartSetSpeed,           /* dcb_set_speed */
    Mk64fUsartGetSpeed,           /* dcb_get_speed */
    Mk64fUsartSetDataBits,        /* dcb_set_data_bits */
    Mk64fUsartGetDataBits,        /* dcb_get_data_bits */
    Mk64fUsartSetParity,          /* dcb_set_parity */
    Mk64fUsartGetParity,          /* dcb_get_parity */
    Mk64fUsartSetStopBits,        /* dcb_set_stop_bits */
    Mk64fUsartGetStopBits,        /* dcb_get_stop_bits */
    Mk64fUsartSetStatus,          /* dcb_set_status */
    Mk64fUsartGetStatus,          /* dcb_get_status */
    0,       					/* dcb_set_clock_mode */
    0,       					/* dcb_get_clock_mode */
};

/*!
 * \name mcf5xxxx USART1 Device
 */
/*@{*/

/*!
 * \brief mcf5xxxx device information structure.
 *
 * An application must pass a pointer to this structure to
 * NutRegisterDevice() before using the serial communication
 * driver of the AVR's on-chip USART1.
 *
 * The device is named \b uart1.
 *
 * \showinitializer
 */
NUTDEVICE devMk64fUsart_1 = {
    0,                          /* Pointer to next device, dev_next. */
    {'u', 'a', 'r', 't', '1', 0, 0, 0, 0},    /* Unique device name, dev_name. */
    IFTYP_CHAR,                 /* Type of device, dev_type. */
    1,                          /* Base address, dev_base. */
    0,                          /* First interrupt number, dev_irq (not used). */
    0,                          /* Interface control block, dev_icb (not used). */
    &dcb_usart1,                /* Driver control block, dev_dcb. */
    UsartInit,                  /* Driver initialization routine, dev_init. */
    UsartIOCtl,                 /* Driver specific control function, dev_ioctl. */
    UsartRead,                  /* Read from device, dev_read. */
    UsartWrite,                 /* Write to device, dev_write. */
    //UsartWrite_P,               /* Write data from program space to device, dev_write_P. */
    UsartOpen,                  /* Open a device or file, dev_open. */
    UsartClose,                 /* Close a device or file, dev_close. */
    UsartSize                   /* Request file size, dev_size. */
};

#define MK64F_USART_BASE UART4
#define MK64F_USART_IRQn UART4_IRQn
#define MK64F_USART_BAUDRATE 19200
#define MK64F_USART_PARITY kUART_ParityDisabled
#define MK64F_USART_STOP_BIT kUART_OneStopBit
#define MK64F_USART_CLOCK_FREQ (CLOCK_GetFreq(UART4_CLK_SRC))

#define MK64F_USART_GPIO_RE1 BOARD_USART4_RE1_GPIO
#define MK64F_USART_PIN_RE1 BOARD_USART4_RE1_PIN
#define MK64F_USART_GPIO_DE1 BOARD_USART4_DE1_GPIO
#define MK64F_USART_PIN_DE1 BOARD_USART4_DE1_PIN

/* USART1 Interrupt Handler */
#define sig_USART	    sig_USART4

/* Define Interrupt Level */
#define IH_USART_LEVEL 	IH_USART1_LEVEL

/* USART1 device control structure */
#define dcb_usart   dcb_usart1

#ifdef NUTTRACER
#define TRACE_INT_UART_CTS TRACE_INT_UART1_CTS
#define TRACE_INT_UART_RXCOMPL TRACE_INT_UART1_RXCOMPL
#define TRACE_INT_UART_TXEMPTY TRACE_INT_UART1_TXEMPTY
#endif


#ifdef UART1_SW_FLOWCONTROL
#define UART_SW_FLOWCONTROL
#endif

#ifndef UART_HDX_BIT
#define UART_HDX_BIT
#endif


#include "mk64f_usart.inc.c"
