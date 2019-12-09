/******************************************************************************
 *
 *                Copyright (c) 2017-2019 by Löwenware Ltd
 *             Please, refer LICENSE file for legal information
 *
 ******************************************************************************/

/**
 * @file mk64f_clk.h
 * @author Ilja Kartašov <ik@lowenware.com>
 * @brief 
 *
 * @see https://lowenware.com/
 */

#ifndef MK64F_CLK_H_4CFE80F4_B186_4DC8_AB2F_7C15C49E3337
#define MK64F_CLK_H_4CFE80F4_B186_4DC8_AB2F_7C15C49E3337

#include <arch/cm3/nxp/vendor/fsl_clock.h>

#define BOARD_BOOTCLOCKRUN_CORE_CLOCK  120000000U  /*!< Core clock frequency: 120000000Hz */
#define BOARD_XTAL0_CLK_HZ             50000000U  /*!< Board xtal0 frequency in Hz */
#define BOARD_BOOTCLOCKVLPR_CORE_CLOCK 4000000U  /*!< Core clock frequency: 4000000Hz */

#define SysCtlClockGet() MK64F_ClockGet(kCLOCK_CoreSysClk)

/*! @brief MCG set for BOARD_BootClockRUN configuration.
 */
extern const mcg_config_t mcgConfig_BOARD_BootClockRUN;
/*! @brief SIM module set for BOARD_BootClockRUN configuration.
 */
extern const sim_clock_config_t simConfig_BOARD_BootClockRUN;
/*! @brief OSC set for BOARD_BootClockRUN configuration.
 */
extern const osc_config_t oscConfig_BOARD_BootClockRUN;


int SetSysClock(void);

unsigned int MK64F_ClockGet(int idx);

#endif /* !MK64F_CLK_H */
