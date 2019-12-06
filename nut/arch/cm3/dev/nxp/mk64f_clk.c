#include <arch/cm3/nxp/vendor/MK64F12.h>
#include <arch/cm3/nxp/vendor/MK64F12_features.h>
#include <arch/cm3/nxp/mk64f_clk.h>

/* PLL fixed multiplier when there is not PRDIV and VDIV. */
#define PLL_FIXED_MULT (375U)
/* Max frequency of the reference clock used for internal clock trim. */
#define TRIM_REF_CLK_MIN (8000000U)
/* Min frequency of the reference clock used for internal clock trim. */
#define TRIM_REF_CLK_MAX (16000000U)
/* Max trim value of fast internal reference clock. */
#define TRIM_FIRC_MAX (5000000U)
/* Min trim value of fast internal reference clock. */
#define TRIM_FIRC_MIN (3000000U)
/* Max trim value of fast internal reference clock. */
#define TRIM_SIRC_MAX (39063U)
/* Min trim value of fast internal reference clock. */
#define TRIM_SIRC_MIN (31250U)

#define MCG_S_IRCST_VAL ((MCG->S & MCG_S_IRCST_MASK) >> MCG_S_IRCST_SHIFT)
#define MCG_S_CLKST_VAL ((MCG->S & MCG_S_CLKST_MASK) >> MCG_S_CLKST_SHIFT)
#define MCG_S_IREFST_VAL ((MCG->S & MCG_S_IREFST_MASK) >> MCG_S_IREFST_SHIFT)
#define MCG_S_PLLST_VAL ((MCG->S & MCG_S_PLLST_MASK) >> MCG_S_PLLST_SHIFT)
#define MCG_C1_FRDIV_VAL ((MCG->C1 & MCG_C1_FRDIV_MASK) >> MCG_C1_FRDIV_SHIFT)
#define MCG_C2_LP_VAL ((MCG->C2 & MCG_C2_LP_MASK) >> MCG_C2_LP_SHIFT)
#define MCG_C2_RANGE_VAL ((MCG->C2 & MCG_C2_RANGE_MASK) >> MCG_C2_RANGE_SHIFT)
#define MCG_SC_FCRDIV_VAL ((MCG->SC & MCG_SC_FCRDIV_MASK) >> MCG_SC_FCRDIV_SHIFT)
#define MCG_S2_PLLCST_VAL ((MCG->S2 & MCG_S2_PLLCST_MASK) >> MCG_S2_PLLCST_SHIFT)
#define MCG_C7_OSCSEL_VAL ((MCG->C7 & MCG_C7_OSCSEL_MASK) >> MCG_C7_OSCSEL_SHIFT)
#define MCG_C4_DMX32_VAL ((MCG->C4 & MCG_C4_DMX32_MASK) >> MCG_C4_DMX32_SHIFT)
#define MCG_C4_DRST_DRS_VAL ((MCG->C4 & MCG_C4_DRST_DRS_MASK) >> MCG_C4_DRST_DRS_SHIFT)
#define MCG_C7_PLL32KREFSEL_VAL ((MCG->C7 & MCG_C7_PLL32KREFSEL_MASK) >> MCG_C7_PLL32KREFSEL_SHIFT)
#define MCG_C5_PLLREFSEL0_VAL ((MCG->C5 & MCG_C5_PLLREFSEL0_MASK) >> MCG_C5_PLLREFSEL0_SHIFT)
#define MCG_C11_PLLREFSEL1_VAL ((MCG->C11 & MCG_C11_PLLREFSEL1_MASK) >> MCG_C11_PLLREFSEL1_SHIFT)
#define MCG_C11_PRDIV1_VAL ((MCG->C11 & MCG_C11_PRDIV1_MASK) >> MCG_C11_PRDIV1_SHIFT)
#define MCG_C12_VDIV1_VAL ((MCG->C12 & MCG_C12_VDIV1_MASK) >> MCG_C12_VDIV1_SHIFT)
#define MCG_C5_PRDIV0_VAL ((MCG->C5 & MCG_C5_PRDIV0_MASK) >> MCG_C5_PRDIV0_SHIFT)
#define MCG_C6_VDIV0_VAL ((MCG->C6 & MCG_C6_VDIV0_MASK) >> MCG_C6_VDIV0_SHIFT)

#define OSC_MODE_MASK (MCG_C2_EREFS0_MASK | MCG_C2_HGO0_MASK | MCG_C2_RANGE0_MASK)

#define SIM_CLKDIV1_OUTDIV1_VAL ((SIM->CLKDIV1 & SIM_CLKDIV1_OUTDIV1_MASK) >> SIM_CLKDIV1_OUTDIV1_SHIFT)
#define SIM_CLKDIV1_OUTDIV2_VAL ((SIM->CLKDIV1 & SIM_CLKDIV1_OUTDIV2_MASK) >> SIM_CLKDIV1_OUTDIV2_SHIFT)
#define SIM_CLKDIV1_OUTDIV3_VAL ((SIM->CLKDIV1 & SIM_CLKDIV1_OUTDIV3_MASK) >> SIM_CLKDIV1_OUTDIV3_SHIFT)
#define SIM_CLKDIV1_OUTDIV4_VAL ((SIM->CLKDIV1 & SIM_CLKDIV1_OUTDIV4_MASK) >> SIM_CLKDIV1_OUTDIV4_SHIFT)
#define SIM_SOPT1_OSC32KSEL_VAL ((SIM->SOPT1 & SIM_SOPT1_OSC32KSEL_MASK) >> SIM_SOPT1_OSC32KSEL_SHIFT)
#define SIM_SOPT2_PLLFLLSEL_VAL ((SIM->SOPT2 & SIM_SOPT2_PLLFLLSEL_MASK) >> SIM_SOPT2_PLLFLLSEL_SHIFT)

#define MCG_PLL_DISABLE                                   0U  /*!< MCGPLLCLK disabled */
#define OSC_CAP0P                                         0U  /*!< Oscillator 0pF capacitor load */
#define OSC_ER_CLK_DISABLE                                0U  /*!< Disable external reference clock */
#define SIM_OSC32KSEL_RTC32KCLK_CLK                       2U  /*!< OSC32KSEL select: RTC32KCLK clock (32.768kHz) */
#define SIM_PLLFLLSEL_IRC48MCLK_CLK                       3U  /*!< PLLFLL select: IRC48MCLK clock */
#define SIM_PLLFLLSEL_MCGPLLCLK_CLK                       1U  /*!< PLLFLL select: MCGPLLCLK clock */

/* MCG_S_CLKST definition. */
enum _mcg_clkout_stat
{
		kMCG_ClkOutStatFll, /* FLL.            */
		kMCG_ClkOutStatInt, /* Internal clock. */
		kMCG_ClkOutStatExt, /* External clock. */
		kMCG_ClkOutStatPll  /* PLL.            */
};

/* MCG_S_PLLST definition. */
enum _mcg_pllst
{
		kMCG_PllstFll, /* FLL is used. */
		kMCG_PllstPll  /* PLL is used. */
};

/* Slow internal reference clock frequency. */
static unsigned int s_slowIrcFreq = 32768U;
/* Fast internal reference clock frequency. */
static unsigned int s_fastIrcFreq = 4000000U;

/* External XTAL0 (OSC0) clock frequency. */
unsigned int g_xtal0Freq;
/* External XTAL32K clock frequency. */
unsigned int g_xtal32Freq;

static unsigned int CLOCK_GetMcgExtClkFreq(void)
{
		unsigned int freq;

		switch (MCG_C7_OSCSEL_VAL)
		{
				case 0U:
						/* Please call CLOCK_SetXtal0Freq base on board setting before using OSC0 clock. */
						// assert(g_xtal0Freq);
						freq = g_xtal0Freq;
						break;
				case 1U:
						/* Please call CLOCK_SetXtal32Freq base on board setting before using XTAL32K/RTC_CLKIN clock. */
						// assert(g_xtal32Freq);
						freq = g_xtal32Freq;
						break;
				case 2U:
						freq = MCG_INTERNAL_IRC_48M;
						break;
				default:
						freq = 0U;
						break;
		}

		return freq;
}

static unsigned int CLOCK_GetFllExtRefClkFreq(void)
{
		/* FllExtRef = McgExtRef / FllExtRefDiv */
		unsigned char frdiv;
		unsigned char range;
		unsigned char oscsel;

		unsigned int freq = CLOCK_GetMcgExtClkFreq();

		if (!freq)
		{
				return freq;
		}

		frdiv = MCG_C1_FRDIV_VAL;
		freq >>= frdiv;

		range = MCG_C2_RANGE_VAL;
		oscsel = MCG_C7_OSCSEL_VAL;

		/*
			 When should use divider 32, 64, 128, 256, 512, 1024, 1280, 1536.
			 1. MCG_C7[OSCSEL] selects IRC48M.
			 2. MCG_C7[OSCSEL] selects OSC0 and MCG_C2[RANGE] is not 0.
		*/
		if (((0U != range) && (kMCG_OscselOsc == oscsel)) || (kMCG_OscselIrc == oscsel))
		{
				switch (frdiv)
				{
						case 0:
						case 1:
						case 2:
						case 3:
						case 4:
						case 5:
								freq >>= 5u;
								break;
						case 6:
								/* 64*20=1280 */
								freq /= 20u;
								break;
						case 7:
								/* 128*12=1536 */
								freq /= 12u;
								break;
						default:
								freq = 0u;
								break;
				}
		}

		return freq;
}

static unsigned int CLOCK_GetInternalRefClkSelectFreq(void)
{
		if (kMCG_IrcSlow == MCG_S_IRCST_VAL)
		{
				/* Slow internal reference clock selected*/
				return s_slowIrcFreq;
		}
		else
		{
				/* Fast internal reference clock selected*/
				return s_fastIrcFreq >> MCG_SC_FCRDIV_VAL;
		}
}

static unsigned int CLOCK_GetFllRefClkFreq(void)
{
		/* If use external reference clock. */
		if (kMCG_FllSrcExternal == MCG_S_IREFST_VAL)
		{
				return CLOCK_GetFllExtRefClkFreq();
		}
		/* If use internal reference clock. */
		else
		{
				return s_slowIrcFreq;
		}
}

static unsigned int CLOCK_GetPll0RefFreq(void)
{
		/* MCG external reference clock. */
		return CLOCK_GetMcgExtClkFreq();
}


static unsigned char CLOCK_GetOscRangeFromFreq(unsigned int freq)
{
		unsigned char range;

		if (freq <= 39063U)
		{
				range = 0U;
		}
		else if (freq <= 8000000U)
		{
				range = 1U;
		}
		else
		{
				range = 2U;
		}

		return range;
}


unsigned int CLOCK_GetPll0Freq(void)
{
		unsigned int mcgpll0clk;

		/* If PLL0 is not enabled, return 0. */
		if (!(MCG->S & MCG_S_LOCK0_MASK))
		{
				return 0U;
		}

		mcgpll0clk = CLOCK_GetPll0RefFreq();

		/*
		 * Please call CLOCK_SetXtal0Freq base on board setting before using OSC0 clock.
		 * Please call CLOCK_SetXtal1Freq base on board setting before using OSC1 clock.
		 */
		// assert(mcgpll0clk);

		mcgpll0clk /= (FSL_FEATURE_MCG_PLL_PRDIV_BASE + MCG_C5_PRDIV0_VAL);
		mcgpll0clk *= (FSL_FEATURE_MCG_PLL_VDIV_BASE + MCG_C6_VDIV0_VAL);

		return mcgpll0clk;
}

unsigned int CLOCK_GetFllFreq(void)
{
		static const uint16_t fllFactorTable[4][2] = {{640, 732}, {1280, 1464}, {1920, 2197}, {2560, 2929}};

		unsigned char drs, dmx32;
		unsigned int freq;

		/* If FLL is not enabled currently, then return 0U. */
		if ((MCG->C2 & MCG_C2_LP_MASK) || (MCG->S & MCG_S_PLLST_MASK))
		{
				return 0U;
		}

		/* Get FLL reference clock frequency. */
		freq = CLOCK_GetFllRefClkFreq();
		if (!freq)
		{
				return freq;
		}

		drs = MCG_C4_DRST_DRS_VAL;
		dmx32 = MCG_C4_DMX32_VAL;

		return freq * fllFactorTable[drs][dmx32];
}

unsigned int CLOCK_GetOutClkFreq(void)
{
		unsigned int mcgoutclk;
		unsigned int clkst = MCG_S_CLKST_VAL;

		switch (clkst)
		{
				case kMCG_ClkOutStatPll:
						mcgoutclk = CLOCK_GetPll0Freq();
						break;
				case kMCG_ClkOutStatFll:
						mcgoutclk = CLOCK_GetFllFreq();
						break;
				case kMCG_ClkOutStatInt:
						mcgoutclk = CLOCK_GetInternalRefClkSelectFreq();
						break;
				case kMCG_ClkOutStatExt:
						mcgoutclk = CLOCK_GetMcgExtClkFreq();
						break;
				default:
						mcgoutclk = 0U;
						break;
		}
		return mcgoutclk;
}

unsigned int CLOCK_GetOsc0ErClkFreq(void)
{
		if (OSC0->CR & OSC_CR_ERCLKEN_MASK)
		{
				/* Please call CLOCK_SetXtal0Freq base on board setting before using OSC0 clock. */
				// assert(g_xtal0Freq);
				return g_xtal0Freq;
		}
		else
		{
				return 0U;
		}
}

unsigned int CLOCK_GetEr32kClkFreq(void)
{
		unsigned int freq;

		switch (SIM_SOPT1_OSC32KSEL_VAL)
		{
				case 0U: /* OSC 32k clock  */
						freq = (CLOCK_GetOsc0ErClkFreq() == 32768U) ? 32768U : 0U;
						break;
				case 2U: /* RTC 32k clock  */
						/* Please call CLOCK_SetXtal32Freq base on board setting before using XTAL32K/RTC_CLKIN clock. */
						// assert(g_xtal32Freq);
						freq = g_xtal32Freq;
						break;
				case 3U: /* LPO clock      */
						freq = LPO_CLK_FREQ;
						break;
				default:
						freq = 0U;
						break;
		}
		return freq;
}

unsigned int CLOCK_GetPllFllSelClkFreq(void)
{
		unsigned int freq;

		switch (SIM_SOPT2_PLLFLLSEL_VAL)
		{
				case 0U: /* FLL. */
						freq = CLOCK_GetFllFreq();
						break;
				case 1U: /* PLL. */
						freq = CLOCK_GetPll0Freq();
						break;
				case 3U: /* MCG IRC48M. */
						freq = MCG_INTERNAL_IRC_48M;
						break;
				default:
						freq = 0U;
						break;
		}

		return freq;
}

unsigned int CLOCK_GetPlatClkFreq(void)
{
		return CLOCK_GetOutClkFreq() / (SIM_CLKDIV1_OUTDIV1_VAL + 1);
}

unsigned int CLOCK_GetFlashClkFreq(void)
{
		return CLOCK_GetOutClkFreq() / (SIM_CLKDIV1_OUTDIV4_VAL + 1);
}

unsigned int CLOCK_GetFlexBusClkFreq(void)
{
		return CLOCK_GetOutClkFreq() / (SIM_CLKDIV1_OUTDIV3_VAL + 1);
}

unsigned int CLOCK_GetBusClkFreq(void)
{
		return CLOCK_GetOutClkFreq() / (SIM_CLKDIV1_OUTDIV2_VAL + 1);
}

unsigned int CLOCK_GetCoreSysClkFreq(void)
{
		return CLOCK_GetOutClkFreq() / (SIM_CLKDIV1_OUTDIV1_VAL + 1);
}

unsigned int CLOCK_GetInternalRefClkFreq(void)
{
		/* If MCGIRCLK is gated. */
		if (!(MCG->C1 & MCG_C1_IRCLKEN_MASK))
		{
				return 0U;
		}

		return CLOCK_GetInternalRefClkSelectFreq();
}

unsigned int CLOCK_GetFixedFreqClkFreq(void)
{
		unsigned int freq = CLOCK_GetFllRefClkFreq();

		/* MCGFFCLK must be no more than MCGOUTCLK/8. */
		if ((freq) && (freq <= (CLOCK_GetOutClkFreq() / 8U)))
		{
				return freq;
		}
		else
		{
				return 0U;
		}
}

unsigned int MK64F_ClockGet(clock_index_t idx)
{
		unsigned int freq;

		switch (idx)
		{
				case kCLOCK_CoreSysClk:
				case kCLOCK_PlatClk:
						freq = CLOCK_GetOutClkFreq() / (SIM_CLKDIV1_OUTDIV1_VAL + 1);
						break;
				case kCLOCK_BusClk:
						freq = CLOCK_GetOutClkFreq() / (SIM_CLKDIV1_OUTDIV2_VAL + 1);
						break;
				case kCLOCK_FlexBusClk:
						freq = CLOCK_GetOutClkFreq() / (SIM_CLKDIV1_OUTDIV3_VAL + 1);
						break;
				case kCLOCK_FlashClk:
						freq = CLOCK_GetOutClkFreq() / (SIM_CLKDIV1_OUTDIV4_VAL + 1);
						break;
				case kCLOCK_PllFllSelClk:
						freq = CLOCK_GetPllFllSelClkFreq();
						break;
				case kCLOCK_Er32kClk:
						freq = CLOCK_GetEr32kClkFreq();
						break;
				case kCLOCK_Osc0ErClk:
						freq = CLOCK_GetOsc0ErClkFreq();
						break;
				case kCLOCK_McgFixedFreqClk:
						freq = CLOCK_GetFixedFreqClkFreq();
						break;
				case kCLOCK_McgInternalRefClk:
						freq = CLOCK_GetInternalRefClkFreq();
						break;
				case kCLOCK_McgFllClk:
						freq = CLOCK_GetFllFreq();
						break;
				case kCLOCK_McgPll0Clk:
						freq = CLOCK_GetPll0Freq();
						break;
				case kCLOCK_McgIrc48MClk:
						freq = MCG_INTERNAL_IRC_48M;
						break;
				case kCLOCK_LpoClk:
						freq = LPO_CLK_FREQ;
						break;
				default:
						freq = 0U;
						break;
		}

		return freq;
}

int CLOCK_SetExternalRefClkConfig(mcg_oscsel_t oscsel)
{
    int needDelay;
    unsigned int i;

    if (MCG_C7_OSCSEL_VAL != oscsel)
    {
        /* If change OSCSEL, need to delay, ERR009878. */
        needDelay = 1;
    }
    else
    {
        needDelay = 0;
    }

    MCG->C7 = (MCG->C7 & ~MCG_C7_OSCSEL_MASK) | MCG_C7_OSCSEL(oscsel);
    if (needDelay)
    {
        /* ERR009878 Delay at least 50 micro-seconds for external clock change valid. */
        i = 1500U;
        while (i--)
        {
            __NOP();
        }
    }

    return 0;
}

void CLOCK_EnablePll0(mcg_pll_config_t const *config)
{
    // assert(config);

    uint8_t mcg_c5 = 0U;

    mcg_c5 |= MCG_C5_PRDIV0(config->prdiv);
    MCG->C5 = mcg_c5; /* Disable the PLL first. */

    MCG->C6 = (MCG->C6 & ~MCG_C6_VDIV0_MASK) | MCG_C6_VDIV0(config->vdiv);

    /* Set enable mode. */
    MCG->C5 |= ((unsigned int)kMCG_PllEnableIndependent | (unsigned int)config->enableMode);

    /* Wait for PLL lock. */
    while (!(MCG->S & MCG_S_LOCK0_MASK))
    {
    }
}

int CLOCK_SetPbeMode(mcg_pll_clk_select_t pllcs, mcg_pll_config_t const *config)
{
    // assert(config);

    /*
       This function is designed to change MCG to PBE mode from PEE/BLPE/FBE,
       but with this workflow, the source mode could be all modes except PEI/PBI.
     */
    MCG->C2 &= ~MCG_C2_LP_MASK; /* Disable lowpower. */

    /* Change to use external clock first. */
    MCG->C1 = ((MCG->C1 & ~(MCG_C1_CLKS_MASK | MCG_C1_IREFS_MASK)) | MCG_C1_CLKS(kMCG_ClkOutSrcExternal));

    /* Wait for CLKST clock status bits to show clock source is ext ref clk */
    while ((MCG->S & (MCG_S_IREFST_MASK | MCG_S_CLKST_MASK)) !=
           (MCG_S_IREFST(kMCG_FllSrcExternal) | MCG_S_CLKST(kMCG_ClkOutStatExt)))
    {
    }

    /* Disable PLL first, then configure PLL. */
    MCG->C6 &= ~MCG_C6_PLLS_MASK;
    while (MCG->S & MCG_S_PLLST_MASK)
    {
    }

    /* Configure the PLL. */
    {
        CLOCK_EnablePll0(config);
    }

    /* Change to PLL mode. */
    MCG->C6 |= MCG_C6_PLLS_MASK;

    /* Wait for PLL mode changed. */
    while (!(MCG->S & MCG_S_PLLST_MASK))
    {
    }

    return 0;
}

static void CLOCK_CONFIG_SetFllExtRefDiv(uint8_t frdiv)
{
    MCG->C1 = ((MCG->C1 & ~MCG_C1_FRDIV_MASK) | MCG_C1_FRDIV(frdiv));
}

int CLOCK_BootToPeeMode(mcg_oscsel_t oscsel, mcg_pll_clk_select_t pllcs, mcg_pll_config_t const *config)
{
    // assert(config);

    CLOCK_SetExternalRefClkConfig(oscsel);

    CLOCK_SetPbeMode(pllcs, config);

    /* Change to use PLL output clock. */
    MCG->C1 = (MCG->C1 & ~MCG_C1_CLKS_MASK) | MCG_C1_CLKS(kMCG_ClkOutSrcOut);
    while (MCG_S_CLKST_VAL != kMCG_ClkOutStatPll)
    {
    }

    return 0;
}

const mcg_config_t mcgConfig_BOARD_BootClockRUN =
    {
        .mcgMode = kMCG_ModePEE,                  /* PEE - PLL Engaged External */
        .irclkEnableMode = kMCG_IrclkEnable,      /* MCGIRCLK enabled, MCGIRCLK disabled in STOP mode */
        .ircs = kMCG_IrcSlow,                     /* Slow internal reference clock selected */
        .fcrdiv = 0x0U,                           /* Fast IRC divider: divided by 1 */
        .frdiv = 0x0U,                            /* FLL reference clock divider: divided by 32 */
        .drs = kMCG_DrsLow,                       /* Low frequency range */
        .dmx32 = kMCG_Dmx32Default,               /* DCO has a default range of 25% */
        .oscsel = kMCG_OscselOsc,                 /* Selects System Oscillator (OSCCLK) */
        .pll0Config =
            {
                .enableMode = MCG_PLL_DISABLE,    /* MCGPLLCLK disabled */
                .prdiv = 0x13U,                   /* PLL Reference divider: divided by 20 */
                .vdiv = 0x18U,                    /* VCO divider: multiplied by 48 */
            },
    };
const sim_clock_config_t simConfig_BOARD_BootClockRUN =
    {
        .pllFllSel = SIM_PLLFLLSEL_MCGPLLCLK_CLK, /* PLLFLL select: MCGPLLCLK clock */
        .er32kSrc = SIM_OSC32KSEL_RTC32KCLK_CLK,  /* OSC32KSEL select: RTC32KCLK clock (32.768kHz) */
        .clkdiv1 = 0x1240000U,                    /* SIM_CLKDIV1 - OUTDIV1: /1, OUTDIV2: /2, OUTDIV3: /3, OUTDIV4: /5 */
    };
const osc_config_t oscConfig_BOARD_BootClockRUN =
    {
        .freq = 50000000U,                        /* Oscillator frequency: 50000000Hz */
        .capLoad = (OSC_CAP0P),                   /* Oscillator capacity load: 0pF */
        .workMode = kOSC_ModeExt,                 /* Use external clock */
        .oscerConfig =
            {
                .enableMode = kOSC_ErClkEnable,   /* Enable external reference clock, disable external reference clock in STOP mode */
            }
    };

int SetSysClock(void)
{
	/* Set the system clock dividers in SIM to safe value. */
	CLOCK_SetSimSafeDivs();
	/* Initializes OSC0 according to board configuration. */
	CLOCK_InitOsc0(&oscConfig_BOARD_BootClockRUN);
	CLOCK_SetXtal0Freq(oscConfig_BOARD_BootClockRUN.freq);
	/* Configure the Internal Reference clock (MCGIRCLK). */
	CLOCK_SetInternalRefClkConfig(mcgConfig_BOARD_BootClockRUN.irclkEnableMode,
																mcgConfig_BOARD_BootClockRUN.ircs, 
																mcgConfig_BOARD_BootClockRUN.fcrdiv);
	/* Configure FLL external reference divider (FRDIV). */
	CLOCK_CONFIG_SetFllExtRefDiv(mcgConfig_BOARD_BootClockRUN.frdiv);
	/* Set MCG to PEE mode. */
	CLOCK_BootToPeeMode(mcgConfig_BOARD_BootClockRUN.oscsel,
											kMCG_PllClkSelPll0,
											&mcgConfig_BOARD_BootClockRUN.pll0Config);
	/* Set the clock configuration in SIM module. */
	CLOCK_SetSimConfig(&simConfig_BOARD_BootClockRUN);
	/* Set SystemCoreClock variable. */
	SystemCoreClock = BOARD_BOOTCLOCKRUN_CORE_CLOCK;

	return 0;
}

void CLOCK_InitOsc0(osc_config_t const *config)
{
    unsigned char range = CLOCK_GetOscRangeFromFreq(config->freq);

    OSC_SetCapLoad(OSC0, config->capLoad);
    OSC_SetExtRefClkConfig(OSC0, &config->oscerConfig);

    MCG->C2 = ((MCG->C2 & ~OSC_MODE_MASK) | MCG_C2_RANGE(range) | (unsigned char)config->workMode);

    if ((kOSC_ModeExt != config->workMode) && (OSC0->CR & OSC_CR_ERCLKEN_MASK))
    {
        /* Wait for stable. */
        while (!(MCG->S & MCG_S_OSCINIT0_MASK))
        {
        }
    }
}

int CLOCK_SetInternalRefClkConfig(unsigned char enableMode, mcg_irc_mode_t ircs, unsigned char fcrdiv)
{
    unsigned int mcgOutClkState = MCG_S_CLKST_VAL;
    mcg_irc_mode_t curIrcs = (mcg_irc_mode_t)MCG_S_IRCST_VAL;
    unsigned char curFcrdiv = MCG_SC_FCRDIV_VAL;

    /* If need to update the FCRDIV. */
    if (fcrdiv != curFcrdiv)
    {
        /* If fast IRC is in use currently, change to slow IRC. */
        if ((kMCG_IrcFast == curIrcs) && ((mcgOutClkState == kMCG_ClkOutStatInt) || (MCG->C1 & MCG_C1_IRCLKEN_MASK)))
        {
            MCG->C2 = ((MCG->C2 & ~MCG_C2_IRCS_MASK) | (MCG_C2_IRCS(kMCG_IrcSlow)));
            while (MCG_S_IRCST_VAL != kMCG_IrcSlow)
            {
            }
        }
        /* Update FCRDIV. */
        MCG->SC = (MCG->SC & ~(MCG_SC_FCRDIV_MASK | MCG_SC_ATMF_MASK | MCG_SC_LOCS0_MASK)) | MCG_SC_FCRDIV(fcrdiv);
    }

    /* Set internal reference clock selection. */
    MCG->C2 = (MCG->C2 & ~MCG_C2_IRCS_MASK) | (MCG_C2_IRCS(ircs));
    MCG->C1 = (MCG->C1 & ~(MCG_C1_IRCLKEN_MASK | MCG_C1_IREFSTEN_MASK)) | (unsigned char)enableMode;

    /* If MCGIRCLK is used, need to wait for MCG_S_IRCST. */
    if ((mcgOutClkState == kMCG_ClkOutStatInt) || (enableMode & kMCG_IrclkEnable))
    {
        while (MCG_S_IRCST_VAL != ircs)
        {
        }
    }

    return 0;
}

void CLOCK_SetSimConfig(sim_clock_config_t const *config)
{
    SIM->CLKDIV1 = config->clkdiv1;
    CLOCK_SetPllFllSelClock(config->pllFllSel);
    CLOCK_SetEr32kClock(config->er32kSrc);
}
