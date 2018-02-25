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
 * @brief Device specific configuration file for MKE06Z4 (header file)
 *
 * Provides a system configuration function and a global variable that contains
 * the system frequency. It configures the device and initializes the oscillator
 * (PLL) that is part of the microcontroller device.
 */

#ifndef SYSTEM_MKE06Z4_H_
#define SYSTEM_MKE06Z4_H_                        /**< Symbol preventing repeated inclusion */

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>


#define DISABLE_WDOG    1

#ifndef CLOCK_SETUP
  #define CLOCK_SETUP   1
#endif
/* Predefined clock setups
   0 ... Internal Clock Source (ICS) in FLL Engaged Internal (FEI) mode
         Default  part configuration.
         Reference clock source for ICS module is the slow internal clock source 32.768kHz
         Core clock = 20.97MHz, BusClock = 20.97MHz
   1 ... Internal Clock Source (ICS) in FLL Engaged External (FEE) mode
         Maximum achievable clock frequency configuration.
         Reference clock source for ICS module is an external 8MHz crystal
         Core clock = 40MHz, BusClock = 20MHz
   2 ... Internal Clock Source (ICS) in Bypassed Low Power Internal (FBILP) mode
         Core clock/Bus clock derived directly from an  internal clock 32.769kHz with no multiplication
         The clock settings is ready for Very Low Power Run mode.
         Core clock = 32.769kHz, BusClock = 32.769kHz
   3 ... Internal Clock Source (ICS) in Bypassed Low Power External (BLPE) mode
         Core clock/Bus clock derived directly from the external 8MHz crystal
         The clock settings is ready for Very Low Power Run mode.
         Core clock = 8MHz, BusClock = 8MHz
*/

/*----------------------------------------------------------------------------
  Define clock source values
 *----------------------------------------------------------------------------*/
#if (CLOCK_SETUP == 0)
    #define CPU_XTAL_CLK_HZ                 8000000u /* Value of the external crystal or oscillator clock frequency in Hz */
    #define CPU_INT_CLK_HZ                  32768u   /* Value of the internal oscillator clock frequency in Hz  */
    #define DEFAULT_SYSTEM_CLOCK            20971520u /* Default System clock value */
	#error "DEFAULT_BUS_CLOCK not defined!"
#elif (CLOCK_SETUP == 1)
    #define CPU_XTAL_CLK_HZ                 8000000u /* Value of the external crystal or oscillator clock frequency in Hz */
    #define CPU_INT_CLK_HZ                  32768u   /* Value of the internal oscillator clock frequency in Hz  */
    #define DEFAULT_SYSTEM_CLOCK            40000000u /* Default System clock value */
	#define DEFAULT_BUS_CLOCK               20000000u
#elif (CLOCK_SETUP == 2)
    #define CPU_XTAL_CLK_HZ                 8000000u /* Value of the external crystal or oscillator clock frequency in Hz */
    #define CPU_INT_CLK_HZ                  32768u   /* Value of the internal oscillator clock frequency in Hz  */
    #define DEFAULT_SYSTEM_CLOCK            32768u   /* Default System clock value */
	#error "DEFAULT_BUS_CLOCK not defined!"
#elif (CLOCK_SETUP == 3)
    #define CPU_XTAL_CLK_HZ                 8000000u /* Value of the external crystal or oscillator clock frequency in Hz */
    #define CPU_INT_CLK_HZ                  32768u   /* Value of the internal oscillator clock frequency in Hz  */
    #define DEFAULT_SYSTEM_CLOCK            8000000u /* Default System clock value */
	#error "DEFAULT_BUS_CLOCK not defined!"
#endif /* (CLOCK_SETUP == 4) */


/**
 * @brief System clock frequency (core clock)
 *
 * The system clock frequency supplied to the SysTick timer and the processor
 * core clock. This variable can be used by the user application to setup the
 * SysTick timer or configure other parameters. It may also be used by debugger to
 * query the frequency of the debug timer or configure the trace clock speed
 * SystemCoreClock is initialized with a correct predefined value.
 */
extern uint32_t SystemCoreClock;

/**
 * @brief Setup the microcontroller system.
 *
 * Typically this function configures the oscillator (PLL) that is part of the
 * microcontroller device. For systems with variable clock speed it also updates
 * the variable SystemCoreClock. SystemInit is called from startup_device file.
 */
void SystemInit (void);

/**
 * @brief Updates the SystemCoreClock variable.
 *
 * It must be called whenever the core clock is changed during program
 * execution. SystemCoreClockUpdate() evaluates the clock register settings and calculates
 * the current core clock.
 */
void SystemCoreClockUpdate (void);

#ifdef __cplusplus
}
#endif

#endif  /* #if !defined(SYSTEM_MKE06Z4_H_) */
