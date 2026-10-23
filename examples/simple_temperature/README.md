# CS123x Simple Temperature Calibration & Reading Example

This example demonstrates how to use the **cs123x** component to perform single-point calibration on the chip's internal temperature sensor and continuously acquire physical temperature readings in degrees Celsius (°C).

---

## 🚀 Features Demonstrated

1. **ADC Initialization & Hardware Sync**:  Initializes GPIO lines, writes the default or user‑provided configuration to the ADC and verifies correct register read‑back from it.
2. **Single-Point Live Calibration:** Calibrates the internal temperature sensor using on-the-fly ambient temperature sampling via `calibrate_temp()`.
3. **Optimized Temperature Acquisition:** The ADC initializes directly on `CS123X_CH_TEMP` and acquires physical temperature readings in °C via `read_temp()` without any channel switching or register overhead.
4. **Continuous Monitoring:** Main loop sampling physical temperature averaged across multiple readings every second.

---

## 🛠️ Hardware Requirements

- Any **ESP32** development board.
- CS1237 or CS1238 ADC module connected to a scale / load cell / bridge sensor.
- External reference weight (e.g., 100g or 1kg).

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
  idf.py create-project-from-example "fmazz97/cs123x:simple_temperature"
  ```

- Then **Build Project**, **Flash Device** and finally **Monitor Device**.

### Option B: ESP-IDF on PlatformIO

- Download the example project from the PlatformIO Registry or directly [by clicking here](https://dl.registry.platformio.org/download/fmazz97/library/cs123x/2.0.4/example/ESP-IDF_simple_temperature.zip).

- Then **Build**, **Upload** and finally **Serial Monitor**.

---

## 📝 Usage Steps

1. **Declare your known weight:** Set your reference mass and unit at the top of `main.cpp` before compiling:

   ```cpp
   #define KNOWN_WEIGHT 100.0f
   #define UNIT "kg"
   ```

When the program boots:

1. **Setup Calibration:** The ADC initializes directly on `CS123X_CH_TEMP` and performs live temperature calibration against `CURRENT_ROOM_TEMP` (default: 25.0 °C).

2. **Temperature Loop:** The application enters a continuous loop, reading and outputting internal chip temperature in °C every second over the serial console.

---

## 📄 License

This example is distributed under the same MIT License as the [CS123x](https://github.com/FMazz97/CS123x) library. See [LICENSE](https://github.com/FMazz97/CS123x/blob/main/LICENSE) for more for details.
