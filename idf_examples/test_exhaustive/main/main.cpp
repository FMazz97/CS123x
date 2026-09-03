/**
 * @file main.cpp
 * @brief ESP-IDF native translation of TestExhaustive.ino — comprehensive
 *        hardware and API test suite for the cs123x driver.
 */

#include "cs123x.h"

#include <inttypes.h>

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char* TAG = "cs123x_test";

#ifndef DOUT_PIN
#define DOUT_PIN GPIO_NUM_4
#endif
#ifndef SCLK_PIN
#define SCLK_PIN GPIO_NUM_5
#endif

static cs123x adc(CS123X_TYPE_CS1237, DOUT_PIN, SCLK_PIN);
static CS123X_Config default_conf = {CS123X_CH_A, CS123X_GAIN_128, CS123X_RATE_10Hz, CS123X_INT_REF_OFF};

static void restore_default_config() {
    if (adc.get_config() != default_conf) {
        ESP_LOGI(TAG, "%s", adc.set_config(default_conf) ? "Correctly restored default configuration." : "Default configuration not restored.");
    } else {
        ESP_LOGI(TAG, "Default configuration already set.");
    }
}

extern "C" void app_main(void) {
    ESP_LOGI(TAG, "==================================================");
    ESP_LOGI(TAG, "    CS123x EXHAUSTIVE API & HARDWARE TEST");
    ESP_LOGI(TAG, "==================================================");

    // ---------------------------------------------------------------
    // 1. INITIALIZATION
    // ---------------------------------------------------------------
    ESP_LOGI(TAG, "[1] INITIALIZATION TEST (begin)");
    while (!adc.begin()) {
        ESP_LOGW(TAG, "Initialization failed/timeout. Retrying in 500ms...");
        vTaskDelay(pdMS_TO_TICKS(500));
    }
    ESP_LOGI(TAG, "[OK] ADC initialized and configuration verified.");

    // ---------------------------------------------------------------
    // 2. EXHAUSTIVE PGA / GAIN TEST
    // ---------------------------------------------------------------
    ESP_LOGI(TAG, "[2] EXHAUSTIVE PGA / GAIN TEST");
    const CS123X_Gain pga_list[] = {CS123X_GAIN_1, CS123X_GAIN_2, CS123X_GAIN_64, CS123X_GAIN_128};
    const char* pga_names[] = {"GAIN_1", "GAIN_2", "GAIN_64", "GAIN_128"};

    for (uint8_t i = 0; i < 4; i++) {
        bool ok = adc.set_gain(pga_list[i], true);
        CS123X_Gain read_back = adc.get_gain();
        ESP_LOGI(TAG, "  Setting %s -> HW Sync: %s | Readback: %s",
                 pga_names[i], ok ? "[OK]" : "[FAIL]",
                 read_back == pga_list[i] ? "[MATCH]" : "[MISMATCH]");
    }
    restore_default_config();

    // ---------------------------------------------------------------
    // 3. EXHAUSTIVE DATA RATE TEST
    // ---------------------------------------------------------------
    ESP_LOGI(TAG, "[3] EXHAUSTIVE DATA RATE TEST");
    const CS123X_Rate rate_list[] = {CS123X_RATE_10Hz, CS123X_RATE_40Hz, CS123X_RATE_640Hz, CS123X_RATE_1280Hz};
    const char* rate_names[] = {"10Hz", "40Hz", "640Hz", "1280Hz"};

    for (uint8_t i = 0; i < 4; i++) {
        bool ok = adc.set_rate(rate_list[i], true);
        CS123X_Rate read_back = adc.get_rate();
        uint32_t timeout = adc.get_timeout_ms();
        ESP_LOGI(TAG, "  Setting %s -> HW Sync: %s | Timeout: %" PRIu32 "ms | Readback: %s",
                 rate_names[i], ok ? "[OK]" : "[FAIL]", timeout,
                 read_back == rate_list[i] ? "[MATCH]" : "[MISMATCH]");
    }
    restore_default_config();

    // ---------------------------------------------------------------
    // 4. EXHAUSTIVE CHANNEL TEST
    // ---------------------------------------------------------------
    ESP_LOGI(TAG, "[4] EXHAUSTIVE CHANNEL TEST");
    const CS123X_Channel ch_list[] = {CS123X_CH_A, CS123X_CH_B, CS123X_CH_TEMP, CS123X_CH_SHORT};
    const char* ch_names[] = {"CH_A", "CH_B", "CH_TEMP", "CH_SHORT"};

    for (uint8_t i = 0; i < 4; i++) {
        bool ok = adc.set_ch(ch_list[i], true);
        CS123X_Channel read_back = adc.get_ch();
        ESP_LOGI(TAG, "  Setting %s -> Result: %s | Readback: %s",
                 ch_names[i], ok ? "[OK]" : "[REJECTED/FAIL]",
                 read_back == ch_list[i] ? "[MATCH]" : "[MISMATCH]");
    }
    restore_default_config();

    // ---------------------------------------------------------------
    // 5. EXHAUSTIVE INT_REF TEST
    // ---------------------------------------------------------------
    ESP_LOGI(TAG, "[5] EXHAUSTIVE INT_REF TEST");
    const CS123X_IntRef ref_list[] = {CS123X_INT_REF_ON, CS123X_INT_REF_OFF};
    const char* ref_names[] = {"REF_ON", "REF_OFF"};

    for (uint8_t i = 0; i < 2; i++) {
        bool ok = adc.set_ref(ref_list[i], true);
        CS123X_IntRef read_back = adc.get_ref();
        ESP_LOGI(TAG, "  Setting %s -> HW Sync: %s | Readback: %s",
                 ref_names[i], ok ? "[OK]" : "[FAIL]",
                 read_back == ref_list[i] ? "[MATCH]" : "[MISMATCH]");
    }
    restore_default_config();

    // ---------------------------------------------------------------
    // 6. BATCH CONFIGURATION TEST
    // ---------------------------------------------------------------
    ESP_LOGI(TAG, "[6] BATCH CONFIGURATION TEST");
    bool batch_ok = adc.set_config(CS123X_CH_SHORT, CS123X_GAIN_64, CS123X_RATE_40Hz, true);
    ESP_LOGI(TAG, "  set_config(CH_SHORT, GAIN_64, 40Hz) -> %s", batch_ok ? "[OK]" : "[FAIL]");

    CS123X_Config snapshot = adc.get_config();
    ESP_LOGI(TAG, "  get_config() snapshot -> ch: %d | gain: %d | rate: %d | int_ref: %d",
             snapshot.channel, snapshot.gain, snapshot.rate, snapshot.intRef);
    restore_default_config();

    // ---------------------------------------------------------------
    // 7. DUAL-CHANNEL READ TEST
    // ---------------------------------------------------------------
    ESP_LOGI(TAG, "[7] DUAL-CHANNEL READ TEST");
    CS123X_DualReading dual = adc.read_dual_channel(CS123X_CH_A, CS123X_CH_TEMP, true);
    ESP_LOGI(TAG, "  read_dual_channel(CH_A, CH_TEMP) -> ch1: %" PRId32 " | ch2: %" PRId32, dual.ch1, dual.ch2);
    ESP_LOGI(TAG, "  Chip left on channel: %d", adc.get_ch());
    restore_default_config();

    // ---------------------------------------------------------------
    // 8. OFFSET, SCALE & CALIBRATION METHODS TEST
    // ---------------------------------------------------------------
    ESP_LOGI(TAG, "[8] OFFSET, SCALE & CALIBRATION TEST");
    adc.set_offset(12345L);
    adc.set_scale(420.5f);
    ESP_LOGI(TAG, "  Manual set_offset(12345) -> get_offset: %" PRId32, adc.get_offset());
    ESP_LOGI(TAG, "  Manual set_scale(420.5)  -> get_scale: %.2f", adc.get_scale());

    bool tare_ok = adc.tare(3);
    ESP_LOGI(TAG, "  tare(3 samples) -> %s | New Offset: %" PRId32, tare_ok ? "[OK]" : "[FAIL]", adc.get_offset());

    bool cal_ok = adc.calibrate_scale(100.0f, 3);
    ESP_LOGI(TAG, "  calibrate_scale(100g, 3 samples) -> %s | New Scale Factor: %.4f",
             cal_ok ? "[OK]" : "[FAIL]", adc.get_scale());

    // ---------------------------------------------------------------
    // 9. TEMPERATURE SENSOR TEST
    // ---------------------------------------------------------------
    ESP_LOGI(TAG, "[9] TEMPERATURE SENSOR TEST");
    adc.set_temp_calibration(25.0f, 8388608L);
    ESP_LOGI(TAG, "  set_temp_calibration(25.0C, 8388608L) -> [OK]");

    bool t_cal1 = adc.set_temp_calibration(25.0f);
    ESP_LOGI(TAG, "  set_temp_calibration(25.0C) -> %s", t_cal1 ? "[OK]" : "[FAIL]");

    float temp_val = adc.read_temperature(3);
    ESP_LOGI(TAG, "  read_temperature(3 samples) -> %.2f C", temp_val);

    // ---------------------------------------------------------------
    // 10. POWER MANAGEMENT TEST
    // ---------------------------------------------------------------
    ESP_LOGI(TAG, "[10] POWER MANAGEMENT TEST");
    adc.power_down();
    ESP_LOGI(TAG, "  power_down() executed.");
    vTaskDelay(pdMS_TO_TICKS(500));
    adc.power_up();
    ESP_LOGI(TAG, "  power_up() executed.");
    restore_default_config();

    ESP_LOGI(TAG, "==================================================");
    ESP_LOGI(TAG, "    SETUP TEST COMPLETED - STARTING READ LOOP");
    ESP_LOGI(TAG, "==================================================");

    // ---------------------------------------------------------------
    // 11. CONTINUOUS DATA READING TEST
    // ---------------------------------------------------------------
    while (true) {
        int32_t raw_single = adc.read();
        int32_t raw_avg = adc.read_average(3);
        int32_t value_raw = adc.get_value(3);
        float weight_units = adc.get_units(3);
        float voltage = adc.read_voltage();
        float temperature = adc.read_temperature(3);

        ESP_LOGI(TAG, "Raw: %" PRId32 " | Avg: %" PRId32 " | Value: %" PRId32 " | Units: %.2f | Voltage: %.6f V | Temp C: %.2f",
                 raw_single, raw_avg, value_raw, weight_units, voltage, temperature);

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}