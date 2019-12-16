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
#define DEF_TWIBUS Mk64fIc2Bus0
#endif

struct _NUTTWIICB {
    /********** Master mode *********/

    /*! \brief Bus slave address.
     */
    volatile uint_fast16_t tw_mm_sla;

    /*! \brief Bus current error condition.
     */
    volatile int_fast8_t tw_mm_err;

    /*! \brief Bus last error condition.
     */
    volatile int_fast8_t tw_mm_error;

    /*! \brief Bus nodes internal address register length.
     */
    uint8_t *tw_mm_iadr;

    /*! \brief Bus nodes internal address register.
     */
    volatile uint_fast8_t tw_mm_iadrlen;

    /*! \brief Bus transmission data buffer pointer.
     */
    const uint8_t *tw_mm_txbuf;

    /*! \brief Bus transmission data block length.
     */
    volatile uint_fast16_t tw_mm_txlen;

    /*! \brief Bus transmissinn position.
     */
    volatile uint_fast16_t tw_mm_txidx;

    /*! \brief Bus reception data buffer pointer.
     */
    uint8_t *tw_mm_rxbuf;

    /*! \brief Bus reception data block length.
     */
    volatile uint_fast16_t tw_mm_rxlen;

    /*! \brief Bus reception position.
     */
    volatile uint_fast16_t tw_mm_rxidx;


    /*! \brief Bus data direction.
     */
    volatile uint_fast8_t tw_mm_dir;

    volatile int_fast8_t tw_mm_rxbegin;

    /*! \brief Transmission Ongoing Mutex.
     */
    HANDLE tw_mm_mtx;
};

typedef struct _NUTTWIICB NUTTWIICB;

extern NUTTWIBUS Mk64fIc2Bus0;


#endif /* !MK64F_TWI_H */
