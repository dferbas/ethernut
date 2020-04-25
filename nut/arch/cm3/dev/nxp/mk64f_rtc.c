/*
 * Copyright (C) 2012 by Rob van Lieshout (info@pragmalab.nl)
 *                       Ole Reinhardt (ole.reinhardt@embedded-it.de)
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

#include <string.h>

#include <cfg/os.h>
#include <cfg/clock.h>
#include <cfg/arch.h>

#include <sys/heap.h>
#include <sys/event.h>
#include <sys/timer.h>

#include <dev/irqreg.h>
#include <dev/rtc.h>

#include <arch/cm3/nxp/mk64f_rtc.h>

#define SECONDS_IN_A_DAY (86400U)
#define SECONDS_IN_A_HOUR (3600U)
#define SECONDS_IN_A_MINUTE (60U)
#define DAYS_IN_A_YEAR (365U)
#define YEAR_RANGE_START (1970U)
#define YEAR_RANGE_END (2099U)
/*
 * typedef struct _mk64f_rtc_dcb mk64f_rtc_dcb;
 * struct _mk64f_rtc_dcb {
 *    uint32_t flags;
 * };
 */

static bool Mk64fCheckDatetimeFormat(const struct _tm *tm)
{
  uint8_t daysPerMonth[] = {0U, 31U, 28U, 31U, 30U, 31U, 30U, 31U, 31U, 30U, 31U, 30U, 31U};

  if ((tm->tm_year < YEAR_RANGE_START) || (tm->tm_year > YEAR_RANGE_END) || (tm->tm_mon > 12U) ||
    (tm->tm_mon < 1U) || (tm->tm_hour >= 24U) || (tm->tm_min >= 60U) || (tm->tm_sec >= 60U)) {
    return false;
  }

  if ((((tm->tm_year & 3U) == 0U) && (tm->tm_year % 100U != 0U)) || (tm->tm_year % 400U == 0U)) {
    daysPerMonth[2] = 29U;
  }

  if ((tm->tm_mday > daysPerMonth[tm->tm_mon]) || (tm->tm_mday < 1U)) {
    return false;
  }

  return true;
}

static uint32_t Mk64fConvertDatetimeToSeconds(const struct _tm *tm)
{
  /* Number of days from begin of the non Leap-year*/
  /* Number of days from begin of the non Leap-year*/
  uint16_t monthDays[] = {0U, 0U, 31U, 59U, 90U, 120U, 151U, 181U, 212U, 243U, 273U, 304U, 334U};
  uint32_t seconds;

  /* Compute number of days from 1970 till given year*/
  seconds = ((uint32_t)tm->tm_year - 1970U) * DAYS_IN_A_YEAR;
  /* Add leap year days */
  seconds += (((uint32_t)tm->tm_year / 4U) - (1970U / 4U));
  /* Add number of days till given month*/
  seconds += monthDays[tm->tm_mon];
  /* Add days in given month. We subtract the current day as it is
   * represented in the hours, minutes and seconds field*/
  seconds += ((uint32_t)tm->tm_mday - 1U);
  /* For leap year if month less than or equal to Febraury, decrement day counter*/
  if ((0U == (tm->tm_year & 3U)) && (tm->tm_mon <= 2U))
  {
    seconds--;
  }

  seconds = (seconds * SECONDS_IN_A_DAY) + ((uint32_t)tm->tm_hour * SECONDS_IN_A_HOUR) +
            ((uint32_t)tm->tm_min * SECONDS_IN_A_MINUTE) + tm->tm_sec;

  return seconds;
}

static void Mk64fConvertSecondsToDatetime(uint32_t seconds, struct _tm *tm)
{
  uint32_t x;
  uint32_t secondsRemaining, days;
  uint16_t daysInYear;
  /* Table of days in a month for a non leap year. First entry in the table is not used,
   * valid months start from 1
   */
  uint8_t daysPerMonth[] = {0U, 31U, 28U, 31U, 30U, 31U, 30U, 31U, 31U, 30U, 31U, 30U, 31U};
  memset(tm, 0, sizeof(*tm));

  /* Start with the seconds value that is passed in to be converted to date time format */
  secondsRemaining = seconds;

  /* Calcuate the number of days, we add 1 for the current day which is represented in the
   * hours and seconds field
   */
  days = secondsRemaining / SECONDS_IN_A_DAY + 1U;

  /* Update seconds left*/
  secondsRemaining = secondsRemaining % SECONDS_IN_A_DAY;

  /* Calculate the datetime hour, minute and second fields */
  tm->tm_hour   = (uint8_t)(secondsRemaining / SECONDS_IN_A_HOUR);
  secondsRemaining = secondsRemaining % SECONDS_IN_A_HOUR;
  tm->tm_min = (uint8_t)(secondsRemaining / 60U);
  tm->tm_sec = (uint8_t)(secondsRemaining % SECONDS_IN_A_MINUTE);

  /* Calculate year */
  daysInYear     = DAYS_IN_A_YEAR;
  tm->tm_year = YEAR_RANGE_START;
  while (days > daysInYear) {
    /* Decrease day count by a year and increment year by 1 */
    days -= daysInYear;
    tm->tm_year++;

    /* Adjust the number of days for a leap year */
    if (0U != (tm->tm_year & 3U)) {
      daysInYear = DAYS_IN_A_YEAR;
    } else {
      daysInYear = DAYS_IN_A_YEAR + 1U;
    }
  }

  /* Adjust the days in February for a leap year */
  if (0U == (tm->tm_year & 3U)) {
    daysPerMonth[2] = 29U;
  }

  for (x = 1U; x <= 12U; x++) {
    if (days <= daysPerMonth[x]) {
      tm->tm_mon = (uint8_t)x;
      break;
    } else {
      days -= daysPerMonth[x];
    }
  }

  tm->tm_mday = (uint8_t)days;
}


static void Mk64fRtcReset(NUTRTC *rtc)
{
  RTC_Type *base = RTC;
  base->CR |= RTC_CR_SWR_MASK;
  base->CR &= ~RTC_CR_SWR_MASK;
  base->TSR = 1U;
}

/*!
 * \brief Get date and time from an LPC 17xx hardware clock.
 *
 * \param tm Points to a structure that receives the date and time
 *           information.
 *
 * \return 0 on success or -1 in case of an error.
 */
static int Mk64fRtcGetClock(NUTRTC *rtc, struct _tm *tm)
{
  RTC_Type *base = RTC;
  if (tm) {
    uint32_t seconds = base->TSR;
    Mk64fConvertSecondsToDatetime(seconds, tm);
    return 0;
  } else {
    return -1;
  }
}

/*!
 * \brief Set the LPC 17xx hardware clock.
 *
 * \param tm Points to a structure which contains the date and time
 *           information.
 *
 * \return 0 on success or -1 in case of an error.
 */
static int Mk64fRtcSetClock(NUTRTC *rtc, const struct _tm *tm)
{
  RTC_Type *base = RTC;

  if (tm) {
    if (!(Mk64fCheckDatetimeFormat(tm))) {
      return -1;
    }

    /* Set time in seconds */
    base->TSR = Mk64fConvertDatetimeToSeconds(tm);
    return 0;
  } else {
    return -1;
  }
}

/*!
 * \brief Interrupt handler for RTC Alarm
 *
 */
static void Mk64fRtcInterrupt(void *arg)
{
  RTC_Type *base = RTC;
  NUTRTC *rtc = (NUTRTC *)arg;

  if (RTC_SR_TAF_MASK == (RTC_SR_TAF_MASK & base->SR)) {
    NutEventPostFromIrq(&rtc->alarm);
  }
}

/*!
 * \brief Set an alarm using the LPC 17xx hardware clock.
 *
 * \param Idx is ignored (LPC 17xx only has 1 Alarm)
 *           information.
 *
 * \param tm    Points to a structure which contains the date and time
 *              information. May be NULL to clear the alarm.
 * \param aflgs Each bit enables a specific comparision.
 *              - Bit 0: Seconds
 *              - Bit 1: Minutes
 *              - Bit 2: Hours
 *              - Bit 3: Day of month
 *              - Bit 4: Month
 *              - Bit 7: Day of week (Sunday is zero)
 *              - Bit 8: Year
 *              - Bit 9: Day of year
 *
 * \return 0 on success or -1 in case of an error.
 */
static int Mk64fRtcSetAlarm(NUTRTC *rtc, int idx, const struct _tm *tm, int aflags)
{
  RTC_Type *base = RTC;

  if (tm) {
    uint32_t alarmSeconds = 0;
    uint32_t currSeconds  = 0;


    if (!(Mk64fCheckDatetimeFormat(tm))) {
      return -1;
    }

    alarmSeconds = Mk64fConvertDatetimeToSeconds(tm);

    /* Get the current time */
    currSeconds = base->TSR;

    /* Return error if the alarm time has passed */
    if (alarmSeconds < currSeconds) {
        return -1;
    }

    /* Set alarm in seconds*/
    base->TAR = alarmSeconds;

    return 0;
  } else {
    return -1;
  }
}

/*!
 * \brief Get an alarm using the LPC17xx hardware clock.
 *
 * \param Idx is ignored (LPC17xx only has 1 Alarm)
 *           information.
 * \param tm Points to a structure which contains the date and time
 *           information.
 *
 * \param aflags Points to an unsigned long that receives the enable flags.
 *
 * \return 0 on success or -1 in case of an error.
 */
static int Mk64fRtcGetAlarm(NUTRTC *rtc, int idx, struct _tm *tm, int *aflags)
{
  RTC_Type *base = RTC;

  if (tm) {
    uint32_t alarmSeconds = 0;
    memset(tm, 0, sizeof(struct _tm));

    /* Get alarm in seconds  */
    alarmSeconds = base->TAR;

    Mk64fConvertSecondsToDatetime(alarmSeconds, tm);
  } else {
    return -1;
  }
}

/*!
 * \brief Get status of the LPC17xx hardware clock.
 *
 * \param sflags Points to an unsigned long that receives the status flags.
 *               - Bit 0: Power fail.
 *               - Bit 5: Alarm occured.
 * \return 0 on success or -1 in case of an error.
 */
static int Mk64fRtcGetStatus(NUTRTC *rtc, uint32_t *sflags)
{
  uint32_t flags = 0;
  RTC_Type *base = RTC;

  if (RTC_SR_TIF_MASK == (RTC_SR_TIF_MASK & base->SR)) {
    flags |= RTC_STATUS_INACCURATE;
  }
  if (RTC_SR_TOF_MASK == (RTC_SR_TOF_MASK & base->SR)) {
    flags |= RTC_STATUS_INACCURATE;
  }
  if (RTC_SR_TAF_MASK == (RTC_SR_TAF_MASK & base->SR)) {
    flags |= RTC_STATUS_AL0;
  }

  if (sflags) {
      *sflags = flags;
      return 0;
  } else {
      return -1;
  }
}

static int Mk64fRtcClearStatus(NUTRTC *rtc, uint32_t sflags)
{
  RTC_Type *base = RTC;

  if (sflags & RTC_STATUS_INACCURATE) {
    base->TSR = 1U;
  }

  if (sflags & RTC_STATUS_AL0) {
    base->TAR = 0U;
  }

  return 0;
}

static int Mk64fRtcInit(NUTRTC *rtc)
{
  bool wakeupSelect = false,
       updateMode = false,
       supervisorAccess = false;

  uint32_t compensationInterval = 0,
           compensationTime = 0;

  RTC_Type *base = RTC;
  uint32_t reg = 0;

  CLOCK_EnableClock(kCLOCK_Rtc0);

  /* Issue a software reset if timer is invalid */
  Mk64fRtcGetStatus(rtc, &reg);
  if (reg & RTC_STATUS_INACCURATE) {
    Mk64fRtcReset(rtc);
  }

  reg = base->CR;
  /* Setup the update mode and supervisor access mode */
  reg &= ~(RTC_CR_UM_MASK | RTC_CR_SUP_MASK);
  reg |= RTC_CR_UM(updateMode) | RTC_CR_SUP(supervisorAccess);

  reg &= ~(RTC_CR_WPS_MASK);
  reg |= RTC_CR_WPS(wakeupSelect);

  base->CR = reg;

  /* Configure the RTC time compensation register */
  base->TCR = (RTC_TCR_CIR(compensationInterval) | RTC_TCR_TCR(compensationTime));

  if (NutRegisterIrqHandler(&sig_RTC, Mk64fRtcInterrupt, rtc) != 0) {
    return -1;
  }

  // enable alarm interrupt
  base->IER |= RTC_IER_TAIE_MASK;

  /* enable RTC (run) */
  rtc->alarm = NULL;

  NutIrqEnable(&sig_RTC);

  /* Enable the RTC 32KHz oscillator */
  base->CR |= RTC_CR_OSCE_MASK;

  return(0);
}

NUTRTC rtcMk64f  = {
  /*.dcb           = */ NULL,                   /*!< Driver control block */
  /*.rtc_init      = */ Mk64fRtcInit,         /*!< Hardware initialization, rtc_init */
  /*.rtc_gettime   = */ Mk64fRtcGetClock,     /*!< Read date and time, rtc_gettime */
  /*.rtc_settime   = */ Mk64fRtcSetClock,     /*!< Set date and time, rtc_settime */
  /*.rtc_getalarm  = */ Mk64fRtcGetAlarm,     /*!< Read alarm date and time, rtc_getalarm */
  /*.rtc_setalarm  = */ Mk64fRtcSetAlarm,     /*!< Set alarm date and time, rtc_setalarm */
  /*.rtc_getstatus = */ Mk64fRtcGetStatus,    /*!< Read status flags, rtc_getstatus */
  /*.rtc_clrstatus = */ Mk64fRtcClearStatus,  /*!< Clear status flags, rtc_clrstatus */
  /*.alarm         = */ NULL,                   /*!< Handle for alarm event queue */
};

