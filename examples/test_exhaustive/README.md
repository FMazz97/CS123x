# CS123x Exhaustive API & Hardware Test Example

This example provides a comprehensive hardware verification and API test suite for the **cs123x** component. It systematically tests all configuration parameters, gain settings, sample rates, channels, internal reference modes, temperature calibration, dual-channel reading, and power management sequences.

---

## 🚀 Features Demonstrated

1. **ADC Initialization & Hardware Sync:** Initializes GPIO lines, writes the default or user‑provided configuration to the ADC and verifies correct register read‑back from it.
2. **Exhaustive Gain Test:** Sweeps through all 4 valid gain settings (`GAIN_1`, `GAIN_2`, `GAIN_64`, `GAIN_128`) and verifies hardware write/readback consistency.
3. **Exhaustive Data Rate Test:** Cycles through all 4 supported data output rates (`10Hz`, `40Hz`, `640Hz`, `1280Hz`) and validates timeout handling.
4. **Exhaustive Channel Test:** Verifies switching between all available input channels (`CH_A`, `CH_B` (CS1238 only), `CH_TEMP`, `CH_SHORT`).
5. **Internal Reference Test:** Toggles internal reference configuration (`REF_ON`, `REF_OFF`) — only for software test, see [`Hardware Architecture & Voltage Reference`](https://github.com/FMazz97/CS123x#hardware-architecture--voltage-reference) for more information.
6. **Batch Configuration:** Validates bulk updates via `set_config()` and configuration snapshotting via `get_config()`.
7. **Dual-Channel Interleaved Reading:** Reads from two distinct channels in a single interleaved sequence using `read_dual_channel()` function to avoid a wasted conversion cycle per switch, unlike calling `read()` and `set_ch()` separately.
8. **Offset, Scale & Calibration Methods:** Tests manual setters/getters, tare operation, and live scale factor calculation.
9. **Temperature Sensor Calibration & Read:** Demonstrates manual and on-the-fly temperature calibration and physical temperature acquisition.
10. **Power Management:** Verifies sleep mode cycles via `power_down()` and `power_up()`.
11. **Continuous Data Reading:** Continuous measurement loop demonstrating raw values, moving averages, net counts, physical units, differential voltage conversion and temperature acquisition.

---

## 🛠️ Hardware Requirements

- Any **ESP32** development board.
- **CS1237** or **CS1238** ADC module connected to a scale / load cell / bridge sensor.

---

## ⚠️ Important Hardware Considerations

### Current Limit for Low-Impedance Sensors

Stock CS1237/CS1238 modules are current-limited by resistor **R1 (1 kΩ)**. For low-impedance sensors (such as **350 Ω load cells**), reduce R1 by adding an external resistor between **DVDD** and **AVDD**.

For full hardware details, visit the [Current Limit for Low-Impedance Sensors](https://github.com/FMazz97/CS123x#%EF%B8%8F-important-current-limit-for-low-impedance-sensors) section on [CS123x Repository](https://github.com/FMazz97/CS123x).

---

## ⚙️ Building & Flashing

Examples downloaded directly from the registries include only the files required for the selected ecosystem:

- **ESP Component Registry** packages contain all necessary project files and the `idf_component.yml` manifest required for native ESP-IDF dependency resolution.
- **PlatformIO Registry** packages contain all necessary project files and the `platformio.ini` entry point required for PlatformIO initialization and dependency resolution (`lib_deps`).

In both cases, the example is ready to build without additional setup.

> **Note:** If you switch between ESP-IDF (`idf.py`) and PlatformIO on the same local checkout, run `idf.py fullclean` (or delete the `build/` directory) first. The two toolchains do not share build caches and may conflict otherwise.

### Option A: ESP-IDF (native, `idf.py`)

- Download the example project from the ESP Registry by:

  ```bash
  idf.py create-project-from-example "fmazz97/cs123x:test_exhaustive"
  ```

- Then **Build Project**, **Flash Device** and finally **Monitor Device**.

### Option B: ESP-IDF on PlatformIO

- Download the example project from the PlatformIO Registry or directly [by clicking here](https://dl.registry.platformio.org/download/fmazz97/library/cs123x/2.0.4/example/ESP-IDF_test_exhaustive.zip).

- Then **Build**, **Upload** and finally **Serial Monitor**.

---

## 📝 Usage Steps

When the program boots:

1. The test suite will automatically run through steps `[1]` to `[10]`, logging `['RESULT']` and/or diagnostic output for each hardware API function.

2. Once setup tests finish, the program enters a continuous loop at step `[11]`, printing single-sample raw counts, averaged counts, net counts, units, computed voltage and internal temperature every second.

---

## 📄 License

This example is distributed under the same MIT License as the [CS123x](https://github.com/FMazz97/CS123x) library. See [LICENSE](https://github.com/FMazz97/CS123x/blob/main/LICENSE) for more for details.
