#include "stm32f401.h"

#include <cstdint>

extern "C" void SystemInit(void);

namespace {

// Run from HSI 16 MHz (no PLL). Enough for 115200 UART and Simantic.
void enable_hsi_clock() {
    RCC->CR |= (1U << 0);  // HSION
    while ((RCC->CR & (1U << 1)) == 0) {
    }
    RCC->CFGR = (RCC->CFGR & ~0x3U);  // SW = HSI
    while ((RCC->CFGR & 0xCU) != 0) {
    }
}

void init_usart2_115200() {
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;
    RCC->APB1ENR |= RCC_APB1ENR_USART2EN;

    // PA2=TX, PA3=RX — AF7
    GPIOA->MODER = (GPIOA->MODER & ~((3U << 4) | (3U << 6))) | ((2U << 4) | (2U << 6));
    GPIOA->OSPEEDR |= (3U << 4) | (3U << 6);
    GPIOA->AFR[0] = (GPIOA->AFR[0] & ~((0xFU << 8) | (0xFU << 12))) | ((7U << 8) | (7U << 12));

    USART2->CR1 = 0;
    // BRR for 16 MHz / 115200 ≈ 139 (0x8B)
    USART2->BRR = 0x8BU;
    USART2->CR1 = USART_CR1_TE | USART_CR1_RE | USART_CR1_UE;
}

}  // namespace

extern "C" void SystemInit(void) {
    enable_hsi_clock();
    init_usart2_115200();
}

extern "C" void delay_ms(std::uint32_t ms) {
    // Busy-wait calibrated roughly for 16 MHz HSI (enough for demo pacing).
    for (std::uint32_t i = 0; i < ms; ++i) {
        for (std::uint32_t n = 0; n < 2000U; ++n) {
            asm volatile("" ::: "memory");
            (void)n;
        }
    }
}
