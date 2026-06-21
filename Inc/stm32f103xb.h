/**
 * @brief  Minimal register definitions for STM32F103xB (Value Line)
 *         Only the registers needed for GPIO blink are defined.
 */

#ifndef STM32F103XB_H
#define STM32F103XB_H

#include <stdint.h>

/* ------------------------------------------------------------------ */
/*  Memory-mapped peripheral base addresses                           */
/* ------------------------------------------------------------------ */
#define PERIPH_BASE           (0x40000000UL)
#define APB1PERIPH_BASE        PERIPH_BASE
#define APB2PERIPH_BASE       (PERIPH_BASE + 0x00010000UL)
#define AHBPERIPH_BASE        (PERIPH_BASE + 0x00020000UL)

/* ------------------------------------------------------------------ */
/*  AHB peripherals – RCC                                             */
/* ------------------------------------------------------------------ */
#define RCC_BASE              (AHBPERIPH_BASE  + 0x00001000UL)

/* ------------------------------------------------------------------ */
/*  APB2 peripherals – GPIO                                            */
/* ------------------------------------------------------------------ */
#define GPIOC_BASE            (APB2PERIPH_BASE + 0x00001000UL)

/* ------------------------------------------------------------------ */
/*  RCC register map (excerpt)                                        */
/* ------------------------------------------------------------------ */
typedef struct {
    volatile uint32_t CR;            /* 0x00 */
    volatile uint32_t CFGR;          /* 0x04 */
    volatile uint32_t CIR;           /* 0x08 */
    volatile uint32_t APB2RSTR;      /* 0x0C */
    volatile uint32_t APB1RSTR;      /* 0x10 */
    volatile uint32_t AHBENR;        /* 0x14 */
    volatile uint32_t APB2ENR;       /* 0x18 */
    volatile uint32_t APB1ENR;       /* 0x1C */
    volatile uint32_t BDCR;          /* 0x20 */
    volatile uint32_t CSR;           /* 0x24 */
} RCC_TypeDef;

/* ------------------------------------------------------------------ */
/*  GPIO register map                                                 */
/* ------------------------------------------------------------------ */
typedef struct {
    volatile uint32_t CRL;           /* 0x00 – pins  0 …  7 */
    volatile uint32_t CRH;           /* 0x04 – pins  8 … 15 */
    volatile uint32_t IDR;           /* 0x08 */
    volatile uint32_t ODR;           /* 0x0C */
    volatile uint32_t BSRR;          /* 0x10 */
    volatile uint32_t BRR;           /* 0x14 */
    volatile uint32_t LCKR;          /* 0x18 */
} GPIO_TypeDef;

/* ------------------------------------------------------------------ */
/*  Peripheral pointers                                                */
/* ------------------------------------------------------------------ */
#define RCC   ((RCC_TypeDef  *) RCC_BASE)
#define GPIOC ((GPIO_TypeDef *) GPIOC_BASE)

/* ------------------------------------------------------------------ */
/*  RCC_APB2ENR bits                                                  */
/* ------------------------------------------------------------------ */
#define RCC_APB2ENR_IOPCEN   (1UL << 4)   /* GPIOC clock enable */

#endif /* STM32F103XB_H */
