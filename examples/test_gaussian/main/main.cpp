/// @file main.cpp
/// @brief Gaussian noise characterization suite for CS123x ADC devices.
/// @details Performs high‑resolution statistical analysis on raw ADC output using
///          Welford’s algorithm for numerically stable mean and variance computation.
///          Reports RMS noise, peak‑to‑peak noise, LSB‑equivalent voltage noise,
///          effective number of bits (ENOB), noise‑free resolution, and channel/gain/rate
///          configuration details.
/// @warning TL431 Current Limitation: Stock modules are current-limited by R1 (1 kΩ).
///          For low-impedance sensors (e.g., 350 Ω load cells), reduce R1 by adding a resistor
///          between DVDD and AVDD. For full details, see the README section
///          "Current Limit for Low-Impedance Sensors":
///          https://github.com/FMazz97/CS123x#%EF%B8%8F-important-current-limit-for-low-impedance-sensors
/// @note Effective Precision Reference:
///          - CS1237: 20.0b @5V / 19.5b @3.3V
///          - CS1238: 20.7b @5V / 20.2b @3.3V
///       Typical peak‑to‑peak noise: 0.180 µV
///       These values correspond to datasheet effective resolution at PGA=128 and 10 Hz.
/// @author FMazz97 (https://github.com/FMazz97)
/// @see CS123x GitHub Repository: https://github.com/FMazz97/CS123x
/// @copyright MIT License
/// @example main.cpp

#include <inttypes.h>
#include <math.h>
#include <stdio.h>

#include "cs123x.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char* TAG = "cs123x_gaussian";

// Test pin configuration (adjust according to your hardware setup)
#ifndef DOUT_PIN
#define DOUT_PIN GPIO_NUM_4
#endif
#ifndef SCLK_PIN
#define SCLK_PIN GPIO_NUM_5
#endif

// Test parameters
#define VREF_VOLTS 2.5f            // TL431 reference voltage on module
static uint16_t n_samples = 1000;  // Default number of samples

// Initial sensor instance configured for CS1237 at default configuration
// (Change to CS123X_TYPE_CS1238 if testing a CS1238 chip)
static cs123x adc(CS123X_TYPE_CS1237, DOUT_PIN, SCLK_PIN);

// Returns the numeric gain multiplier (1x, 2x, 64x, 128x)
static uint16_t get_gain_multiplier(CS123X_Gain g) {
    switch (g) {
        case CS123X_GAIN_1:
            return 1;
        case CS123X_GAIN_2:
            return 2;
        case CS123X_GAIN_64:
            return 64;
        case CS123X_GAIN_128:
            return 128;
        default:
            return 1;
    }
}

// Returns the active channel name as text
static const char* get_channel_name(CS123X_Channel ch) {
    switch (ch) {
        case CS123X_CH_A:
            return "CH_A";
        case CS123X_CH_B:
            return "CH_B";
        case CS123X_CH_TEMP:
            return "CH_TEMP";
        case CS123X_CH_SHORT:
            return "CH_SHORT";
        default:
            return "UNKNOWN";
    }
}

// Returns the sampling rate in Hz
static uint16_t get_rate_hz(CS123X_Rate r) {
    switch (r) {
        case CS123X_RATE_10Hz:
            return 10;
        case CS123X_RATE_40Hz:
            return 40;
        case CS123X_RATE_640Hz:
            return 640;
        case CS123X_RATE_1280Hz:
            return 1280;
        default:
            return 10;
    }
}

static void print_help(void) {
    CS123X_Config cfg = adc.get_config();
    ESP_LOGI(TAG, "====================================");
    ESP_LOGI(TAG, "  CS123x Advanced Gaussian Evaluation");
    ESP_LOGI(TAG, "====================================");
    ESP_LOGI(TAG, "Current Config -> Channel: %s | Gain: %ux | Speed: %u Hz | Samples: %u",
             get_channel_name(cfg.channel),
             get_gain_multiplier(cfg.gain),
             get_rate_hz(cfg.rate),
             n_samples);
    ESP_LOGI(TAG, "------------------------------------");
    ESP_LOGI(TAG, " [r] -> Run Gaussian Noise Test");
    ESP_LOGI(TAG, " [n] -> Set Number of Samples");
    ESP_LOGI(TAG, " [c] -> Cycle Channel (A -> B -> Temp -> Short)");
    ESP_LOGI(TAG, " [g] -> Cycle Gain (1x -> 2x -> 64x -> 128x)");
    ESP_LOGI(TAG, " [f] -> Cycle Rate (10Hz -> 40Hz -> 640Hz -> 1280Hz)");
    ESP_LOGI(TAG, " [h] -> Print this menu");
    ESP_LOGI(TAG, "====================================\n");
}

static void run_gaussian_test(void) {
    ESP_LOGI(TAG, "[GAUSSIAN TEST] Collecting raw samples...");

    // Welford’s algorithm accumulators for stable mean/variance
    double mean = 0.0;
    double M2 = 0.0;

    int32_t min_val = INT32_MAX;
    int32_t max_val = INT32_MIN;

    uint16_t valid_samples = 0;
    int8_t last_percent = -1;

    for (uint16_t i = 1; i <= n_samples; i++) {
        int c = getchar();
        if (c == 27) {  // ESC key

            // Insert newline and flush stream to align logs
            printf("\n");
            fflush(stdout);
            ESP_LOGW(TAG, "[ABORT] Test interrupted by user!");
            return;
        }

        int32_t raw = adc.read();

        // Skip invalid samples caused by communication errors
        if (raw == CS123X_TIMEOUT_ERROR || raw == CS123X_SWITCH_ERROR) continue;

        valid_samples++;

        // Track peak‑to‑peak noise
        if (raw < min_val) min_val = raw;
        if (raw > max_val) max_val = raw;

        // Welford incremental mean/variance update
        double delta = raw - mean;
        mean += delta / valid_samples;
        double delta2 = raw - mean;
        M2 += delta * delta2;

        // Update progress only when percentage increases
        int current_percent = (i * 100) / n_samples;
        if (current_percent > last_percent) {
            last_percent = current_percent;
            printf("\rProgress: %d%% ", current_percent);
            fflush(stdout);
        }
    }

    // Insert newline and flush stream to align logs
    printf("\n");
    fflush(stdout);

    if (valid_samples == 0) {
        ESP_LOGE(TAG, "\n[ERROR] All samples timed out!");
        return;
    }

    // Compute variance and standard deviation
    double variance = M2 / valid_samples;
    double stddev = sqrt(variance);

    // Peak‑to‑peak noise in LSBs
    int32_t noise_pp = max_val - min_val;

    // Convert noise to millivolts:
    // LSB voltage = Vref / (2^24 * Gain)
    CS123X_Gain gain_enum = adc.get_gain();
    uint16_t gain_val = get_gain_multiplier(gain_enum);

    double lsb_voltage_volts = (double)VREF_VOLTS / ((double)CS123X_MAX_VALUE * 2 * gain_val);
    double noise_pp_uv = (noise_pp * lsb_voltage_volts) * 1e6;
    double stddev_uv = (stddev * lsb_voltage_volts) * 1e6;

    // Effective resolution (ENOB) from RMS noise:
    // ENOB = 24 − log2(RMS_noise_LSB)
    double lost_bits_rms = (stddev > 0) ? log(stddev) / log(2.0) : 0;
    double enob_rms = 24.0 - lost_bits_rms;

    // Noise‑free resolution from peak‑to‑peak noise:
    // NoiseFreeBits = 24 − log2(PP_noise_LSB)
    double lost_bits_pp = (noise_pp > 0) ? log(noise_pp) / log(2.0) : 0;
    double noise_free_bits = 24.0 - lost_bits_pp;

    // Print statistical report
    ESP_LOGI(TAG, "=== Gaussian Noise Statistics ===");
    ESP_LOGI(TAG, "Valid Samples:   %" PRIu16, valid_samples);
    ESP_LOGI(TAG, "Mean (Offset):   %.2f", mean);
    ESP_LOGI(TAG, "Std Dev (RMS):   %.2f LSB  (%.3f uV)", stddev, stddev_uv);
    ESP_LOGI(TAG, "Variance:        %.2f", variance);
    ESP_LOGI(TAG, "Min Raw:         %" PRId32, min_val);
    ESP_LOGI(TAG, "Max Raw:         %" PRId32, max_val);
    ESP_LOGI(TAG, "Noise P-P:       %" PRId32 " LSB  (%.3f uV) [Typ: 0.180 uV @ 128x,10Hz]", noise_pp, noise_pp_uv);
    ESP_LOGI(TAG, "-----------------------------------");
    ESP_LOGI(TAG, "ENOB (RMS):      %.2f bits [Typ: 20.0b-5V / 19.5b-3.3V (CS1237) | 20.7b-5V / 20.2b-3.3V (CS1238) @128x,10Hz]", enob_rms);
    ESP_LOGI(TAG, "Noise-Free Resolution: %.2f bits", noise_free_bits);
    ESP_LOGI(TAG, "===================================\n");
}

extern "C" void app_main(void) {
    // Initialize ADC and retry until configuration is validated
    while (!adc.begin()) {
        ESP_LOGW(TAG, "[WARN] Initialization failed/timeout. Retrying in 500ms...");
        vTaskDelay(pdMS_TO_TICKS(500));
    }
    ESP_LOGI(TAG, "[OK] ADC initialized and configuration verified.");
    print_help();

    while (true) {
        int c = getchar();
        if (c == EOF) {
            vTaskDelay(pdMS_TO_TICKS(50));
            continue;
        }

        char cmd = (char)c;

        // -------------------------
        // RUN GAUSSIAN NOISE TEST
        // -------------------------
        if (cmd == 'r' || cmd == 'R') {
            run_gaussian_test();
        }

        // -------------------------
        // Set number of samples
        // -------------------------
        else if (cmd == 'n' || cmd == 'N') {
            ESP_LOGI(TAG, "Insert new sample count (e.g. 500, 2000):");
            char buf[16];
            uint8_t idx = 0;

            while (idx < sizeof(buf) - 1) {
                int ch = getchar();
                if (ch != EOF && ch != '\n' && ch != '\r') {
                    buf[idx++] = (char)ch;
                    putchar(ch);
                    fflush(stdout);
                } else if ((ch == '\n' || ch == '\r') && idx > 0) {
                    break;
                }
                vTaskDelay(pdMS_TO_TICKS(10));
            }
            buf[idx] = '\0';
            printf("\n");

            long val = strtol(buf, NULL, 10);
            if (val >= 10 && val <= 65535) {
                n_samples = (uint16_t)val;
                ESP_LOGI(TAG, "[OK] n_samples set to: %u", n_samples);
            } else {
                ESP_LOGE(TAG, "[ERROR] Invalid range! Value must be between 10 and 65535.");
            }
        }

        // -------------------------
        // Cycle through input channels
        // -------------------------
        else if (cmd == 'c' || cmd == 'C') {
            CS123X_Channel next_ch;
            switch (adc.get_ch()) {
                case CS123X_CH_A:
                    next_ch = CS123X_CH_B;
                    break;
                case CS123X_CH_B:
                    next_ch = CS123X_CH_TEMP;
                    break;
                case CS123X_CH_TEMP:
                    next_ch = CS123X_CH_SHORT;
                    break;
                default:
                    next_ch = CS123X_CH_A;
                    break;
            }

            if (adc.set_ch(next_ch)) {
                ESP_LOGI(TAG, "[OK] Channel set to: %s", get_channel_name(next_ch));
            } else if (next_ch == CS123X_CH_B) {
                adc.set_ch(CS123X_CH_TEMP);
                ESP_LOGI(TAG, "[OK] CS1237 detected (No CH_B). Switched to CH_TEMP.");
            }
        }

        // -------------------------
        // Cycle through gain settings
        // -------------------------
        else if (cmd == 'g' || cmd == 'G') {
            CS123X_Gain next_gain;
            switch (adc.get_gain()) {
                case CS123X_GAIN_1:
                    next_gain = CS123X_GAIN_2;
                    break;
                case CS123X_GAIN_2:
                    next_gain = CS123X_GAIN_64;
                    break;
                case CS123X_GAIN_64:
                    next_gain = CS123X_GAIN_128;
                    break;
                default:
                    next_gain = CS123X_GAIN_1;
                    break;
            }
            if (adc.set_gain(next_gain)) {
                ESP_LOGI(TAG, "[OK] Gain set to: %ux", get_gain_multiplier(next_gain));
            }
        }

        // -------------------------
        // Cycle through sampling rates
        // -------------------------
        else if (cmd == 'f' || cmd == 'F') {
            CS123X_Rate next_rate;
            switch (adc.get_rate()) {
                case CS123X_RATE_10Hz:
                    next_rate = CS123X_RATE_40Hz;
                    break;
                case CS123X_RATE_40Hz:
                    next_rate = CS123X_RATE_640Hz;
                    break;
                case CS123X_RATE_640Hz:
                    next_rate = CS123X_RATE_1280Hz;
                    break;
                default:
                    next_rate = CS123X_RATE_10Hz;
                    break;
            }
            if (adc.set_rate(next_rate)) {
                ESP_LOGI(TAG, "[OK] Sampling Rate set to: %u Hz", get_rate_hz(next_rate));
            }
        }

        // -------------------------
        // Print help menu
        // -------------------------
        else if (cmd == 'h' || cmd == 'H') {
            print_help();
        }
    }
}