/// @file main.cpp
/// @brief Practical scale tare, calibration, and weight measurement example.
/// @details Demonstrates step-by-step zero-point alignment (tare), scale factor calibration
///          using a known weight, and continuous weight acquisition in physical units.
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
#include <math.h>

#include "cs123x.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char* TAG = "cs123x_simple_scale";

// Test pin configuration (adjust according to your hardware setup)
#ifndef DOUT_PIN
#define DOUT_PIN GPIO_NUM_4
#endif
#ifndef SCLK_PIN
#define SCLK_PIN GPIO_NUM_5
#endif

// Reference weight used for calibration (e.g., 100.0 grams, kg, or lbs)
#define KNOWN_WEIGHT 100.0f
#define UNIT "kg"

static cs123x adc(CS123X_TYPE_CS1237, DOUT_PIN, SCLK_PIN);

extern "C" void app_main(void) {
    ESP_LOGI(TAG, "================================================");
    ESP_LOGI(TAG, "     CS123x SCALE CALIBRATION & MEASUREMENT     ");
    ESP_LOGI(TAG, "================================================");

    // ---------------------------------------------------------------------------
    // 1. INITIALIZATION
    // ---------------------------------------------------------------------------
    ESP_LOGI(TAG, "[1] INITIALIZATION TEST (begin)");
    while (!adc.begin()) {
        ESP_LOGW(TAG, "    [WARN] Initialization failed/timeout. Retrying in 500ms...");
        vTaskDelay(pdMS_TO_TICKS(500));
    }
    ESP_LOGI(TAG, "    [OK] ADC initialized and configuration verified.");

    // ---------------------------------------------------------------------------
    // 2. TARE (ZERO CALIBRATION)
    // ---------------------------------------------------------------------------
    ESP_LOGI(TAG, "[2] TARE PROCEDURE");
    ESP_LOGI(TAG, "    Ensure scale platform is completely empty...");
    vTaskDelay(pdMS_TO_TICKS(2000));  // Time to remove any load

    if (adc.tare(10)) {  // Average across 10 readings
        ESP_LOGI(TAG, "    [OK] Tare successful! Offset saved: %" PRId32, adc.get_offset());
    } else {
        ESP_LOGE(TAG, "    [FAIL] Tare failed due to hardware timeout.");
    }

    // ---------------------------------------------------------------------------
    // 3. SCALE FACTOR CALIBRATION
    // ---------------------------------------------------------------------------
    // Option A: Live automatic calibration on-the-fly
    ESP_LOGI(TAG, "[3] SCALE FACTOR CALIBRATION");
    ESP_LOGI(TAG, "    Place your known weight (%.1f %s) on scale...", KNOWN_WEIGHT, UNIT);
    vTaskDelay(pdMS_TO_TICKS(5000));  // Time to place the weight

    if (adc.calibrate_scale(KNOWN_WEIGHT, 10)) {
        ESP_LOGI(TAG, "    [OK] Calibration successful! Scale factor: %.4f", adc.get_scale());
    } else {
        ESP_LOGE(TAG, "    [FAIL] Calibration failed. Check load cell wiring/weight.");
    }

    /*
      Option B: Restoring calibration parameters saved in NVS/Flash
      adc.set_scale(123.4f);
    */

    ESP_LOGI(TAG, "================================================");
    ESP_LOGI(TAG, "      CALIBRATION COMPLETE - STARTING LOOP      ");
    ESP_LOGI(TAG, "================================================\n");

    // ---------------------------------------------------------------------------
    // CONTINUOUS MEASUREMENT LOOP
    // ---------------------------------------------------------------------------
    while (true) {
        // Read weight in calibrated units (averaged over 3 samples)
        int32_t value_raw = adc.read_net_counts(3);  // Raw - Offset
        float weight_units = adc.read_net_units(3);  // (Raw - Offset) / Scale

        if (isnan(weight_units)) {
            ESP_LOGE(TAG, "[ERROR] Hardware read timeout!");
        } else {
            ESP_LOGI(TAG, "Net Counts: %" PRId32 " | Weight: %.2f %s", value_raw, weight_units, UNIT);
        }

        adc.power_down();
        vTaskDelay(pdMS_TO_TICKS(5000));
        adc.power_up();
    }
}