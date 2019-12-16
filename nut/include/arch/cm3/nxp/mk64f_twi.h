/******************************************************************************
 *
 *                Copyright (c) 2017-2019 by Löwenware Ltd
 *             Please, refer LICENSE file for legal information
 *
 ******************************************************************************/

/**
 * @file mk64f_twi.h
 * @author Ilja Kartašov <ik@lowenware.com>
 * @brief 
 *
 * @see https://lowenware.com/
 */

#ifndef MK64F_TWI_H_541E1301_9605_4936_A11C_0277DC1F1BFF
#define MK64F_TWI_H_541E1301_9605_4936_A11C_0277DC1F1BFF

#ifndef DEF_TWIBUS
#ifdef I2CBUS1_AS_DEFAULT
#define DEF_TWIBUS Mk64fTwiBus1
#else
#define DEF_TWIBUS Mk64fTwiBus0
#endif
#endif

struct _NUTTWIICB {
	char dummy;
};

typedef struct _NUTTWIICB NUTTWIICB;

#endif /* !MK64F_TWI_H */
