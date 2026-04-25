# Handoff — ESP32-S3 1.28" Rotary Clock + Weather

This file is a one-page brief for a Claude Code session resuming this project
on a local machine that has the hardware plugged in. Once the project is
running cleanly you can delete it.

## Hardware

- **ELECROW 1.28" Rotary IPS Display**
- MCU: ESP32-S3R8 (Xtensa LX7 dual-core 240 MHz)
- Memory: 512 KB SRAM + 8 MB OPI PSRAM
- Flash: 8 MB QIO
- Display: GC9A01 240×240 round IPS, SPI

Pin map currently wired in `platformio.ini` (overridable via `-DTFT_*` flags):

| Function | GPIO |
|----------|------|
| TFT MOSI | 11 |
| TFT SCLK | 12 |
| TFT CS   | 10 |
| TFT DC   | 8  |
| TFT RST  | tied to system reset (-1) |
| TFT BL   | 2  |

These are the documented Elecrow S3 1.28" rotary defaults but **have not yet
been confirmed on this unit**. If the screen stays blank, this is the first
suspect.

## Project layout

```
esp32-clock-weather/
├── platformio.ini      board/PSRAM/flash + TFT_eSPI pin overrides
├── partitions.csv      8 MB layout, LittleFS partition for /config.json
├── data/config.json    uploaded to LittleFS via `pio run -t uploadfs`
└── src/
    ├── main.cpp        boot sequence + 1 Hz tick loop
    ├── config.{h,cpp}  LittleFS + ArduinoJson loader, IANA→POSIX TZ table
    ├── net.{h,cpp}     WiFi.begin + configTzTime SNTP sync
    ├── weather.{h,cpp} Open-Meteo HTTPS client → WMO code → WeatherKind
    └── ui.{h,cpp}      TFT_eSPI render: weather background + ticks +
                        hour/min/sec hands + center digital + day/date
```

UI renders into a 240×240 16bpp `TFT_eSprite` back-buffer (≈113 KB) for
flicker-free updates, with a direct-draw fallback if the allocation fails.

Weather backgrounds are **procedural** (no bitmap assets): clear day/night,
partly cloudy, cloudy, fog, rain, snow, storm.

## Build / flash

```sh
cd esp32-clock-weather

# 1. edit data/config.json with WiFi / lat-lon / units
# 2. flash firmware
pio run -t upload

# 3. flash LittleFS image (config.json)
pio run -t uploadfs

# 4. monitor (esp32_exception_decoder is enabled, so panics get symbolized)
pio device monitor
```

## Where we left off (open issue)

Latest run on the user's hardware crashed before any application output:

```
Guru Meditation Error: Core 1 panic'ed (StoreProhibited).
PC      : 0x4200830b
EXCVADDR: 0x00000010   ← writing to (this+0x10) where this == nullptr
```

This was on a build that **incorrectly targeted ESP32-C3** with C3 pin
numbers (the early git history shows that). The most recent commit on the
branch retargets to ESP32-S3R8 and updates the pin map. **That fix has not
been re-tested by the user yet.**

### What to do first on the local machine

1. `git pull` and confirm you're on `claude/esp32-clock-weather-app-UgTU6`.
2. Edit `data/config.json` with real WiFi credentials and location.
3. `pio run -t upload && pio run -t uploadfs`, then `pio device monitor`.
4. Watch for the `[boot]` and `[ui]` progress lines added in `setup()` and
   `UI::begin()`. They tell you exactly which step a crash happens at:
   - No `=== ESP32-S3 1.28" rotary clock+weather ===` at all → USB-CDC
     issue or panic before `Serial` is ready.
   - Stops after `[ui] tft.init` → still a pin-map issue. Compare against
     the actual board schematic / Elecrow wiki and update the `-DTFT_*`
     flags in `platformio.ini`.
   - Stops at `[ui] sprite back-buffer allocated` → drawing-path problem.
   - Reaches `[boot] WiFi connecting` → display path is working; problem
     (if any) is in network/weather code.
5. If a panic happens, the `esp32_exception_decoder` monitor filter should
   resolve `PC` to a symbol. If it doesn't, run:
   `xtensa-esp32s3-elf-addr2line -pfiaC -e .pio/build/elecrow-rotary-128/firmware.elf <PC>`

## Conventions for this project

- Keep the firmware C++17, no exceptions, no RTTI (Arduino defaults).
- `log_i` / `log_w` / `log_e` for normal logging; `Serial.println` only for
  the boot diagnostics that need to fire before the logger is reliable.
- Don't add features beyond the spec (analog rim + digital center +
  day/date + weather background + JSON config) without asking the user.
- Don't introduce LVGL or any other UI framework — the procedural TFT_eSPI
  rendering is intentional.

## Useful references

- TFT_eSPI: <https://github.com/Bodmer/TFT_eSPI>
- Open-Meteo current weather: <https://open-meteo.com/en/docs>
- WMO weather codes: see comments in `src/weather.cpp` (`Weather::classify`)
- ESP32-S3 Arduino core PSRAM notes: `qio_opi` memory_type for OPI PSRAM
