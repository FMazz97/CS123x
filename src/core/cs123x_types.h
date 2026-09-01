/// @file cs123x_types.h
/// @brief Shared enums, structs, and sentinel values for the CS123x core.
/// @details Framework-agnostic: no Arduino/ESP-IDF dependency.
/// @copyright MIT License

#ifndef CS123X_TYPES_H
#define CS123X_TYPES_H

#include <stdint.h>

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

#endif /* CS123X_TYPES_H */