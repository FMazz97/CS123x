/// @file CS123x.cpp
/// @brief Implementation file for the CS123x 24-bit ADC library.
/// @details Implements low-level bit-banging communication, register write/verify
///          mechanisms with automatic rollback, tare/scale calibration logic,
///          and internal temperature sensor reading routines.
/// @author FMazz97 (https://github.com/FMazz97)
/// @copyright MIT License

#include "CS123x.h"

#include <Arduino.h>

// Hardware-synchronized critical sections:
// FreeRTOS portMUX ensures multi-core & task safety on ESP32,
// while standard interrupt toggling is used for single-core MCUs (AVR, SAMD, STM32).
#if defined(ARDUINO_ARCH_ESP32)
static portMUX_TYPE cs123x_mux = portMUX_INITIALIZER_UNLOCKED;
#define CS123X_ENTER_CRITICAL() portENTER_CRITICAL(&cs123x_mux)
#define CS123X_EXIT_CRITICAL() portEXIT_CRITICAL(&cs123x_mux)
#else
#define CS123X_ENTER_CRITICAL() noInterrupts()
#define CS123X_EXIT_CRITICAL() interrupts()
#endif

// DOUT Pin Mode Configuration:
// Plain INPUT is required on Espressif MCUs (ESP8266/ESP32) to prevent internal
// weak pull-up parasitic capacitance from distorting fast serial data sampling.
// Standard INPUT_PULLUP is safe and preferred on all other architectures.
#if defined(ARDUINO_ARCH_ESP8266) || defined(ARDUINO_ARCH_ESP32)
#define CS123X_DOUT_MODE INPUT
#else
#define CS123X_DOUT_MODE INPUT_PULLUP
#endif

// GPIO Timing Synchronization:
// Enforces a 1us pulse delay on high-speed 32-bit CPUs to meet CS123x timing specs.
// Bypassed on 16 MHz AVR architectures where digitalWrite latency is already sufficient.
#ifndef CS123X_BIT_DELAY
#if defined(__AVR__)
#define CS123X_BIT_DELAY() ((void)0)
#else
#define CS123X_BIT_DELAY() delayMicroseconds(1)
#endif
#endif

CS123x::CS123x(CS123X_Type cs123xType, uint8_t dout, uint8_t sclk,
               CS123X_Gain gain, CS123X_Rate rate,
               CS123X_Channel channel, CS123X_IntRef intRef) : _dout(dout),
                                                               _sclk(sclk),
                                                               // Fallback to safe defaults if invalid parameters are provided
                                                               _cs123xType((cs123xType <= CS123X_TYPE_CS1238) ? cs123xType : CS123X_TYPE_CS1237),
                                                               _baseGain((gain <= CS123X_GAIN_128) ? gain : CS123X_GAIN_128),
                                                               _rate((rate <= CS123X_RATE_1280Hz) ? rate : CS123X_RATE_10Hz),
                                                               _refOff((intRef <= CS123X_INT_REF_OFF) ? intRef : CS123X_INT_REF_OFF) {
    // Channel validation considering chip type
    if (channel <= CS123X_CH_SHORT) {
        if (_cs123xType == CS123X_TYPE_CS1237 && channel == CS123X_CH_B) {
            _channel = CS123X_CH_A;  // CS1237 doesn't have CH_B
        } else {
            _channel = channel;
        }
    } else {
        _channel = CS123X_CH_A;
    }

    // Fix gain = 1 for if on-chip temperature sensor was enabled
    _gain = (_channel == CS123X_CH_TEMP) ? CS123X_GAIN_1 : _baseGain;
}

bool CS123x::begin() {
    pinMode(_dout, CS123X_DOUT_MODE);
    pinMode(_sclk, OUTPUT);
    powerDown();
    delayMicroseconds(200);
    powerUp();

    return setConfig(true);
}

void CS123x::powerUp() {
    digitalWrite(_sclk, LOW);
}

void CS123x::powerDown() {
    digitalWrite(_sclk, HIGH);
}

bool CS123x::isReady() const {
    return !digitalRead(_dout);
}

int32_t CS123x::forceRead() {
    int32_t value = 0;
    CS123X_ENTER_CRITICAL();

    value = readBits(24);
    voidPulses(3);

    CS123X_EXIT_CRITICAL();

    // fix negative value from 24 to 32 bit
    if (value & 0x800000) value |= 0xFF000000;

    return value;
}

bool CS123x::setConfig(bool verify) {
    uint8_t config = 0x00;
    // built config byte
    // Bit 7 -> reserved (always = 0)
    // Bit 6 -> INT_REF
    // Bit 5:4 -> RATE, Bit 3:2 -> GAIN, Bit 1:0 -> CHANNEL
    config |= (_refOff & 0b1) << 6;
    config |= (_rate & 0b11) << 4;
    config |= (_gain & 0b11) << 2;
    config |= (_channel & 0b11);

    uint32_t start = millis();
    while (!isReady()) {
        if (millis() - start >= getTimeoutMs()) return false;
        yield();  // Yield to background system tasks
    }

    // Avoid interruption during critical stage
    CS123X_ENTER_CRITICAL();

    voidPulses(24);  // bit 1-24, no conversion acquisition
    voidPulses(3);   // bit 25-27, no previously update acquisition

    voidPulses(2);  // bit 28-29

    digitalWrite(_dout, HIGH);  // avoid LOW status at pinMode change
    pinMode(_dout, OUTPUT);     // change pinMode to write data
    writeBits(0x65, 7);         // 0x65 is write configuration register (7bit) bit 30-36

    voidPulses(1);  // bit 37

    writeBits(config, 8);              // bit 38-45
    pinMode(_dout, CS123X_DOUT_MODE);  // change pinMode to normal operation

    voidPulses(1);  // bit 46

    CS123X_EXIT_CRITICAL();

    if (verify) {
        while (isReady()) {
            yield();  // Yield to background system tasks.
        }
        uint32_t start = millis();
        while (!isReady()) {
            if (millis() - start >= getTimeoutMs()) return false;
            yield();  // Yield to background system tasks
        }

        uint8_t readBack = getConfig(); // in case of CS123X_TIMEOUT_ERROR will be returned false
        return (readBack & 0x80) && ((readBack & 0x7F) == config);
    } else
        return true;
}

uint8_t CS123x::getConfig() {
    bool update = false;
    uint8_t config = 0;

    uint32_t start = millis();
    while (!isReady()) {
        if (millis() - start >= getTimeoutMs()) return CS123X_TIMEOUT_ERROR;
        yield();  // Yield to background system tasks
    }

    // Avoid interruption during critical stage
    CS123X_ENTER_CRITICAL();

    voidPulses(24);                      // bit 1-24, no conversion acquisition
    update = (readBits(3) & 0x04) != 0;  // bit 25-27, only 25th bit contain previously update status
    voidPulses(2);                       // bit 28-29

    digitalWrite(_dout, HIGH);         // avoid LOW status at pinMode change
    pinMode(_dout, OUTPUT);            // change pinMode to write data
    writeBits(0x56, 7);                // 0x56 is read configuration register (7bit) bit 30-36
    pinMode(_dout, CS123X_DOUT_MODE);  // change pinMode to read data and normal operation

    voidPulses(1);  // bit 37

    config = readBits(8);  // bit 38-45

    voidPulses(1);  // bit 46

    CS123X_EXIT_CRITICAL();

    // Use reserved bit[7] to store update value
    return (update ? 0x80 : 0x00) | config;
}

uint32_t CS123x::readBits(uint8_t numBits) {
    uint32_t value = 0;

    for (uint8_t i = 0; i < numBits; i++) {
        digitalWrite(_sclk, HIGH);
        CS123X_BIT_DELAY();
        value = (value << 1) | digitalRead(_dout);
        digitalWrite(_sclk, LOW);
        CS123X_BIT_DELAY();
    }
    return value;
}

void CS123x::writeBits(uint8_t data, uint8_t numBits) {
    for (int8_t i = numBits - 1; i >= 0; i--) {  // from MSB to LSB
        digitalWrite(_dout, (data >> i) & 0x01);
        digitalWrite(_sclk, HIGH);
        CS123X_BIT_DELAY();
        digitalWrite(_sclk, LOW);
        CS123X_BIT_DELAY();
    }
}

void CS123x::voidPulses(uint8_t count) {
    for (uint8_t i = 0; i < count; i++) {
        digitalWrite(_sclk, HIGH);
        CS123X_BIT_DELAY();
        digitalWrite(_sclk, LOW);
        CS123X_BIT_DELAY();
    }
}

uint32_t CS123x::getTimeoutMs() const {
    // Rate		|		Setting time		| 	Timeot
    // 10Hz		|		300ms						|		350ms
    // 40Hz		|		75ms						|		100ms
    // 640Hz	|		6.25ms					|		20ms
    // 1280Hz	|		3.125ms					|		10ms
    static constexpr uint32_t timeouts[4] = {500, 200, 100, 100};

    return 1000;//timeouts[_rate & 0x03];
}

int32_t CS123x::read() {
    uint32_t start = millis();
    while (!isReady()) {
        if (millis() - start >= getTimeoutMs()) return CS123X_TIMEOUT_ERROR;
        yield();  // Yield to background system tasks
    }
    return forceRead();
}

int32_t CS123x::readAverage(uint8_t samples) {
    if (samples <= 1) {
        return read();
    }

    int64_t sum = 0;
    uint8_t validCount = 0;

    for (uint8_t i = 0; i < samples; ++i) {
        int32_t raw = read();

        if (raw != CS123X_TIMEOUT_ERROR) {
            sum += raw;
            validCount++;
        }
    }

    if (validCount == 0) {
        return CS123X_TIMEOUT_ERROR;
    }

    return static_cast<int32_t>(sum / validCount);
}

bool CS123x::tare(uint8_t samples) {
    int32_t raw = readAverage(samples);
    if (raw == CS123X_TIMEOUT_ERROR) return false;

    _offset = raw;
    return true;
}

bool CS123x::calibrateScale(float knownWeight, uint8_t samples) {
    if (knownWeight <= 0.0f) return false;  // Avoid negative weight

    int32_t raw = readAverage(samples);
    if (raw == CS123X_TIMEOUT_ERROR) return false;

    int32_t deltaRaw = raw - _offset;

    if (deltaRaw == 0) return false;  // Avoid to divide by 0 in getUnits()

    _scale = static_cast<float>(deltaRaw) / knownWeight;
    return true;
}

int32_t CS123x::getValue(uint8_t samples) {
    int32_t raw = readAverage(samples);
    if (raw == CS123X_TIMEOUT_ERROR) {
        return CS123X_TIMEOUT_ERROR;
    }
    return raw - _offset;
}

float CS123x::getUnits(uint8_t samples) {
    if (_scale == 0.0f) return NAN;  // Avoid to divide by 0, scale not calibrated

    int32_t val = getValue(samples);
    if (val == CS123X_TIMEOUT_ERROR) return NAN;

    return static_cast<float>(val) / _scale;
}

bool CS123x::setRef(CS123X_IntRef intRef, bool verify) {
    if (intRef > CS123X_INT_REF_OFF) return false;  // Reject values > 1

    CS123X_IntRef currentRef = _refOff;
    _refOff = intRef;

    if (!setConfig(verify)) {
        _refOff = currentRef;  // Rollback
        return false;
    }
    return true;
}

bool CS123x::setRate(CS123X_Rate rate, bool verify) {
    if (rate > CS123X_RATE_1280Hz) return false;  // Reject values > 3

    CS123X_Rate currentRate = _rate;
    _rate = rate;

    if (!setConfig(verify)) {
        _rate = currentRate;  // Rollback
        return false;
    }
    return true;
}

bool CS123x::setGain(CS123X_Gain gain, bool verify) {
    if (gain > CS123X_GAIN_128) return false;  // Reject values > 3

    CS123X_Gain currentBaseGain = _baseGain;  // Backup
    CS123X_Gain currentGain = _gain;

    _baseGain = gain;

    // Fix gain = 1 for if on-chip temperature sensor was enabled
    if (_channel == CS123X_CH_TEMP)
        _gain = CS123X_GAIN_1;
    else
        _gain = gain;

    if (!setConfig(verify)) {
        _baseGain = currentBaseGain;  // Rollback
        _gain = currentGain;
        return false;
    }
    return true;
}

bool CS123x::setCh(CS123X_Channel channel, bool verify) {
    if (channel > CS123X_CH_SHORT) return false;  // Check basic limit (0-3)

    // Block Channel B selection on single-channel chip (CS1237)
    if (_cs123xType == CS123X_TYPE_CS1237 && channel == CS123X_CH_B) return false;

    CS123X_Channel currentChannel = _channel;  // Backup
    CS123X_Gain currentGain = _gain;

    // Fix gain for on-chip temperature sensor
    if (channel == CS123X_CH_TEMP) {
        _gain = CS123X_GAIN_1;
    } else if (_channel == CS123X_CH_TEMP) {
        _gain = _baseGain;  // Restore gain selection for other channel
    }

    _channel = channel;

    if (!setConfig(verify)) {
        _channel = currentChannel;  // Rollback
        _gain = currentGain;
        return false;
    }
    return true;
}

bool CS123x::setTempCalibration(float refTempC) {
    CS123X_Channel previousChannel = _channel;

    if (!setCh(CS123X_CH_TEMP)) return false;

    int32_t rawCode = read();

    if (!setCh(previousChannel)) return false;  // Fallback exit
    if (rawCode == CS123X_TIMEOUT_ERROR) return false;

    _refTempC = refTempC;
    _refCode = rawCode;
    return true;
}

void CS123x::setTempCalibration(float refTempC, int32_t refCode) {
    _refTempC = refTempC;
    _refCode = refCode;
}

float CS123x::readTemperature(uint8_t samples) {
    if (_refCode == 0) return NAN;  // Uncalibrated state _refCode = 0 shuold be at 0K, impossible

    CS123X_Channel previousChannel = _channel;

    if (!setCh(CS123X_CH_TEMP)) return NAN;

    int32_t rawCode = readAverage(samples);

    if (!setCh(previousChannel)) return NAN;  // Fallback exit

    if (rawCode == CS123X_TIMEOUT_ERROR) return NAN;

    // B = Yb * (273.15 + Ta) / Ya - 273.15
    return ((float)rawCode * (273.15f + _refTempC)) / (float)_refCode - 273.15f;
}
