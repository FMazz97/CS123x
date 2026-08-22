# CS123x - Arduino Library for Chipsea CS1237 & CS1238 24-bit ADCs

Arduino library for Chipsea [CS1237](https://en.chipsea.com/product/details/?id=1155&pid=77) and [CS1238](https://en.chipsea.com/product/details/?id=1156&pid=77) 24-bit ADCs. Designed for weight scales, load cells, and bridge sensors with full PGA control, flexible sampling rates, two-point scale calibration, internal temperature monitoring, and internal short-circuit offset diagnostics.

## Hardware Architecture & Voltage Reference

These breakout modules feature an onboard **TL431 precision shunt reference (2.5V)** to supply a low-noise analog voltage to the bridge excitation, effectively isolating sensitive weight measurements from digital MCU power supply noise.

### Voltage Reference Modes

The library defaults to the **External Reference** mode to match the out-of-the-box hardware configuration of the reference modules shown below.

* **External Reference (`CS123X_INT_REF_OFF`):**
  * The reference modules ship with the jumper pads (**R5** on CS1237 / **R6** on CS1238) **OPEN**. In this state, the module uses the onboard TL431 to supply a clean 2.5V reference.
  * Leaving the internal reference OFF avoids introducing unwanted noise, ripple, and measurement instability caused by two reference sources interacting on the $AVDD/E+$ line.

* **Internal Reference (`CS123X_INT_REF_ON`):**
  * Enables the internal reference generator inside the CS123x chip, which drives $REFOUT$ to output $DVDD/VCC$ directly.
  * **Not recommended on stock hardware:** Closing the **R5/R6** solder pads ties $REFOUT$ directly to the same $AVDD/E+$ node already driven by the onboard TL431. Without removing it, the two sources would actively contend on that node instead of one cleanly replacing the other. On the reference modules, the TL431 shares a trace with $AVDD/E+$ that can't be isolated without PCB rework, so in practice this bridge should be left open.
<p align="center">
  <img src="https://raw.githubusercontent.com/FMazz97/CS123x/main/assets/cs123x_modules.jpg" alt="Chipsea CS1237 and CS1238 Breakout Boards" width="550"><br>
  <sub><strong>Reference Hardware Target:</strong> Purple breakout modules for CS1237 (left) and CS1238 (right) featuring an onboard TL431 precision voltage reference IC.</sub>
</p>

## ⚠️ Important: Current Limit for Low-Impedance Sensors

The onboard TL431 voltage reference is current-limited by resistor **R1 (1 kΩ)**. While sufficient for high-impedance sensors ($\ge 1.7\text{ k}\Omega$), it cannot supply enough current for low/medium-impedance transducers such as standard **350 Ω full-bridge load cells**.

### Issue Summary
Any sensor connected to the $AVDD/E+$ rail draws excitation current through **R1**. The reference node stays in regulation only as long as the current it can supply covers what the sensor and the TL431 itself both need:

$$I_{\text{avail}} \geq I_{\text{total}}$$

**Current required by the sensor and the TL431's own bias:**

$$I_{\text{total}} = I_{\text{sensor}} + I_{\text{bias(TL431)}} = \frac{V_{\text{REF}}}{R_{\text{sensor}}} + \sim 1\text{ mA}$$

**Current actually available from DVDD/VCC through R1:**

$$I_{\text{avail}} = \frac{V_{\text{DVDD/VCC}} - V_{\text{REF}}}{R_1}$$

#### Example: Standard 350 Ω Load Cell
* **Sensor Demand:** $2.5\text{ V} / 350\ \Omega \approx 7.1\text{ mA}$
* **Total Budget Needed:** $7.1\text{ mA} + 1.0\text{ mA} \approx \mathbf{8.1\text{ mA}}$

However, the stock resistor **R1 = 1 kΩ** severely restricts the available current supplied from $DVDD/VCC$:
* **At DVDD/VCC = 3.3 V:** $I_{\text{avail}} = (3.3\text{ V} - 2.5\text{ V}) / 1\text{ k}\Omega = \mathbf{0.8\text{ mA}}$ *(insufficient)*
* **At DVDD/VCC = 5.0 V:** $I_{\text{avail}} = (5.0\text{ V} - 2.5\text{ V}) / 1\text{ k}\Omega = \mathbf{2.5\text{ mA}}$ *(insufficient)*

**Symptom:** The reference voltage collapses well below 2.5 V when a low-impedance load is connected, dropping below the CS123x minimum reference threshold (1.5 V) and causing saturated, noisy, or stuck ADC readings.

### ✅ Fix
Lower the effective series resistance by adding a resistor between the **DVDD/VCC** and **$AVDD/E+$** nodes or soldering a resistor directly in parallel with **R1** according to the formula above.

For standard **350 Ω load cells**, the recommended values are:

| DVDD/VCC Voltage | Parallel Resistor | Equivalent R1 | Available Current | Notes |
| :--- | :--- | :--- | :--- | :--- |
| **3.3V / 5.0V** | **100 Ω** | ~91 Ω | ~8.8mA at 3.3V<br>~27.4mA at 5.0V | **Universal:** Works reliably for both 3.3V and 5V supply rails. |
| **5.0V Only** | **220 Ω** | ~180 Ω | ~13.9mA at 5.0V | **5V Rail Only:** Lower quiescent power, but insufficient if running at 3.3V. |

> **Note:** Measure the input resistance across the power/excitation terminals (`AVDD/E+` <=> `AGND/E-`) with a multimeter if you are unsure of your sensor's impedance.
>
> **CS1238:** Channel A and Channel B share the same $AVDD/E+$ excitation rail. If using two sensors simultaneously, size the fix for their combined current draw.
>
> Use the formulas above with the sensor's resistance and supply voltage to work out whether, and how much, R1 needs to be adjusted — keeping an eye on the current/power the TL431 would need to dissipate if the sensor were ever disconnected.

---

## Key Features

* **Dual Chip Support:** Native C++ driver for both Chipsea **CS1237** (single channel) and **CS1238** (2 differential channels) 24-bit ADCs.
* **Full ADC Configuration:** Runtime control over PGA Gain (1x to 128x), Output Data Rate (10 Hz to 1280 Hz), Channel Selection, and Reference Source.
* **Dual Execution Modes (Safe Blocking vs. Fast Non-Blocking):**
  * **Blocking with Hardware Verification (DEFAULT):** By default, methods like `read()`, `begin()`, and register setters operate safely in blocking mode with dynamic timeouts and cooperative `yield()` calls preventing WatchDog Timer resets on ESP8266/ESP32 even across repeated calls (e.g. inside `readAverage()`). Setters default to `verify = true`, reading back internal hardware registers to guarantee write success.
  * **Fast / Non-Blocking Mode:** For ultra-fast configuration or event-driven loops, register verification can be disabled by passing `verify = false` to register setters. Non-blocking polling can be built using `isReady()` and `forceRead()` directly in your main loop or attach a hardware interrupt on the `DOUT` pin's falling edge (data-ready signal) instead of polling `isReady()`.
* **Internal Temperature Sensing:** Seamless temperature measurements in °C (`readTemperature()`), with automatic channel switching and gain restoration.
* **Internal Short-Circuit Diagnostics:** Switch to the on-chip short-circuit channel (`CS123X_CH_SHORT`) to measure zero-offset drift without physically disconnecting the sensor.
* **Weighing Engine:** Integrated tare zeroing (`tare()`), two-point factor calibration (`calibrateScale()`), and physical unit scaling (`getUnits()`).

> For the complete list of methods, parameters, and return values, see the fully Doxygen-documented [`CS123x.h`](https://github.com/FMazz97/CS123x/blob/main/src/CS123x.h) header.

### Tested Microcontrollers:

* **Arduino Uno Rev3** (ATmega328P - 5V logic)
* **Arduino Nano** (ATmega328P - 5V logic)
* **Arduino Mega 2560** (ATmega2560 - 5V logic)
* **ESP32-WROOM-32** on NodeMCU-32S V1.1 (ESP32 - 3.3V logic)
* **ESP32-WROOM-32** on the [Cheap Yellow Display (ESP32-2432S028)](https://github.com/witnessmenow/ESP32-Cheap-Yellow-Display) (ESP32 - 3.3V logic)
* **ESP32-S3** on ESP32-S3-DevKitC-1 (ESP32-S3 - 3.3V logic)
* **ESP32-C3** on ESP32-C3 SuperMini (ESP32-C3 - 3.3V logic)
* **ESP8266** on NodeMCU V2 (ESP8266 - 3.3V logic)
* **ESP8266** on ESP-01S (ESP8266 - 3.3V logic)

> **Note:** All ESP32/ESP8266 targets above were tested using the **Arduino framework**, as declared in the library manifest, using the [`TestExhaustive`](https://github.com/FMazz97/CS123x/blob/main/examples/TestExhaustive/TestExhaustive.ino) example sketch.

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

### Analog Pin Connections (Module to Load Cell)

| Pin Symbol (PCB) | Description | Load Cell Wire |
| :--- | :--- | :--- |
| **E+ / AVDD** | Bridge Excitation Voltage (+) | Red Wire ($E+$) |
| **E- / AGND** | Analog Ground (-) | Black Wire ($E-$) / Cable shield |
| **A+** | Channel A Non-Inverting Signal | Green / White Wire ($S+$) |
| **A-** | Channel A Inverting Signal | White / Green Wire ($S-$) |
| **B+ / B-** | Channel B Differential Signal *(CS1238 only)* | Second Load Cell Signal |

---

## Motivation

While the HX711 is a widely popular choice for basic static weighing applications, modern force-measurement projects often require higher sampling rates and advanced internal diagnostics.

The Chipsea CS123x family bridges this gap by offering configurable output data rates up to **1280 SPS** to overcome HX711 80Hz limit, higher effective resolution (ENOB up to 20 bits), and built-in diagnostic features.

This library provides a robust, production-grade C++ driver for the Arduino ecosystem, making it easy to harness the full potential of CS1237 and CS1238 ADCs in both hobbyist and industrial applications.

### CS123x vs. HX711 Comparison

Both chips are 24-bit Sigma-Delta (Σ-Δ) ADCs designed for strain gauge sensors, but they target different application requirements:

| Feature | HX711 | CS123x Family (CS1237 / CS1238) |
| :--- | :--- | :--- |
| **Max Sampling Rate** | 10 or 80 SPS | **Up to 1280 SPS** (10, 40, 640, 1280 Hz) |
| **Effective Resolution (ENOB)** | ~18.5 Bits (at 10 Hz, Gain 128) | **Up to 20.7 Bits** (at 10 Hz, Gain 128) |
| **Temperature Diagnostics** | None | **Integrated On-Chip Temp Sensor** |
| **Offset Calibration Mode** | External zeroing | **Internal Short-Circuit Mode** |
| **Target Application** | Static weighing & low-cost scales | Dynamic checkweighing, fast process control, thermal compensation |

#### Key Advantages of the CS123x:

* **High-Speed Dynamic Weighing:** Sampling rates up to 1280 SPS enable accurate in-motion weighing (conveyor checkweighers), rapid force tracking, and responsive closed-loop control (PID loops).
* **Integrated Temperature Monitoring:** On-chip temperature sensor enables real-time software thermal drift compensation.
* **Advanced Diagnostics:** Built-in internal short mode allows precise zero-point offset calibration without disconnecting the load cell.
* **Full Software Control:** PGA gain (1x, 2x, 64x, 128x) and channel selection are fully programmable on-the-fly via software registers.

> **Note on Architecture & Application Scope:**
> Like most high-resolution scale ICs, the CS123x uses a **Sigma-Delta (Σ-Δ)** architecture with a digital filter (Sinc3). This makes it ideal for strain gauge load cells in dynamic weighing, material testing, and industrial process automation.
> For sub-millisecond impact or ballistic testing, dedicated **SAR ADCs** paired with **piezoelectric load cells** are typically required—though at a significantly higher system cost and complexity. The CS123x delivers high-speed capability for low-cost strain gauge sensors at an accessible price point.

---

## Quick Start

### Installation

#### Arduino IDE

**Via Library Manager (recommended):**
Open the **Library Manager** (`Ctrl+Shift+I`), search for `CS123x`, and click **Install**.

**Manual installation:**
Download or clone this repository into your Arduino `libraries` folder
(`Documents/Arduino/libraries/CS123x`), then restart the IDE.

#### PlatformIO

**Via Registry (recommended):**
Add the library to your `platformio.ini` project configuration:
```ini
lib_deps =
    CS123x
```
To pin a specific version (recommended for reproducible builds):
```ini
lib_deps =
    CS123x@^1.0.3
```

**Manual installation:**
Reference the repository directly in `platformio.ini`, without going through
the registry:
```ini
lib_deps =
    https://github.com/FMazz97/CS123x.git
```
---

## Usage Examples

### Basic Weight Measurement & Taring

The `SimpleScale` example demonstrates the typical scale workflow:

1. **`begin()`** — initializes the ADC and verifies the hardware configuration.
2. **`tare(samples)`** — zeroes the scale with the platform empty, storing the offset.
3. **`calibrateScale(knownWeight, samples)`** — derives the scale factor from a known reference weight placed on the load cell.
4. **`getUnits(samples)`** — continuously returns the net weight in physical units (`(raw - offset) / scale`), ready to print or log.

```cpp
CS123x adc(CS123X_TYPE_CS1237, DOUT_PIN, SCLK_PIN);

adc.begin();
adc.tare(CALIBRATION_SAMPLES);
adc.calibrateScale(KNOWN_WEIGHT, CALIBRATION_SAMPLES);

// In loop():
float weight = adc.getUnits(SAMPLES);
```

See [`SimpleScale.ino`](https://github.com/FMazz97/CS123x/blob/main/examples/SimpleScale/SimpleScale.ino) for the full sketch, including serial diagnostics and error handling.

### Other examples
See the [`examples/`](https://github.com/FMazz97/CS123x/tree/main/examples) directory for complete, ready-to-run Arduino sketches.

---

## Credits & Acknowledgments

* **Hardware Design & Reference Schematics:** Special thanks to [yasir-shahzad](https://github.com/yasir-shahzad) for providing open-source hardware documentation, schematics, and module images (licensed under [GNU GPL v3](https://www.gnu.org/licenses/gpl-3.0.html)):
  * [CS1237 24-Bit ADC Module Repository](https://github.com/yasir-shahzad/CS1237-24-Bit-ADC-Module)
  * [CS1238 24-Bit ADC Module Repository](https://github.com/yasir-shahzad/CS1238-24-Bit-ADC-Module)

---

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.
