/*
 * Copyright (C) 2001-2005 by egnite Software GmbH. All rights reserved.
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

/*!
 * \file arch/avr/dev/ih_int4.c
 * \brief AVR external interrupt 4.
 *
 * \verbatim
 * $Id: ih_int4.c 4706 2012-10-06 17:42:01Z haraldkipp $
 * \endverbatim
 */

#include <dev/irqreg.h>

/*!
 * \addtogroup xgIrqReg
 */
/*@{*/

static int AvrInterrupt4Ctl(int cmd, void *param);

IRQ_HANDLER sig_INTERRUPT4 = {
#ifdef NUT_PERFMON
    0,                          /* Interrupt counter, ir_count. */
#endif
    NULL,                       /* Passed argument, ir_arg. */
    NULL,                       /* Handler subroutine, ir_handler. */
    AvrInterrupt4Ctl            /* Interrupt control, ir_ctl. */
};

/*!
 * \brief External interrupt 4 control.
 *
 * \param cmd   Control command.
 *              - NUT_IRQCTL_INIT Initialize and disable interrupt.
 *              - NUT_IRQCTL_STATUS Query interrupt status.
 *              - NUT_IRQCTL_ENABLE Enable interrupt.
 *              - NUT_IRQCTL_DISABLE Disable interrupt.
 *              - NUT_IRQCTL_GETMODE Query interrupt mode.
 *              - NUT_IRQCTL_SETMODE Set interrupt mode.
 *              - NUT_IRQCTL_GETPRIO Query interrupt priority.
 *              - NUT_IRQCTL_SETPRIO Set interrupt priority.
 *              - NUT_IRQCTL_GETCOUNT Query and clear interrupt counter.
 * \param param Pointer to optional parameter.
 *
 * \return 0 on success, -1 otherwise.
 */
static int AvrInterrupt4Ctl(int cmd, void *param)
{
    int rc = 0;
    unsigned int *ival = (unsigned int *) param;
    int_fast8_t enabled = bit_is_set(EIMSK, INT4);
    uint8_t bval;

    /* Disable interrupt. */
    cbi(EIMSK, INT4);

    switch (cmd) {
    case NUT_IRQCTL_INIT:
        /* Initialize to low level triggered. */
        cbi(EICR, ISC40);
        cbi(EICR, ISC41);
    case NUT_IRQCTL_CLEAR:
        /* Clear any pending interrupt. */
        outb(EIFR, _BV(INTF4));
        break;
    case NUT_IRQCTL_STATUS:
        if (bit_is_set(EIFR, INTF4)) {
            *ival = 1;
        } else {
            *ival = 0;
        }
        if (enabled) {
            *ival |= 0x80;
        }
        break;
    case NUT_IRQCTL_ENABLE:
        enabled = 1;
        break;
    case NUT_IRQCTL_DISABLE:
        enabled = 0;
        break;
    case NUT_IRQCTL_GETMODE:
        bval = inb(EICR) & _BV(ISC41 | ISC40);
        if (bval == _BV(ISC41)) {
            *ival = NUT_IRQMODE_FALLINGEDGE;
        } else if (bval == _BV(ISC40)) {
            *ival = NUT_IRQMODE_EDGE;
        } else if (bval == _BV(ISC41 | ISC40)) {
            *ival = NUT_IRQMODE_RISINGEDGE;
        } else {
            *ival = NUT_IRQMODE_LOWLEVEL;
        }
        break;
    case NUT_IRQCTL_SETMODE:
        if (*ival == NUT_IRQMODE_LOWLEVEL) {
            cbi(EICR, ISC40);
            cbi(EICR, ISC41);
        } else if (*ival == NUT_IRQMODE_EDGE) {
            sbi(EICR, ISC40);
            cbi(EICR, ISC41);
        } else if (*ival == NUT_IRQMODE_FALLINGEDGE) {
            cbi(EICR, ISC40);
            sbi(EICR, ISC41);
        } else if (*ival == NUT_IRQMODE_RISINGEDGE) {
            sbi(EICR, ISC40);
            sbi(EICR, ISC41);
        } else {
            rc = -1;
        }
        break;
    case NUT_IRQCTL_GETPRIO:
        *ival = 4;
        break;
#ifdef NUT_PERFMON
    case NUT_IRQCTL_GETCOUNT:
        *ival = (unsigned int) sig_INTERRUPT4.ir_count;
        sig_INTERRUPT4.ir_count = 0;
        break;
#endif
    default:
        rc = -1;
        break;
    }

    /* Enable interrupt. */
    if (enabled) {
        sbi(EIMSK, INT4);
    }
    return rc;
}

/*! \fn INT4_vect(void)
 * \brief External interrupt 4 entry.
 */
#ifdef __IMAGECRAFT__
#pragma interrupt_handler INT4_vect:iv_INT4
#endif
NUTSIGNAL(INT4_vect, sig_INTERRUPT4)


/*@}*/
