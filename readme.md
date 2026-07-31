# CS123x - Arduino Library for Chipsea CS1237 & CS1238 24-bit ADCs

Arduino library for Chipsea [CS1237](https://en.chipsea.com/product/details/?id=1155&pid=77) and [CS1238](https://en.chipsea.com/product/details/?id=1156&pid=77) 24-bit ADCs. Designed for weight scales, load cells, and bridge sensors with full PGA control, flexible sampling rates, two-point scale calibration, and internal temperature monitoring.

### Motivation

While the HX711 is a widely popular choice for basic static weighing applications, modern force-measurement projects often require higher sampling rates and advanced internal diagnostics. 

The Chipsea CS123x family bridges this gap by offering configurable output data rates up to **1280 SPS** to overcome HX711 80Hz limit, higher effective resolution (ENOB up to 20 bits), and built-in diagnostic features. 

This library provides a robust, production-grade C++ driver for the Arduino ecosystem, making it easy to harness the full potential of CS1237 and CS1238 ADCs in both hobbyist and industrial applications.

### CS123x vs. HX711 Comparison

Both chips are 24-bit Sigma-Delta (Σ-Δ) ADCs designed for strain gauge sensors, but they target different application requirements:

| Feature | HX711 | CS123x Family (CS1237 / CS1238) |
| :--- | :--- | :--- |
| **Max Sampling Rate** | 10 or 80 SPS | **Up to 1280 SPS** (10, 40, 640, 1280 Hz) |
| **Effective Resolution (ENOB)** | ~18.5 Bits (at 10 Hz, Gain 128) | **Up to 20.7 Bits** (at 10 Hz, Gain 128) |
| **Configurability** | Hardware pin strapping & clock timing | **2-Wire SPI Register Control** |
| **Temperature Diagnostics** | None | **Integrated On-Chip Temp Sensor** |
| **Offset Calibration Mode** | External zeroing | **Internal Short-Circuit Mode (`INT_SHORT`)** |
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

## Hardware Architecture & Voltage Reference

These popular breakout modules feature an onboard **TL431 precision shunt reference (2.5V)** to supply a low-noise analog voltage to the bridge excitation pin $AVDD$ or $+E$, effectively isolating sensitive weight measurements from digital MCU power supply noise.

### Voltage Reference Modes (`setRef`)

The library defaults to **`CS123X_INT_REF_OFF`** to match the out-of-the-box hardware configuration of commercial modules.

* **External Reference (`CS123X_INT_REF_OFF` - DEFAULT):**
  * **Factory default setting.** Commercial breakout modules ship with the jumper pads (**R5** on CS1237 / **R6** on CS1238) **OPEN**. In this state, the module uses the onboard TL431 to supply a clean 2.5V reference.
  * Leaving the internal reference OFF avoids introducing unwanted noise, ripple, and measurement instability caused by two reference sources interacting on the $AVDD$ line.

* **Internal Reference (`CS123X_INT_REF_ON`):**
  * Enables the internal reference generator inside the CS123x chip.
  * **Hardware Customization Note:** The open **R5/R6** solder pads are provided on the PCB for hardware versatility. Closing this bridge (with a 0Ω resistor or solder blob) bypasses the TL431 regulator (e.g., connecting $AVDD$ directly to $DVDD$). Enable `CS123X_INT_REF_ON` **only** if you have hardware-modified the module to bypass the external TL431 circuit.

<p align="center">
  <img src="assets/cs123x_modules.jpg" alt="Chipsea CS1237 and CS1238 Breakout Boards" width="550"><br>
  <sub><strong>Reference Hardware Target:</strong> Purple breakout modules for CS1237 (left) and CS1238 (right) featuring onboard TL431 precision reference circuit.</sub>
</p>

---

## Key Features

* **Dual Chip Support:** Native C++ driver for both Chipsea **CS1237** (single channel) and **CS1238** (2 differential channels) 24-bit ADCs.
* **Full ADC Configuration:** Runtime control over PGA Gain (1x to 128x), Output Data Rate (10 Hz to 1280 Hz), Channel Selection, and Reference Source.
* **Dual Execution Modes (Safe Blocking vs. Fast Non-Blocking):**
  * **Blocking with Hardware Verification (DEFAULT):** By default, methods like `read()`, `begin()`, and register setters operate safely in blocking mode with dynamic timeouts and cooperative `yield()` calls. Setters default to `verify = true`, reading back internal hardware registers to guarantee write success.
  * **Fast / Non-Blocking Mode:** For ultra-fast configuration or event-driven loops, register verification can be disabled by passing `verify = false` to setters or `begin()`. Non-blocking polling can be built using `isReady()` and `forceRead()` directly in your main loop.
* **Weighing Engine:** Integrated tare zeroing (`tare()`), two-point factor calibration (`calibrateScale()`), and physical unit scaling (`getUnits()`).
* **Internal Temperature Sensing:** Seamless temperature measurements in °C (`readTemperature()`), with automatic channel switching and gain restoration.

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
<!-- DO NOT SHOW UNTIL ARDUINO AND PLATFORMIO LIBRARY MANEAGER WAS UPTDATE
---

## Quick Start

### Installation

* **Arduino IDE:** Open the **Library Manager** (`Ctrl+Shift+I`), search for `CS123x`, and click **Install**.
* **PlatformIO:** Add the library to your `platformio.ini` project configuration:

```ini
lib_deps =
    CS123x
```-->

---

## Usage Examples

###  Basic Weight Measurement & Taring
This example initializes the CS123x ADC with safe default parameters, performs an automatic zero tare, and outputs scaled weight values.

```cpp
#include <Arduino.h>
#include "CS123x.h"

// Hardware Pin Configuration
#define DOUT_PIN 4
#define SCLK_PIN 5

// Reference weight used for calibration (e.g., 100.0 grams, kg, or lbs)
#define KNOWN_WEIGHT 100.0f

CS123x adc(CS123X_TYPE_CS1237, DOUT_PIN, SCLK_PIN);

void setup() {
  Serial.begin(115200);
  while (!Serial && millis() < 2000)
    ;

  Serial.println(F("\n================================================"));
  Serial.println(F("     CS123x SCALE CALIBRATION & MEASUREMENT     "));
  Serial.println(F("================================================"));

  // ---------------------------------------------------------------------------
  // 1. INITIALIZATION
  // ---------------------------------------------------------------------------
  Serial.println(F("\n[1] INITIALIZATION TEST (begin)"));
  while (!adc.begin()) {
    Serial.println(F("    [WARN] Initialization failed/timeout. Retrying in 500ms..."));
    delay(500);
  }
  Serial.println(F("    [OK] ADC initialized and configuration verified."));

  // ---------------------------------------------------------------------------
  // 2. TARE (ZERO CALIBRATION)
  // ---------------------------------------------------------------------------
  Serial.println(F("\n[2] TARE PROCEDURE"));
  Serial.println(F("    Ensure scale platform is completely empty..."));
  delay(2000);  // Time to remove any load

  if (adc.tare(10)) {  // Average across 10 readings
    Serial.print(F("    [OK] Tare successful! Offset saved: "));
    Serial.println(adc.getOffset());
  } else {
    Serial.println(F("    [FAIL] Tare failed due to hardware timeout."));
  }

  // ---------------------------------------------------------------------------
  // 3. SCALE FACTOR CALIBRATION
  // ---------------------------------------------------------------------------
  // Option A: Live automatic calibration on-the-fly
  Serial.println(F("\n[3] SCALE FACTOR CALIBRATION"));
  Serial.print(F("    Place your known weight ("));
  Serial.print(KNOWN_WEIGHT);
  Serial.println(F(" units) on scale..."));
  delay(5000);  // Time to place the weight

  if (adc.calibrateScale(KNOWN_WEIGHT, 10)) {
    Serial.print(F("    [OK] Calibration successful! Scale factor: "));
    Serial.println(adc.getScale());
  } else {
    Serial.println(F("    [FAIL] Calibration failed. Check load cell wiring/weight."));
  }

  /* 
    Option B: Restoring calibration parameters saved in EEPROM/Flash
    adc.setScale(123.4f); 
  */

  Serial.println(F("\n================================================"));
  Serial.println(F("      CALIBRATION COMPLETE - STARTING LOOP      "));
  Serial.println(F("================================================\n"));
}

void loop() {
  // Read weight in calibrated units (averaged over 3 samples)
  int32_t valueRaw = adc.getValue(3);   // Raw - Offset
  float weightUnits = adc.getUnits(3);  // (Raw - Offset) / Scale

  if (isnan(weightUnits)) {
    Serial.println(F("[ERROR] Hardware read timeout!"));
  } else {
    Serial.print(F("Net Counts: "));
    Serial.print(valueRaw, 0);
    Serial.print(F(" | Weight: "));
    Serial.print(weightUnits, 2);
    Serial.println(F(" units"));
  }

  adc.powerDown();
  delay(5000);
  adc.powerUp();
}
```
###  Other examples
See the [`examples/`](examples/) directory for complete, ready-to-run Arduino sketches.

---

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.
