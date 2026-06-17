# Architecture

## Goals

Binary Arcade Game is organized as a firmware-style simulation:

- keep game rules deterministic and independent of hardware
- make bus protocols observable and testable
- model FlashIAP behavior without requiring a microcontroller
- keep the build portable with only `make` and a C++20 compiler

## Subsystems

### Game Engine

`GameEngine` owns score, high score, player position, obstacle rows, RNG state,
and the active `Framebuffer`. Every `tick()` consumes an `Input` bitmask,
advances obstacles, resolves collision, updates score, and renders the next
frame.

### Framebuffer

`Framebuffer` stores 16 columns by 8 rows as row bitmasks. It can render ASCII
for demos and pack the LED state into 16 bytes for I2C transfer.

### I2C Display

`I2cDisplay` converts a framebuffer into a small binary packet:

```text
0xA5 | payload_length | 16 packed bytes | crc_hi | crc_lo
```

The transport itself is an `I2cBus` interface. Tests use `RecordingI2cBus` to
verify address, length, sync byte, and payload behavior.

### UART Telemetry

`UartTelemetry` emits newline-delimited status records:

```text
score=13;state=game_over;crc=35222
```

The checksum covers the score/state body before the `crc=` field.

### SPI Sprite ROM

`SpiSpriteRom` issues a compact read command:

```text
0x03 | sprite_id | length_hi | length_lo
```

`MemorySpiDevice` models a sprite ROM with 16-byte sprite slots.

### FlashIAP

`FlashIap` is an append-only save log. Each record contains:

- magic
- sequence
- high score
- games played
- format version
- CRC-16/CCITT
- commit marker

On load, it scans all valid records and returns the highest sequence number.
When no erased record slot remains, it erases the least-worn page and continues
writing. This keeps the behavior deterministic while preserving the core
constraints of MCU internal flash.

### System Coordinator

`ArcadeSystem` wires the firmware loop:

1. tick the game engine
2. flush the current framebuffer over I2C
3. emit score/state over UART
4. persist a new high score through FlashIAP

This keeps hardware effects at the edge and makes the core game logic simple to
test.
