/******************************************************************************
 *
 *                Copyright (c) 2017-2019 by Löwenware Ltd
 *             Please, refer LICENSE file for legal information
 *
 ******************************************************************************/

/**
 * @file mk64f_i2c0.c
 * @author Ilja Kartašov <ik@lowenware.com>
 * @brief 
 *
 * @see https://lowenware.com/
 */

#include <arch/mk64f.h>
#include <cfg/twi.h>
#include <dev/twif.h>
#include <dev/gpio.h>

/*!
 * \brief TWI/I2C bus structure.
 */
NUTTWIBUS Mk64fIc2Bus0 = {
  /*.bus_base =    */  I2C0_BASE,       /* Bus base address. */
  /*.bus_sig_ev =  */  &sig_I2C0,       /* Bus data and event interrupt handler. */
  /*.bus_sig_er =  */  NULL,            /* Bus error interrupt handler. */
  /*.bus_mutex =   */  NULL,            /* Bus lock queue. */
  /*.bus_icb   =   */  NULL,            /* Bus Runtime Data Pointer */
  /*.bus_dma_tx =  */  0,
  /*.bus_dma_rx =  */  0,
  /*.bus_initbus = */  NULL, /* Initialize bus controller. */
  /*.bus_recover = */  NULL,            /* Recover bus in case a slave hangs with SCL low */
};
