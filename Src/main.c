/**
 ******************************************************************************
 * @file           : main.c
 * @brief          : Blink GPIOC13 – register-level, no HAL/library dependency
 ******************************************************************************
 */

#include <stdint.h>
#include "stm32f103xb.h"

/* ------------------------------------------------------------------ */
/*  SystemInit – called by startup before main                        */
/*  HSI (8 MHz) is the default clock after reset, nothing to do here. */
/* ------------------------------------------------------------------ */
void SystemInit(void)
{
    /* Leave the system running on HSI 8 MHz */
}

/* ------------------------------------------------------------------ */
/*  Rough software delay (busy loop)                                  */
/*  ~ count * 4 cycles @ 8 MHz → ~ 0.5 µs per count                  */
/* ------------------------------------------------------------------ */
static void delay_ms(uint32_t ms)
{
    /* calibrated for ~1 ms at HSI 8 MHz (very rough; adjust if
       you switch to 72 MHz PLL later) */
    volatile uint32_t i;
    while (ms--) {
        for (i = 0; i < 2000; ++i) {
            __asm("nop");
        }
    }
}

/* ------------------------------------------------------------------ */
/*  main                                                              */
/* ------------------------------------------------------------------ */
int main(void)
{
    /* 1. Enable GPIOC clock (APB2 bus) */
    RCC->APB2ENR |= RCC_APB2ENR_IOPCEN;

    /* 2. Configure PC13 as push-pull output (max 10 MHz)
          CRH  bits  [23:22] = CNF13 = 00 (general push-pull)
                     [21:20] = MODE13 = 01 (output, 10 MHz) */
    GPIOC->CRH &= ~(0xFU << 20);   /* clear PC13 config */
    GPIOC->CRH |=  (0x1U << 20);   /* MODE13 = 01, CNF13 = 00 */

    /* 3. Two-phase blink: slow (1s) then fast (100ms), repeat */
    while (1) {
        /* Phase 1: slow blink ~5 cycles (1s on, 1s off) */
        for (int n = 0; n < 5; ++n) {
            GPIOC->BSRR = (1UL << 13);
            delay_ms(1000);
            GPIOC->BRR  = (1UL << 13);
            delay_ms(1000);
        }

        /* Phase 2: fast blink ~20 cycles (100ms on, 100ms off) */
        for (int n = 0; n < 20; ++n) {
            GPIOC->BSRR = (1UL << 13);
            delay_ms(100);
            GPIOC->BRR  = (1UL << 13);
            delay_ms(100);
        }
    }
}
