/******************************************************************************
 *
 *                Copyright (c) 2017-2019 by Löwenware Ltd
 *             Please, refer LICENSE file for legal information
 *
 ******************************************************************************/

/**
 * @file ih_mk64f.c
 * @author Ilja Kartašov <ik@lowenware.com>
 * @brief 
 *
 * @see https://lowenware.com/
 */

#include <cfg/arch.h>
#include <cfg/devices.h>
#include <arch/cm3.h>
#include <dev/irqreg.h>

#ifndef NUT_IRQPRI_DEF
#define NUT_IRQPRI_DEF  4
#endif

CREATE_HANDLER(I2C0,       I2C0,     NUT_IRQPRI_DEF);     // I2C 1 Data/Event


CREATE_HANDLER(USART0,        UART0_RX_TX,          NUT_IRQPRI_DEF);  // USART 0
CREATE_HANDLER(USART4,        UART4_RX_TX,          NUT_IRQPRI_DEF);  // USART 1
CREATE_HANDLER(USART5,        UART5_RX_TX,          NUT_IRQPRI_DEF);  // USART 2
CREATE_HANDLER(RTC,           RTC,        NUT_IRQPRI_DEF);    /* Real Time Clock */
