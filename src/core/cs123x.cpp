/// @file cs123x.cpp
/// @brief Implementation of the framework-agnostic CS123x core.
/// @details All hardware access goes through CS123X_* macros provided by the
///          HAL selected in cs123x.h — this file contains no direct Arduino
///          or ESP-IDF API calls.
/// @copyright MIT License

#include "cs123x.h"
#include <math.h>

#if defined(ARDUINO_ARCH_ESP32)
portMUX_TYPE cs123x_mux = portMUX_INITIALIZER_UNLOCKED;
#endif

cs123x::cs123x(CS123X_Type cs123x_type, uint8_t dout, uint8_t sclk,
               CS123X_Channel channel, CS123X_Gain gain, CS123X_Rate rate,
               CS123X_IntRef int_ref) : _cs123x_type((cs123x_type <= CS123X_TYPE_CS1238) ? cs123x_type : CS123X_TYPE_CS1237),
                                        _dout(dout),
                                        _sclk(sclk),
                                        // Fallback to safe defaults if invalid parameters are provided
                                        _base_gain((gain <= CS123X_GAIN_128) ? gain : CS123X_GAIN_128),
                                        _rate((rate <= CS123X_RATE_1280Hz) ? rate : CS123X_RATE_10Hz),
                                        _int_ref((int_ref <= CS123X_INT_REF_OFF) ? int_ref : CS123X_INT_REF_OFF) {
    // Channel validation considering chip type
    if (channel <= CS123X_CH_SHORT) {
        if (_cs123x_type == CS123X_TYPE_CS1237 && channel == CS123X_CH_B) {
            _channel = CS123X_CH_A;  // CS1237 doesn't have CH_B
        } else {
            _channel = channel;
        }
    } else {
        _channel = CS123X_CH_A;
    }

    // Fix gain = 1 for if on-chip temperature sensor was enabled
    _gain = (_channel == CS123X_CH_TEMP) ? CS123X_GAIN_1 : _base_gain;
}

bool cs123x::begin() {
    CS123X_SET_DOUT_INPUT();
    CS123X_SET_SCLK_OUTPUT();
    init_fast_io();
    power_up();

    return !(read_val_and_write_register(true) >= CS123X_TIMEOUT_ERROR);
}

void cs123x::init_fast_io() {
    CS123X_INIT_FAST_IO();
}

void cs123x::power_up() {
    CS123X_SCLK_LOW();
}

void cs123x::power_down() {
    CS123X_SCLK_HIGH();
}

bool cs123x::is_ready() const {
    return !CS123X_DOUT_READ();
}

bool cs123x::wait_ready(bool status) {
    uint32_t start = CS123X_MILLIS();
    uint32_t last_yield = start;
    while (is_ready() != status) {
        uint32_t now = CS123X_MILLIS();
        if (now - start >= get_timeout_ms()) return false;
        if (now != last_yield) {
            CS123X_YIELD();  // Yield to background system tasks.
            last_yield = CS123X_MILLIS();
        }
    }
    return true;
}

int32_t cs123x::read_val_and_write_register(bool verify) {
    int32_t value = 0;
    uint8_t config = 0x00;
    // built config byte
    // Bit 7 -> reserved (always = 0)
    // Bit 6 -> INT_REF
    // Bit 5:4 -> RATE, Bit 3:2 -> GAIN, Bit 1:0 -> CHANNEL
    config |= (_int_ref & 0b1) << 6;
    config |= (_rate & 0b11) << 4;
    config |= (_gain & 0b11) << 2;
    config |= (_channel & 0b11);

    if (!wait_ready(true)) return CS123X_TIMEOUT_ERROR;

    // Avoid interruption during critical stage
    CS123X_CRITICAL_VAR();
    CS123X_ENTER_CRITICAL();

    value = read_bits(24);  // bit 1-24
    void_pulses(3);         // bit 25-27, no previously update acquisition

    void_pulses(2);  // bit 28-29

    CS123X_DOUT_HIGH();        // avoid LOW status at pinMode change
    CS123X_SET_DOUT_OUTPUT();  // change pinMode to write data
    write_bits(0x65, 7);       // 0x65 is write configuration register (7bit) bit 30-36

    void_pulses(1);  // bit 37

    write_bits(config, 8);    // bit 38-45
    CS123X_SET_DOUT_INPUT();  // change pinMode to normal operation

    void_pulses(1);  // bit 46

    CS123X_EXIT_CRITICAL();

    // fix negative value from 24 to 32 bit
    if (value & 0x800000) value |= 0xFF000000;

    if (verify) {
        if (!wait_ready(false)) return CS123X_SWITCH_ERROR;
        if (!wait_ready(true)) return CS123X_SWITCH_ERROR;

        int32_t read_back = read_register();
        if (read_back >= CS123X_TIMEOUT_ERROR)
            return CS123X_SWITCH_ERROR;
        else if ((read_back & 0x80) && ((read_back & 0x7F) == config))
            return value;
        else
            return CS123X_SWITCH_ERROR;
    } else
        return value;
}

int32_t cs123x::read_register() {
    bool update = false;
    uint8_t config = 0;

    if (!wait_ready(true)) return CS123X_TIMEOUT_ERROR;

    // Avoid interruption during critical stage
    CS123X_CRITICAL_VAR();
    CS123X_ENTER_CRITICAL();

    void_pulses(24);                      // bit 1-24, no conversion acquisition
    update = (read_bits(3) & 0x04) != 0;  // bit 25-27, only 25th bit contain previously update status
    void_pulses(2);                       // bit 28-29

    CS123X_DOUT_HIGH();        // avoid LOW status at pinMode change
    CS123X_SET_DOUT_OUTPUT();  // change pinMode to write data
    write_bits(0x56, 7);       // 0x56 is read configuration register (7bit) bit 30-36
    CS123X_SET_DOUT_INPUT();   // change pinMode to read data and normal operation

    void_pulses(1);  // bit 37

    config = read_bits(8);  // bit 38-45

    void_pulses(1);  // bit 46

    CS123X_EXIT_CRITICAL();

    // Use reserved bit[7] to store update value
    return (update ? 0x80 : 0x00) | config;
}

uint32_t cs123x::read_bits(uint8_t num_bits) {
    uint32_t value = 0;

    for (uint8_t i = 0; i < num_bits; i++) {
        CS123X_SCLK_HIGH();
        CS123X_BIT_DELAY();
        value = (value << 1) | CS123X_DOUT_READ();
        CS123X_SCLK_LOW();
        CS123X_BIT_DELAY();
    }
    return value;
}

void cs123x::write_bits(uint8_t data, uint8_t num_bits) {
    for (int8_t i = num_bits - 1; i >= 0; i--) {  // from MSB to LSB
        if ((data >> i) & 0x01)
            CS123X_DOUT_HIGH();
        else
            CS123X_DOUT_LOW();
        CS123X_SCLK_HIGH();
        CS123X_BIT_DELAY();
        CS123X_SCLK_LOW();
        CS123X_BIT_DELAY();
    }
}

void cs123x::void_pulses(uint8_t count) {
    for (uint8_t i = 0; i < count; i++) {
        CS123X_SCLK_HIGH();
        CS123X_BIT_DELAY();
        CS123X_SCLK_LOW();
        CS123X_BIT_DELAY();
    }
}

uint32_t cs123x::get_timeout_ms() const {
    // Rate		|	Setting time (t2)	|   Conversion time (t9)    |   Timeout
    // 10Hz		|	300ms				|	100ms                   |	325ms
    // 40Hz		|	75ms				|	25ms                    |	100ms
    // 640Hz	|	6.25ms				|	1.5625ms                |	15ms
    // 1280Hz	|	3.125ms				|	0.78125ms               |	10ms
    static constexpr uint32_t timeouts[4] = {325, 100, 15, 10};

    return timeouts[_rate & 0x03];
}

bool cs123x::set_config(CS123X_Channel channel, CS123X_Gain gain, CS123X_Rate rate, bool verify) {
    if (channel > CS123X_CH_SHORT) return false;  // Reject values > 3
    if (gain > CS123X_GAIN_128) return false;     // Reject values > 3
    if (rate > CS123X_RATE_1280Hz) return false;  // Reject values > 3

    // Block Channel B selection on single-channel chip (CS1237)
    if (_cs123x_type == CS123X_TYPE_CS1237 && channel == CS123X_CH_B) return false;

    // Backups value for rollback
    CS123X_Channel current_channel = _channel;
    CS123X_Gain current_gain = _gain;
    CS123X_Gain current_base_gain = _base_gain;
    CS123X_Rate current_rate = _rate;

    _channel = channel;
    _base_gain = gain;
    _gain = (channel == CS123X_CH_TEMP) ? CS123X_GAIN_1 : gain;
    _rate = rate;

    if (read_val_and_write_register(verify) >= CS123X_TIMEOUT_ERROR) {
        // Rollback to previous value in case of fail readback
        _channel = current_channel;
        _gain = current_gain;
        _base_gain = current_base_gain;
        _rate = current_rate;
        return false;
    }
    return true;
}

bool cs123x::set_ref(CS123X_IntRef int_ref, bool verify) {
    if (int_ref > CS123X_INT_REF_OFF) return false;  // Reject values > 1

    CS123X_IntRef current_ref = _int_ref;
    _int_ref = int_ref;

    if (read_val_and_write_register(verify) >= CS123X_TIMEOUT_ERROR) {
        _int_ref = current_ref;  // Rollback
        return false;
    }
    return true;
}

int32_t cs123x::force_read() {
    int32_t value = 0;
    CS123X_CRITICAL_VAR();
    CS123X_ENTER_CRITICAL();

    value = read_bits(24);
    void_pulses(3);

    CS123X_EXIT_CRITICAL();

    // fix negative value from 24 to 32 bit
    if (value & 0x800000) value |= 0xFF000000;

    return value;
}

int32_t cs123x::read() {
    if (!wait_ready(true)) return CS123X_TIMEOUT_ERROR;
    return force_read();
}

CS123X_DualReading cs123x::read_dual_channel(CS123X_Channel channel1, CS123X_Channel channel2,
                                             CS123X_Gain gain1, CS123X_Gain gain2,
                                             bool verify) {
    bool ch1_invalid = (channel1 > CS123X_CH_SHORT) || (gain1 > CS123X_GAIN_128) ||
                       (_cs123x_type == CS123X_TYPE_CS1237 && channel1 == CS123X_CH_B);

    bool ch2_invalid = (channel2 > CS123X_CH_SHORT) || (gain2 > CS123X_GAIN_128) ||
                       (_cs123x_type == CS123X_TYPE_CS1237 && channel2 == CS123X_CH_B);
    if (ch1_invalid || ch2_invalid) {
        return {
            ch1_invalid ? CS123X_INVALID_PARAM : 0,
            ch2_invalid ? CS123X_INVALID_PARAM : 0};
    }

    // Effective gains to be set
    CS123X_Gain eff_gain1 = (channel1 == CS123X_CH_TEMP) ? CS123X_GAIN_1 : gain1;
    CS123X_Gain eff_gain2 = (channel2 == CS123X_CH_TEMP) ? CS123X_GAIN_1 : gain2;

    // ensure correct configuration was applied
    if (_channel != channel1 || _gain != eff_gain1) {
        if (!set_config(channel1, eff_gain1, _rate, verify)) {
            return {CS123X_SWITCH_ERROR, CS123X_SWITCH_ERROR};
        }
    }

    // Backups value for rollback
    CS123X_Channel current_channel = _channel;
    CS123X_Gain current_gain = _gain;
    CS123X_Gain current_base_gain = _base_gain;

    // Prepare configuration for switch channel after reading
    _channel = channel2;
    _gain = eff_gain2;
    _base_gain = gain2;

    CS123X_DualReading result;
    result.ch1 = read_val_and_write_register(verify);
    if (result.ch1 >= CS123X_TIMEOUT_ERROR) {
        _channel = current_channel;
        _gain = current_gain;
        _base_gain = current_base_gain;
        result.ch2 = CS123X_SWITCH_ERROR;
        return result;
    }

    // Backups value for rollback
    current_channel = _channel;
    current_gain = _gain;
    current_base_gain = _base_gain;

    // Restore configuration to 1st channel after reading
    _channel = channel1;
    _gain = eff_gain1;
    _base_gain = gain1;

    result.ch2 = read_val_and_write_register(verify);
    if (result.ch2 >= CS123X_TIMEOUT_ERROR) {
        _channel = current_channel;
        _gain = current_gain;
        _base_gain = current_base_gain;
        return result;
    }

    return result;
}

int32_t cs123x::read_average(uint16_t samples) {
    if (samples <= 1) {
        return read();
    }

    int64_t sum = 0;
    uint8_t valid_count = 0;

    for (uint8_t i = 0; i < samples; ++i) {
        int32_t raw = read();

        if (raw != CS123X_TIMEOUT_ERROR) {
            sum += raw;
            valid_count++;
        }
    }

    if (valid_count == 0) {
        return CS123X_TIMEOUT_ERROR;
    }

    return static_cast<int32_t>(sum / valid_count);
}

float cs123x::read_voltage(float v_ref, uint8_t samples) {
    int32_t raw = read_average(samples);
    if (raw == CS123X_TIMEOUT_ERROR) return NAN;

    static constexpr uint8_t gain_multipliers[4] = {1, 2, 64, 128};
    uint32_t denom = static_cast<uint32_t>(CS123X_MAX_VALUE) * 2 * gain_multipliers[_gain & 0x03];
    return static_cast<float>(raw) * v_ref / denom;
}

bool cs123x::tare(uint8_t samples) {
    int32_t raw = read_average(samples);
    if (raw == CS123X_TIMEOUT_ERROR) return false;

    _offset = raw;
    return true;
}

bool cs123x::calibrate_scale(float known_weight, uint8_t samples) {
    if (known_weight <= 0.0f) return false;  // Avoid negative weight

    int32_t raw = read_average(samples);
    if (raw == CS123X_TIMEOUT_ERROR) return false;

    int32_t delta_raw = raw - _offset;

    if (delta_raw == 0) return false;  // Avoid to divide by 0 in get_units()

    _scale = static_cast<float>(delta_raw) / known_weight;
    return true;
}

int32_t cs123x::get_value(uint8_t samples) {
    int32_t raw = read_average(samples);
    if (raw == CS123X_TIMEOUT_ERROR) {
        return CS123X_TIMEOUT_ERROR;
    }
    return raw - _offset;
}

float cs123x::get_units(uint8_t samples) {
    if (_scale == 0.0f) return NAN;  // Avoid to divide by 0, scale not calibrated

    int32_t val = get_value(samples);
    if (val == CS123X_TIMEOUT_ERROR) return NAN;

    return static_cast<float>(val) / _scale;
}

bool cs123x::set_temp_calibration(float ref_temp_c) {
    CS123X_Channel previous_channel = _channel;

    if (previous_channel != CS123X_CH_TEMP) {
        if (!set_ch(CS123X_CH_TEMP)) return false;
    }

    int32_t raw_code = read();

    if (previous_channel != CS123X_CH_TEMP) {
        if (!set_ch(previous_channel)) return false;  // Fallback exit
    }

    if (raw_code == CS123X_TIMEOUT_ERROR) return false;

    _ref_temp_c = ref_temp_c;
    _ref_code = raw_code;
    return true;
}

void cs123x::set_temp_calibration(float ref_temp_c, int32_t ref_code) {
    _ref_temp_c = ref_temp_c;
    _ref_code = ref_code;
}

float cs123x::read_temperature(uint8_t samples, bool verify) {
    if (_ref_code == 0) return NAN;  // Uncalibrated state _ref_code = 0 should be at 0K, impossible

    CS123X_Channel previous_channel = _channel;

    if (previous_channel != CS123X_CH_TEMP) {
        if (!set_ch(CS123X_CH_TEMP, verify)) return NAN;
    }

    int32_t raw_code = read_average(samples);

    if (previous_channel != CS123X_CH_TEMP) {
        if (!set_ch(previous_channel, verify)) return NAN;  // Fallback exit
    }

    if (raw_code == CS123X_TIMEOUT_ERROR) return NAN;

    // B = Yb * (273.15 + Ta) / Ya - 273.15
    return (static_cast<float>(raw_code) * (273.15f + _ref_temp_c)) / _ref_code - 273.15f;
}