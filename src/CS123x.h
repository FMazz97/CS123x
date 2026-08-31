/// @file CS123x.h
/// @brief Main header file for the CS123x 24-bit ADC library (CS1237 / CS1238).
/// @details Provides class definitions, constant definitions, hardware register masks,
///          and public API declarations for interfacing with CS1237 and CS1238 ADCs
///          over a custom 2-wire serial interface.
/// @author FMazz97 (https://github.com/FMazz97)
/// @see CS123x GitHub Repository: https://github.com/FMazz97/CS123x
/// @copyright MIT License

#ifndef CS123X_H
#define CS123X_H

#include <Arduino.h>

/**
 * @brief CS123X ADC model type.
 */
enum CS123X_Type : uint8_t {
    /** @brief CS1237 ADC model (single differential channel) */
    CS123X_TYPE_CS1237 = 0x00,
    /** @brief CS1238 ADC model (selectable A/B channels) */
    CS123X_TYPE_CS1238 = 0x01
};

/**
 * @brief Input channel selection.
 */
enum CS123X_Channel : uint8_t {
    /** @brief Input channel A (Default on ADC) */
    CS123X_CH_A = 0x00,
    /** @brief Input channel B (CS1238 only) */
    CS123X_CH_B = 0x01,
    /** @brief On-chip temperature sensor */
    CS123X_CH_TEMP = 0x02,
    /** @brief Internal short circuit for offset calibration/measurement */
    CS123X_CH_SHORT = 0x03
};

/**
 * @brief Programmable Gain Amplifier (PGA) selection.
 */
enum CS123X_Gain : uint8_t {
    /** @brief Programmable Gain Amplifier (PGA) = 1x */
    CS123X_GAIN_1 = 0x00,
    /** @brief Programmable Gain Amplifier (PGA) = 2x */
    CS123X_GAIN_2 = 0x01,
    /** @brief Programmable Gain Amplifier (PGA) = 64x */
    CS123X_GAIN_64 = 0x02,
    /** @brief Programmable Gain Amplifier (PGA) = 128x (Default on ADC) */
    CS123X_GAIN_128 = 0x03
};

/**
 * @brief Data output rate selection.
 */
enum CS123X_Rate : uint8_t {
    /** @brief Data output rate 10 Hz (Default on ADC) */
    CS123X_RATE_10Hz = 0x00,
    /** @brief Data output rate 40 Hz */
    CS123X_RATE_40Hz = 0x01,
    /** @brief Data output rate 640 Hz */
    CS123X_RATE_640Hz = 0x02,
    /** @brief Data output rate 1280 Hz */
    CS123X_RATE_1280Hz = 0x03
};

/**
 * @brief Internal voltage reference configuration.
 */
enum CS123X_IntRef : uint8_t {
    /** @brief Enable internal voltage reference (VREF = VDD, Default on ADC) */
    CS123X_INT_REF_ON = 0x00,
    /** @brief Disable internal voltage reference (requires external VREF) */
    CS123X_INT_REF_OFF = 0x01
};

/**
 * @brief Complete internal configuration.
 */
struct CS123X_Config {
    /** @brief Input channel selection*/
    CS123X_Channel channel;
    /** @brief Programmable Gain Amplifier (PGA) selection. */
    CS123X_Gain gain;
    /** @brief Data output rate selection*/
    CS123X_Rate rate;
    /** @brief Internal voltage reference configuration. */
    CS123X_IntRef intRef;

    /// Compares two configurations for equality.
    bool operator==(const CS123X_Config& config) const {
        return channel == config.channel && gain == config.gain && rate == config.rate && intRef == config.intRef;
    }

    /// Compares two configurations for inequality.
    bool operator!=(const CS123X_Config& config) const {
        return channel != config.channel || gain != config.gain || rate != config.rate || intRef != config.intRef;
    }
};

/**
 * @brief Result of a dual-channel read + config-switch operation.
 * @note Each field holds a raw ADC reading, or `CS123X_TIMEOUT_ERROR`/`CS123X_SWITCH_ERROR`/`CS123X_INVALID_PARAM`
 *       on failure — in the `CS123X_SWITCH_ERROR` case, the reading depends on an unconfirmed channel switch
 *       and must be considered invalid.
 */
struct CS123X_DualReading {
    int32_t ch1;  ///< Raw ADC reading from the 1st channel.
    int32_t ch2;  ///< Raw ADC reading from the 2nd channel.

    /// Compares two dual-channel readings for equality.
    bool operator==(const CS123X_DualReading& dualReading) const {
        return ch1 == dualReading.ch1 && ch2 == dualReading.ch2;
    }

    /// Compares two dual-channel readings for inequality.
    bool operator!=(const CS123X_DualReading& dualReading) const {
        return ch1 != dualReading.ch1 || ch2 != dualReading.ch2;
    }
};

// =============================================================================
// Hardware Constants & Sentinel Values
// =============================================================================

/** @brief Minimum 24-bit output value (0x800000 in 2s complement) */
constexpr int32_t CS123X_MIN_VALUE = 0xFF800000;

/** @brief Maximum 24-bit output value (+0x7FFFFF in 2s complement) */
constexpr int32_t CS123X_MAX_VALUE = 0x007FFFFF;

/** @brief Sentinel error value returned by read() on hardware communication timeout */
constexpr int32_t CS123X_TIMEOUT_ERROR = 0x7FFFFFF0;

/** @brief Sentinel error value returned when a channel-switch write/verify fails
 *  during a combined read+switch operation (e.g. readDualChannel()).
 */
constexpr int32_t CS123X_SWITCH_ERROR = 0x7FFFFFF1;

/** @brief Sentinel error value returned when one or more parameters passed to
 *  a configuration call are invalid (e.g. readDualChannel()).
 *  No hardware communication is attempted in this case.
 */
constexpr int32_t CS123X_INVALID_PARAM = 0x7FFFFFF2;

// New sentinels: keep contiguous & >= CS123X_TIMEOUT_ERROR (for function(verify) path errors); below it, add separately.

// =============================================================================
// Class definition
// =============================================================================

/**
 * @brief Driver class for the CS1237 and CS1238 24-bit ADC chips.
 */
class CS123x {
   private:
    CS123X_Type _cs123xType;  ///< ADC chip model (CS123X_TYPE_...)
    uint8_t _dout;            ///< Bidirectional Data Output / Input GPIO pin
    uint8_t _sclk;            ///< Serial Clock / Power Down GPIO pin

#if defined(__AVR__)
    volatile uint8_t* _sclkPortReg;     ///< Direct AVR PORTx register for SCLK (fast I/O)
    volatile uint8_t* _doutOutPortReg;  ///< Direct AVR PORTx register for DOUT output (fast I/O)
    volatile uint8_t* _doutInPortReg;   ///< Direct AVR PINx register for DOUT input (fast I/O)
    uint8_t _sclkBitMask;               ///< Bitmask for SCLK pin within its AVR port
    uint8_t _doutBitMask;               ///< Bitmask for DOUT pin within its AVR port
#endif

    CS123X_Channel _channel;  ///< Currently selected input channel
    CS123X_Gain _gain;        ///< Active PGA gain setting
    CS123X_Gain _baseGain;    ///< Baseline PGA gain setting for normal channels
    CS123X_Rate _rate;        ///< Active data rate setting
    CS123X_IntRef _intRef;    ///< Internal reference voltage state

    int32_t _offset = 0;  ///< Raw ADC zero-load offset in counts (tare value)
    float _scale = 1.0f;  ///< Scale factor (ADC counts per physical unit)

    float _refTempC = 25.0f;  ///< Ambient reference temperature in °C during sensor calibration
    int32_t _refCode = 0;     ///< Raw ADC code recorded at reference temperature (0 = uncalibrated)
    
    /**
     * @brief Polls isReady() with a bounded timeout, yielding at most once per
     *        millisecond to minimize overhead on fast polling loops (e.g. at
     *        1280 Hz) while still preventing Watchdog Timer starvation on longer waits.
     * @param status If true, waits for isReady()==true; if false, waits for
     *        isReady()==false (used for the post-write DRDY-high phase).
     * @return true if the condition was met in time, false on timeout.
     */
    bool waitReady(bool status);

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
    int32_t readValAndWriteRegister(bool verify = true);

    /**
     * @brief Reads the configuration register from the ADC.
     * @return Raw configuration byte read from chip (Bit 0-6) and the update flag (Bit 7).
     */
    int32_t readRegister();

    /**
     * @brief Reads a specified number of bits over the 2-wire serial protocol.
     * @param numBits Number of bits to read.
     * @return 32-bit unsigned integer containing right-aligned read bits.
     */
    uint32_t readBits(uint8_t numBits);

    /**
     * @brief Writes a specified number of bits to the DOUT pin.
     * @param data Byte/value containing the bits to send.
     * @param numBits Number of bits to send (starting from MSB).
     */
    void writeBits(uint8_t data, uint8_t numBits);

    /**
     * @brief Generates dummy clock pulses (SCLK) to advance internal ADC state without data transfer.
     * @param count Number of clock pulses to generate.
     */
    void voidPulses(uint8_t count);

    /**
     * @brief Caches direct AVR PORT/PIN register pointers and bitmasks for SCLK/DOUT,
     *        bypassing digitalWrite()/digitalRead() overhead in the timing-critical
     *        bit-bang loop. No-op on non-AVR architectures.
     * @note Must be called after pin numbers are known (called from begin()).
     */
    void initFastIO();

   public:
    /**
     * @brief Constructor for the CS123x class.
     *
     * @param cs123xType ADC model type (`CS123X_TYPE_CS1237` or `CS123X_TYPE_CS1238`).
     * @param dout Arduino digital pin connected to the ADC DOUT pin.
     * @param sclk Arduino digital pin connected to the ADC SCLK pin.
     * @param channel Input channel selection (Default: `CS123X_CH_A`).
     * @param gain Initial PGA gain (Default: `CS123X_GAIN_128`).
     * @param rate Initial conversion rate (Default: `CS123X_RATE_10Hz`).
     * @param intRef Internal reference mode (Default: `CS123X_INT_REF_OFF`).
     */
    CS123x(CS123X_Type cs123xType, uint8_t dout, uint8_t sclk,
           CS123X_Channel channel = CS123X_CH_A,
           CS123X_Gain gain = CS123X_GAIN_128,
           CS123X_Rate rate = CS123X_RATE_10Hz,
           CS123X_IntRef intRef = CS123X_INT_REF_OFF);

    /** @brief Default virtual destructor. */
    virtual ~CS123x() = default;

    /**
     * @brief Initializes GPIO pins and applies initial configuration to the ADC.
     * @note Call this function inside your sketch's `setup()`.
     * @return true if the ADC responds and configures correctly, false on communication failure.
     */
    bool begin();

    /**
     * @brief Wakes up the ADC from power-down mode.
     */
    void powerUp();

    /**
     * @brief Puts the ADC into low-power mode by driving SCLK HIGH.
     */
    void powerDown();

    /**
     * @brief Checks if the ADC conversion is complete and data is ready to read.
     * @return true if data is ready (DOUT pin is LOW), false otherwise.
     */
    bool isReady() const;

    /**
     * @brief Calculates the dynamic polling timeout based on current data rate.
     * @return Timeout duration in milliseconds.
     */
    uint32_t getTimeoutMs() const;

    /**
     * @brief Configures channel, PGA gain, and sample rate in a single operation.
     *
     * @param channel `CS123X_CH_A`, `CS123X_CH_B` (CS1238 only), `CS123X_CH_TEMP`, or `CS123X_CH_SHORT`.
     * @param gain `CS123X_GAIN_1`, `CS123X_GAIN_2`, `CS123X_GAIN_64`, or `CS123X_GAIN_128`.
     * @param rate `CS123X_RATE_10Hz`, `CS123X_RATE_40Hz`, `CS123X_RATE_640Hz`, or `CS123X_RATE_1280Hz`.
     * @param verify If true, reads back the register to confirm configuration.
     * @return true if configuration was successfully written (and optionally verified);
     *         false on invalid parameters, or on hardware timeout/verification failure.
     * @note If `CS123X_CH_TEMP` is selected, the PGA gain is automatically overridden to 1x (`CS123X_GAIN_1`),
     *        while the provided `gain` is always stored as the baseline value,
     *        restored automatically when switching away from `CS123X_CH_TEMP`.
     * @note On write or verification failure, all internal configuration state is rolled back to previous values.
     */
    bool setConfig(CS123X_Channel channel, CS123X_Gain gain, CS123X_Rate rate, bool verify = true);

    /**
     * @brief Convenience overload of `setConfig` using a `CS123X_Config` struct
     * @param config `CS123X_Config` struct which contain desidered `CS123X_CH_...`,
     *        `CS123X_GAIN_...` and `CS123X_RATE_...`.
     * @param verify If true, reads back the register to confirm configuration.
     * @see setConfig(CS123X_Channel, CS123X_Gain, CS123X_Rate, bool)
     */
    bool setConfig(CS123X_Config config, bool verify = true) {
        return setConfig(config.channel, config.gain, config.rate, verify);
    }

    /**
     * @brief Get active configuration state.
     * @return `CS123X_Config` struct with current `CS123X_CH_`, `CS123X_GAIN_`, `CS123X_RATE_` and `CS123X_INT_REF_`.
     */
    CS123X_Config getConfig() const {
        return {_channel, _gain, _rate, _intRef};
    }

    /**
     * @brief Configures internal voltage reference generator.
     * @param intRef `CS123X_INT_REF_ON` to enable or `CS123X_INT_REF_OFF` to disable.
     * @param verify If true, reads back register to confirm configuration.
     * @return true if configuration was successfully written (and optionally verified);
     *         false on invalid parameters, or on hardware timeout/verification failure.
     */
    bool setRef(CS123X_IntRef intRef, bool verify = true);

    /// @brief Gets internal reference state (CS123X_INT_REF_...).
    CS123X_IntRef getRef() const {
        return _intRef;
    }

    /**
     * @brief Sets the Output Data Rate (ODR).
     * @param rate `CS123X_RATE_10Hz`, `CS123X_RATE_40Hz`, `CS123X_RATE_640Hz`, or `CS123X_RATE_1280Hz`.
     * @param verify If true, reads back register to confirm configuration..
     * @return true if configuration was successfully written (and optionally verified);
     *         false on invalid parameters, or on hardware timeout/verification failure.
     */
    bool setRate(CS123X_Rate rate, bool verify = true) {
        return setConfig(_channel, _baseGain, rate, verify);
    }

    /// @brief Gets active data rate setting (CS123X_RATE_...).
    CS123X_Rate getRate() const {
        return _rate;
    }

    /**
     * @brief Configures the Programmable Gain Amplifier (PGA).
     * @param gain `CS123X_GAIN_1`, `CS123X_GAIN_2`, `CS123X_GAIN_64`, or `CS123X_GAIN_128`.
     * @param verify If true, reads back register to confirm configuration..
     * @return true if configuration was successfully written (and optionally verified);
     *         false on invalid parameters, or on hardware timeout/verification failure.
     * @note If called while `CS123X_CH_TEMP` is active, the value is saved as baseline
     *       and will be applied automatically upon returning to a standard channel.
     */
    bool setGain(CS123X_Gain gain, bool verify = true) {
        return setConfig(_channel, gain, _rate, verify);
    }

    /**
     * @brief Gets active PGA gain setting (CS123X_GAIN_...).
     * @note Returns CS123X_GAIN_1 while CS123X_CH_TEMP is selected.
     */
    CS123X_Gain getGain() const {
        return _gain;
    }

    /**
     * @brief Gets baseline PGA gain setting for non-temperature channels (CS123X_GAIN_...).
     * @note Equal to getGain() outside of the temperature channel.
     */
    CS123X_Gain getBaseGain() const {
        return _baseGain;
    }

    /**
     * @brief Selects the active analog input channel.
     * @param channel `CS123X_CH_A`, `CS123X_CH_B` (CS1238 only), `CS123X_CH_TEMP`, or `CS123X_CH_SHORT`.
     * @param verify If true, reads back register to confirm configuration..
     * @return true if configuration was successfully written (and optionally verified);
     *         false on invalid parameters, or on hardware timeout/verification failure.
     * @note Selecting `CS123X_CH_TEMP` automatically overrides PGA to 1x (`CS123X_GAIN_1`).
     *       Switching back to standard channels automatically restores the baseline PGA setting.
     */
    bool setCh(CS123X_Channel channel, bool verify = true) {
        return setConfig(channel, _baseGain, _rate, verify);
    }

    /// @brief Gets currently selected input channel (CS123X_CH_...).
    CS123X_Channel getCh() const {
        return _channel;
    }

    /**
     * @brief Forces an immediate 24-bit reading without waiting for DOUT ready signal.
     * @warning Use only if you have already verified `isReady()` is true.
     * @return Signed 32-bit integer (`int32_t`) sign-extended from 24-bit ADC data.
     */
    int32_t forceRead();

    /**
     * @brief Synchronously waits for the ADC to become ready and returns a sign-extended 24-bit reading.
     *
     * Performs blocking polling on the hardware data-ready state with a bounded timeout.
     * Includes cooperative yield() calls during polling to prevent
     * Watchdog Timer (WDT) resets on ESP8266/ESP32 / RTOS environments.
     *
     * @return 32-bit signed reading, or `CS123X_TIMEOUT_ERROR` on timeout or bus failure.
     */
    int32_t read();

    /**
     * @brief Reads both channels of a dual-channel measurement, interleaving each
     *        read with the register write for the next channel switch, avoiding a
     *        wasted conversion cycle per switch, unlike calling read() and setCh()
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
     *
     * @return A `CS123X_DualReading` with one raw ADC reading per channel. Each
     *         field holds `CS123X_INVALID_PARAM` if its channel/gain pair was
     *         invalid (no hardware communication attempted), `CS123X_TIMEOUT_ERROR`
     *         on hardware timeout, or `CS123X_SWITCH_ERROR` if a channel switch
     *         wasn't confirmed — in the latter two cases the reading depends on an
     *         unconfirmed/failed switch and must be considered invalid.
     *
     * @note At the end of the reading, `channel1` and `gain1` will be the channel and the baseline gain
     *       left on at the end of the call; or the last-confirmed verified (`verify` = `true`) status.
     * @note Rejects `CS123X_CH_B` on a CS1237 (single-channel chip) for either parameter.
     */
    CS123X_DualReading readDualChannel(CS123X_Channel channel1, CS123X_Channel channel2,
                                       CS123X_Gain gain1, CS123X_Gain gain2, bool verify = true);

    /**
     * @brief Convenience overload of readDualChannel() using the current baseline gain (`_baseGain`) for both channels.
     * @see readDualChannel(CS123X_Channel, CS123X_Channel, CS123X_Gain, CS123X_Gain, bool)
     */
    CS123X_DualReading readDualChannel(CS123X_Channel channel1, CS123X_Channel channel2, bool verify = true) {
        return readDualChannel(channel1, channel2, _baseGain, _baseGain, verify);
    }

    /**
     * @brief Convenience overload of readDualChannel() using the current (`_channel`) and baseline gain
     *        (`_baseGain`) as the first channel.
     * @see readDualChannel(CS123X_Channel, CS123X_Channel, CS123X_Gain, CS123X_Gain, bool)
     */
    CS123X_DualReading readDualChannel(CS123X_Channel channel2, CS123X_Gain gain2, bool verify = true) {
        return readDualChannel(_channel, channel2, _baseGain, gain2, verify);
    }

    /**
     * @brief Convenience overload of readDualChannel() using the current channel
     *        and baseline gain (`_channel`, `_baseGain`) for the first channel and the
     *        current baseline gain (`_baseGain`) for the second.
     * @see readDualChannel(CS123X_Channel, CS123X_Channel, CS123X_Gain, CS123X_Gain, bool)
     */
    CS123X_DualReading readDualChannel(CS123X_Channel channel2, bool verify = true) {
        return readDualChannel(_channel, channel2, _baseGain, _baseGain, verify);
    }

    /**
     * @brief Reads multiple raw samples and returns their arithmetic mean.
     *
     * Automatically discards transient hardware timeouts. Safe for high sample counts
     * as underlying read operations cooperatively yield CPU execution time (yield()).
     *
     * @param samples Number of samples to average (default: 10).
     * @return Average raw 24-bit ADC code, or CS123X_TIMEOUT_ERROR if all reads fail.
     */
    int32_t readAverage(uint16_t samples = 10);

    /**
     * @brief Reads the differential input voltage.
     * @param vRef Reference voltage in Volts (default: 2.5f to follow reference brekeout board with TL431).
     * @param samples Number of samples to average (default: 1).
     * @return The differential input voltage, or NAN if all reads fail.
     */
    float readVoltage(float vRef = 2.5f, uint8_t samples = 1);

    /**
     * @brief Sets the zero point (tare) by averaging current readings at no load.
     *
     * Updates the internal offset value. Ensures that subsequent calls to
     * getValue() or getUnits() return zero under the current load state.
     *
     * @param samples Number of samples to average (default: 10).
     * @return true if tare was set successfully, false on hardware timeout.
     */
    bool tare(uint8_t samples = 10);

    /**
     * @brief Calculates and sets the scale factor using a known calibration weight.
     *
     * Call this function with a known weight placed on the load cell AFTER performing tare().
     * Formula applied: scale = (rawAverage - offset) / knownWeight.
     * @note For maximum measurement accuracy, use a calibration weight close to the load cell's
     *       full-scale capacity. Larger masses minimize the relative impact of ADC noise
     *       and non-linearity errors.
     * @param knownWeight The weight applied to the scale in desired units (e.g., grams).
     * @param samples Number of samples to average (default: 10).
     * @return true if calibration succeeded, false on timeout, invalid weight (<= 0), or scale == 0.
     */
    bool calibrateScale(float knownWeight, uint8_t samples = 10);

    /**
     * @brief Reads average ADC value and subtracts the tare offset.
     *
     * @param samples Number of samples to average (default: 1).
     * @return Raw ADC code minus offset, or CS123X_TIMEOUT_ERROR on hardware timeout.
     */
    int32_t getValue(uint8_t samples = 1);

    /**
     * @brief Calculates the weight in physical units (e.g., grams, kg).
     *
     * Computes: (getValue(samples) / scale).
     *
     * @param samples Number of samples to average (default: 1).
     * @return Weight in physical units, or NAN if uncalibrated (scale == 0) or on timeout.
     */
    float getUnits(uint8_t samples = 1);

    /// @brief Manually sets the tare offset directly (e.g., restored from EEPROM/Flash).
    void setOffset(int32_t offset) {
        _offset = offset;
    }

    /// @brief Gets the current tare offset (e.g., to save to EEPROM).
    int32_t getOffset() const {
        return _offset;
    }

    /// @brief Manually sets the calibration scale factor directly (e.g., restored from EEPROM).
    void setScale(float scale) {
        _scale = scale;
    }

    /// @brief Gets the current scale factor (e.g., to save to EEPROM).
    float getScale() const {
        return _scale;
    }

    /**
     * @brief Calibrates the temperature sensor live by reading the current raw code from the IC.
     * @param refTempC Current known ambient/chip temperature in Celsius (e.g., 25.0f).
     * @return true if calibration succeeded, false on hardware timeout.
     */
    bool setTempCalibration(float refTempC);

    /**
     * @brief Manually sets reference parameters (e.g., restored from EEPROM/Flash).
     * @param refTempC Reference temperature in Celsius.
     * @param refCode Raw 24-bit ADC reading associated with refTempC.
     */
    void setTempCalibration(float refTempC, int32_t refCode);

    /**
     * @brief Reads internal temperature sensor and calculates temperature in Celsius.
     *
     * Temporarily switches hardware to the internal temperature channel (forcing PGA = 1x),
     * performs sampling via readAverage(), and automatically restores the previously
     * active channel upon completion.
     *
     * @param samples Number of ADC readings to average for noise reduction (default: 1).
     * @param verify If true, reads back register to confirm configuration.
     * @return Temperature in °C, or `NAN` if uncalibrated (_refCode == 0), on hardware timeout,
     *         or if channel switching/restoration fails.
     */
    float readTemperature(uint8_t samples = 1, bool verify = true);
};

#endif /* CS123x_H */
