/// @file CS123x.cpp
/// @brief Implementation file for the CS123x 24-bit ADC library.
/// @details Implements low-level bit-banging communication, register write/verify
///          mechanisms with automatic rollback, tare/scale calibration logic,
///          and internal temperature sensor reading routines.
/// @author FMazz97 (https://github.com/FMazz97)
/// @see CS123x GitHub Repository: https://github.com/FMazz97/CS123x
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

// Fast pin I/O primitives:
// On AVR, these bypass digitalWrite()/digitalRead() (several µs each on classic
// 16MHz cores) via cached direct PORT/PIN register access. This is essential at
// high ODR (640Hz/1280Hz): the chip's internal conversion cycle (as short as
// ~0.78ms at 1280Hz) can complete WHILE a slow 46-clock register transaction is
// still in progress, colliding with and corrupting it.
// On other architectures, digitalWrite()/digitalRead() are kept unchanged.
#if defined(__AVR__)
#define CS123X_SCLK_HIGH() (*_sclkPortReg |= _sclkBitMask)
#define CS123X_SCLK_LOW() (*_sclkPortReg &= ~_sclkBitMask)
#define CS123X_DOUT_HIGH() (*_doutOutPortReg |= _doutBitMask)
#define CS123X_DOUT_LOW() (*_doutOutPortReg &= ~_doutBitMask)
#define CS123X_DOUT_READ() ((*_doutInPortReg & _doutBitMask) ? 1 : 0)
#else
#define CS123X_SCLK_HIGH() digitalWrite(_sclk, HIGH)
#define CS123X_SCLK_LOW() digitalWrite(_sclk, LOW)
#define CS123X_DOUT_HIGH() digitalWrite(_dout, HIGH)
#define CS123X_DOUT_LOW() digitalWrite(_dout, LOW)
#define CS123X_DOUT_READ() digitalRead(_dout)
#endif

// GPIO Timing Synchronization:
// A minimum pulse width/settling delay is required on every architecture to meet
// CS123x timing specs (datasheet: SCLK pulse width t5 >= 455ns).
// On 32-bit MCUs (ESP32/ESP8266/SAMD/STM32/...), this delay is mandatory because
// direct digitalWrite()/digitalRead() calls alone are too fast and/or too jittery
// to reliably guarantee it.
// On AVR, fast direct PORT/PIN register access (see CS123X_SCLK_HIGH() etc.) bypasses
// the digitalWrite()/digitalRead() overhead that used to provide this margin "for free",
// so the same explicit delay is required here too.
// Defined as an overridable macro (not a hardcoded call) so it possible to redefine it
// before including this header for finer-grained control (e.g. a longer margin for a
// noisier wiring/setup, or __builtin_avr_delay_cycles() for sub-microsecond tuning on AVR).
#ifndef CS123X_BIT_DELAY
#define CS123X_BIT_DELAY() delayMicroseconds(1)
#endif

CS123x::CS123x(CS123X_Type cs123xType, uint8_t dout, uint8_t sclk,
               CS123X_Gain gain, CS123X_Rate rate,
               CS123X_Channel channel, CS123X_IntRef intRef) : _cs123xType((cs123xType <= CS123X_TYPE_CS1238) ? cs123xType : CS123X_TYPE_CS1237),
                                                               _dout(dout),
                                                               _sclk(sclk),
                                                               // Fallback to safe defaults if invalid parameters are provided
                                                               _baseGain((gain <= CS123X_GAIN_128) ? gain : CS123X_GAIN_128),
                                                               _rate((rate <= CS123X_RATE_1280Hz) ? rate : CS123X_RATE_10Hz),
                                                               _intRef((intRef <= CS123X_INT_REF_OFF) ? intRef : CS123X_INT_REF_OFF) {
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
    initFastIO();
    powerUp();

    if (readValAndWriteRegister(true) >= CS123X_TIMEOUT_ERROR)
        return false;
    else
        return true;
}

void CS123x::initFastIO() {
#if defined(__AVR__)
    _sclkPortReg = portOutputRegister(digitalPinToPort(_sclk));
    _sclkBitMask = digitalPinToBitMask(_sclk);

    _doutOutPortReg = portOutputRegister(digitalPinToPort(_dout));
    _doutInPortReg = portInputRegister(digitalPinToPort(_dout));
    _doutBitMask = digitalPinToBitMask(_dout);
#endif
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

int32_t CS123x::readValAndWriteRegister(bool verify) {
    int32_t value = 0;
    uint8_t config = 0x00;
    // built config byte
    // Bit 7 -> reserved (always = 0)
    // Bit 6 -> INT_REF
    // Bit 5:4 -> RATE, Bit 3:2 -> GAIN, Bit 1:0 -> CHANNEL
    config |= (_intRef & 0b1) << 6;
    config |= (_rate & 0b11) << 4;
    config |= (_gain & 0b11) << 2;
    config |= (_channel & 0b11);

    uint32_t start = millis();
    while (!isReady()) {
        if (millis() - start >= getTimeoutMs()) return CS123X_TIMEOUT_ERROR;
        yield();  // Yield to background system tasks
    }

    // Avoid interruption during critical stage
    CS123X_ENTER_CRITICAL();

    value = readBits(24);  // bit 1-24
    voidPulses(3);         // bit 25-27, no previously update acquisition

    voidPulses(2);  // bit 28-29

    digitalWrite(_dout, HIGH);  // avoid LOW status at pinMode change
    pinMode(_dout, OUTPUT);     // change pinMode to write data
    writeBits(0x65, 7);         // 0x65 is write configuration register (7bit) bit 30-36

    voidPulses(1);  // bit 37

    writeBits(config, 8);              // bit 38-45
    pinMode(_dout, CS123X_DOUT_MODE);  // change pinMode to normal operation

    voidPulses(1);  // bit 46

    CS123X_EXIT_CRITICAL();

    // fix negative value from 24 to 32 bit
    if (value & 0x800000) value |= 0xFF000000;

    if (verify) {
        uint32_t start = millis();
        while (isReady()) {
            if (millis() - start >= getTimeoutMs()) return CS123X_SWITCH_ERROR;
            yield();  // Yield to background system tasks.
        }
        start = millis();
        while (!isReady()) {
            if (millis() - start >= getTimeoutMs()) return CS123X_SWITCH_ERROR;
            yield();  // Yield to background system tasks
        }

        int32_t readBack = readRegister();
        if (readBack >= CS123X_TIMEOUT_ERROR)
            return CS123X_SWITCH_ERROR;
        else if ((readBack & 0x80) && ((readBack & 0x7F) == config))
            return value;
        else
            return CS123X_SWITCH_ERROR;
    } else
        return value;
}

int32_t CS123x::readRegister() {
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
        CS123X_SCLK_HIGH();
        CS123X_BIT_DELAY();
        value = (value << 1) | CS123X_DOUT_READ();
        CS123X_SCLK_LOW();
        CS123X_BIT_DELAY();
    }
    return value;
}

void CS123x::writeBits(uint8_t data, uint8_t numBits) {
    for (int8_t i = numBits - 1; i >= 0; i--) {  // from MSB to LSB
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

void CS123x::voidPulses(uint8_t count) {
    for (uint8_t i = 0; i < count; i++) {
        CS123X_SCLK_HIGH();
        CS123X_BIT_DELAY();
        CS123X_SCLK_LOW();
        CS123X_BIT_DELAY();
    }
}

uint32_t CS123x::getTimeoutMs() const {
    // Rate		|	Setting time (t2)	|   Conversion time (t9)    |   Timeout
    // 10Hz		|	300ms				|	100ms                   |	350ms
    // 40Hz		|	75ms				|	25ms                    |	110ms
    // 640Hz	|	6.25ms				|	1.5625ms                |	30ms
    // 1280Hz	|	3.125ms				|	0.78125ms               |	15ms
    static constexpr uint32_t timeouts[4] = {350, 110, 30, 15};

    return timeouts[_rate & 0x03];
}

bool CS123x::setConfig(CS123X_Channel channel, CS123X_Gain gain, CS123X_Rate rate, bool verify) {
    if (channel > CS123X_CH_SHORT) return false;  // Reject values > 3
    if (gain > CS123X_GAIN_128) return false;     // Reject values > 3
    if (rate > CS123X_RATE_1280Hz) return false;  // Reject values > 3

    // Block Channel B selection on single-channel chip (CS1237)
    if (_cs123xType == CS123X_TYPE_CS1237 && channel == CS123X_CH_B) return false;

    // Backups value for rollback
    CS123X_Channel currentChannel = _channel;
    CS123X_Gain currentGain = _gain;
    CS123X_Gain currentBaseGain = _baseGain;
    CS123X_Rate currentRate = _rate;

    _channel = channel;
    _baseGain = gain;
	_gain = (channel == CS123X_CH_TEMP) ? CS123X_GAIN_1 : gain;
    _rate = rate;

    if (readValAndWriteRegister(verify) >= CS123X_TIMEOUT_ERROR) {
        // Rollback to previuos value in case of fail readback
        _channel = currentChannel;
        _gain = currentGain;
        _baseGain = currentBaseGain;
        _rate = currentRate;
        return false;
    }
    return true;
}

bool CS123x::setRef(CS123X_IntRef intRef, bool verify) {
    if (intRef > CS123X_INT_REF_OFF) return false;  // Reject values > 1

    CS123X_IntRef currentRef = _intRef;
    _intRef = intRef;

    if (readValAndWriteRegister(verify) >= CS123X_TIMEOUT_ERROR) {
        _intRef = currentRef;  // Rollback
        return false;
    }
    return true;
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

float CS123x::readVoltage(float vRef, uint8_t samples) {
    int32_t raw = readAverage(samples);
    if (raw == CS123X_TIMEOUT_ERROR) return NAN;

    uint32_t denom = static_cast<uint32_t>(CS123X_MAX_VALUE) * 2 * _gain;
    return static_cast<float>(raw) * vRef / denom;
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

bool CS123x::setTempCalibration(float refTempC) {
    CS123X_Channel previousChannel = _channel;

    if (previousChannel != CS123X_CH_TEMP) {
        if (!setCh(CS123X_CH_TEMP)) return false;
    }

    int32_t rawCode = read();

    if (previousChannel != CS123X_CH_TEMP) {
        if (!setCh(previousChannel)) return false;  // Fallback exit
    }

    if (rawCode == CS123X_TIMEOUT_ERROR) return false;

    _refTempC = refTempC;
    _refCode = rawCode;
    return true;
}

void CS123x::setTempCalibration(float refTempC, int32_t refCode) {
    _refTempC = refTempC;
    _refCode = refCode;
}

float CS123x::readTemperature(uint8_t samples, bool verify) {
    if (_refCode == 0) return NAN;  // Uncalibrated state _refCode = 0 shuold be at 0K, impossible

    CS123X_Channel previousChannel = _channel;

    if (previousChannel != CS123X_CH_TEMP) {
        if (!setCh(CS123X_CH_TEMP, verify)) return NAN;
    }

    int32_t rawCode = readAverage(samples);

    if (previousChannel != CS123X_CH_TEMP) {
        if (!setCh(previousChannel, verify)) return NAN;  // Fallback exit
    }

    if (rawCode == CS123X_TIMEOUT_ERROR) return NAN;

    // B = Yb * (273.15 + Ta) / Ya - 273.15
    return (static_cast<float>(rawCode) * (273.15f + _refTempC)) / _refCode - 273.15f;
}
