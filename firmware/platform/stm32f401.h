#pragma once

#include <cstdint>

// Minimal STM32F401 register map for USART2 + GPIOA (mbed-style Serial on PA2/PA3).

inline constexpr std::uintptr_t PERIPH_BASE = 0x40000000UL;
inline constexpr std::uintptr_t APB1PERIPH_BASE = PERIPH_BASE;
inline constexpr std::uintptr_t AHB1PERIPH_BASE = PERIPH_BASE + 0x00020000UL;

inline constexpr std::uintptr_t RCC_BASE = AHB1PERIPH_BASE + 0x3800UL;
inline constexpr std::uintptr_t GPIOA_BASE = AHB1PERIPH_BASE + 0x0000UL;
inline constexpr std::uintptr_t FLASH_R_BASE = AHB1PERIPH_BASE + 0x3C00UL;
inline constexpr std::uintptr_t USART2_BASE = APB1PERIPH_BASE + 0x4400UL;

struct RccType {
    volatile std::uint32_t CR;
    volatile std::uint32_t PLLCFGR;
    volatile std::uint32_t CFGR;
    volatile std::uint32_t CIR;
    volatile std::uint32_t AHB1RSTR;
    volatile std::uint32_t AHB2RSTR;
    std::uint32_t RESERVED0[2];
    volatile std::uint32_t APB1RSTR;
    volatile std::uint32_t APB2RSTR;
    std::uint32_t RESERVED1[2];
    volatile std::uint32_t AHB1ENR;
    volatile std::uint32_t AHB2ENR;
    std::uint32_t RESERVED2[2];
    volatile std::uint32_t APB1ENR;
    volatile std::uint32_t APB2ENR;
};

struct GpioType {
    volatile std::uint32_t MODER;
    volatile std::uint32_t OTYPER;
    volatile std::uint32_t OSPEEDR;
    volatile std::uint32_t PUPDR;
    volatile std::uint32_t IDR;
    volatile std::uint32_t ODR;
    volatile std::uint32_t BSRR;
    volatile std::uint32_t LCKR;
    volatile std::uint32_t AFR[2];
};

struct UsartType {
    volatile std::uint32_t SR;
    volatile std::uint32_t DR;
    volatile std::uint32_t BRR;
    volatile std::uint32_t CR1;
    volatile std::uint32_t CR2;
    volatile std::uint32_t CR3;
    volatile std::uint32_t GTPR;
};

inline RccType* const RCC = reinterpret_cast<RccType*>(RCC_BASE);
inline GpioType* const GPIOA = reinterpret_cast<GpioType*>(GPIOA_BASE);
inline UsartType* const USART2 = reinterpret_cast<UsartType*>(USART2_BASE);

inline constexpr std::uint32_t RCC_AHB1ENR_GPIOAEN = 1U << 0;
inline constexpr std::uint32_t RCC_APB1ENR_USART2EN = 1U << 17;

inline constexpr std::uint32_t USART_SR_TXE = 1U << 7;
inline constexpr std::uint32_t USART_SR_TC = 1U << 6;
inline constexpr std::uint32_t USART_CR1_UE = 1U << 13;
inline constexpr std::uint32_t USART_CR1_TE = 1U << 3;
inline constexpr std::uint32_t USART_CR1_RE = 1U << 2;

// Cortex-M SysTick (for crude delays).
struct SysTickType {
    volatile std::uint32_t CTRL;
    volatile std::uint32_t LOAD;
    volatile std::uint32_t VAL;
    volatile std::uint32_t CALIB;
};
inline SysTickType* const SYSTICK = reinterpret_cast<SysTickType*>(0xE000E010UL);
