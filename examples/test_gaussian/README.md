# CS123x Gaussian Noise Test Example

This example provides a high-resolution statistical analysis and noise characterization tool for the **cs123x** component. It uses Welford's algorithm for numerically stable mean and variance computation, reporting RMS noise, peak-to-peak noise, LSB-equivalent voltage noise, effective number of bits (ENOB), and noise-free resolution.

---

## 🚀 Features Demonstrated

1. **ADC Initialization & Hardware Sync:** Initializes GPIO lines, writes the default or user-provided configuration to the ADC and verifies correct register read-back from it.
2. **Interactive Terminal CLI:** Real-time command-line interface over serial to trigger tests, cycle gains, sampling rates, input channels and set sample counts.
3. **Statistical Analysis via Welford's Algorithm:** Calculates mean (offset), variance, standard deviation (RMS noise in LSB and uV) and peak-to-peak noise without floating-point overflow.
4. **ENOB & Noise-Free Resolution Calculation:** Computes Effective Number of Bits (`ENOB = 24 - log2(RMS_noise)`) and Noise-Free Resolution (`24 - log2(PP_noise)`) in real-time.
5. **Dynamic Parameter Cycling:** Allows live changing of PGA gains (`1x`, `2x`, `64x`, `128x`), output data rates (`10Hz`, `40Hz`, `640Hz`, `1280Hz`), and channels (`CH_A`, `CH_B` (CS1238 only), `CH_TEMP`, `CH_SHORT`) on the fly.

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
  idf.py create-project-from-example "fmazz97/cs123x:test_gaussian"
  ```

- Then **Build Project**, **Flash Device** and finally **Monitor Device**.

### Option B: ESP-IDF on PlatformIO

- Download the example project from the PlatformIO Registry or directly [by clicking here](https://dl.registry.platformio.org/download/fmazz97/library/cs123x/2.0.3/example/ESP-IDF_test_gaussian.zip).

- Then **Build**, **Upload** and finally **Serial Monitor**.

---

## 📝 Usage Steps

When the program boots:

1. **Interactive Menu:** After initialization, an interactive menu is printed to the serial console on boot.

2. **CLI Controls:**
   - `[r]` -> Run **Gaussian Noise Test** across the configured sample size.
   - `[n]` -> Change number of **samples** (default: 1000).
   - `[c]` -> Cycle active input **channel** (`CH_A` -> `CH_B` -> `CH_TEMP` -> `CH_SHORT`).
   - `[g]` -> Cycle **gain** setting (`1x` -> `2x` -> `64x` -> `128x`).
   - `[f]` -> Cycle **sampling rate** setting (`10Hz` -> `40Hz` -> `640Hz` -> `1280Hz`).
   - `[h]` -> Reprint **help** menu and current configuration status.

3. **Results & Evaluation:** The statistical summary outputs mean (offset), RMS noise, peak-to-peak noise, ENOB, and noise-free resolution bits directly in the serial log.

---

## 📄 License

This example is distributed under the same MIT License as the [CS123x](https://github.com/FMazz97/CS123x) library. See [LICENSE](https://github.com/FMazz97/CS123x/blob/main/LICENSE) for more for details.
