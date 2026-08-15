#pragma once

#include "stm32f401.h"

#include <cstddef>
#include <cstdint>
#include <cstring>

// Thin mbed-like Serial API over USART2 (PA2/PA3 @ 115200).
class Serial {
  public:
    Serial() = default;

    void baud(int /*baudrate*/) {
        // Fixed at 115200 in SystemInit for Simantic predictability.
    }

    void putc(char c) {
        while ((USART2->SR & USART_SR_TXE) == 0) {
        }
        USART2->DR = static_cast<std::uint8_t>(c);
    }

    void write(const char* data, std::size_t length) {
        for (std::size_t i = 0; i < length; ++i) {
            putc(data[i]);
        }
    }

    void write(const std::uint8_t* data, std::size_t length) {
        write(reinterpret_cast<const char*>(data), length);
    }

    void printf(const char* fmt, ...) __attribute__((format(printf, 2, 3)));

    Serial& operator<<(const char* text) {
        if (text != nullptr) {
            write(text, std::strlen(text));
        }
        return *this;
    }
};

inline Serial pc;
