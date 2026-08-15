#include "mbed.h"

#include <cstdarg>
#include <cstdio>

void Serial::printf(const char* fmt, ...) {
    char buffer[160];
    va_list args;
    va_start(args, fmt);
    const int n = vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);
    if (n > 0) {
        write(buffer, static_cast<std::size_t>(n < static_cast<int>(sizeof(buffer))
                                                   ? n
                                                   : static_cast<int>(sizeof(buffer) - 1)));
    }
}
