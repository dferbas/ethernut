# ethernut
Kinetis port of Ethernut (Nut/OS)

Idea is to use existing code for Context switching (used for STM32),
Freescale (NXP) SoC peripheral drivers (MCU Xpresso SDK) and add Nut/OS driver "wrappers",
reuse chip drivers used in an application running on MCF52259 cpu (mk68, Coldfire).

Currently found differences:
1) Kinetis needs flash security settings, located at 0x400, 0x10 long.
Without this, an application cannot even start, because it cannot read flash address 0 (SP value).
