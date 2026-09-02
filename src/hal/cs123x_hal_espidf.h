/// @file cs123x_hal_idf.h
/// @brief ESP-IDF (native, non-Arduino) HAL for the CS123x core: GPIO, timing,
///        and critical section primitives — ESP-IDF v5.x APIs.
/// @copyright MIT License

#ifndef CS123X_HAL_ESPIDF_H
#define CS123X_HAL_ESPIDF_H

#include "driver/gpio.h"
#include "esp_rom_sys.h"     // esp_rom_delay_us() — IDF v5.x location
#include "esp_timer.h"       // esp_timer_get_time()
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"   // taskYIELD()

// -----------------------------------------------------------------------------
// Critical section: same dual-core FreeRTOS spinlock mechanism Arduino-ESP32
// itself uses under the hood (portENTER_CRITICAL/portMUX_TYPE), used natively here.
// Single definition lives in core/cs123x.cpp; every other translation unit only
// sees this `extern` declaration (no per-file duplicate mutex, no unused-var warning).
// -----------------------------------------------------------------------------
extern portMUX_TYPE cs123x_idf_mux;
#define CS123X_CRITICAL_VAR()
#define CS123X_ENTER_CRITICAL() portENTER_CRITICAL(&cs123x_idf_mux)
#define CS123X_EXIT_CRITICAL() portEXIT_CRITICAL(&cs123x_idf_mux)

// -----------------------------------------------------------------------------
// GPIO primitives. No AVR-style "fast I/O" caching needed here: gpio_set_level()/
// gpio_get_level() already are the direct register access — same rationale
// already verified for Arduino-ESP32, which delegates to the same IDF gpio driver.
// -----------------------------------------------------------------------------
#define CS123X_SCLK_HIGH() gpio_set_level(static_cast<gpio_num_t>(_sclk), 1)
#define CS123X_SCLK_LOW() gpio_set_level(static_cast<gpio_num_t>(_sclk), 0)
#define CS123X_DOUT_HIGH() gpio_set_level(static_cast<gpio_num_t>(_dout), 1)
#define CS123X_DOUT_LOW() gpio_set_level(static_cast<gpio_num_t>(_dout), 0)
#define CS123X_DOUT_READ() gpio_get_level(static_cast<gpio_num_t>(_dout))

#define CS123X_INIT_FAST_IO() ((void)0)

// -----------------------------------------------------------------------------
// Pin direction. gpio_config() call cost matches what pinMode() already does
// under the hood on Arduino-ESP32 (same underlying IDF driver) — no new timing
// concern beyond what's already validated at 1280Hz on that HAL.
// Pull-up intentionally DISABLED on the DOUT input, mirroring the Arduino HAL's
// plain-INPUT choice on Espressif targets (weak pull-up parasitic capacitance
// distorts fast serial sampling — see cs123x_hal_arduino.h).
// -----------------------------------------------------------------------------
inline void cs123x_hal_idf_pin_output(gpio_num_t pin) {
    gpio_config_t io_conf = {};
    io_conf.pin_bit_mask = (1ULL << pin);
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.intr_type = GPIO_INTR_DISABLE;
    gpio_config(&io_conf);
}

inline void cs123x_hal_idf_pin_input(gpio_num_t pin) {
    gpio_config_t io_conf = {};
    io_conf.pin_bit_mask = (1ULL << pin);
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.intr_type = GPIO_INTR_DISABLE;
    gpio_config(&io_conf);
}

#define CS123X_SET_SCLK_OUTPUT() cs123x_hal_idf_pin_output(static_cast<gpio_num_t>(_sclk))
#define CS123X_SET_DOUT_OUTPUT() cs123x_hal_idf_pin_output(static_cast<gpio_num_t>(_dout))
#define CS123X_SET_DOUT_INPUT() cs123x_hal_idf_pin_input(static_cast<gpio_num_t>(_dout))

// -----------------------------------------------------------------------------
// Timing.
// -----------------------------------------------------------------------------
#define CS123X_MILLIS() (static_cast<uint32_t>(esp_timer_get_time() / 1000))
#define CS123X_YIELD() taskYIELD()

#ifndef CS123X_BIT_DELAY
#define CS123X_BIT_DELAY() esp_rom_delay_us(1)
#endif

#endif /* CS123X_HAL_ESPIDF_H */