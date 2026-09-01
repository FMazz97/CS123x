/// @file CS123x.h
/// @brief Arduino-facing facade for the CS123x 24-bit ADC library (CS1237 / CS1238).
/// @details Thin camelCase wrapper over the framework-agnostic `cs123x` core —
///          every method forwards 1:1, no logic lives here.
/// @author FMazz97 (https://github.com/FMazz97)
/// @see CS123x GitHub Repository: https://github.com/FMazz97/CS123x
/// @copyright MIT License

#ifndef CS123X_H
#define CS123X_H

#include "core/cs123x.h"

/**
 * @brief Driver class for the CS1237 and CS1238 24-bit ADC chips (Arduino facade).
 * @see cs123x for the full method documentation (identical behavior, snake_case).
 */
class CS123x {
   private:
    cs123x _core;

   public:
    CS123x(CS123X_Type cs123xType, uint8_t dout, uint8_t sclk,
           CS123X_Channel channel = CS123X_CH_A,
           CS123X_Gain gain = CS123X_GAIN_128,
           CS123X_Rate rate = CS123X_RATE_10Hz,
           CS123X_IntRef intRef = CS123X_INT_REF_OFF)
        : _core(cs123xType, dout, sclk, channel, gain, rate, intRef) {}

    bool begin() { return _core.begin(); }
    void powerUp() { _core.power_up(); }
    void powerDown() { _core.power_down(); }
    bool isReady() const { return _core.is_ready(); }
    uint32_t getTimeoutMs() const { return _core.get_timeout_ms(); }

    bool setConfig(CS123X_Channel channel, CS123X_Gain gain, CS123X_Rate rate, bool verify = true) {
        return _core.set_config(channel, gain, rate, verify);
    }
    bool setConfig(CS123X_Config config, bool verify = true) { return _core.set_config(config, verify); }
    CS123X_Config getConfig() const { return _core.get_config(); }

    bool setRef(CS123X_IntRef intRef, bool verify = true) { return _core.set_ref(intRef, verify); }
    CS123X_IntRef getRef() const { return _core.get_ref(); }

    bool setRate(CS123X_Rate rate, bool verify = true) { return _core.set_rate(rate, verify); }
    CS123X_Rate getRate() const { return _core.get_rate(); }

    bool setGain(CS123X_Gain gain, bool verify = true) { return _core.set_gain(gain, verify); }
    CS123X_Gain getGain() const { return _core.get_gain(); }
    CS123X_Gain getBaseGain() const { return _core.get_base_gain(); }

    bool setCh(CS123X_Channel channel, bool verify = true) { return _core.set_ch(channel, verify); }
    CS123X_Channel getCh() const { return _core.get_ch(); }

    int32_t forceRead() { return _core.force_read(); }
    int32_t read() { return _core.read(); }

    CS123X_DualReading readDualChannel(CS123X_Channel channel1, CS123X_Channel channel2,
                                       CS123X_Gain gain1, CS123X_Gain gain2, bool verify = true) {
        return _core.read_dual_channel(channel1, channel2, gain1, gain2, verify);
    }
    CS123X_DualReading readDualChannel(CS123X_Channel channel1, CS123X_Channel channel2, bool verify = true) {
        return _core.read_dual_channel(channel1, channel2, verify);
    }
    CS123X_DualReading readDualChannel(CS123X_Channel channel2, CS123X_Gain gain2, bool verify = true) {
        return _core.read_dual_channel(channel2, gain2, verify);
    }
    CS123X_DualReading readDualChannel(CS123X_Channel channel2, bool verify = true) {
        return _core.read_dual_channel(channel2, verify);
    }

    int32_t readAverage(uint16_t samples = 10) { return _core.read_average(samples); }
    float readVoltage(float vRef = 2.5f, uint8_t samples = 1) { return _core.read_voltage(vRef, samples); }

    bool tare(uint8_t samples = 10) { return _core.tare(samples); }
    bool calibrateScale(float knownWeight, uint8_t samples = 10) { return _core.calibrate_scale(knownWeight, samples); }
    int32_t getValue(uint8_t samples = 1) { return _core.get_value(samples); }
    float getUnits(uint8_t samples = 1) { return _core.get_units(samples); }

    void setOffset(int32_t offset) { _core.set_offset(offset); }
    int32_t getOffset() const { return _core.get_offset(); }
    void setScale(float scale) { _core.set_scale(scale); }
    float getScale() const { return _core.get_scale(); }

    bool setTempCalibration(float refTempC) { return _core.set_temp_calibration(refTempC); }
    void setTempCalibration(float refTempC, int32_t refCode) { _core.set_temp_calibration(refTempC, refCode); }
    float readTemperature(uint8_t samples = 1, bool verify = true) { return _core.read_temperature(samples, verify); }
};

#endif /* CS123X_H */