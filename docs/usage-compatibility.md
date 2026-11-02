# Usage & Compatibility

[← Back to README](https://github.com/FMazz97/CS123x/blob/main/README.md)

## Key Features

- **Dual Chip Support:** Native C++ driver for both Chipsea **CS1237** (single differential channel) and **CS1238** (2 differential channels) 24-bit ADCs.
- **Dual Framework:** One framework-agnostic core, usable natively from either the **Arduino** ecosystem or **ESP-IDF** (v5.x–6.x).
- **Full ADC Configuration:** Runtime control of PGA Gain (1x-128x), Output Data Rate (10-1280 Hz), Channel Selection, and Reference Source. All parameters can be set individually or together via `setConfig()`/`getConfig()` and the `CS123X_Config` struct (supports `==`/`!=` comparison).
- **Dual-Channel Reads:** `readDualChannel()` interleaves reads across two channels without wasting a conversion cycle per switch. Useful for CS1238 dual-sensor setups, or CS1237 patterns like alternating `CS123X_CH_A`/`CS123X_CH_TEMP` for thermal compensation.
- **Voltage Readout:** `readVoltage()` converts raw ADC codes to the differential input voltage, accounting for the configured PGA gain and reference voltage.
- **Dual Execution Modes (Safe Blocking vs. Fast Non-Blocking):**
  - **Blocking with Hardware Verification (DEFAULT):** By default, methods like `read()`, `begin()`, and register setters operate safely in blocking mode with dynamic timeouts. Setters default to `verify = true`, reading back internal hardware registers to guarantee write success.
  - **Fast / Non-Blocking Mode:** For ultra-fast configuration or event-driven loops, register verification can be disabled by passing `verify = false` to register setters. Non-blocking polling can be built using `isReady()` and `readNow()` directly in your main loop or attach a hardware interrupt on the `DOUT` pin's falling edge (data-ready signal) instead of polling `isReady()`.
- **Watchdog-Safe Polling:** Internal wait loops cooperatively yield on ESP8266/ESP32/ESP-IDF (FreeRTOS-aware, tuned to avoid starving the IDLE task) while staying in tight polling during a chip's normal conversion window at 640/1280 Hz. No manual tuning required to keep both throughput and system stability.
- **Internal Temperature Sensing:** Seamless temperature measurements in °C (`readTemp()`), with automatic channel switching and gain restoration. `calibrateTemp()` reads a live reference point from the chip; `setTempCalibration()`/`getTempCalibration()` manage calibration parameters directly (e.g. for EEPROM persistence).
- **Internal Short-Circuit Diagnostics:** Switch to the on-chip short-circuit channel (`CS123X_CH_SHORT`) to measure zero-offset drift without physically disconnecting the sensor.
- **Weighing Engine:** Integrated tare zeroing (`tare()`), two-point factor calibration (`calibrateScale()`), and physical unit scaling (`readNetUnits()`), plus raw net counts via `readNetCounts()`.

> [!TIP]
> For the complete list of methods, parameters, and return values, see the fully Doxygen-documented [`cs123x.h`](https://github.com/FMazz97/CS123x/blob/main/src/core/cs123x.h) (API and configuration logic) and [`cs123x_types.h`](https://github.com/FMazz97/CS123x/blob/main/src/core/cs123x_types.h) (types and constants) headers.

---

## Usage Examples

### Basic Weight Measurement & Taring

The **`Simple Scale`** example (available for both frameworks) demonstrates the typical scale workflow:

1. **`begin()`**: initializes the ADC and verifies the hardware configuration.
2. **`tare(samples)`**: zeroes the scale with the platform empty, storing the offset.
3. **`calibrateScale(knownWeight, samples)`**: derives the scale factor from a known reference weight placed on the weight scale or load cell.
4. **`readNetUnits(samples)`**: continuously returns the net weight in physical units (`(raw - offset) / scale`), ready to print or log.

On ESP-IDF the same methods use snake_case names (`calibrate_scale()`, `read_net_units()`).

- **Arduino:**

  ```cpp
  CS123x adc(CS123X_TYPE_CS1237, DOUT_PIN, SCLK_PIN);

  // in setup():
  adc.begin();
  adc.tare(CALIBRATION_SAMPLES);
  adc.calibrateScale(KNOWN_WEIGHT, CALIBRATION_SAMPLES);

  // In loop():
  float weight = adc.readNetUnits(SAMPLES);
  ```

  See [`SimpleScale.ino`](https://github.com/FMazz97/CS123x/blob/main/examples/SimpleScale/SimpleScale.ino) for the full sketch, including serial diagnostics and error handling.

- **ESP-IDF:**

  ```cpp
  static cs123x adc(CS123X_TYPE_CS1237, DOUT_PIN, SCLK_PIN);

  // In app_main():
  adc.begin();
  adc.tare(CALIBRATION_SAMPLES);
  adc.calibrate_scale(KNOWN_WEIGHT, CALIBRATION_SAMPLES);

  // In the measurement loop:
  float weight = adc.read_net_units(SAMPLES);
  ```

  See [`examples/simple_scale`](https://github.com/FMazz97/CS123x/tree/main/examples/simple_scale) for the full project, including logging and error handling.

### Other examples

See the [`examples/`](https://github.com/FMazz97/CS123x/tree/main/examples) directory for complete, ready-to-run Arduino sketches (`PascalCaseExample/PascalCaseExample.ino`) & ESP-IDF projects (`snake_case_folder_example/`).

---

## Compatibility

This library is built on a framework-agnostic C++ core: all GPIO, timing, and critical-section access goes through a small HAL layer, resolved at compile time for the active framework. No platform-specific code in the core logic itself.

### Arduino

The HAL bridges the core to the standard Arduino API (`digitalWrite()`, `digitalRead()`, `pinMode()`, `millis()`, `yield()`), making the library compatible out-of-the-box with any board featuring a functional Arduino core via the `CS123x` facade.

### ESP-IDF

The HAL bridges the core directly to native ESP-IDF (v5.x–6.x), leveraging framework HAL and FreeRTOS primitives for GPIO, timing, and critical sections via the `cs123x` class—with zero external dependencies required.

### Tested Microcontrollers

Verified across both classic 8-bit AVR boards (5V logic) and 32-bit Espressif targets (3.3V logic), using the supported frameworks for each platform:

- **Arduino AVR** (Arduino framework):
  - **ATmega328P** on [Arduino Uno Rev3](https://store.arduino.cc/products/arduino-uno-rev3) and [Arduino Nano](https://store.arduino.cc/collections/nano-family/products/arduino-nano)
  - **ATmega2560** on [Arduino Mega 2560 Rev3](https://store.arduino.cc/products/arduino-mega-2560-rev3)

- **Espressif ESP:**
  - **ESP32** (Arduino & ESP‑IDF framework):
    - **ESP32-WROOM-32** on [NodeMCU-32S V1.1](https://wiki.geekworm.com/NodeMCU-32S) and [Cheap Yellow Display (ESP32-2432S028)](https://github.com/witnessmenow/ESP32-Cheap-Yellow-Display)
    - **ESP32-S3** on [ESP32-S3-DevKitC-1](https://docs.espressif.com/projects/esp-dev-kits/en/latest/esp32s3/esp32-s3-devkitc-1/user_guide_v1.1.html)
    - **ESP32-C3** on [ESP32-C3 SuperMini](https://lastminuteengineers.com/esp32-c3-super-mini-pinout-reference/)

  - **ESP8266** (Arduino framework):
    - **ESP8266** on [NodeMCU V2](https://wiki.geekworm.com/NodeMcu_ESP8266) and [ESP-01S](https://www.instructables.com/How-to-use-the-ESP8266-01-pins)

> [!NOTE]
> All microcontrollers above were successfully tested using the Arduino [`TestExhaustive`](https://github.com/FMazz97/CS123x/blob/main/examples/TestExhaustive/TestExhaustive.ino) and the ESP‑IDF [`test_exhaustive`](https://github.com/FMazz97/CS123x/blob/main/examples/test_exhaustive/main/main.cpp) examples.
