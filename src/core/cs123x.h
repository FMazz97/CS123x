/// @file cs123x.h
/// @brief Core driver for the CS1237 / CS1238 24-bit ADC — framework-agnostic
///        public API (this is the class an ESP-IDF component would use directly).
/// @details The Arduino-facing name `CS123x` is a thin camelCase facade over this
///          class — see the top-level `CS123x.h`.
/// @copyright MIT License

#ifndef CS123X_CORE_H
#define CS123X_CORE_H

#include "cs123x_types.h"

#if defined(ARDUINO)
#include "../hal/cs123x_hal_arduino.h"
#elif defined(ESP_PLATFORM)
// TODO: ESP-IDF native HAL, not yet implemented.
// #include "../hal/cs123x_hal_idf.h"
#error "CS123x core: ESP-IDF native HAL not implemented yet — build with the Arduino framework for now."
#else
#error "CS123x core: unsupported platform (expected ARDUINO or ESP_PLATFORM)"
#endif

/**
 * @brief Driver class for the CS1237 and CS1238 24-bit ADC chips.
 */
class cs123x {
   private:
    CS123X_Type _cs123x_type;  ///< ADC chip model (CS123X_TYPE_...)
    uint8_t _dout;             ///< Bidirectional Data Output / Input GPIO pin
    uint8_t _sclk;             ///< Serial Clock / Power Down GPIO pin

#if defined(__AVR__)
    volatile uint8_t* _sclk_port_reg;      ///< Direct AVR PORTx register for SCLK (fast I/O)
    volatile uint8_t* _dout_out_port_reg;  ///< Direct AVR PORTx register for DOUT output (fast I/O)
    volatile uint8_t* _dout_in_port_reg;   ///< Direct AVR PINx register for DOUT input (fast I/O)
    uint8_t _sclk_bit_mask;                ///< Bitmask for SCLK pin within its AVR port
    uint8_t _dout_bit_mask;                ///< Bitmask for DOUT pin within its AVR port
#endif

    CS123X_Channel _channel;  ///< Currently selected input channel
    CS123X_Gain _gain;        ///< Active PGA gain setting
    CS123X_Gain _base_gain;   ///< Baseline PGA gain setting for normal channels
    CS123X_Rate _rate;        ///< Active data rate setting
    CS123X_IntRef _int_ref;   ///< Internal reference voltage state

    int32_t _offset = 0;  ///< Raw ADC zero-load offset in counts (tare value)
    float _scale = 1.0f;  ///< Scale factor (ADC counts per physical unit)

    float _ref_temp_c = 25.0f;  ///< Ambient reference temperature in °C during sensor calibration
    int32_t _ref_code = 0;      ///< Raw ADC code recorded at reference temperature (0 = uncalibrated)

    /**
     * @brief Polls is_ready() with a bounded timeout, yielding at most once per
     *        millisecond to minimize overhead on fast polling loops (e.g. at
     *        1280 Hz) while still preventing Watchdog Timer starvation on longer waits.
     * @param status If true, waits for is_ready()==true; if false, waits for
     *        is_ready()==false (used for the post-write DRDY-high phase).
     * @return true if the condition was met in time, false on timeout.
     */
    bool wait_ready(bool status);

    /**
     * @brief Reads the raw ADC value from the currently active channel, then writes
     *        a new configuration byte to the internal ADC register in the same
     *        bit-bang transaction for next reading process.
     * @param verify If true, reads back the register after writing to verify success.
     * @return Signed 32-bit integer (`int32_t`) sign-extended from 24-bit ADC data on
     *         success, `CS123X_TIMEOUT_ERROR` if the chip didn't respond in time, or
     *         `CS123X_SWITCH_ERROR` if the register write couldn't be verified
     *         (the returned reading should still be considered invalid in that case,
     *         since the following read depends on the channel switch having succeeded).
     */
    int32_t read_val_and_write_register(bool verify = true);

    /**
     * @brief Reads the configuration register from the ADC.
     * @return Raw configuration byte read from chip (Bit 0-6) and the update flag (Bit 7).
     */
    int32_t read_register();

    /**
     * @brief Reads a specified number of bits over the 2-wire serial protocol.
     * @param num_bits Number of bits to read.
     * @return 32-bit unsigned integer containing right-aligned read bits.
     */
    uint32_t read_bits(uint8_t num_bits);

    /**
     * @brief Writes a specified number of bits to the DOUT pin.
     * @param data Byte/value containing the bits to send.
     * @param num_bits Number of bits to send (starting from MSB).
     */
    void write_bits(uint8_t data, uint8_t num_bits);

    /**
     * @brief Generates dummy clock pulses (SCLK) to advance internal ADC state without data transfer.
     * @param count Number of clock pulses to generate.
     */
    void void_pulses(uint8_t count);

    /**
     * @brief Caches direct AVR PORT/PIN register pointers and bitmasks for SCLK/DOUT
     *        via the HAL's CS123X_INIT_FAST_IO(), bypassing per-call overhead in the
     *        timing-critical bit-bang loop. No-op on non-AVR architectures.
     * @note Must be called after pin numbers are known (called from begin()).
     */
    void init_fast_io();

   public:
    /**
     * @brief Constructor for the cs123x class.
     *
     * @param cs123x_type ADC model type (`CS123X_TYPE_CS1237` or `CS123X_TYPE_CS1238`).
     * @param dout Digital pin connected to the ADC DOUT pin.
     * @param sclk Digital pin connected to the ADC SCLK pin.
     * @param channel Input channel selection (Default: `CS123X_CH_A`).
     * @param gain Initial PGA gain (Default: `CS123X_GAIN_128`).
     * @param rate Initial conversion rate (Default: `CS123X_RATE_10Hz`).
     * @param int_ref Internal reference mode (Default: `CS123X_INT_REF_OFF`).
     */
    cs123x(CS123X_Type cs123x_type, uint8_t dout, uint8_t sclk,
           CS123X_Channel channel = CS123X_CH_A,
           CS123X_Gain gain = CS123X_GAIN_128,
           CS123X_Rate rate = CS123X_RATE_10Hz,
           CS123X_IntRef int_ref = CS123X_INT_REF_OFF);

    /** @brief Default virtual destructor. */
    virtual ~cs123x() = default;

    /**
     * @brief Initializes GPIO pins and applies initial configuration to the ADC.
     * @return true if the ADC responds and configures correctly, false on communication failure.
     */
    bool begin();

    /// @brief Wakes up the ADC from power-down mode.
    void power_up();

    /// @brief Puts the ADC into low-power mode by driving SCLK HIGH.
    void power_down();

    /**
     * @brief Checks if the ADC conversion is complete and data is ready to read.
     * @return true if data is ready (DOUT pin is LOW), false otherwise.
     */
    bool is_ready() const;

    /**
     * @brief Calculates the dynamic polling timeout based on current data rate.
     * @return Timeout duration in milliseconds.
     */
    uint32_t get_timeout_ms() const;

    /**
     * @brief Configures channel, PGA gain, and sample rate in a single operation.
     * @param channel `CS123X_CH_A`, `CS123X_CH_B` (CS1238 only), `CS123X_CH_TEMP`, or `CS123X_CH_SHORT`.
     * @param gain `CS123X_GAIN_1`, `CS123X_GAIN_2`, `CS123X_GAIN_64`, or `CS123X_GAIN_128`.
     * @param rate `CS123X_RATE_10Hz`, `CS123X_RATE_40Hz`, `CS123X_RATE_640Hz`, or `CS123X_RATE_1280Hz`.
     * @param verify If true, reads back the register to confirm configuration.
     * @return true if configuration was successfully written (and optionally verified);
     *         false on invalid parameters, or on hardware timeout/verification failure.
     * @note If `CS123X_CH_TEMP` is selected, the PGA gain is automatically overridden to 1x (`CS123X_GAIN_1`),
     *       while the provided `gain` is always stored as the baseline value,
     *       restored automatically when switching away from `CS123X_CH_TEMP`.
     * @note On write or verification failure, all internal configuration state is rolled back to previous values.
     */
    bool set_config(CS123X_Channel channel, CS123X_Gain gain, CS123X_Rate rate, bool verify = true);

    /**
     * @brief Convenience overload of `set_config` using a `CS123X_Config` struct.
     * @see set_config(CS123X_Channel, CS123X_Gain, CS123X_Rate, bool)
     */
    bool set_config(CS123X_Config config, bool verify = true) {
        return set_config(config.channel, config.gain, config.rate, verify);
    }

    /**
     * @brief Get active configuration state.
     * @return `CS123X_Config` struct with current channel, gain, rate and internal reference.
     */
    CS123X_Config get_config() const {
        return {_channel, _gain, _rate, _int_ref};
    }

    /**
     * @brief Configures internal voltage reference generator.
     * @param int_ref `CS123X_INT_REF_ON` to enable or `CS123X_INT_REF_OFF` to disable.
     * @param verify If true, reads back register to confirm configuration.
     * @return true on success, false on invalid parameters or hardware failure.
     */
    bool set_ref(CS123X_IntRef int_ref, bool verify = true);

    /// @brief Gets internal reference state (CS123X_INT_REF_...).
    CS123X_IntRef get_ref() const {
        return _int_ref;
    }

    /**
     * @brief Sets the Output Data Rate (ODR).
     * @param rate `CS123X_RATE_10Hz`, `CS123X_RATE_40Hz`, `CS123X_RATE_640Hz`, or `CS123X_RATE_1280Hz`.
     * @param verify If true, reads back register to confirm configuration.
     * @return true on success, false on invalid parameters or hardware failure.
     */
    bool set_rate(CS123X_Rate rate, bool verify = true) {
        return set_config(_channel, _base_gain, rate, verify);
    }

    /// @brief Gets active data rate setting (CS123X_RATE_...).
    CS123X_Rate get_rate() const {
        return _rate;
    }

    /**
     * @brief Configures the Programmable Gain Amplifier (PGA).
     * @param gain `CS123X_GAIN_1`, `CS123X_GAIN_2`, `CS123X_GAIN_64`, or `CS123X_GAIN_128`.
     * @param verify If true, reads back register to confirm configuration.
     * @return true on success, false on invalid parameters or hardware failure.
     * @note If called while `CS123X_CH_TEMP` is active, the value is saved as baseline
     *       and will be applied automatically upon returning to a standard channel.
     */
    bool set_gain(CS123X_Gain gain, bool verify = true) {
        return set_config(_channel, gain, _rate, verify);
    }

    /**
     * @brief Gets active PGA gain setting (CS123X_GAIN_...).
     * @note Returns CS123X_GAIN_1 while CS123X_CH_TEMP is selected.
     */
    CS123X_Gain get_gain() const {
        return _gain;
    }

    /**
     * @brief Gets baseline PGA gain setting for non-temperature channels (CS123X_GAIN_...).
     * @note Equal to get_gain() outside of the temperature channel.
     */
    CS123X_Gain get_base_gain() const {
        return _base_gain;
    }

    /**
     * @brief Selects the active analog input channel.
     * @param channel `CS123X_CH_A`, `CS123X_CH_B` (CS1238 only), `CS123X_CH_TEMP`, or `CS123X_CH_SHORT`.
     * @param verify If true, reads back register to confirm configuration.
     * @return true on success, false on invalid parameters or hardware failure.
     * @note Selecting `CS123X_CH_TEMP` automatically overrides PGA to 1x (`CS123X_GAIN_1`).
     *       Switching back to standard channels automatically restores the baseline PGA setting.
     */
    bool set_ch(CS123X_Channel channel, bool verify = true) {
        return set_config(channel, _base_gain, _rate, verify);
    }

    /// @brief Gets currently selected input channel (CS123X_CH_...).
    CS123X_Channel get_ch() const {
        return _channel;
    }

    /**
     * @brief Forces an immediate 24-bit reading without waiting for DOUT ready signal.
     * @warning Use only if you have already verified `is_ready()` is true.
     * @return Signed 32-bit integer (`int32_t`) sign-extended from 24-bit ADC data.
     */
    int32_t force_read();

    /**
     * @brief Synchronously waits for the ADC to become ready and returns a sign-extended 24-bit reading.
     * @return 32-bit signed reading, or `CS123X_TIMEOUT_ERROR` on timeout or bus failure.
     */
    int32_t read();

    /**
     * @brief Reads both channels of a dual-channel measurement, interleaving each
     *        read with the register write for the next channel switch, avoiding a
     *        wasted conversion cycle per switch, unlike calling read() and set_ch()
     *        separately.
     *
     * Generalizes beyond CS1238 dual-sensor setups: also applies to CS1237 use
     * cases like alternating `CS123X_CH_A`/`CS123X_CH_TEMP` for thermal
     * compensation, or `CS123X_CH_A`/`CS123X_CH_SHORT` for periodic auto-zero.
     *
     * @param channel1 First channel to read.
     * @param channel2 Second channel to read.
     * @param gain1 PGA gain for `channel1`. Overridden to `CS123X_GAIN_1` if `channel1` is `CS123X_CH_TEMP`.
     * @param gain2 PGA gain for `channel2`. Overridden to `CS123X_GAIN_1` if `channel2` is `CS123X_CH_TEMP`.
     * @param verify If true, reads back the register after each channel switch to confirm success.
     * @return A `CS123X_DualReading` with one raw ADC reading per channel. Each
     *         field holds `CS123X_INVALID_PARAM` if its channel/gain pair was
     *         invalid (no hardware communication attempted), `CS123X_TIMEOUT_ERROR`
     *         on hardware timeout, or `CS123X_SWITCH_ERROR` if a channel switch
     *         wasn't confirmed — in the latter two cases the reading depends on an
     *         unconfirmed/failed switch and must be considered invalid.
     * @note At the end of the reading, `channel1` and `gain1` will be the channel and the baseline gain
     *       left on at the end of the call; or the last-confirmed verified (`verify` = `true`) status.
     * @note Rejects `CS123X_CH_B` on a CS1237 (single-channel chip) for either parameter.
     */
    CS123X_DualReading read_dual_channel(CS123X_Channel channel1, CS123X_Channel channel2,
                                         CS123X_Gain gain1, CS123X_Gain gain2, bool verify = true);

    /// @brief Convenience overload of read_dual_channel() using the current baseline gain for both channels.
    /// @see read_dual_channel(CS123X_Channel, CS123X_Channel, CS123X_Gain, CS123X_Gain, bool)
    CS123X_DualReading read_dual_channel(CS123X_Channel channel1, CS123X_Channel channel2, bool verify = true) {
        return read_dual_channel(channel1, channel2, _base_gain, _base_gain, verify);
    }

    /// @brief Convenience overload of read_dual_channel() using the current channel/gain as the first channel.
    /// @see read_dual_channel(CS123X_Channel, CS123X_Channel, CS123X_Gain, CS123X_Gain, bool)
    CS123X_DualReading read_dual_channel(CS123X_Channel channel2, CS123X_Gain gain2, bool verify = true) {
        return read_dual_channel(_channel, channel2, _base_gain, gain2, verify);
    }

    /// @brief Convenience overload of read_dual_channel() using the current channel/gain for both slots.
    /// @see read_dual_channel(CS123X_Channel, CS123X_Channel, CS123X_Gain, CS123X_Gain, bool)
    CS123X_DualReading read_dual_channel(CS123X_Channel channel2, bool verify = true) {
        return read_dual_channel(_channel, channel2, _base_gain, _base_gain, verify);
    }

    /**
     * @brief Reads multiple raw samples and returns their arithmetic mean.
     * @param samples Number of samples to average (default: 10).
     * @return Average raw 24-bit ADC code, or CS123X_TIMEOUT_ERROR if all reads fail.
     */
    int32_t read_average(uint16_t samples = 10);

    /**
     * @brief Reads the differential input voltage.
     * @param v_ref Reference voltage in Volts (default: 2.5f to follow reference breakout board with TL431).
     * @param samples Number of samples to average (default: 1).
     * @return The differential input voltage, or NAN if all reads fail.
     */
    float read_voltage(float v_ref = 2.5f, uint8_t samples = 1);

    /**
     * @brief Sets the zero point (tare) by averaging current readings at no load.
     * @param samples Number of samples to average (default: 10).
     * @return true if tare was set successfully, false on hardware timeout.
     */
    bool tare(uint8_t samples = 10);

    /**
     * @brief Calculates and sets the scale factor using a known calibration weight.
     * @param known_weight The weight applied to the scale in desired units (e.g., grams).
     * @param samples Number of samples to average (default: 10).
     * @return true if calibration succeeded, false on timeout, invalid weight (<= 0), or scale == 0.
     */
    bool calibrate_scale(float known_weight, uint8_t samples = 10);

    /**
     * @brief Reads average ADC value and subtracts the tare offset.
     * @param samples Number of samples to average (default: 1).
     * @return Raw ADC code minus offset, or CS123X_TIMEOUT_ERROR on hardware timeout.
     */
    int32_t get_value(uint8_t samples = 1);

    /**
     * @brief Calculates the weight in physical units (e.g., grams, kg).
     * @param samples Number of samples to average (default: 1).
     * @return Weight in physical units, or NAN if uncalibrated (scale == 0) or on timeout.
     */
    float get_units(uint8_t samples = 1);

    /// @brief Manually sets the tare offset directly (e.g., restored from EEPROM/Flash).
    void set_offset(int32_t offset) {
        _offset = offset;
    }

    /// @brief Gets the current tare offset (e.g., to save to EEPROM).
    int32_t get_offset() const {
        return _offset;
    }

    /// @brief Manually sets the calibration scale factor directly (e.g., restored from EEPROM).
    void set_scale(float scale) {
        _scale = scale;
    }

    /// @brief Gets the current scale factor (e.g., to save to EEPROM).
    float get_scale() const {
        return _scale;
    }

    /**
     * @brief Calibrates the temperature sensor live by reading the current raw code from the IC.
     * @param ref_temp_c Current known ambient/chip temperature in Celsius (e.g., 25.0f).
     * @return true if calibration succeeded, false on hardware timeout.
     */
    bool set_temp_calibration(float ref_temp_c);

    /**
     * @brief Manually sets reference parameters (e.g., restored from EEPROM/Flash).
     * @param ref_temp_c Reference temperature in Celsius.
     * @param ref_code Raw 24-bit ADC reading associated with ref_temp_c.
     */
    void set_temp_calibration(float ref_temp_c, int32_t ref_code);

    /**
     * @brief Reads internal temperature sensor and calculates temperature in Celsius.
     * @param samples Number of ADC readings to average for noise reduction (default: 1).
     * @param verify If true, reads back register to confirm configuration.
     * @return Temperature in °C, or `NAN` if uncalibrated, on hardware timeout,
     *         or if channel switching/restoration fails.
     */
    float read_temperature(uint8_t samples = 1, bool verify = true);
};

#endif /* CS123X_CORE_H */