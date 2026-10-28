# Current validation note

The original measurements below predate the completed-game counter fix and
improved lane randomness. They are retained as historical baseline data.
After the fix, a local 250,000-tick run took 184,896 us (0.740 us/tick,
1.35M ticks/sec) and wrote 10,754 save records. Every completed game now updates
the simulated log, including games that do not beat the high score.
These are **host simulator** measurements, not ESP32 performance results.
The new ESP32 port saves to NVS once on game over and targets 60 simulation
steps/second with 25 network snapshots/second; hardware timing is unmeasured.

## Historical baseline

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
