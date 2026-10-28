# Binary Arcade Game

## ESP32-S3 color upgrade: Orbital

The [Freenove FNK0082 firmware](firmware/esp32/README.md) runs an expanded space
shooter on the ESP32-S3 and serves its graphical browser display over local
Wi-Fi. Includes shaded asteroids, ship thrust, particles, shields, combos,
progressive difficulty, keyboard/touch controls and optional kit joystick input.
The kit does not include a color TFT; your phone or laptop is the display.

Build with `pio run -d firmware/esp32`. Run all host regressions with `make test`.

## Original simulator

Retro embedded arcade firmware simulator in C++20. The project models a small
binary LED-matrix game with explicit I2C display frames, UART telemetry, SPI
sprite reads, and a custom FlashIAP-style high-score log.

The code is host-runnable for testing, but the boundaries are shaped like
firmware: game logic is isolated from bus drivers, FlashIAP persistence is
append-only and CRC-checked, and mock transports make each subsystem testable.

An **mbed-style STM32F401CC firmware** port lives in [`firmware/`](firmware/)
and runs under the [Simantic](https://simantic.dev) CLI (Cortex-M4 — closest
supported MCU to classic mbed Cortex-M3 boards).

## Features

- 16x8 binary framebuffer with packed LED rows and ASCII preview rendering
- Deterministic arcade engine with player movement, fire input, falling bits,
  score tracking, and game-over frame rendering
- I2C display driver with sync byte, payload length, packed pixels, and
  CRC-16/CCITT
- UART telemetry stream with score, state, and checksum
- SPI sprite ROM reader using command/response transfer semantics
- Custom FlashIAP log for high scores with sequence numbers, CRC validation,
  page erase rotation, and wear counters
- Unit and integration tests for every major subsystem
- CI workflow that builds, tests, and runs a benchmark smoke test
- Mbed-style MCU firmware + Simantic simulation target (`firmware/`)

## Quick Start

```sh
make
make test
make run
make benchmark
```

Expected test output:

```text
All binary arcade tests passed
```

### Firmware + Simantic

```sh
cd firmware
make
sim STM32F401CC run --elf ./build/binary-arcade-firmware.elf --timeout 3 --ascii
```

See [firmware/README.md](firmware/README.md) for details.
## Architecture

```text
                +------------------+
Input --------> |   GameEngine     | ---- framebuffer ----+
                | score/state/rng  |                      |
                +------------------+                      v
                                                        I2cDisplay
                                                            |
                                                        I2cBus

GameEngine ---- score/state ----> UartTelemetry ----> UartSink

GameEngine ---- high score -----> ArcadeSystem -----> FlashIap
                                                        pages + CRC

SpiSpriteRom ---- command -----> SpiDevice ---- sprite bytes
```

The central coordinator is `ArcadeSystem`. It advances `GameEngine`, flushes
the framebuffer over `I2cDisplay`, emits UART telemetry, and persists new high
scores through `FlashIap`.

More detail is in [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md).

## Performance

Historical baseline before the game-count and lane-randomness fixes (Apple M4 / Darwin arm64 with Apple clang 21). See the updated notes in `docs/PERFORMANCE.md`; these numbers do not describe the ESP32 port:

| Metric | Value |
|--------|-------|
| Iterations | 250,000 ticks |
| Mean tick time | 0.601 us |
| Throughput | 1.66M ticks/sec |
| I2C frames emitted | 250,000 |
| UART bytes emitted | 8,143,300 |
| FlashIAP writes | 11 |

Reproduce with:

```sh
make benchmark
```

See [docs/PERFORMANCE.md](docs/PERFORMANCE.md) for the full run output.

## Test Coverage Map

| Area | Test |
|------|------|
| CRC protocol guard | `test_crc_known_vector` |
| Framebuffer packing/rendering | `test_framebuffer_pack_and_ascii` |
| Game movement/scoring | `test_game_movement_fire_and_game_over` |
| I2C display framing | `test_i2c_display_frame` |
| UART telemetry format | `test_uart_telemetry` |
| SPI sprite read path | `test_spi_sprite_read` |
| FlashIAP persistence/wear | `test_flash_iap_persists_latest_and_wears_pages` |
| Full firmware loop | `test_system_integrates_drivers_and_flash` |

## Repository Layout

```text
include/binary_arcade/  Public subsystem headers
src/                    Firmware simulator implementation
tests/                  Lightweight C++ test runner
tools/                  Benchmark tool
firmware/               Mbed-style STM32F401 port for Simantic
docs/                   Architecture and performance notes
.github/workflows/     CI build/test/benchmark workflow
```

## Porting Notes

To move from host simulation to hardware, implement the `I2cBus`, `UartSink`,
and `SpiDevice` interfaces using MCU HAL calls. `FlashIap` is intentionally
byte-vector backed in this repo, but its record format and append/erase behavior
mirror the constraints of real flash: records are CRC-protected, writes append
to erased slots, and page erases are counted.
