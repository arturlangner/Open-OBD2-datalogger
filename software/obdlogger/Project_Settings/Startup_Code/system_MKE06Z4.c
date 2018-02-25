/*
** ###################################################################
**     Compilers:           ARM Compiler
**                          Freescale C/C++ for Embedded ARM
**                          GNU C Compiler
**                          GNU C Compiler - CodeSourcery Sourcery G++
**                          IAR ANSI C/C++ Compiler for ARM
**
**     Reference manual:    MKE06P80M48SF0RM, Rev. 1, Dec 2013
**     Version:             rev. 1.4, 2014-05-28
**     Build:               b140528
**
**     Abstract:
**         Provides a system configuration function and a global variable that
**         contains the system frequency. It configures the device and initializes
**         the oscillator (PLL) that is part of the microcontroller device.
**
**     Copyright (c) 2014 Freescale Semiconductor, Inc.
**     All rights reserved.
**
**     Redistribution and use in source and binary forms, with or without modification,
**     are permitted provided that the following conditions are met:
**
**     o Redistributions of source code must retain the above copyright notice, this list
**       of conditions and the following disclaimer.
**
**     o Redistributions in binary form must reproduce the above copyright notice, this
**       list of conditions and the following disclaimer in the documentation and/or
**       other materials provided with the distribution.
**
**     o Neither the name of Freescale Semiconductor, Inc. nor the names of its
**       contributors may be used to endorse or promote products derived from this
**       software without specific prior written permission.
**
**     THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
**     ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
**     WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
**     DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
**     ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
**     (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
**     LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
**     ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
**     (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
**     SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
**
**     http:                 www.freescale.com
**     mail:                 support@freescale.com
**
**     Revisions:
**     - rev. 1.0 (2013-07-30)
**         Initial version.
**     - rev. 1.1 (2013-10-29)
**         Definition of BITBAND macros updated to support peripherals with 32-bit acces disabled.
**     - rev. 1.2 (2013-10-30)
**         Update according to reference manual rev. 1.
**     - rev. 1.3 (2014-01-10)
**         CAN module: corrected address of TSIDR1 register.
**         CAN module: corrected name of MSCAN_TDLR bit DLC to TDLC.
**         FTM0 module: added access macro for EXTTRIG register.
**         NVIC module: registers access macros improved.
**         SCB module: unused bits removed, mask, shift macros improved.
**         Defines of interrupt vectors aligned to RM.
**     - rev. 1.4 (2014-05-28)
**         The declaration of clock configurations has been moved to separate header file system_MKE02Z2.h
**         Module access macro {module}_BASES replaced by {module}_BASE_PTRS.
**         I2C module: renamed status register S to S1 to match RM naming.
**
** ###################################################################
*/

/*!
 * @file MKE06Z4
 * @version 1.4
 * @date 2014-05-28
 * @brief Device specific configuration file for MKE06Z4 (implementation file)
 *
 * Provides a system configuration function and a global variable that contains
 * the system frequency. It configures the device and initializes the oscillator
 * (PLL) that is part of the microcontroller device.
 */

#include <stdint.h>
#include "MKE06Z4.h"



/* ----------------------------------------------------------------------------
   -- Core clock
   ---------------------------------------------------------------------------- */

uint32_t SystemCoreClock = DEFAULT_SYSTEM_CLOCK;

/* ----------------------------------------------------------------------------
   -- SystemInit()
   ---------------------------------------------------------------------------- */

void SystemInit (void) {

#if (DISABLE_WDOG)
  /* WDOG->TOVAL: TOVAL=0xE803 */
  WDOG->TOVAL = WDOG_TOVAL_TOVAL(0xE803); /* Timeout value */
  /* WDOG->CS2: WIN=0,FLG=0,??=0,PRES=0,??=0,??=0,CLK=1 */
  WDOG->CS2 = WDOG_CS2_CLK(0x01);       /* 1-kHz clock source */
  /* WDOG->CS1: EN=0,INT=0,UPDATE=1,TST=0,DBG=0,WAIT=1,STOP=1 */
  WDOG->CS1 = WDOG_CS1_UPDATE_MASK |
             WDOG_CS1_TST(0x00) |
             WDOG_CS1_WAIT_MASK |
             WDOG_CS1_STOP_MASK;

#endif /* (DISABLE_WDOG) */
#if (CLOCK_SETUP == 0)
  /* ICS->C2: BDIV|=1 */
  ICS->C2 |= ICS_C2_BDIV(0x01);         /* Update system prescalers */
  /* SIM->CLKDIV: ??=0,??=0,OUTDIV1=0,??=0,??=0,??=0,OUTDIV2=0,??=0,??=0,??=0,OUTDIV3=0,??=0,??=0,??=0,??=0,??=0,??=0,??=0,??=0,??=0,??=0,??=0,??=0,??=0,??=0,??=0,??=0,??=0,??=0,??=0,??=0 */
  SIM->CLKDIV = SIM_CLKDIV_OUTDIV1(0x00); /* Update system prescalers */
  /* Switch to FEI Mode */
  /* ICS_C1: CLKS=0,RDIV=0,IREFS=1,IRCLKEN=1,IREFSTEN=0 */
  ICS->C1 = ICS_C1_CLKS(0x00) |
           ICS_C1_RDIV(0x00) |
           ICS_C1_IREFS_MASK |
           ICS_C1_IRCLKEN_MASK;
  /* ICS->C2: BDIV=1,LP=0 */
  ICS->C2 = (uint8_t)((ICS->C2 & (uint8_t)~(uint8_t)(
            ICS_C2_BDIV(0x06) |
            ICS_C2_LP_MASK
           )) | (uint8_t)(
            ICS_C2_BDIV(0x01)
           ));
  /* OSC->CR: OSCEN=0,??=0,OSCSTEN=0,OSCOS=0,??=0,RANGE=0,HGO=0,OSCINIT=0 */
  OSC->CR = 0x00U;
  while((ICS->S & ICS_S_IREFST_MASK) == 0x00U) { /* Check that the source of the FLL reference clock is the internal reference clock. */
  }
  while((ICS->S & 0x0CU) != 0x00U) {    /* Wait until output of the FLL is selected */
  }
#elif (CLOCK_SETUP == 1)
  /* SIM->CLKDIV: ??=0,??=0,OUTDIV1=0,??=0,??=0,??=0,OUTDIV2=1,??=0,??=0,??=0,OUTDIV3=1,??=0,??=0,??=0,??=0,??=0,??=0,??=0,??=0,??=0,??=0,??=0,??=0,??=0,??=0,??=0,??=0,??=0,??=0,??=0,??=0 */
  SIM->CLKDIV = (SIM_CLKDIV_OUTDIV2_MASK | SIM_CLKDIV_OUTDIV3_MASK); /* Update system prescalers */
  /* Switch to FEE Mode */
  /* ICS->C2: BDIV=0,LP=0 */
  ICS->C2 &= (uint8_t)~(uint8_t)((ICS_C2_BDIV(0x07) | ICS_C2_LP_MASK));
  /* OSC->CR: OSCEN=1,??=0,OSCSTEN=0,OSCOS=1,??=0,RANGE=1,HGO=0,OSCINIT=0 */
  OSC->CR = (OSC_CR_OSCEN_MASK | OSC_CR_OSCOS_MASK | OSC_CR_RANGE_MASK);
  /* ICS->C1: CLKS=0,RDIV=3,IREFS=0,IRCLKEN=1,IREFSTEN=0 */
  ICS->C1 = (ICS_C1_CLKS(0x00) | ICS_C1_RDIV(0x03) | ICS_C1_IRCLKEN_MASK);
  while((ICS->S & ICS_S_IREFST_MASK) != 0x00U) { /* Check that the source of the FLL reference clock is the external reference clock. */
  }
  while((ICS->S & 0x0CU) != 0x00U) {    /* Wait until output of the FLL is selected */
  }
#elif (CLOCK_SETUP == 2)
  /* SIM->CLKDIV: ??=0,??=0,OUTDIV1=0,??=0,??=0,??=0,OUTDIV2=0,??=0,??=0,??=0,OUTDIV3=0,??=0,??=0,??=0,??=0,??=0,??=0,??=0,??=0,??=0,??=0,??=0,??=0,??=0,??=0,??=0,??=0,??=0,??=0,??=0,??=0 */
  SIM->CLKDIV = SIM_CLKDIV_OUTDIV1(0x00); /* Update system prescalers */
  /* Switch to FBI Mode */
  /* ICS->C1: CLKS=1,RDIV=0,IREFS=1,IRCLKEN=1,IREFSTEN=0 */
  ICS->C1 = ICS_C1_CLKS(0x01) |
           ICS_C1_RDIV(0x00) |
           ICS_C1_IREFS_MASK |
           ICS_C1_IRCLKEN_MASK;
  /* ICS->C2: BDIV=0,LP=0 */
  ICS->C2 &= (uint8_t)~(uint8_t)((ICS_C2_BDIV(0x07) | ICS_C2_LP_MASK));
  /* OSC->CR: OSCEN=0,??=0,OSCSTEN=0,OSCOS=0,??=0,RANGE=0,HGO=0,OSCINIT=0 */
  OSC->CR = 0x00U;
  while((ICS->S & ICS_S_IREFST_MASK) == 0x00U) { /* Check that the source of the FLL reference clock is the internal reference clock. */
  }
  while((ICS->S & 0x0CU) != 0x04U) {    /* Wait until internal reference clock is selected as ICS output */
  }
  /* Switch to BLPI Mode */
  /* ICS->C2: BDIV=0,LP=1 */
  ICS->C2 = (uint8_t)((ICS->C2 & (uint8_t)~(uint8_t)(
            ICS_C2_BDIV(0x07)
           )) | (uint8_t)(
            ICS_C2_LP_MASK
           ));
  while((ICS->S & ICS_S_IREFST_MASK) == 0x00U) { /* Check that the source of the FLL reference clock is the internal reference clock. */
  }
#elif (CLOCK_SETUP == 3)
  /* SIM->CLKDIV: ??=0,??=0,OUTDIV1=0,??=0,??=0,??=0,OUTDIV2=0,??=0,??=0,??=0,OUTDIV3=0,??=0,??=0,??=0,??=0,??=0,??=0,??=0,??=0,??=0,??=0,??=0,??=0,??=0,??=0,??=0,??=0,??=0,??=0,??=0,??=0 */
  SIM->CLKDIV = SIM_CLKDIV_OUTDIV1(0x00); /* Update system prescalers */
  /* Switch to FBE Mode */
  /* ICS->C2: BDIV=0,LP=0 */
  ICS->C2 &= (uint8_t)~(uint8_t)((ICS_C2_BDIV(0x07) | ICS_C2_LP_MASK));
  /* OSC->CR: OSCEN=1,??=0,OSCSTEN=0,OSCOS=1,??=0,RANGE=1,HGO=0,OSCINIT=0 */
  OSC->CR = (OSC_CR_OSCEN_MASK | OSC_CR_OSCOS_MASK | OSC_CR_RANGE_MASK);
  /* ICS->C1: CLKS=2,RDIV=3,IREFS=0,IRCLKEN=1,IREFSTEN=0 */
  ICS->C1 = (ICS_C1_CLKS(0x02) | ICS_C1_RDIV(0x03) | ICS_C1_IRCLKEN_MASK);
  while((ICS->S & ICS_S_IREFST_MASK) != 0x00U) { /* Check that the source of the FLL reference clock is the external reference clock. */
  }
  while((ICS->S & 0x0CU) != 0x08U) {    /* Wait until external reference clock is selected as ICS output */
  }
  /* Switch to BLPE Mode */
  /* ICS->C2: BDIV=0,LP=1 */
  ICS->C2 = (uint8_t)((ICS->C2 & (uint8_t)~(uint8_t)(
            ICS_C2_BDIV(0x07)
           )) | (uint8_t)(
            ICS_C2_LP_MASK
           ));
  while((ICS->S & 0x0CU) != 0x08U) {    /* Wait until external reference clock is selected as ICS output */
  }
#endif

}

/* ----------------------------------------------------------------------------
   -- SystemCoreClockUpdate()
   ---------------------------------------------------------------------------- */

void SystemCoreClockUpdate (void) {

  uint32_t ICSOUTClock;                                                        /* Variable to store output clock frequency of the ICS module */
  uint8_t Divider;

  if ((ICS->C1 & ICS_C1_CLKS_MASK) == 0x0u) {
    /* Output of FLL is selected */
    if ((ICS->C1 & ICS_C1_IREFS_MASK) == 0x0u) {
      /* External reference clock is selected */
      ICSOUTClock = CPU_XTAL_CLK_HZ;                                         /* System oscillator drives ICS clock */
      Divider = (uint8_t)(1u << ((ICS->C1 & ICS_C1_RDIV_MASK) >> ICS_C1_RDIV_SHIFT));
      ICSOUTClock = (ICSOUTClock / Divider);  /* Calculate the divided FLL reference clock */
      if ((OSC->CR & OSC_CR_RANGE_MASK) != 0x0u) {
        ICSOUTClock /= 32u;                                                  /* If high range is enabled, additional 32 divider is active */
      }
    } else {
      ICSOUTClock = CPU_INT_CLK_HZ;                                          /* The internal reference clock is selected */
    }
    ICSOUTClock *= 1280u;                                                    /* Apply 1280 FLL multiplier */
  } else if ((ICS->C1 & ICS_C1_CLKS_MASK) == 0x40u) {
    /* Internal reference clock is selected */
    ICSOUTClock = CPU_INT_CLK_HZ;
  } else if ((ICS->C1 & ICS_C1_CLKS_MASK) == 0x80u) {
    /* External reference clock is selected */
    ICSOUTClock = CPU_XTAL_CLK_HZ;
  } else {
    /* Reserved value */
    return;
  }
  ICSOUTClock = ICSOUTClock >> ((ICS->C2 & ICS_C2_BDIV_MASK) >> ICS_C2_BDIV_SHIFT);
  SystemCoreClock = (ICSOUTClock / (1u + ((SIM->CLKDIV & SIM_CLKDIV_OUTDIV1_MASK) >> SIM_CLKDIV_OUTDIV1_SHIFT)));

}
