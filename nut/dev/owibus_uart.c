/*
 * Copyright (C) 2012/2014/2015, 2017 by Uwe Bonnes
 *                                    (bon@elektron.ikp.physik.tu-darmstadt.de)
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

/*!
 * \file dev/owibus_uart.c
 * \brief Common implementation of the One-Wire via Uart.
 *
 * \verbatim
 * $Id: owibus_uart.c 6833 2019-11-16 18:56:47Z u_bonnes $
 * \endverbatim
 */
#if defined(OWI_DEBUG)
# include <stdio.h>
#endif
#include <cfg/arch.h>
#include <cfg/owi.h>
#include <stdint.h>
#include <fcntl.h>
#include <dev/uart.h>
#include <dev/owibus.h>
#include <dev/owibus_uart.h>

/*!
 * \addtogroup xgOwibusUart
 */
/*@{*/

/*!
 * \brief Initialize the Owibus uart device.
 *
 * If NUTOWIINFO_UART->uart is given, the uart is registered and opened.
 * If NUTOWIINFO_UART->uart_fd is given (not 0), uart_fd is used as usart file.
 *
 * \param bus Specifies the One-Wire bus.
 *
 * \return 0 on success, -1 otherwise.
 */

int Uart_OwiSetup(NUTOWIBUS *bus)
{
    NUTOWIINFO_UART *owcb;
    int uart_fd;
    uint32_t timeout = 5;
    uint32_t stopbits = 2;
    int res;

    owcb = (NUTOWIINFO_UART *) bus->owibus_info;
    if (owcb->uart_fd) {
        return OWI_SUCCESS;
    }
    res = NutRegisterDevice(owcb->uart, 0, 0);
    if (res ) {
        return OWI_INVALID_HW;
    }

    uart_fd = _open(owcb->uart->dev_name, _O_RDWR | _O_BINARY);
    if (uart_fd == -1) {
        return OWI_INVALID_HW;
    }

    if (owcb->owimode) {
        uint32_t mode = 1;
        int res;
        res = _ioctl(uart_fd, UART_SETOWIMODE, &mode);
        res |= _ioctl(uart_fd, UART_GETOWIMODE, &mode);
        if (res || (mode == 0)) {
            return OWI_INVALID_HW;
        }
    }
    _ioctl(uart_fd, UART_SETREADTIMEOUT, &timeout);
    _ioctl(uart_fd, UART_SETSTOPBITS, &stopbits);

    /* Empty RX buffer, as pin setup might have caused several
       transitions on RX */
    _read(uart_fd, NULL, 0);
    owcb->uart_fd = uart_fd;
    return OWI_SUCCESS;
}

/*!
 * \brief Reset the One-Wire bus and check if device(s) present.
 *
 * \param bus Specifies the One-Wire bus.
 *
 * \return OWI_SUCCESS on success, a negative value otherwise.
 */
int Uart_OwiTouchReset(NUTOWIBUS *bus)
{
    NUTOWIINFO_UART *owcb = (NUTOWIINFO_UART *) (bus->owibus_info);
    uint8_t send_data[1] = { OWI_UART_WRITE_RST };
    uint8_t rec_data[1] = { 0 };
    uint32_t baud_presence = OWI_UART_BAUD_RESET;
    uint32_t baud_owi_rwbit = OWI_UART_BAUD_RWBIT;
    int res;
    int uart_fd = owcb->uart_fd;

    _ioctl(uart_fd, UART_SETSPEED, &baud_presence);
    _write(uart_fd, send_data, 1);
    res = _read(uart_fd, rec_data, 1);
    if (res == -1) {
        return OWI_HW_ERROR;
    }
    _ioctl(uart_fd, UART_SETSPEED, &baud_owi_rwbit);
    if ((rec_data[0] != 0xf0) && !(rec_data[0] & 0x10) && (rec_data[0] != 0)) {
        return OWI_SUCCESS;
    }
    return OWI_PRESENCE_ERR;
}

/*!
 * \brief Exchange one bit on the One-Wire bus.
 *
 * \param bus Specifies the One-Wire bus.
 * \param bit Value for the bit to send.
 *
 * \return The bus state at the read slot on success, a negative value
 *         otherwise.
 */
int Uart_OwiRWBit(NUTOWIBUS *bus, uint_fast8_t bit)
{
    NUTOWIINFO_UART *owcb = (NUTOWIINFO_UART *) (bus->owibus_info);
    uint8_t send_data[1] = { (bit) ? OWI_UART_WRITE_ONE : OWI_UART_WRITE_ZERO };
    uint8_t rec_data[1] = { 0 };

    _write(owcb->uart_fd, send_data, 1);
    if (_read(owcb->uart_fd, rec_data, 1) == -1) {
        return OWI_HW_ERROR;
    }
    if (rec_data[0] & OWI_UART_READ_ONE) {
        return 1;
    }
    return 0;
}

/*!
 * \brief Write a block of data bits to the One-Wire bus.
 *
 * \param bus  Specifies the One-Wire bus.
 * \param data Data bits to send.
 * \param len  Number of bits to send.
 *
 * \return OWI_SUCCESS on success, a negative value otherwise.
 */
int Uart_OwiWriteBlock(NUTOWIBUS *bus, const uint8_t *data, uint_fast8_t len)
{
    int i;
#define OWI_BLOCK
#if defined(OWI_BLOCK)
    NUTOWIINFO_UART *owcb = (NUTOWIINFO_UART *) (bus->owibus_info);
    uint8_t tx_rx_data[8];

    i = 0;
    while(len) {
        uint8_t temp;
        int j, k;
        j = (len > 8)? 8: len;

        temp = data[i];
        for (k = 0; k < j; k++) {
            tx_rx_data[k] = (temp & (1 << k)) ?
                OWI_UART_WRITE_ONE : OWI_UART_WRITE_ZERO;
        }
        _write(owcb->uart_fd, tx_rx_data, j);
        _write(owcb->uart_fd, NULL, 0); /* Flush output */
        len -= j;
#define UART_BAD_FLUSH
/* FIXME!
*  The Flush implementation in dev/usart.h can return before
*  the last tx byte is written. This hapens when flush is called
*  after the last byte was written to the tx register. This is always
*  the case with one byte writes.
*  Fixing needs introduction of a flush handle to all low lever drivers!*/
#if defined(OWI_DEBUG) || defined(UART_BAD_FLUSH)
        k = j;
        uint8_t *p = tx_rx_data;
        while (k) {
            int res = _read(owcb->uart_fd, p, k);
            if (res < 1) {
                return OWI_HW_ERROR;
            }
            k -= res;
            p += res;
        }
# if defined(OWI_DEBUG)
        printf("j %d tx_data :", j);
        for (k = 0; k < j; k++) {
            printf(" %02x",tx_rx_data[k]);
        }
        puts("");
# endif
#else
        _read(owcb->uart_fd, NULL, 0); /* Drain receive buffer. */

#endif
        i++;
    }
#else
    for (i = 0; i < len; i++) {
        int res;
        res = Uart_OwiRWBit(bus, data[i >> 3] & (1 << (i & 0x7)));
        if (res < 0) {
            return OWI_HW_ERROR;
        }
    }
#endif
    return OWI_SUCCESS;
}

/*!
 * \brief Read a block of data bits from the One-Wire bus.
 *
 * \param bus  Specifies the One-Wire bus.
 * \param data Data bits received.
 * \param len  Number of bits to read.
 *
 * \return OWI_SUCCESS on success, a negative value otherwise.
 */
int Uart_OwiReadBlock(NUTOWIBUS *bus, uint8_t *data, uint_fast8_t len)
{
    int res;
    int i;
#if defined(OWI_BLOCK)
    NUTOWIINFO_UART *owcb = (NUTOWIINFO_UART *) (bus->owibus_info);
    static const char owi_ones[8] PROGMEM = {
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff
    };
    i = 0;
    while (len) {
        uint8_t temp = 0;
        int j = (len > 8)? 8 : len;
        uint8_t rx_data[8], *p = rx_data;
        int read = 0;
        int k = j;
#ifdef __HARVARD_ARCH__
        _write_P(owcb->uart_fd, owi_ones, j);
#else
        _write(owcb->uart_fd, owi_ones, j);
#endif
        while (read < j) {
            res = _read(owcb->uart_fd, p, k);
            if (res < 1) {
                return OWI_HW_ERROR;
            } else {
                k -= res;
                p += res;
                while (res) {
                    if (rx_data[read] & OWI_UART_READ_ONE) {
                        temp |= 1 << read;
                    }
                    read++;
                    res--;
                }
            }
        }
#if defined(OWI_DEBUG)
        printf("j %d rx_data", j);
        for (k = 0; k < j; k++) {
            printf(" %02x", rx_data[k]);
        }
        puts("");
#endif
        data[i] = temp;
        i++;
        len -= j;
    }
#else
    memset(data, 0, ((len +7) >> 3));
    for (i = 0; i < len; i++) {
        res = Uart_OwiRWBit(bus, 1);
        if (res < 0) {
            return OWI_HW_ERROR;
        }
        data[i >> 3] |= (res << (i & 0x7));
    }
#endif
    return OWI_SUCCESS;
}

#if defined(OWI0_UART)
extern NUTDEVICE OWI0_UART;
static NUTOWIINFO_UART owcb0 = {
    .uart = &OWI0_UART,
#if defined(OWI0_UART_HALFDUPLEX)
    .owimode = -1,
#endif
};
NUTOWIBUS owiBus0Uart = {
    (uintptr_t )&owcb0, /*!< \brief OWIBUSBUS::owibus_info */
    OWI_MODE_NORMAL,    /*!< \brief OWIBUSBUS::mode */
    Uart_OwiSetup,      /*!< \brief OWIBUSBUS::OwiSetup */
    Uart_OwiTouchReset, /*!< \brief OWIBUSBUS::OwiTouchReset*/
    Uart_OwiReadBlock,  /*!< \brief OWIBUSBUS::OwiReadBlock */
    Uart_OwiWriteBlock  /*!< \brief OWIBUSBUS::OwiWriteBlock */
 };
#endif
/*@}*/
