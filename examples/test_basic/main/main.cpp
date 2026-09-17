/// @file main.cpp
/// @brief Basic functional verification test suite for the CS123x library core features.
/// @details Tests hardware initialization, register setters/getters with verification,
///          tare, scale calibration, temperature reading, power cycling, and continuous acquisition.
/// @warning TL431 Current Limitation: Stock modules are current-limited by R1 (1 kΩ).
///          For low-impedance sensors (e.g., 350 Ω load cells), reduce R1 by adding a resistor
///          between DVDD and AVDD. For full details, see the README section
///          "Current Limit for Low-Impedance Sensors":
///          https://github.com/FMazz97/CS123x#%EF%B8%8F-important-current-limit-for-low-impedance-sensors
/// @author FMazz97 (https://github.com/FMazz97)
/// @see CS123x GitHub Repository: https://github.com/FMazz97/CS123x
/// @copyright MIT License
/// @example main.cpp

#include <inttypes.h>

#include "cs123x.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char* TAG = "cs123x_basic_test";

// Test pin configuration (adjust according to your hardware setup)
#ifndef DOUT_PIN
#define DOUT_PIN GPIO_NUM_4
#endif
#ifndef SCLK_PIN
#define SCLK_PIN GPIO_NUM_5
#endif

// Initial sensor instance configured for CS1237 at default configuration
// (Change to CS123X_TYPE_CS1238 if testing a CS1238 chip)
static cs123x adc(CS123X_TYPE_CS1237, DOUT_PIN, SCLK_PIN);

extern "C" void app_main(void) {
    ESP_LOGI(TAG, "==================================================");
    ESP_LOGI(TAG, "       CS123x BASIC API & HARDWARE TEST");
    ESP_LOGI(TAG, "==================================================");

    // ---------------------------------------------------------------------------
    // 1. INITIALIZATION
    // ---------------------------------------------------------------------------
    ESP_LOGI(TAG, "[1] INITIALIZATION TEST (begin)");
    while (!adc.begin()) {
        ESP_LOGW(TAG, "[WARN] Initialization failed/timeout. Retrying in 500ms...");
        vTaskDelay(pdMS_TO_TICKS(500));
    }
    ESP_LOGI(TAG, "[OK] ADC initialized and configuration verified.");

    // ---------------------------------------------------------------------------
    // 2. TESTING SETTERS WITH ROLLBACK AND RETURN VALUE CHECK AND GETTERS
    // ---------------------------------------------------------------------------
    ESP_LOGI(TAG, "[2] TESTING SETTERS (Hardware Sync & Rollback)...");

    bool ok_gain = adc.set_gain(CS123X_GAIN_64, true);
    ESP_LOGI(TAG, "set_gain(GAIN_64): %s", ok_gain ? "[OK]" : "[FAIL]");

    bool ok_rate = adc.set_rate(CS123X_RATE_40Hz, true);
    ESP_LOGI(TAG, "set_rate(RATE_40Hz): %s", ok_rate ? "[OK]" : "[FAIL]");

    bool ok_ch = adc.set_ch(CS123X_CH_A, true);
    ESP_LOGI(TAG, "set_ch(CH_A): %s", ok_ch ? "[OK]" : "[FAIL]");

    bool ok_ref = adc.set_ref(CS123X_INT_REF_OFF, true);
    ESP_LOGI(TAG, "set_ref(INT_REF_OFF): %s", ok_ref ? "[OK]" : "[FAIL]");

    ESP_LOGI(TAG, "Read back -> Gain: %d | Rate: %d | Ch: %d | Ref: %d",
             adc.get_gain(), adc.get_rate(), adc.get_ch(), adc.get_ref());

    // ---------------------------------------------------------------------------
    // 3. OFFSET, SCALE & CALIBRATION METHODS TEST
    // ---------------------------------------------------------------------------
    ESP_LOGI(TAG, "[3] OFFSET, SCALE & CALIBRATION TEST");
    if (adc.tare(5)) {
        ESP_LOGI(TAG, "[OK] Tare done. Current offset: %" PRId32, adc.get_offset());
    } else {
        ESP_LOGE(TAG, "[FAIL] Tare fail (Timeout).");
    }

    if (adc.calibrate_scale(100.0f, 5)) {  // Scale calibration with a knownWeight of 100g/kg/ecc...
        ESP_LOGI(TAG, "[OK] Calibration done. Scale factor: %.4f", adc.get_scale());
    } else {
        ESP_LOGE(TAG, "[FAIL] Calibration fail.");
    }

    // ---------------------------------------------------------------------------
    // 4. TEMPERATURE SENSOR TEST
    // ---------------------------------------------------------------------------
    ESP_LOGI(TAG, "[4] TEMPERATURE SENSOR TEST");

    // Overload 1: Manual calibration
    adc.set_temp_calibration(25.0f, 123456L);
    ESP_LOGI(TAG, "[OK] Manual set_temp_calibration done.");

    // Overload 2: Automatic calibration on-the-fly
    bool ok_temp1 = adc.calibrate_temp(25.0f);
    ESP_LOGI(TAG, "calibrate_temp(25.0C): %s", ok_temp1 ? "[OK]" : "[FAIL]");

    // ---------------------------------------------------------------------------
    // 5. POWER MANAGEMENT TEST
    // ---------------------------------------------------------------------------
    ESP_LOGI(TAG, "[5] POWER MANAGEMENT TEST");
    adc.power_down();
    vTaskDelay(pdMS_TO_TICKS(10));
    adc.power_up();
    ESP_LOGI(TAG, "[OK] Power Cycle executed.");

    // Final optimal configuration for the main read loop
    adc.set_gain(CS123X_GAIN_128, true);
    adc.set_rate(CS123X_RATE_10Hz, true);
    adc.set_ch(CS123X_CH_A, true);

    ESP_LOGI(TAG, "==================================================");
    ESP_LOGI(TAG, "    SETUP TEST COMPLETED - STARTING READ LOOP");
    ESP_LOGI(TAG, "==================================================");

    // ---------------------------------------------------------------------------
    // 6. CONTINUOUS DATA READING TEST
    // ---------------------------------------------------------------------------
    while (true) {
        // Synchronous reads (internally handle is_ready polling and timeouts)
        int32_t raw_single = adc.read();
        int32_t raw_avg = adc.read_average(3);

        int32_t value_raw = adc.read_net_counts(3);  // (Raw - Offset)
        float weight_units = adc.read_net_units(3);  // (Raw - Offset) / Scale

        float voltage = adc.read_voltage();  // Differential input voltage (VREF default: 2.5V)
        float temperature = adc.read_temp(3);

        ESP_LOGI(TAG,
                 "Raw: %" PRId32 " | Avg: %" PRId32 " | NetCounts: %" PRId32 " | NetUnits: %.2f | Voltage: %.6f V | Temp C: %.2f",
                 raw_single, raw_avg, value_raw, weight_units, voltage, temperature);

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}