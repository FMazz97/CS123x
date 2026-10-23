# CS123x Basic API & Hardware Test Example

This example provides a streamlined functional verification test suite for the core features of the **cs123x** component. It tests hardware initialization, configuration setters with verification, tare operation, scale calibration, temperature sensor calibration, power management, and continuous data acquisition.

---

## 🚀 Features Demonstrated

1. **ADC Initialization & Hardware Sync:** Initializes GPIO lines, writes the default or user-provided configuration to the ADC and verifies correct register read-back from it.
2. **Configuration Setters & Getters:** Validates `register configuration` methods (gain, output rate, channel selection, internal reference) with hardware read-back verification.
3. **Offset, Scale & Calibration Methods:** Tests manual setters/getters, tare operation, and live scale factor calculation.
4. **Temperature Sensor Calibration & Read:** Demonstrates manual and on-the-fly temperature calibration and physical temperature acquisition.
5. **Power Management:** Verifies sleep mode cycles via `power_down()` and `power_up()`.
6. **Continuous Data Reading:** Continuous measurement loop demonstrating raw values, moving averages, net counts, physical units, differential voltage conversion and temperature acquisition.

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
  idf.py create-project-from-example "fmazz97/cs123x:test_basic"
  ```

- Then **Build Project**, **Flash Device** and finally **Monitor Device**.

### Option B: ESP-IDF on PlatformIO

- Download the example project from the PlatformIO Registry or directly [by clicking here](https://dl.registry.platformio.org/download/fmazz97/library/cs123x/2.0.4/example/ESP-IDF_test_basic.zip).

- Then **Build**, **Upload** and finally **Serial Monitor**.

---

## 📝 Usage Steps

When the program boots:

1. **Setup Tests:** The test suite will automatically run through steps `[1]` to `[5]`, logging `['RESULT']` and diagnostic output for hardware API function.

2. Once setup tests finish, the program enters a continuous loop at step `[6]`, printing single-sample raw counts, averaged counts, net counts, units, computed voltage and internal temperature every second.

---

## 📄 License

This example is distributed under the same MIT License as the [CS123x](https://github.com/FMazz97/CS123x) library. See [LICENSE](https://github.com/FMazz97/CS123x/blob/main/LICENSE) for more for details.
