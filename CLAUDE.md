# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

An LBJ (Chinese Railway Train Proximity Alarm) message receiver for the TTGO LoRa 32 v1.6.1 dev board (ESP32 + SX1276). Receives POCSAG protocol messages at 821.2375 MHz using the SX1276's FSK modem in direct mode. Displays formatted messages on OLED, logs to SD card (plain text + CSV), and serves decoded data over Telnet.

## Build System

PlatformIO with Arduino framework for ESP32. The board is `esp32dev` with build flag `-DLILYGO_T3_V1_6`.

```bash
# Build
pio run

# Build and upload
pio run --target upload

# Monitor serial (115200 baud)
pio device monitor

# Build, upload, and monitor
pio run --target upload && pio device monitor
```

VS Code with PlatformIO IDE extension is the intended development environment. The `.vscode/` config is already set up.

## Key Build Flags (platformio.ini)

- `-DLILYGO_T3_V1_6` — selects the board variant pin mapping in `src/utilities.h`
- Commented flags for core debug level and log level
- `board_build.partitions = huge_app.csv` — uses the huge app partition table

## Architecture

### Data Receive Pipeline

1. **SX1276 radio** (FSK direct mode, `Module` with DIO2 for direct output) captures RF at 821.2375 MHz + ppm offset
2. **RadioLib `PagerClient`** (locally modified, see `src/PagerMod.cpp` and `lib/RadioLib/`) decodes POCSAG frames with BCH(31,21) error correction from `src/BCH3121.cpp`
3. **`readDataLBJ()`** in `src/networks.cpp` parses three POCSAG message types by address:
   - Address `1234000` — Type 0: standard proximity alarm (train number, speed, kilometer post)
   - Address `1234002` — Type 1: extended info (locomotive class/registry number, GB2312-encoded route name, GPS coordinates)
   - Address `1234008` — Type 2: time sync
4. **Output** goes to four sinks: OLED display (via U8g2), SD card log (plain text), SD card CSV, and Telnet clients. Serial receives the same output.

### Source File Roles

| File | Purpose |
|------|---------|
| `src/main.cpp` | Entry point. `setup()` initializes board, WiFi, NTP, radio, SD, Telnet. `loop()` runs AFC (carrier/preamble frequency correction), dispatches received data to a FreeRTOS formatting task (`formatDataTask`), handles serial commands, manages CPU frequency (240→80 MHz idle). |
| `src/networks.hpp/.cpp` | WiFi connection (with optional SmartConfig), NTP time sync, Telnet server setup and command dispatch, LBJ message parsing (`readDataLBJ`), all three output formatters (`printDataSerial`, `appendDataLog`, `appendDataCSV`, `printDataTelnet`), GBK-to-UTF8 conversion, BCD re-encoding. |
| `src/boards.hpp/.cpp` | Hardware init: SPI, I2C, OLED (SSD1306 via U8g2), SD card (HSPI), PMU (AXP192/AXP2101), RTC (DS3231), battery ADC. Provides `millis64()` (64-bit millis from `esp_timer_get_time`). |
| `src/utilities.h` | Board-variant pin definitions selected by preprocessor (`LILYGO_T3_V1_6` etc.). Also defines feature flags like `HAS_DISPLAY`, `HAS_SDCARD`, `INITIAL_PPM`, `AFC_ENABLE`. |
| `src/sdlog.hpp/.cpp` | `SD_LOG` class: log file rotation with INDEX tracking, buffered writes, CSV column headers, size checking. Logs go to `/LOGTEST/LOG_NNNN.txt`, CSV to `/CSVTEST/CSV_NNNN.csv`. |
| `src/PagerMod.cpp` | RadioLib `PagerClient` overrides: `readDataMSA` (multi-structure-array read), `readDataMA` with BCH validation, `gotPreambleState`/`gotCarrierState` for AFC trigger. |
| `src/BCH3121.cpp/.hpp` | BCH(31,21) error correction ported from MMDVM_HS_Hat's POCSAG_HS. |
| `src/loco.h` | Static lookup table mapping 3-digit locomotive number prefixes to Chinese locomotive type names (e.g., "东风4", "韶山8", "CR400AF"). |
| `src/unicon.cpp/.hpp` | GBK (CP936) to Unicode mapping (`ff_oem2uni`) and Unicode to UTF-8 encoding. |

### FreeRTOS Task Model

The `loop()` runs on core 1 (Arduino main loop). When 2+ POCSAG batches are available, `formatDataTask` is spawned pinned to core 1 via `xTaskCreatePinnedToCore`. It decodes the message and formats all outputs, then deletes itself. A 750ms timeout terminates the task if it hangs. If task creation fails due to OOM, `simpleFormatTask()` runs synchronously as a fallback.

### Automatic Frequency Correction (AFC)

The SX1276 module lacks a TCXO, so AFC compensates for crystal drift:
- **Carrier phase**: Measures FEI on carrier detection; applies correction if FEI > 1 kHz and consecutive readings are consistent.
- **Preamble phase**: Same logic during preamble, averaging FEI over up to 32 samples.
- Frequency reverts to the last known good frequency on carrier/preamble timeout (700ms / 600ms).
- Disable via serial command `afc off` or telnet command `afc off`.

## Modified Library: RadioLib

The project ships a locally modified RadioLib in `lib/RadioLib/`. The key modification is in `src/ArduinoHal.cpp` (currently in git diff). RadioLib's `PagerClient` class is further extended in `src/PagerMod.cpp`. Do not replace `lib/RadioLib/` with upstream without porting these modifications.

## Pin Mapping (TTGO LoRa 32 v1.6)

See `src/utilities.h` under `LILYGO_T3_V1_6`:
- SX1276: SPI on 5/19/27, CS=18, DIO0=26, RST=23, DIO1=33, BUSY=32
- SD card: HSPI on 14/2/15, CS=13
- OLED: I2C on 21/22
- LED: GPIO 25 (active HIGH)
- Battery ADC: GPIO 35
- External RTC (optional): shares I2C bus (21/22), enable with `#define HAS_RTC`

## Optional Features (edit `src/utilities.h`)

- `HAS_RTC` — external DS3231 on I2C for battery-backed time
- `HAS_OLED_TIMEOUT` — OLED sleep after 60s without received messages
- `USE_SMARTCONFIG` — ESP32 SmartConfig WiFi provisioning instead of hardcoded credentials
