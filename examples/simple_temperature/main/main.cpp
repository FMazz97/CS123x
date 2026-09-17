/// @file main.cpp
/// @brief Internal temperature sensor calibration and reading example.
/// @details Demonstrates live single-point ambient temperature calibration and
///          continuous temperature reading in °C using automatic channel switching.
/// @warning TL431 Current Limitation: Stock modules are current-limited by R1 (1 kΩ).
///          For low-impedance sensors (e.g., 350 Ω load cells), reduce R1 by adding a resistor
///          between DVDD and AVDD. For full details, see the README section
///          "Current Limit for Low-Impedance Sensors":
///          https://github.com/FMazz97/CS123x#%EF%B8%8F-important-current-limit-for-low-impedance-sensors
/// @author FMazz97 (https://github.com/FMazz97)
/// @see CS123x GitHub Repository: https://github.com/FMazz97/CS123x
/// @copyright MIT License
/// @example main.cpp

#include <math.h>

#include "cs123x.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char* TAG = "cs123x_simple_temp";

// Test pin configuration (adjust according to your hardware setup)
#ifndef DOUT_PIN
#define DOUT_PIN GPIO_NUM_4
#endif
#ifndef SCLK_PIN
#define SCLK_PIN GPIO_NUM_5
#endif

// Known ambient room temperature during calibration (in °C)
#define CURRENT_ROOM_TEMP 25.0f

// Initial sensor instance configured for CS1237 at default configuration
// (Change to CS123X_TYPE_CS1238 if testing a CS1238 chip)
static cs123x adc(CS123X_TYPE_CS1237, DOUT_PIN, SCLK_PIN, CS123X_CH_TEMP);

extern "C" void app_main(void) {
    ESP_LOGI(TAG, "================================================");
    ESP_LOGI(TAG, "   CS123x TEMPERATURE SENSOR CALIBRATION        ");
    ESP_LOGI(TAG, "================================================");

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
    // 2. TEMPERATURE SENSOR CALIBRATION
    // ---------------------------------------------------------------------------
    ESP_LOGI(TAG, "[2] TEMPERATURE CALIBRATION");
    ESP_LOGI(TAG, "Calibrating sensor at current ambient temperature (%.1f C)...", CURRENT_ROOM_TEMP);

    // Option A: Live automatic calibration on-the-fly
    if (adc.calibrate_temp(CURRENT_ROOM_TEMP)) {
        ESP_LOGI(TAG, "[OK] Live temperature calibration succeeded!");
    } else {
        ESP_LOGE(TAG, "[FAIL] Temperature calibration failed (Timeout).");
    }

    /*
      Option B: Restoring calibration parameters saved in NVS / Flash
      adc.set_temp_calibration(25.0f, 8388608L);
    */

    ESP_LOGI(TAG, "================================================");
    ESP_LOGI(TAG, "        SETUP COMPLETED - STARTING LOOP        ");
    ESP_LOGI(TAG, "================================================\n");

    // ---------------------------------------------------------------------------
    // 3. CONTINUOUS TEMPERATURE READING LOOP
    // ---------------------------------------------------------------------------
    while (true) {
        // Read temperature in °C (averaged over 5 samples for noise reduction)
        // Note: read_temp() handles channel switching and PGA adjustment automatically
        float temp_c = adc.read_temp(5);

        if (isnan(temp_c)) {
            ESP_LOGE(TAG, "Failed to read temperature (Timeout or uncalibrated).");
        } else {
            ESP_LOGI(TAG, "Internal Temperature: %.2f C", temp_c);
        }

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}