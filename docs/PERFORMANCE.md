# Performance

Benchmark command:

```sh
make benchmark
```

Environment:

```text
CPU: Apple M4
OS: Darwin 25.3.0 arm64
Compiler: Apple clang 21.0.0
Build flags: -std=c++20 -O2 -Wall -Wextra -Werror -pedantic
```

Measured run:

```text
iterations=250000
total_us=150314
us_per_tick=0.601254
ticks_per_second=1.66319e+06
i2c_frames=250000
uart_bytes=8143300
flash_writes=11
```

## Interpretation

The benchmark exercises the full host firmware loop:

- game tick and collision processing
- framebuffer packing
- I2C packet creation and recording
- UART telemetry formatting
- FlashIAP high-score saves when game-over records improve

At roughly 1.66 million simulated ticks per second on this machine, the core loop
is comfortably above typical LED matrix refresh and human-input rates. On real
hardware, bus latency and flash erase time would dominate; this benchmark is
best read as CPU-side protocol and game-loop overhead.
