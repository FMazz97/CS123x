# CS123x - Dual-Framework Arduino / ESP-IDF Library for Chipsea CS1237 & CS1238 24-bit Differential ADCs

Arduino and ESP-IDF library for Chipsea [CS1237](https://en.chipsea.com/product/details/?id=1155&pid=77) and [CS1238](https://en.chipsea.com/product/details/?id=1156&pid=77) 24-bit differential ADCs. Designed for weight scales, load cells, and bridge sensors with full PGA control, flexible sampling rates, two-point scale calibration, internal temperature monitoring, and internal short-circuit offset diagnostics.

Built on a single framework-agnostic C++ core: use the familiar camelCase `CS123x` facade on Arduino, or the snake_case `cs123x` class directly in native ESP-IDF (v5.x–6.x) projects — same logic, same protocol implementation, zero duplication between the two.

<p align="center">
  <img src="https://raw.githubusercontent.com/FMazz97/CS123x/main/assets/cs123x_modules.jpg" alt="Chipsea CS1237 and CS1238 Breakout Boards" width="550"><br>
  <sub>Purple breakout modules for CS1237 (left) and CS1238 (right).</sub>
</p>

---

## Why This Library?

This library was born out of the need for a high-refresh-rate ADC for dynamic analysis and in-motion measurements, where the most common converters don't offer enough speed.

CS123x aims to fill that gap, providing a modern alternative to popular but slow converters like the HX711, with more flexibility, advanced diagnostics, and sampling rates up to 1280 SPS.

At the time of writing, the documentation available online for the CS1237/CS1238 chips was fragmentary or incomplete, and no reliable Arduino driver existed for them.

### Key Advantages of the CS123x:

* **Precision Scale Readings:** Up to 20.7 effective bits (ENOB) for detecting small weight changes.
* **High-Speed Dynamic Weighing:** Sampling rates up to 1280 SPS enable accurate in-motion weighing (conveyor checkweighers), rapid force tracking, and responsive closed-loop control (PID loops).
* **Integrated Temperature Monitoring:** On-chip temperature sensor enables real-time software thermal drift compensation.
* **Advanced Diagnostics:** Built-in internal short mode allows precise zero-point offset calibration without disconnecting the sensor.
* **Full Software Control:** PGA gain (1x, 2x, 64x, 128x) and channel selection are fully programmable on-the-fly via software registers.

### CS123x vs HX711 Comparison

Both chips are 24-bit Sigma-Delta (Σ-Δ) ADCs designed for strain gauge sensors, but they target different application requirements:

| Feature | HX711 | CS123x Family (CS1237 / CS1238) |
| :--- | :--- | :--- |
| **Max Sampling Rate** | 10 or 80 SPS | **Up to 1280 SPS** (10, 40, 640, 1280 Hz) |
| **Effective Resolution (ENOB)** | ~18.5 Bits (at 10 Hz, Gain 128) | **Up to 20.7 Bits** (at 10 Hz, Gain 128) |
| **Temperature Diagnostics** | None | **Integrated On-Chip Temp Sensor** |
| **Offset Calibration Mode** | External zeroing | **Internal Short-Circuit Mode** |
| **Target Application** | Static weighing & low-cost scales | Dynamic checkweighing, fast process control, thermal compensation |

> **Note on Architecture & Application Scope:**
> Like most high-resolution scale ICs, the CS123x uses a **Sigma-Delta (Σ-Δ)** architecture with a digital filter (Sinc3). This makes it ideal for strain gauge load cells in dynamic weighing, material testing, and industrial process automation.
> For sub-millisecond impact or ballistic testing, dedicated **SAR ADCs** paired with **piezoelectric load cells** are typically required, even if it though at a significantly higher system cost and complexity. The CS123x delivers high-speed capability for low-cost strain gauge sensors at an accessible price point.

---

## Key Features

* **Dual Chip Support:** Native C++ driver for both Chipsea **CS1237** (single differential channel) and **CS1238** (2 differential channels) 24-bit ADCs.
* **Dual Framework:** One framework-agnostic core, usable natively from either the **Arduino** ecosystem or **ESP-IDF** (v5.x–6.x) — see [Compatibility](#compatibility).
* **Full ADC Configuration:** Runtime control of PGA Gain (1x-128x), Output Data Rate (10-1280 Hz), Channel Selection, and Reference Source. All parameters can be set individually or together via `setConfig()`/`getConfig()` and the `CS123X_Config` struct (supports `==`/`!=` comparison).
* **Dual-Channel Reads:** `readDualChannel()` interleaves reads across two channels without wasting a conversion cycle per switch. Useful for CS1238 dual-sensor setups, or CS1237 patterns like alternating `CS123X_CH_A`/`CS123X_CH_TEMP` for thermal compensation.
* **Voltage Readout:** `readVoltage()` converts raw ADC codes to the differential input voltage, accounting for the configured PGA gain and reference voltage.
* **Dual Execution Modes (Safe Blocking vs. Fast Non-Blocking):**
  * **Blocking with Hardware Verification (DEFAULT):** By default, methods like `read()`, `begin()`, and register setters operate safely in blocking mode with dynamic timeouts. Setters default to `verify = true`, reading back internal hardware registers to guarantee write success.
  * **Fast / Non-Blocking Mode:** For ultra-fast configuration or event-driven loops, register verification can be disabled by passing `verify = false` to register setters. Non-blocking polling can be built using `isReady()` and `readNow()` directly in your main loop or attach a hardware interrupt on the `DOUT` pin's falling edge (data-ready signal) instead of polling `isReady()`.
* **Watchdog-Safe Polling:** Internal wait loops cooperatively yield on ESP8266/ESP32/ESP-IDF (FreeRTOS-aware, tuned to avoid starving the IDLE task) while staying in tight polling during a chip's normal conversion window at 640/1280 Hz. No manual tuning required to keep both throughput and system stability.
* **Internal Temperature Sensing:** Seamless temperature measurements in °C (`readTemp()`), with automatic channel switching and gain restoration. `calibrateTemp()` reads a live reference point from the chip; `setTempCalibration()`/`getTempCalibration()` manage calibration parameters directly (e.g. for EEPROM persistence).
* **Internal Short-Circuit Diagnostics:** Switch to the on-chip short-circuit channel (`CS123X_CH_SHORT`) to measure zero-offset drift without physically disconnecting the sensor.
* **Weighing Engine:** Integrated tare zeroing (`tare()`), two-point factor calibration (`calibrateScale()`), and physical unit scaling (`readNetUnits()`), plus raw net counts via `readNetCounts()`.

> For the complete list of methods, parameters, and return values, see the fully Doxygen-documented [`cs123x.h`](https://github.com/FMazz97/CS123x/blob/main/src/core/cs123x.h) (API and configuration logic) and [`cs123x_types.h`](https://github.com/FMazz97/CS123x/blob/main/src/core/cs123x_types.h) (types and constants) headers.

---

## Compatibility

This library is built on a framework-agnostic C++ core: all GPIO, timing, and critical-section access goes through a small HAL layer, resolved at compile time for the active framework. No platform-specific code in the core logic itself.

### Arduino

The HAL bridges the core to the standard Arduino API (`digitalWrite()`, `digitalRead()`, `pinMode()`, `millis()`, `yield()`), making the library compatible out-of-the-box with any board featuring a functional Arduino core via the `CS123x` facade.

### ESP-IDF

The HAL bridges the core directly to native ESP-IDF (v5.x–6.x), leveraging framework HAL and FreeRTOS primitives for GPIO, timing, and critical sections via the `cs123x` class—with zero external dependencies required.

### Tested Microcontrollers:

Verified across both classic 8-bit AVR boards (5V logic) and 32-bit Espressif targets (3.3V logic), using the supported frameworks for each platform.:

* **Arduino AVR** (Arduino framework):
  * **ATmega328P** on [Arduino Uno Rev3](https://store.arduino.cc/products/arduino-uno-rev3) and [Arduino Nano](https://store.arduino.cc/collections/nano-family/products/arduino-nano)
  * **ATmega2560** on [Arduino Mega 2560 Rev3](https://store.arduino.cc/products/arduino-mega-2560-rev3)

* **Espressif ESP** (Arduino & ESP‑IDF framework):
  * **ESP32-WROOM-32** on [NodeMCU-32S V1.1](https://wiki.geekworm.com/NodeMCU-32S) and [Cheap Yellow Display (ESP32-2432S028)](https://github.com/witnessmenow/ESP32-Cheap-Yellow-Display)
  * **ESP32-S3** on [ESP32-S3-DevKitC-1](https://docs.espressif.com/projects/esp-dev-kits/en/latest/esp32s3/esp32-s3-devkitc-1/user_guide_v1.1.html)
  * **ESP32-C3** on [ESP32-C3 SuperMini](https://lastminuteengineers.com/esp32-c3-super-mini-pinout-reference/)
  * **ESP8266** on [NodeMCU V2](https://wiki.geekworm.com/NodeMcu_ESP8266) and [ESP-01S](https://www.instructables.com/How-to-use-the-ESP8266-01-pins)

> **Note:** All microcontrollers above were successfully tested using the Arduino [`TestExhaustive`](https://github.com/FMazz97/CS123x/blob/main/examples/TestExhaustive/TestExhaustive.ino) and the ESP‑IDF [`test_exhaustive`](https://github.com/FMazz97/CS123x/blob/main/idf_examples/test_exhaustive/main/main.cpp) examples.

---

## Installation

### Arduino IDE

[![Arduino Library Manager](https://www.ardu-badge.com/badge/CS123x.svg?)](https://www.ardu-badge.com/CS123x)

* **Via Library Manager (recommended):** Open the **Library Manager** (`Ctrl+Shift+I`), search for `CS123x`, and click **Install**.

* **Manual installation:** Download or clone this repository into your Arduino `libraries` folder (`Documents/Arduino/libraries/CS123x`), then restart the IDE.

### ESP-IDF

[![Component Registry](https://components.espressif.com/components/fmazz97/cs123x/badge.svg)](https://components.espressif.com/components/fmazz97/cs123x)

The library is published on [ESP Component Registry](https://components.espressif.com/components/fmazz97/cs123x) as `cs123x`, without any Arduino dependency.

* **Via `idf.py` (recommended):** In your project directory, run:
  ```bash
    idf.py add-dependency "fmazz97/cs123x^2.0.0"
  ```
  This adds `cs123x` as a dependency of the `main` component.
* **Manual:** add directly to `main/idf_component.yml`:
  ```YAML
  dependencies:
    fmazz97/cs123x: "^2.0.0"
  ```

* **Local development:** To work with a local copy of the library, clone the repository into a folder named `cs123x` in all lowercase:
  ```bash
  git clone https://github.com/FMazz97/CS123x.git cs123x
  ```
  > ESP‑IDF uses the folder name as the component name. The directory must be named cs123x in lowercase, otherwise the component will not be detected.

  Then, reference to the local component path directly in your project's `main/idf_component.yml` manifest:
  ```YAML
  dependencies:
    fmazz97/cs123x:
      version: "*"
      override_path: "../../path/to/cs123x"
  ```
  Finally, declare the dependency in `main/CMakeLists.txt`:
  ```CMake
  idf_component_register(
      SRCS "main.cpp"
      INCLUDE_DIRS "."
      REQUIRES cs123x
  )
  ```

### PlatformIO

[![PlatformIO Registry](https://badges.registry.platformio.org/packages/fmazz97/library/cs123x.svg)](https://registry.platformio.org/libraries/fmazz97/cs123x)

The library is published on the [PlatformIO Registry](https://registry.platformio.org/libraries/fmazz97/cs123x) and works with **both** `framework = arduino` and `framework = espidf`.

* **Via PlatformIO Registry (Arduino or ESP‑IDF — recommended):** Add it to your `platformio.ini` via `lib_deps`:
  ```ini
  ; Pin to a specific version for reproducible builds (recommended)
  lib_deps = fmazz97/cs123x@^2.0.0
  framework = ...
  ```
  This works for both frameworks without changes.

* **Using native ESP‑IDF inside PlatformIO:** in your `platformio.ini` declare:
  ```ini
  framework = espidf
  ```
  Then integrate the component using the native ESP‑IDF dependency system, exactly as described in the above [ESP‑IDF](#esp-idf-1) section.

  > For version pinning, alternative sources (Git, local folder), and other options, see the official guide on [declaring dependencies](https://docs.platformio.org/en/latest/librarymanager/dependencies.html#declaring-dependencies).

---

## Wiring & Pinout

The CS123x uses a custom 2-wire serial protocol over standard digital GPIO pins.

### Digital Pin Connections (MCU to ADC Module)

| Pin Symbol (PCB) | Description | MCU Connection |
| :--- | :--- | :--- |
| **VCC / DVDD** | Digital Power Supply (2.7V – 5.5V) | MCU 3.3V or 5V |
| **GND / DGND** | Digital Ground | MCU GND |
| **SCK / SCLK** | Serial Clock Input / Power-Down Control | Any Digital Output Pin |
| **DT / DOUT** | Bidirectional Data Line / Ready Signal | Any Digital GPIO Pin |

### Analog Pin Connections (Module to Sensor)

| Pin Symbol (PCB) | Description | Sensor (Eg. Load Cell) |
| :--- | :--- | :--- |
| **E+ / AVDD** | Bridge Excitation Voltage (+) | Red Wire (*E+*) |
| **E- / AGND** | Analog Ground (-) | Black Wire (*E-*) — Cable shield |
| **A+** | Channel A Non-Inverting Signal | Green Wire (*S+*) |
| **A-** | Channel A Inverting Signal | White Wire (*S-*) |
| **B+ / B-** | Channel B Differential Signal *(CS1238 only)* | Second Sensor Signal |

---

## Hardware Architecture & Voltage Reference

These breakout modules feature an onboard **TL431 precision shunt reference (2.5V)** to supply a low-noise analog voltage to the bridge excitation, effectively isolating sensitive weight measurements from digital MCU power supply noise.

### Voltage Reference Modes

The library defaults to the **External Reference** mode to match the out-of-the-box hardware configuration of the reference modules shown below.

* **External Reference (`CS123X_INT_REF_OFF`):**
  * The reference modules ship with the jumper pads (**R5** on CS1237 / **R6** on CS1238) **OPEN**. In this state, the module uses the onboard **TL431** to supply a clean **2.5V reference**.
  * Leaving the internal reference **OFF** avoids introducing unwanted noise, ripple, and measurement instability caused by two reference sources interacting on the `AVDD/E+` line.

* **Internal Reference (`CS123X_INT_REF_ON`):**
  * Enables the **internal reference generator** inside the **CS123x** chip, which drives `REFOUT` to output `DVDD/VCC` directly.
  * **Not recommended on stock hardware:** Closing the **R5/R6** solder pads ties `REFOUT` directly to the same `AVDD/E+` node already driven by the onboard **TL431**. Without removing it, the two sources would actively contend on that node instead of one cleanly replacing the other. On the reference modules, the **TL431** shares a trace with `AVDD/E+` that can't be isolated without PCB rework, so in practice this bridge should be left open.
<p align="center">
  <img src="https://raw.githubusercontent.com/FMazz97/CS123x/main/assets/cs123x_modules_details.jpg" alt="Chipsea CS1237 and CS1238 Breakout Boards" width="550"><br>
  <sub><strong>Reference Hardware Target:</strong> Purple breakout modules for CS1237 (left) and CS1238 (right) featuring an onboard TL431 precision voltage reference IC.</sub>
</p>

## ⚠️ Important: Current Limit for Low-Impedance Sensors

The onboard TL431 voltage reference is current-limited by resistor **R1 (1 kΩ)**. While sufficient for high-impedance sensors (≥ 1,7 kΩ), it cannot supply enough current for low/medium-impedance transducers such as standard **350 Ω full-bridge load cells**.

### Issue Summary
Any sensor connected to the `AVDD/E+` rail draws excitation current through **R1**. The reference node stays in regulation only as long as the current it can supply covers what the sensor and the **TL431** itself both need:

<p align="center">
  <i>I<sub>avail</sub> ≥ I<sub>total</sub></i>
</p>

**Current required by the sensor and the TL431's own bias:**

<p align="center">
  <i>I<sub>total</sub> = I<sub>sensor</sub> + I<sub>bias(TL431)</sub> = V<sub>REF</sub> / R<sub>sensor</sub> + ~1 mA</i>
</p>

**Current actually available from DVDD/VCC through R1:**

<p align="center">
  <i>I<sub>avail</sub> = (V<sub>DVDD/VCC</sub> - V<sub>REF</sub>) / R<sub>1</sub></i>
</p>

#### Example: Standard 350 Ω Load Cell
* **Sensor Demand:** 2.5 V / 350 Ω ≈ **7.1 mA**
* **Total Budget Needed:** 7.1 mA + 1.0 mA ≈ **8.1 mA**

However, the stock resistor **R1 = 1 kΩ** severely restricts the available current supplied from `DVDD/VCC`:
* **At 3.3 V:** I<sub>avail</sub> = (3.3 V - 2.5 V) / 1 kΩ = **0.8 mA** *(not enough)*
* **At 5.0 V:** I<sub>avail</sub> = (5.0 V - 2.5 V) / 1 kΩ = **2.5 mA** *(not enough)*

**Symptom:** The reference voltage collapses well below 2.5 V when a low-impedance load is connected, dropping below the CS123x minimum reference threshold (1.5 V) and causing saturated, noisy, or stuck ADC readings.

### ✅ Fix
Lower the effective series resistance by adding a resistor between the `DVDD/VCC` and `AVDD/E+` nodes or soldering a resistor directly in parallel with **R1** according to the formula above.

For standard **350 Ω load cells**, the recommended values are:

| DVDD/VCC Voltage | Parallel Resistor | Equivalent R1 | Available Current | Notes |
| :--- | :--- | :--- | :--- | :--- |
| **3.3V / 5.0V** | **100 Ω** | ~91 Ω | ~8.8mA / ~27.4mA | **Universal:** Works reliably for both 3.3V and 5V supply rails. |
| **5.0V Only** | **220 Ω** | ~180 Ω | ~13.9mA | **5V Rail Only:** Lower quiescent power for 5V supply rails. |

> **Note:** A lower R1 increases the TL431's steady-state current, and with it, its intrinsic noise even without a sensor connected. A bulk capacitor (10µF or larger) between `AVDD/E+` <=> `AGND/E-` is good practice in general, and becomes important once R1 is reduced: in testing, it cut the measured noise floor by roughly 5-6x.
>
> Use the formulas above with the sensor's resistance and supply voltage (for CS1238 the combined draw of both channels) to work out whether, and how much, R1 needs to be adjusted. Keeping an eye on the current/power the TL431 would need to dissipate if the sensor were ever disconnected.

---

## Usage Examples

### Basic Weight Measurement & Taring

The **`Simple Scale`** example (available for both frameworks) demonstrates the typical scale workflow:

1. **`Begin()`**: initializes the ADC and verifies the hardware configuration.
2. **`Tare(samples)`**: zeroes the scale with the platform empty, storing the offset.
3. **`Calibrate Scale(knownWeight, samples)`**: derives the scale factor from a known reference weight placed on the weight scale or load cell.
4. **`Read Net Units(samples)`**: continuously returns the net weight in physical units (`(raw - offset) / scale`), ready to print or log.

* **Arduino:**
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

* **ESP-IDF:**
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

## Credits & Acknowledgments

* **Hardware Design & Reference Schematics:** Special thanks to [yasir-shahzad](https://github.com/yasir-shahzad) for providing open-source hardware documentation, schematics, and module images (licensed under [GNU GPL v3](https://www.gnu.org/licenses/gpl-3.0.html)):
  * [CS1237 24-Bit ADC Module Repository](https://github.com/yasir-shahzad/CS1237-24-Bit-ADC-Module)
  * [CS1238 24-Bit ADC Module Repository](https://github.com/yasir-shahzad/CS1238-24-Bit-ADC-Module)

---

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.
