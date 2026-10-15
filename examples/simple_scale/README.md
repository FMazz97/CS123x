# CS123x Simple Scale Example

This example demonstrates how to use the **cs123x** component to build a practical weight scale with zero-point alignment (tare), scale factor calibration using a known reference weight, and power-down sleep cycles.

---

## 🚀 Features Demonstrated

1. **ADC Initialization & Hardware Sync**:  Initializes GPIO lines, writes the default or user‑provided configuration to the ADC and verifies correct register read‑back from it.
2. **Tare Procedure**: Zeroes the scale platform by capturing offset counts across multiple samples.
3. **Scale Calibration**: Computes the linear scale factor (`Counts` / `Weight`) using a known reference weight.
4. **Net Measurement**: Converts net raw counts (`Raw` - `Offset`) into physical units (`Net Counts` / `Scale Factor`).
5. **Power Management**: Demonstrates `power_down()` and `power_up()` sequences between readings.

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

```bash
idf.py build
idf.py -p <PORT> flash monitor
```

### Option B: ESP-IDF on PlatformIO

1. Open this folder in VS Code with the PlatformIO extension installed.
2. Click **Build**, then **Upload**, then **Monitor**.

---

## 📝 Usage Steps

1. **Declare your known weight:** Set your reference mass and unit at the top of `main.cpp` before compiling:

   ```cpp
   #define KNOWN_WEIGHT 100.0f
   #define UNIT "kg"
   ```

When the program boots:

1. **Empty Scale:** Leave the load cell completely unweighted during step `[2] TARE PROCEDURE.`

2. **Apply Reference Weight:** When prompted at step `[3] SCALE FACTOR CALIBRATION`, place the exact known weight (e.g., `100.0 kg` / `100.0 g`) on the scale.

3. **Continuous Readings:** The log will display net counts and calibrated weight values every 5 seconds.

---

## 📄 License

This example is distributed under the same MIT License as the [CS123x](https://github.com/FMazz97/CS123x) library. See [LICENSE](https://github.com/FMazz97/CS123x/blob/main/LICENSE) for more for details.
