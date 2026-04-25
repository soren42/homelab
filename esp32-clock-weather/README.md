# ESP32 Rotary 1.28" Clock + Weather

A minimalist clock and weather face for the **ELECROW ESP32-C3 Rotary IPS
Display 1.28"** (240x240 round, GC9A01).

- Analog hands sweep around the rim
- Large digital time + day/date in the center
- Weather is shown via a procedurally-rendered background (sun, clouds, rain,
  snow, fog, storm, partly cloudy, day/night)
- Time from NTP, weather from [Open-Meteo](https://open-meteo.com) (no API key)
- All settings come from `data/config.json` (LittleFS)

## Hardware

**ELECROW 1.28" Rotary IPS Display** — ESP32-S3R8 (Xtensa LX7 dual-core
240 MHz, 512 KB SRAM + 8 MB OPI PSRAM, 8 MB QIO flash) + GC9A01 240×240
round display.

Default pin map (overridable via `-DTFT_*` flags in `platformio.ini`):

| Function | GPIO |
|----------|------|
| TFT MOSI | 11   |
| TFT SCLK | 12   |
| TFT CS   | 10   |
| TFT DC   | 8    |
| TFT RST  | tied to system reset |
| TFT BL   | 2    |

If the screen stays blank after a successful build/upload, double-check
these against your board's schematic and update the build flags.

## Build & Flash

Requires [PlatformIO](https://platformio.org/).

```bash
cd esp32-clock-weather

# 1. edit data/config.json with your WiFi / location / units
# 2. flash firmware
pio run -t upload

# 3. flash the LittleFS image (config.json)
pio run -t uploadfs

# 4. (optional) watch logs
pio device monitor
```

After both `upload` and `uploadfs`, reset the board.

## `config.json`

```json
{
  "wifi":    { "ssid": "...", "password": "..." },
  "time":    { "timezone": "America/New_York", "ntp_server": "pool.ntp.org" },
  "weather": { "latitude": 40.7128, "longitude": -74.0060,
               "units": "imperial", "refresh_minutes": 15 },
  "display": { "show_seconds": true, "24_hour": false }
}
```

- `timezone` — IANA name. The firmware maps it to a POSIX TZ string for the
  ESP32 SNTP layer (DST-aware). See `src/config.cpp` for the supported list;
  add entries there if your zone is missing.
- `units` — `imperial` (°F, mph) or `metric` (°C, km/h).
- `refresh_minutes` — how often to re-fetch from Open-Meteo.

## Project layout

```
esp32-clock-weather/
├── platformio.ini      # build config + TFT_eSPI pin overrides
├── partitions.csv      # 4MB layout with LittleFS partition
├── data/config.json    # uploaded to LittleFS via `pio run -t uploadfs`
└── src/
    ├── main.cpp        # boot + loop
    ├── config.{h,cpp}  # JSON config loader + IANA→POSIX TZ map
    ├── net.{h,cpp}     # WiFi + NTP
    ├── weather.{h,cpp} # Open-Meteo client
    └── ui.{h,cpp}      # render: face, hands, digital, weather backgrounds
```

## Notes

- The face renders into a 240×240 16bpp sprite for flicker-free updates. If
  the sprite allocation fails (low heap), the UI silently falls back to direct
  drawing.
- Open-Meteo is called over HTTPS with TLS verification disabled
  (`setInsecure()`); fine for a non-secret weather lookup, but if you want
  pinned certs, supply a root CA in `weather.cpp`.
- WiFi auto-reconnects in the background; the face keeps ticking on stale data
  during outages.
