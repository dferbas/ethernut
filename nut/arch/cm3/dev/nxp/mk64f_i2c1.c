/******************************************************************************
 *
 *                Copyright (c) 2017-2019 by Löwenware Ltd
 *             Please, refer LICENSE file for legal information
 *
 ******************************************************************************/

/**
 * @file mk64f_i2c1.c
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
 * \brief Processor specific Hardware Initiliaization
 *
 */
int Mk64fI2c1Init(void)
{
    /* Enable the I2C signals */
    GpioPinConfigSet(I2C0_SCL_PORT, I2C0_SCL_PIN, I2C0_SCL_PERIPHERAL);
    GpioPinConfigSet(I2C0_SDA_PORT, I2C0_SDA_PIN, I2C0_SDA_PERIPHERAL);

    return 0;
}

/*!
 * \brief TWI/I2C bus structure.
 */
NUTTWIBUS Mk64fIc2Bus1 = {
  /*.bus_base =    */  0,               /* Bus base address. */
  /*.bus_sig_ev =  */  &sig_I2C0,       /* Bus data and event interrupt handler. */
  /*.bus_sig_er =  */  NULL,            /* Bus error interrupt handler. */
  /*.bus_mutex =   */  NULL,            /* Bus lock queue. */
  /*.bus_icb   =   */  NULL,            /* Bus Runtime Data Pointer */
  /*.bus_dma_tx =  */  0,
  /*.bus_dma_rx =  */  0,
  /*.bus_initbus = */  Mk64fI2c0Init, /* Initialize bus controller. */
  /*.bus_recover = */  NULL,            /* Recover bus in case a slave hangs with SCL low */
};
