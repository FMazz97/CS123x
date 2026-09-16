# CS123x Simple Scale Example

This example demonstrates how to use the **CS123x** library to build a practical weight scale with zero-point alignment (tare), scale factor calibration using a known reference weight, and power-down sleep cycles.

---

## 🚀 Features Demonstrated

1. **ADC Initialization & Hardware Sync**: Automatic detection and configuration verification of the CS1237/CS1238 chip.
2. **Tare Procedure**: Zeroes the scale platform by capturing offset counts across multiple samples.
3. **Scale Calibration**: Computes the linear scale factor ($\text{Counts} / \text{Weight}$) using a known reference weight.
4. **Net Measurement**: Acquires raw net counts ($\text{Raw} - \text{Offset}$) and weight in physical units ($\text{Net Counts} / \text{Scale Factor}$).
5. **Power Management**: Demonstrates `power_down()` and `power_up()` sequences between readings.

---

## 🛠️ Hardware Requirements

* Any **ESP32** development board.
* CS1237 or CS1238 ADC module connected to a scale / load cell / bridge sensor.
* External reference weight (e.g., 100g or 1kg).

### Pin Assignment

The default pin configuration is:

| Signal | ESP32 GPIO | CS123x Pin |
| :--- | :--- | :--- |
| **DOUT** | `GPIO 4` | DOUT / DT |
| **SCLK** | `GPIO 5` | SCLK / SCK |

> **Note**: Pin configuration can be overridden at compile-time via CMake or `platformio.ini`.

---

## ⚠️ Important Hardware Considerations

### Current Limit for Low-Impedance Sensors

Stock CS1237/CS1238 modules are current-limited by resistor **R1 (1 kΩ)**. For low-impedance sensors (such as **350 Ω load cells**), reduce R1 by adding an external resistor between **DVDD** and **AVDD**.

For full hardware details, visit the [`Current Limit for Low-Impedance Sensors`](https://github.com/FMazz97/CS123x#%EF%B8%8F-important-current-limit-for-low-impedance-sensors) section on [`CS123x Repository`](https://github.com/FMazz97/CS123x).

---

## ⚙️ Building & Flashing

### Option A: Using ESP-IDF (Command Line)

1. Set up your ESP-IDF environment.
2. Add the component to your project:
   ```bash
   idf.py add-dependency "fmazz97/cs123x^2.0.0"
   ``` 
   > See [`ESP-IDF installation`](https://github.com/FMazz97/CS123x#esp-idf-1) section on [`CS123x Repository`](https://github.com/FMazz97/CS123x).

3. Build the project:
   ```bash
   idf.py build
   ```
4. Flash and open the serial monitor:
   ```bash
   idf.py -p <PORT> flash monitor
   ```
   To customize pins via CMake, add the following to your root `CMakeLists.txt`:
   ```CMake
   target_compile_definitions(main PRIVATE
    DOUT_PIN=GPIO_NUM_27
    SCLK_PIN=GPIO_NUM_22
)
   ```

### Option B: Using PlatformIO

1. Open this folder in VS Code with PlatformIO installed.
2. Add to your `platformio.ini` the following item to enable colored logging and custom pins:
   ```ini
   [env:esp32dev]
   platform = espressif32
   board = esp32dev
   framework = espidf
   monitor_speed = 115200
   monitor_filters = direct
   build_flags = 
       -D DOUT_PIN=GPIO_NUM_27
       -D SCLK_PIN=GPIO_NUM_22
   ```
3. Click `Build & Upload` and finally `Monitor`.

## 📝 Usage Steps

When the program boots:
1. **Declare your known weight:** Set your reference mass and unit at the top of `main.cpp` before compiling:
   ```cpp
   #define KNOWN_WEIGHT 100.0f
   #define UNIT "kg"
   ```

2. **Empty Scale:** Leave the load cell completely unweighted during step `[2] TARE PROCEDURE.`

3. **Apply Reference Weight:** When prompted at step `[3] SCALE FACTOR CALIBRATION`, place the exact known weight (e.g., `100.0 kg` / `100.0 g`) on the scale.

4. **Continuous Readings:** The log will display net counts and calibrated weight values every 5 seconds.

## 📄 License

Distributed under the MIT License. See [LICENSE](https://github.com/FMazz97/CS123x/blob/main/LICENSE) for more information.