# Mbed-style firmware (STM32F401 / Simantic)

This directory ports Binary Arcade to a **mbed-style** firmware image for
**STM32F401CC (Cortex-M4)** — the closest MCU currently available in Simantic
to classic mbed Cortex-M3 boards such as LPC1768.

## What runs on-device / in Simantic

- Shared game engine, I2C display framing, UART telemetry, SPI sprite ROM, and
  FlashIAP high-score log from the host library
- Thin mbed-like `Serial` (`pc.printf`) over USART2 (PA2/PA3 @ 115200)
- Deterministic demo inputs (same seed/`Input` pattern as host `main`)

## Build

Requires Arm GNU Toolchain (`arm-none-eabi-g++`). A portable install under
`~/toolchains/arm-gnu-toolchain-*-arm-none-eabi` is auto-detected.

```sh
cd firmware
make
```

Artifacts:

- `build/binary-arcade-firmware.elf`
- `build/binary-arcade-firmware.bin`

## Simulate with Simantic CLI

```sh
sim STM32F401CC run \
  --elf ./build/binary-arcade-firmware.elf \
  --timeout 3 --ascii --only-messages \
  --expect BINARY_ARCADE_DONE --output ./sim-output.txt
```

Expected markers in UART output:

- `BINARY_ARCADE_BOOT`
- `BINARY_ARCADE_SPI_OK`
- `score=…;state=running;crc=…` (telemetry)
- `i2c addr=0x3C … sync=0xA5`
- `BINARY_ARCADE_I2C_OK`
- `BINARY_ARCADE_FLASH_OK`
- `BINARY_ARCADE_DONE score=… state=game_over`

`simantic.json` pins this project's MCU to STM32F401CC, its ELF path and expected
UART markers. It belongs in this directory (not the repository root), since the
root Makefile builds a host executable. The ESP32 project has a separate
manifest and build prerequisites; see `esp32/README.md`.
