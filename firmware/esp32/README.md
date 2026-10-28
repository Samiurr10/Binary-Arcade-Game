# Orbital — Binary Arcade for Freenove FNK0082

A space shooter for the ESP32-S3 in the Freenove Ultimate Starter Kit (Amazon
ASIN B0BMQ2CPQN). The board runs the authoritative game at a nominal 60 fixed
steps/second. A phone/laptop browser draws the color graphics from 25 Hz Wi-Fi
snapshots, interpolating ship and asteroid positions. Rendering uses Canvas 2D,
procedural shaded asteroid textures, a planetary backdrop, layered stars,
metallic ship shading, engine glow, laser trails, particles and optional audio.
No internet, cloud service, external fonts, image downloads or SD card is needed.

**This is not a color TFT driver or photorealistic 3D renderer.** The FNK0082
kit has a character LCD and LED matrix; those cannot display this color scene.
The browser is the screen. Game rules, collisions, score and persistence run
on the ESP32, not in a duplicate JavaScript game simulation.

## Hardware target and sources

- Freenove black PCB ESP32-S3-WROOM-1 N8R8: 8 MB flash and 8 MB OPI PSRAM.
- QIO flash / OPI PSRAM configuration; UART via the kit's CH343 USB bridge.
- Original STM32 firmware is separate and remains in `firmware/`.
- Camera, SD card and LED matrix are unused by this application.

Verified against the manufacturer's [board preface](https://docs.freenove.com/projects/fnk0082/en/latest/fnk0082/codes/C/Preface.html),
[kit contents/tutorial](https://docs.freenove.com/projects/fnk0082/en/latest/fnk0082/codes/C.html),
and [joystick example](https://github.com/Freenove/Freenove_Ultimate_Starter_Kit_for_ESP32_S3/blob/main/C/Sketches/Sketch_14.1_Joystick/Sketch_14.1_Joystick.ino).

## Build and upload

Install Python 3 and PlatformIO Core, then from the repository root:

```sh
python3 -m pip install platformio==6.1.18
python3 firmware/esp32/tools/embed_web.py
pio run -d firmware/esp32
pio device list
pio run -d firmware/esp32 -t upload --upload-port YOUR_PORT
pio device monitor -p YOUR_PORT -b 115200
```

Replace `YOUR_PORT` with the board's serial port (e.g. `/dev/cu.usbserial...`
or `COM5`). Uploading replaces the board's current application. Use the USB
port connected to CH343/UART. If upload stalls, hold BOOT, tap RESET, release
BOOT, then retry. Reset normally after flashing. See Freenove's CH343 driver
instructions if no serial port appears. This configuration does not route
Serial through native USB CDC.

The application binary is `.pio/build/freenove_esp32s3/firmware.bin` under this
directory. It belongs at **0x10000**, not 0x0. Prefer PlatformIO upload, which
also installs the correct bootloader, partition table and boot_app0 image.

## Play

1. Power the board and join Wi-Fi `BinaryArcade-XXXX`, password `arcade-play`.
2. Keep that Wi-Fi connection when your phone reports “no internet”.
3. Open **http://192.168.4.1** in a current browser. Use HTTP, not HTTPS.
4. Select **Take flight**. The first controller owns the controls; other
   connected browsers can watch. The access point permits up to four stations.
5. Move with WASD/arrows; hold Space to fire. P pauses/resumes; Enter starts
   or retries. On touchscreens, drag toward a destination in the playfield
   and hold the Fire button with a second finger. Enable Sound for synthesized
   browser audio.

Three shields, brief post-hit protection, shield pickups, accelerated asteroid
waves and time-limited hit combos replace the original one-hit binary game.
The board saves best score and completed games to NVS on game over only.
A power loss during a flight loses that flight's unsaved score. Save failures
are reported on serial. NVS uses a versioned checked record.

Input goes neutral and browser-controlled games pause after 400 ms without
valid input. Ownership releases after disconnect or five seconds without valid
input. Reclaim and select Resume after a connection loss. Opening another tab
does not override the current pilot. The AP password is shared/default; change
it in `src/main.cpp` before using in a public setting.

## Optional kit joystick

No wiring is required for browser controls. To enable the kit joystick, build
and upload the named joystick environment:

```sh
pio run -d firmware/esp32 -e freenove_esp32s3_joystick -t upload --upload-port YOUR_PORT
```

With power disconnected, wire:

| Joystick | ESP32-S3 |
|---|---|
| VCC | 3.3 V |
| GND | GND |
| VRx | GPIO 1 (ADC1) |
| VRy | GPIO 2 (ADC1) |
| SW | GPIO 12 |

**Rewire VRx/VRy if you followed the earlier GPIO14/13 instructions.**
Unlike Freenove's standalone chapter 14 example, this game runs Wi-Fi throughout.
GPIO14/13 use ADC2, whose reads can fail while Wi-Fi is active; GPIO1/2 use
ADC1 and avoid that contention. SW remains GPIO12 (digital input). See
[Espressif's ADC limitations](https://docs.espressif.com/projects/esp-idf/en/v4.4.7/esp32s3/api-reference/peripherals/adc.html#adc-limitations).
Use **3.3 V**, not 5 V, for the joystick
supply. Its switch uses the internal pull-up. Center dead zone is 12%; movement
assumes an ADC center near 2048. If a particular joystick drifts or is reversed,
adjust the axis function/sign in `src/main.cpp`. Press the stick to start, hold
to fire. Browser Pause remains available. In joystick mode, the physical stick
owns movement and losing a browser controller does not pause the game. The
camera is not initialized, so its overlapping GPIOs are not driven by firmware.

## Timing, memory and validation

- Fixed-size entity pools: 18 asteroids, 24 lasers, 64 particles, one pickup.
- Fixed 8 KB snapshot buffer; overflow drops a snapshot instead of truncating JSON.
- Finite, bounded input parsing; a single controller; neutral controls on timeout.
- Catch-up simulation limited to six ticks per loop; slow transport can slow
  simulation time rather than allowing unbounded catch-up/watchdog starvation.
- Build result: about 57 KB static RAM, 802 KB application flash. This excludes
  runtime Wi-Fi allocations. PSRAM is configured, but the game pools do not rely
  on dynamic PSRAM allocations.
- 60 Hz simulation and 25 Hz snapshots are targets, **not measured board results**.
- Validated: ESP32-S3 cross-compilation; host gameplay/protocol regression tests;
  address/undefined-behavior sanitizers; browser UI tests with mocked transport
  and snapshots recorded from the real C++ engine.
- Not validated: physical board boot, RF latency, joystick ADC calibration,
  hardware NVS power-cycle persistence, sustained Wi-Fi throughput.

From the repository root, run `make test`. After editing `web/index.html`, run
`python3 firmware/esp32/tools/embed_web.py`; CI rejects stale embedded HTML.
`tools/esp32_replay.cpp` records deterministic native engine snapshots; the web
viewer supports `?replay=preview.json` for an explicitly labelled, noninteractive
recording. Replay mode is for visual verification, not a second game engine.

Browser smoke tests (requires Node.js and Playwright with Chromium):

```sh
make replay
npm install --no-save --package-lock=false playwright
npx playwright install chromium
node tests/test_web.cjs
```

Alternatively set `CHROME_PATH` to a locally installed Chrome executable.
The test uses a mocked WebSocket transport, exercises launch, movement/fire,
pause, state overlays and disconnect, and saves desktop/mobile screenshots
under `build/`. It does not stand in for on-device network tests.

## Simantic review and simulation

There are two independent projects: `firmware/simantic.json` describes the
STM32 simulator demo; `firmware/esp32/simantic.json` describes this ESP32 target.
The STM32 demo passing does **not** validate the ESP32 Wi-Fi/ADC/NVS paths.

The first PR review's ESP32 blocker was the review container: it did not have
PlatformIO, the Arduino framework or the Xtensa toolchain. The ESP32 manifest
specifies the custom build command, but does not install missing software into
Simantic's hosted image. That image must provide PlatformIO 6.1.18 and allow
installation of the pinned packages in `platformio.ini` (or pre-cache them).
The reviewer supports custom commands, even though `platformio` is not one of
its `build.system` enum values. Do not label an Arduino project `esp-idf` merely
to get past detection: it is not an ESP-IDF CMake project.

The manifest uses **ESP32-S3-WROOM-1-N16R8 as a simulation surrogate**, because
`sim models` listed that 8 MB OPI-PSRAM variant but not the physical N8R8 kit.
It has more flash than the kit; our image/partition layout still uses 8 MB.
This is not a claim of exact-board equivalence or validated simulation support.
The hardware build and upload target remain the N8R8 kit.

Build a complete ROM-boot image without touching a board:

```sh
pio run -d firmware/esp32 -e freenove_esp32s3 -t merged
sim --mcu ESP32-S3-WROOM-1-N16R8 \
    --flash firmware/esp32/.pio/build/freenove_esp32s3/firmware-merged.bin \
    --timeout 5 --ascii --output build/esp32-sim-uart.txt
```

This command uses the 0.5/0.6 CLI flag syntax; check `sim --help` if your release
uses subcommands. The installed 0.5.20 CLI rejected this model before boot,
reporting **sim >= 0.6.3 required**. Updating an isolated copy also failed at
its release-manifest endpoint (HTTP 400), so no ESP32 runtime pass is claimed.
Use a compatible release and verify its model status first. Even then, require
`BINARY_ARCADE_READY` in the UART log; a process exiting without that marker is
not proof the application booted. The model's Wi-Fi/ADC/PSRAM support must be
checked separately before treating it as a substitute for hardware tests.

`firmware.bin` is the application only, loaded at 0x10000. `firmware-merged.bin`
also includes the bootloader at 0x0, partition table at 0x8000 and boot_app0 at
0xe000. For `--flash`, use the merged image, not the application alone. The
merge target takes the same image paths and offsets as PlatformIO's uploader.
For physical uploads, prefer `pio run -t upload`; flashing a merged image also
writes its padding across NVS, resetting saved state.

GitHub CI now builds both browser-only and joystick environments and publishes
ELF, application and merged images as downloadable artifacts. These artifacts
let a simulator operator run the exact commit without rebuilding it. They do
not automatically make Simantic's hosted reviewer consume GitHub artifacts.
See [Simantic project configuration](https://simantic.com/docs/simantic-json)
and [ROM-boot image usage](https://simantic.com/docs/guide).
