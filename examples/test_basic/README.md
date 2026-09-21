# cs123x Basic API & Hardware Test Example

This example provides a streamlined functional verification test suite for the core features of the **cs123x** component. It tests hardware initialization, configuration setters with verification, tare operation, scale calibration, temperature sensor calibration, power management, and continuous data acquisition.

---

## 🚀 Features Demonstrated

1. **ADC Initialization & Hardware Sync:** Initializes GPIO lines, writes the default or user-provided configuration to the ADC and verifies correct register read-back from it.
2. **Configuration Setters & Getters:** Validates `register configuration` methods (gain, output rate, channel selection, internal reference) with hardware read-back verification.
8. **Offset, Scale & Calibration Methods:** Tests manual setters/getters, tare operation, and live scale factor calculation.
9. **Temperature Sensor Calibration & Read:** Demonstrates manual and on-the-fly temperature calibration and physical temperature acquisition.
10. **Power Management:** Verifies sleep mode cycles via `power_down()` and `power_up()`.
11. **Continuous Data Reading:** Continuous measurement loop demonstrating raw values, moving averages, net counts, physical units, differential voltage conversion and temperature acquisition.

---

## 🛠️ Hardware Requirements

* Any **ESP32** development board.
* **CS1237** or **CS1238** ADC module connected to a scale / load cell / bridge sensor.

---

## ⚠️ Important Hardware Considerations

### Current Limit for Low-Impedance Sensors

Stock CS1237/CS1238 modules are current-limited by resistor **R1 (1 kΩ)**. For low-impedance sensors (such as **350 Ω load cells**), reduce R1 by adding an external resistor between **DVDD** and **AVDD**.

For full hardware details, visit the [`Current Limit for Low-Impedance Sensors`](https://github.com/FMazz97/CS123x#%EF%B8%8F-important-current-limit-for-low-impedance-sensors) section on [`CS123x Repository`](https://github.com/FMazz97/CS123x).

---

## ⚙️ Building & Flashing

This example already includes both an ESP-IDF-native `CMakeLists.txt`/`main/idf_component.yml` and a ready-to-use `platformio.ini` — no manual setup needed for either toolchain. Pick whichever you already have installed.

> **Note:** If you switch between ESP-IDF (`idf.py`) and PlatformIO on the same folder, run `idf.py fullclean` (or delete the `build/` directory) first — the two toolchains don't share build caches and will conflict otherwise.

### Option A: ESP-IDF (native, `idf.py`)

```bash
idf.py build
idf.py -p <PORT> flash monitor
```

### Option B: ESP-IDF on PlatformIO

1. Open this folder in VS Code with the PlatformIO extension installed.
2. Click **Build**, then **Upload**, then **Monitor**.

---

## 📝 Usage Steps

When the program boots:

1. **Setup Tests:** The test suite will automatically run through steps `[1]` to `[5]`, logging `['RESULT']` and diagnostic output for hardware API function.

2. Once setup tests finish, the program enters a continuous loop at step `[6]`, printing single-sample raw counts, averaged counts, net counts, units, computed voltage and internal temperature every second.

---

## 📄 License

Distributed under the MIT License. See [LICENSE](https://github.com/FMazz97/CS123x/blob/main/LICENSE) for more information.