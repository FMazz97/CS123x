/// @file cs123x_hal_arduino.h
/// @brief Arduino framework HAL for the CS123x core: GPIO, timing, and critical
///        section primitives, resolved at compile-time per target architecture.
/// @copyright MIT License

#ifndef CS123X_HAL_ARDUINO_H
#define CS123X_HAL_ARDUINO_H

#include <Arduino.h>

// -----------------------------------------------------------------------------
// Target-specific critical section wrappers.
// Uses FreeRTOS portMUX on ESP32, scoped InterruptLock on ESP8266,
// and hardware interrupt disable/restore flags for AVR, ARM, and RP2040.
// -----------------------------------------------------------------------------
#if defined(__AVR__)
#define CS123X_CRITICAL_VAR() uint8_t __cs123x_sreg
#define CS123X_ENTER_CRITICAL() \
    do {                        \
        __cs123x_sreg = SREG;   \
        cli();                  \
    } while (0)
#define CS123X_EXIT_CRITICAL() SREG = __cs123x_sreg
#elif defined(ARDUINO_ARCH_ESP32)
extern portMUX_TYPE cs123x_mux;
#define CS123X_CRITICAL_VAR()
#define CS123X_ENTER_CRITICAL() portENTER_CRITICAL(&cs123x_mux)
#define CS123X_EXIT_CRITICAL() portEXIT_CRITICAL(&cs123x_mux)
#elif defined(ARDUINO_ARCH_ESP8266)
// esp8266::InterruptLock is an RAII guard that saves/restores interrupt state via scope.
// ENTER opens a scope to construct the lock; EXIT closes it to trigger the destructor.
// WARNING: Avoid early `return` inside the critical block, or manually close the scope brace first.
#include <interrupts.h>
#define CS123X_CRITICAL_VAR()
#define CS123X_ENTER_CRITICAL() \
    {                           \
        esp8266::InterruptLock __cs123x_lock
#define CS123X_EXIT_CRITICAL() }
/* TO BE
#elif defined(ARDUINO_ARCH_SAMD) || defined(ARDUINO_ARCH_STM32) || defined(ARDUINO_ARCH_STM32F1)
#define CS123X_CRITICAL_VAR() uint32_t __cs123x_primask
#define CS123X_ENTER_CRITICAL() do { __cs123x_primask = __get_PRIMASK(); __disable_irq(); } while (0)
#define CS123X_EXIT_CRITICAL() __set_PRIMASK(__cs123x_primask)
#elif defined(ARDUINO_ARCH_RP2040)
#define CS123X_CRITICAL_VAR() uint32_t __cs123x_irq
#define CS123X_ENTER_CRITICAL() do { __cs123x_irq = save_and_disable_interrupts(); } while (0)
#define CS123X_EXIT_CRITICAL() restore_interrupts(__cs123x_irq)*/
#else  // Fallback
#define CS123X_CRITICAL_VAR()
#define CS123X_ENTER_CRITICAL() noInterrupts()
#define CS123X_EXIT_CRITICAL() interrupts()
#endif

// -----------------------------------------------------------------------------
// DOUT Pin Mode Configuration:
// Plain INPUT is required on Espressif MCUs (ESP8266/ESP32) to prevent internal
// weak pull-up parasitic capacitance from distorting fast serial data sampling.
// Standard INPUT_PULLUP is safe and preferred on all other architectures.
// -----------------------------------------------------------------------------
#if defined(ARDUINO_ARCH_ESP8266) || defined(ARDUINO_ARCH_ESP32)
#define CS123X_DOUT_MODE INPUT
#else
#define CS123X_DOUT_MODE INPUT_PULLUP
#endif

// -----------------------------------------------------------------------------
// Fast pin I/O primitives:
// On AVR, these bypass digitalWrite()/digitalRead() (several µs each on classic
// 16MHz cores) via cached direct PORT/PIN register access. This is essential at
// high ODR (640Hz/1280Hz): the chip's internal conversion cycle (as short as
// ~0.78ms at 1280Hz) can complete WHILE a slow 46-clock register transaction is
// still in progress, colliding with and corrupting it.
// On other architectures, digitalWrite()/digitalRead() are kept unchanged.
// -----------------------------------------------------------------------------
#if defined(__AVR__)
#define CS123X_SCLK_HIGH() (*_sclk_port_reg |= _sclk_bit_mask)
#define CS123X_SCLK_LOW() (*_sclk_port_reg &= ~_sclk_bit_mask)
#define CS123X_DOUT_HIGH() (*_dout_out_port_reg |= _dout_bit_mask)
#define CS123X_DOUT_LOW() (*_dout_out_port_reg &= ~_dout_bit_mask)
#define CS123X_DOUT_READ() ((*_dout_in_port_reg & _dout_bit_mask) ? 1 : 0)

// Direct PORT/PIN register caching, called once from begin() via init_fast_io().
// Uses Arduino's pin-to-register mapping helpers — kept entirely inside the HAL,
// the core never calls these directly.
#define CS123X_INIT_FAST_IO()                                             \
    do {                                                                  \
        _sclk_port_reg = portOutputRegister(digitalPinToPort(_sclk));     \
        _sclk_bit_mask = digitalPinToBitMask(_sclk);                      \
        _dout_out_port_reg = portOutputRegister(digitalPinToPort(_dout)); \
        _dout_in_port_reg = portInputRegister(digitalPinToPort(_dout));   \
        _dout_bit_mask = digitalPinToBitMask(_dout);                      \
    } while (0)
#else
#define CS123X_SCLK_HIGH() digitalWrite(_sclk, HIGH)
#define CS123X_SCLK_LOW() digitalWrite(_sclk, LOW)
#define CS123X_DOUT_HIGH() digitalWrite(_dout, HIGH)
#define CS123X_DOUT_LOW() digitalWrite(_dout, LOW)
#define CS123X_DOUT_READ() digitalRead(_dout)
#define CS123X_INIT_FAST_IO() ((void)0)
#endif

// -----------------------------------------------------------------------------
// Pin direction / generic timing primitives.
// -----------------------------------------------------------------------------
#define CS123X_SET_SCLK_OUTPUT() pinMode(_sclk, OUTPUT)
#define CS123X_SET_DOUT_OUTPUT() pinMode(_dout, OUTPUT)
#define CS123X_SET_DOUT_INPUT() pinMode(_dout, CS123X_DOUT_MODE)

#define CS123X_MILLIS() millis()
#define CS123X_YIELD() yield()

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

#endif /* CS123X_HAL_ARDUINO_H */