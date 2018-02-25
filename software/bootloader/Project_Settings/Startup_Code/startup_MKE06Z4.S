/* ---------------------------------------------------------------------------------------*/
/*  @file:    startup_MKE06Z4.s                                                           */
/*  @purpose: CMSIS Cortex-M0P Core Device Startup File                                   */
/*            MKE06Z4                                                                     */
/*  @version: 1.4                                                                         */
/*  @date:    2014-5-28                                                                   */
/*  @build:   b140606                                                                     */
/* ---------------------------------------------------------------------------------------*/
/*                                                                                        */
/* Copyright (c) 1997 - 2014 , Freescale Semiconductor, Inc.                              */
/* All rights reserved.                                                                   */
/*                                                                                        */
/* Redistribution and use in source and binary forms, with or without modification,       */
/* are permitted provided that the following conditions are met:                          */
/*                                                                                        */
/* o Redistributions of source code must retain the above copyright notice, this list     */
/*   of conditions and the following disclaimer.                                          */
/*                                                                                        */
/* o Redistributions in binary form must reproduce the above copyright notice, this       */
/*   list of conditions and the following disclaimer in the documentation and/or          */
/*   other materials provided with the distribution.                                      */
/*                                                                                        */
/* o Neither the name of Freescale Semiconductor, Inc. nor the names of its               */
/*   contributors may be used to endorse or promote products derived from this            */
/*   software without specific prior written permission.                                  */
/*                                                                                        */
/* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND        */
/* ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED          */
/* WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE                 */
/* DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR       */
/* ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES         */
/* (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;           */
/* LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON         */
/* ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT                */
/* (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS          */
/* SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.                           */
/*****************************************************************************/
/* Version: GCC for ARM Embedded Processors                                  */
/*****************************************************************************/
    .syntax unified
    .arch armv6-m

    .section .isr_vector, "a"
    .align 2
    .globl __isr_vector
__isr_vector:
    .long   __StackTop                                      /* Top of Stack */
    .long   Reset_Handler                                   /* Reset Handler */
    .long   NMI_Handler                                     /* NMI Handler*/
    .long   HardFault_Handler                               /* Hard Fault Handler*/
    .long   0                                               /* Reserved*/
    .long   0                                               /* Reserved*/
    .long   0                                               /* Reserved*/
    .long   0                                               /* Reserved*/
    .long   0                                               /* Reserved*/
    .long   0                                               /* Reserved*/
    .long   0                                               /* Reserved*/
    .long   SVC_Handler                                     /* SVCall Handler*/
    .long   0                                               /* Reserved*/
    .long   0                                               /* Reserved*/
    .long   PendSV_Handler                                  /* PendSV Handler*/
    .long   SysTick_Handler                                 /* SysTick Handler*/

                                                            /* External Interrupts*/
    .long   Reserved16_IRQHandler                           /* Reserved interrupt*/
    .long   Reserved17_IRQHandler                           /* Reserved interrupt*/
    .long   Reserved18_IRQHandler                           /* Reserved interrupt*/
    .long   Reserved19_IRQHandler                           /* Reserved interrupt*/
    .long   Reserved20_IRQHandler                           /* Reserved interrupt*/
    .long   FTMRE_IRQHandler                                /* Command complete and read collision*/
    .long   PMC_IRQHandler                                  /* Low-voltage detect, low-voltage warning*/
    .long   IRQ_IRQHandler                                  /* External Interrupt*/
    .long   I2C0_IRQHandler                                 /* I2C0 interrupt*/
    .long   I2C1_IRQHandler                                 /* I2C1 interrupt*/
    .long   SPI0_IRQHandler                                 /* SPI0 single interrupt vector for all sources*/
    .long   SPI1_IRQHandler                                 /* SPI1 single interrupt vector for all sources*/
    .long   UART0_IRQHandler                                /* UART0 status and error*/
    .long   UART1_IRQHandler                                /* UART1 status and error*/
    .long   UART2_IRQHandler                                /* UART2 status and error*/
    .long   ADC_IRQHandler                                  /* ADC interrupt*/
    .long   ACMP0_IRQHandler                                /* ACMP0 interrupt*/
    .long   FTM0_IRQHandler                                 /* FTM0 single interrupt vector for all sources*/
    .long   FTM1_IRQHandler                                 /* FTM1 single interrupt vector for all sources*/
    .long   FTM2_IRQHandler                                 /* FTM2 single interrupt vector for all sources*/
    .long   RTC_IRQHandler                                  /* RTC overflow*/
    .long   ACMP1_IRQHandler                                /* ACMP1 interrupt*/
    .long   PIT_CH0_IRQHandler                              /* PIT CH0 overflow*/
    .long   PIT_CH1_IRQHandler                              /* PIT CH1 overflow*/
    .long   KBI0_IRQHandler                                 /* Keyboard interrupt0(32bit)*/
    .long   KBI1_IRQHandler                                 /* Keyboard interrupt1(32bit)*/
    .long   Reserved42_IRQHandler                           /* Reserved interrupt*/
    .long   ICS_IRQHandler                                  /* ICS interrupt*/
    .long   WDOG_IRQHandler                                 /* Watchdog timeout*/
    .long   PWT_IRQHandler                                  /* PWT interrupt*/
    .long   MSCAN_RX_IRQHandler                             /* MSCAN Rx Interrupt*/
    .long   MSCAN_TX_IRQHandler                             /* MSCAN Tx, Err and Wake-up interrupt*/

    .size    __isr_vector, . - __isr_vector

/* Flash Configuration */
    .section .FlashConfig, "a"
    .long 0xFFFFFFFF
    .long 0xFFFFFFFF
    .long 0xFFFFFFFF
    .long 0xFFFEFFFF

    .equ _NVIC_ICER,  0xE000E180
    .equ _NVIC_ICPR,  0xE000E280
    .text
    .thumb

/* Reset Handler */

    .thumb_func
    .align 2
    .globl   Reset_Handler
    .type    Reset_Handler, %function
Reset_Handler:
    cpsid   i               /* Mask interrupts */
    ldr r0, =_NVIC_ICER    /* Disable interrupts and clear pending flags */
    ldr r1, =_NVIC_ICPR
    ldr r2, =0xFFFFFFFF
    str r2, [r0]            /* NVIC_ICER - clear enable IRQ register */
    str r2, [r1]            /* NVIC_ICPR - clear pending IRQ register */
#ifndef __NO_SYSTEM_INIT
    bl SystemInit
#endif
    cpsie   i               /* Unmask interrupts */
/*     Loop to copy data from read only memory to RAM. The ranges
 *      of copy from/to are specified by following symbols evaluated in
 *      linker script.
 *      __etext: End of code section, i.e., begin of data sections to copy from.
 *      __data_start__/__data_end__: RAM address range that data should be
 *      copied to. Both must be aligned to 4 bytes boundary.  */

    ldr    r1, =__etext
    ldr    r2, =__data_start__
    ldr    r3, =__data_end__

    subs    r3, r2
    ble     .LC0

.LC1:
    subs    r3, 4
    ldr    r0, [r1,r3]
    str    r0, [r2,r3]
    bgt    .LC1
.LC0:

#ifdef __STARTUP_CLEAR_BSS
/*     This part of work usually is done in C library startup code. Otherwise,
 *     define this macro to enable it in this startup.
 *
 *     Loop to zero out BSS section, which uses following symbols
 *     in linker script:
 *      __bss_start__: start of BSS section. Must align to 4
 *      __bss_end__: end of BSS section. Must align to 4
 */
    ldr r1, =__bss_start__
    ldr r2, =__bss_end__

    subs    r2, r1
    ble .LC3

    movs    r0, 0
.LC2:
    str r0, [r1, r2]
    subs    r2, 4
    bge .LC2
.LC3:
#endif
#ifndef __START
#define __START _start
#endif
    bl    __START
    .pool
    .size Reset_Handler, . - Reset_Handler

  .align	1
  .thumb_func
  .weak	Default_Handler
  .type	Default_Handler, %function
Default_Handler:
  b	.
  .size	Default_Handler, . - Default_Handler

/*    Macro to define default handlers. Default handler
 *    will be weak symbol and just dead loops. They can be
 *    overwritten by other handlers */
  .macro	def_irq_handler	handler_name
  .weak	\handler_name
  .set	\handler_name, Default_Handler
  .endm

/* Exception Handlers */
    def_irq_handler    NMI_Handler
    def_irq_handler    HardFault_Handler
    def_irq_handler    SVC_Handler
    def_irq_handler    PendSV_Handler
    def_irq_handler    SysTick_Handler
    def_irq_handler    Reserved16_IRQHandler
    def_irq_handler    Reserved17_IRQHandler
    def_irq_handler    Reserved18_IRQHandler
    def_irq_handler    Reserved19_IRQHandler
    def_irq_handler    Reserved20_IRQHandler
    def_irq_handler    FTMRE_IRQHandler
    def_irq_handler    PMC_IRQHandler
    def_irq_handler    IRQ_IRQHandler
    def_irq_handler    I2C0_IRQHandler
    def_irq_handler    I2C1_IRQHandler
    def_irq_handler    SPI0_IRQHandler
    def_irq_handler    SPI1_IRQHandler
    def_irq_handler    UART0_IRQHandler
    def_irq_handler    UART1_IRQHandler
    def_irq_handler    UART2_IRQHandler
    def_irq_handler    ADC_IRQHandler
    def_irq_handler    ACMP0_IRQHandler
    def_irq_handler    FTM0_IRQHandler
    def_irq_handler    FTM1_IRQHandler
    def_irq_handler    FTM2_IRQHandler
    def_irq_handler    RTC_IRQHandler
    def_irq_handler    ACMP1_IRQHandler
    def_irq_handler    PIT_CH0_IRQHandler
    def_irq_handler    PIT_CH1_IRQHandler
    def_irq_handler    KBI0_IRQHandler
    def_irq_handler    KBI1_IRQHandler
    def_irq_handler    Reserved42_IRQHandler
    def_irq_handler    ICS_IRQHandler
    def_irq_handler    WDOG_IRQHandler
    def_irq_handler    PWT_IRQHandler
    def_irq_handler    MSCAN_RX_IRQHandler
    def_irq_handler    MSCAN_TX_IRQHandler
    def_irq_handler    DefaultISR

    .end
