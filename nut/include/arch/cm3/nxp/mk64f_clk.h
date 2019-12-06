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

#define BOARD_BOOTCLOCKRUN_CORE_CLOCK             120000000U  /*!< Core clock frequency: 120000000Hz */

#define SysCtlClockGet() MK64F_ClockGet(kCLOCK_CoreSysClk)

/*! @brief External XTAL0 (OSC0) clock frequency.
 *
 * The XTAL0/EXTAL0 (OSC0) clock frequency in Hz. When the clock is set up, use the
 * function CLOCK_SetXtal0Freq to set the value in the clock driver. For example,
 * if XTAL0 is 8 MHz:
 * @code
 * CLOCK_InitOsc0(...); // Set up the OSC0
 * CLOCK_SetXtal0Freq(80000000); // Set the XTAL0 value to the clock driver.
 * @endcode
 *
 * This is important for the multicore platforms where only one core needs to set up the
 * OSC0 using the CLOCK_InitOsc0. All other cores need to call the CLOCK_SetXtal0Freq
 * to get a valid clock frequency.
 */
extern unsigned int g_xtal0Freq;

/*! @brief External XTAL32/EXTAL32/RTC_CLKIN clock frequency.
 *
 * The XTAL32/EXTAL32/RTC_CLKIN clock frequency in Hz. When the clock is set up, use the
 * function CLOCK_SetXtal32Freq to set the value in the clock driver.
 *
 * This is important for the multicore platforms where only one core needs to set up
 * the clock. All other cores need to call the CLOCK_SetXtal32Freq
 * to get a valid clock frequency.
 */
extern unsigned int g_xtal32Freq;

/*! @brief IRC48M clock frequency in Hz. */
#define MCG_INTERNAL_IRC_48M 48000000U

#if (defined(OSC) && !(defined(OSC0)))
#define OSC0 OSC
#endif

/*!
 * @brief LPO clock frequency.
 */
#define LPO_CLK_FREQ 1000U

/*! @brief Peripherals clock source definition. */
#define SYS_CLK kCLOCK_CoreSysClk
#define BUS_CLK kCLOCK_BusClk

#define I2C0_CLK_SRC BUS_CLK
#define I2C1_CLK_SRC BUS_CLK
#define I2C2_CLK_SRC BUS_CLK
#define DSPI0_CLK_SRC BUS_CLK
#define DSPI1_CLK_SRC BUS_CLK
#define DSPI2_CLK_SRC BUS_CLK
#define UART0_CLK_SRC SYS_CLK
#define UART1_CLK_SRC SYS_CLK
#define UART2_CLK_SRC BUS_CLK
#define UART3_CLK_SRC BUS_CLK
#define UART4_CLK_SRC BUS_CLK
#define UART5_CLK_SRC BUS_CLK

typedef enum _clock_index
{

    /* ----------------------------- System layer clock -------------------------------*/
    kCLOCK_CoreSysClk,    /*!< Core/system clock                                         */
    kCLOCK_PlatClk,       /*!< Platform clock                                            */
    kCLOCK_BusClk,        /*!< Bus clock                                                 */
    kCLOCK_FlexBusClk,    /*!< FlexBus clock                                             */
    kCLOCK_FlashClk,      /*!< Flash clock                                               */
    kCLOCK_FastPeriphClk, /*!< Fast peripheral clock                                     */
    kCLOCK_PllFllSelClk,  /*!< The clock after SIM[PLLFLLSEL].                           */

    /* ---------------------------------- OSC clock -----------------------------------*/
    kCLOCK_Er32kClk,       /*!< External reference 32K clock (ERCLK32K)                   */
    kCLOCK_Osc0ErClk,      /*!< OSC0 external reference clock (OSC0ERCLK)                 */
    kCLOCK_Osc1ErClk,      /*!< OSC1 external reference clock (OSC1ERCLK)                 */
    kCLOCK_Osc0ErClkUndiv, /*!< OSC0 external reference undivided clock(OSC0ERCLK_UNDIV). */

    /* ----------------------------- MCG and MCG-Lite clock ---------------------------*/
    kCLOCK_McgFixedFreqClk,   /*!< MCG fixed frequency clock (MCGFFCLK)                      */
    kCLOCK_McgInternalRefClk, /*!< MCG internal reference clock (MCGIRCLK)                   */
    kCLOCK_McgFllClk,         /*!< MCGFLLCLK                                                 */
    kCLOCK_McgPll0Clk,        /*!< MCGPLL0CLK                                                */
    kCLOCK_McgPll1Clk,        /*!< MCGPLL1CLK                                                */
    kCLOCK_McgExtPllClk,      /*!< EXT_PLLCLK                                                */
    kCLOCK_McgPeriphClk,      /*!< MCG peripheral clock (MCGPCLK)                            */
    kCLOCK_McgIrc48MClk,      /*!< MCG IRC48M clock                                          */

    /* --------------------------------- Other clock ----------------------------------*/
    kCLOCK_LpoClk, /*!< LPO clock                                                 */

} clock_index_t;

/*! @brief MCG FLL reference clock source select. */
typedef enum _mcg_fll_src
{
    kMCG_FllSrcExternal, /*!< External reference clock is selected          */
    kMCG_FllSrcInternal  /*!< The slow internal reference clock is selected */
} mcg_fll_src_t;

/*! @brief MCG internal reference clock select */
typedef enum _mcg_irc_mode
{
    kMCG_IrcSlow, /*!< Slow internal reference clock selected */
    kMCG_IrcFast  /*!< Fast internal reference clock selected */
} mcg_irc_mode_t;

/*! @brief MCG OSC Clock Select */
typedef enum _mcg_oscsel
{
    kMCG_OscselOsc, /*!< Selects System Oscillator (OSCCLK) */
    kMCG_OscselRtc, /*!< Selects 32 kHz RTC Oscillator      */
    kMCG_OscselIrc  /*!< Selects 48 MHz IRC Oscillator      */
} mcg_oscsel_t;

typedef struct _oscer_config
{
    unsigned char enableMode; /*!< OSCERCLK enable mode. OR'ed value of @ref _oscer_enable_mode. */

} oscer_config_t;

typedef enum _osc_mode
{
    kOSC_ModeExt = 0U, /*!< Use an external clock.   */
    kOSC_ModeOscLowPower = MCG_C2_EREFS0_MASK, /*!< Oscillator low power. */
    kOSC_ModeOscHighGain = 0U | MCG_C2_EREFS0_MASK | MCG_C2_HGO0_MASK 
} osc_mode_t;

typedef struct _osc_config
{
    unsigned int freq;              /*!< External clock frequency.    */
    unsigned char capLoad;            /*!< Capacitor load setting.      */
    osc_mode_t workMode;        /*!< OSC work mode setting.       */
    oscer_config_t oscerConfig; /*!< Configuration for OSCERCLK.  */
} osc_config_t;

typedef struct _sim_clock_config
{
    unsigned char pllFllSel; /*!< PLL/FLL/IRC48M selection.    */
    unsigned char er32kSrc;  /*!< ERCLK32K source selection.   */
    unsigned int clkdiv1;  /*!< SIM_CLKDIV1.                 */
} sim_clock_config_t;


typedef struct _mcg_pll_config
{
    unsigned char enableMode; /*!< Enable mode. OR'ed value of @ref _mcg_pll_enable_mode. */
    unsigned char prdiv;      /*!< Reference divider PRDIV.    */
    unsigned char vdiv;       /*!< VCO divider VDIV.           */
} mcg_pll_config_t;

typedef enum _mcg_pll_clk_select
{
    kMCG_PllClkSelPll0, /*!< PLL0 output clock is selected  */
    kMCG_PllClkSelPll1  /* PLL1 output clock is selected    */
} mcg_pll_clk_select_t;

typedef enum _mcg_clkout_src
{
    kMCG_ClkOutSrcOut,      /*!< Output of the FLL is selected (reset default)  */
    kMCG_ClkOutSrcInternal, /*!< Internal reference clock is selected           */
    kMCG_ClkOutSrcExternal, /*!< External reference clock is selected           */
} mcg_clkout_src_t;

enum _mcg_pll_enable_mode
{
    kMCG_PllEnableIndependent = MCG_C5_PLLCLKEN0_MASK, /*!< MCGPLLCLK enable independent of the
                                                           MCG clock mode. Generally, the PLL
                                                           is disabled in FLL modes
                                                           (FEI/FBI/FEE/FBE). Setting the PLL clock
                                                           enable independent, enables the
                                                           PLL in the FLL modes.          */
    kMCG_PllEnableInStop = MCG_C5_PLLSTEN0_MASK        /*!< MCGPLLCLK enable in STOP mode. */
};

enum _oscer_enable_mode
{
    kOSC_ErClkEnable = OSC_CR_ERCLKEN_MASK,       /*!< Enable.              */
    kOSC_ErClkEnableInStop = OSC_CR_EREFSTEN_MASK /*!< Enable in stop mode. */
};

enum _mcg_irclk_enable_mode
{
    kMCG_IrclkEnable = MCG_C1_IRCLKEN_MASK,       /*!< MCGIRCLK enable.              */
    kMCG_IrclkEnableInStop = MCG_C1_IREFSTEN_MASK /*!< MCGIRCLK enable in stop mode. */
};

typedef enum _mcg_drs
{
    kMCG_DrsLow,     /*!< Low frequency range       */
    kMCG_DrsMid,     /*!< Mid frequency range       */
    kMCG_DrsMidHigh, /*!< Mid-High frequency range  */
    kMCG_DrsHigh     /*!< High frequency range      */
} mcg_drs_t;

typedef enum _mcg_dmx32
{
    kMCG_Dmx32Default, /*!< DCO has a default range of 25% */
    kMCG_Dmx32Fine     /*!< DCO is fine-tuned for maximum frequency with 32.768 kHz reference */
} mcg_dmx32_t;

typedef enum _mcg_mode
{
    kMCG_ModeFEI = 0U, /*!< FEI   - FLL Engaged Internal         */
    kMCG_ModeFBI,      /*!< FBI   - FLL Bypassed Internal        */
    kMCG_ModeBLPI,     /*!< BLPI  - Bypassed Low Power Internal  */
    kMCG_ModeFEE,      /*!< FEE   - FLL Engaged External         */
    kMCG_ModeFBE,      /*!< FBE   - FLL Bypassed External        */
    kMCG_ModeBLPE,     /*!< BLPE  - Bypassed Low Power External  */
    kMCG_ModePBE,      /*!< PBE   - PLL Bypassed External        */
    kMCG_ModePEE,      /*!< PEE   - PLL Engaged External         */
    kMCG_ModeError     /*!< Unknown mode                         */
} mcg_mode_t;

typedef struct _mcg_config
{
    mcg_mode_t mcgMode; /*!< MCG mode.                   */

    /* ----------------------- MCGIRCCLK settings ------------------------ */
    uint8_t irclkEnableMode; /*!< MCGIRCLK enable mode.       */
    mcg_irc_mode_t ircs;     /*!< Source, MCG_C2[IRCS].       */
    uint8_t fcrdiv;          /*!< Divider, MCG_SC[FCRDIV].    */

    /* ------------------------ MCG FLL settings ------------------------- */
    uint8_t frdiv;       /*!< Divider MCG_C1[FRDIV].      */
    mcg_drs_t drs;       /*!< DCO range MCG_C4[DRST_DRS]. */
    mcg_dmx32_t dmx32;   /*!< MCG_C4[DMX32].              */
    mcg_oscsel_t oscsel; /*!< OSC select MCG_C7[OSCSEL].  */

    /* ------------------------ MCG PLL settings ------------------------- */
    mcg_pll_config_t pll0Config; /*!< MCGPLL0CLK configuration.   */

} mcg_config_t;

static inline void CLOCK_SetXtal0Freq(unsigned int freq)
{
    g_xtal0Freq = freq;
}

static inline void CLOCK_SetXtal32Freq(unsigned int freq)
{
    g_xtal32Freq = freq;
}

static inline void CLOCK_SetSimSafeDivs(void)
{
    SIM->CLKDIV1 = 0x01240000U;
}

static inline void OSC_SetCapLoad(OSC_Type *base, uint8_t capLoad)
{
    uint8_t reg = base->CR;

    reg &= ~(OSC_CR_SC2P_MASK | OSC_CR_SC4P_MASK | OSC_CR_SC8P_MASK | OSC_CR_SC16P_MASK);
    reg |= capLoad;

    base->CR = reg;
}

static inline void OSC_SetExtRefClkConfig(OSC_Type *base, oscer_config_t const *config)
{
    uint8_t reg = base->CR;

    reg &= ~(OSC_CR_ERCLKEN_MASK | OSC_CR_EREFSTEN_MASK);
    reg |= config->enableMode;

    base->CR = reg;
}

static inline void CLOCK_SetPllFllSelClock(uint32_t src)
{
    SIM->SOPT2 = ((SIM->SOPT2 & ~SIM_SOPT2_PLLFLLSEL_MASK) | SIM_SOPT2_PLLFLLSEL(src));
}

static inline void CLOCK_SetEr32kClock(uint32_t src)
{
    SIM->SOPT1 = ((SIM->SOPT1 & ~SIM_SOPT1_OSC32KSEL_MASK) | SIM_SOPT1_OSC32KSEL(src));
}

unsigned int MK64F_ClockGet(clock_index_t idx);

int SetSysClock(void);

void CLOCK_InitOsc0(osc_config_t const *config);

int CLOCK_SetInternalRefClkConfig(unsigned char enableMode, mcg_irc_mode_t ircs, unsigned char fcrdiv);

void CLOCK_SetSimConfig(sim_clock_config_t const *config);

#endif /* !MK64F_CLK_H */
