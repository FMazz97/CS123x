/// @file BasicTest.ino
/// @brief Functional verification test for the CS123x library core features.
/// @details Tests hardware initialization, register setters/getters with verification, 
///          tare, scale calibration, temperature reading, power cycling, and continuous acquisition.
/// @warning TL431 Current Limitation: Stock modules are current-limited by R1 (1 kΩ).
///          For low-impedance sensors (e.g., 350 Ω load cells), reduce R1 by adding a resistor 
///          between DVDD and AVDD. For full details, see the README section
///          "Current Limit for Low-Impedance Sensors":
///          https://github.com/FMazz97/CS123x#%EF%B8%8F-important-current-limit-for-low-impedance-sensors
/// @author FMazz97 (https://github.com/FMazz97)
/// @see CS123x GitHub Repository: https://github.com/FMazz97/CS123x
/// @copyright MIT License
/// @example BasicTest.ino

#include <Arduino.h>
#include "CS123x.h"

// Test pin configuration (adjust according to your hardware setup)
#define DOUT_PIN 4
#define SCLK_PIN 5

// Initial sensor instance configured for CS1237 at defatult configuration
// (Change to CS123X_TYPE_CS1238 if testing a CS1238 chip)
CS123x adc(CS123X_TYPE_CS1237, DOUT_PIN, SCLK_PIN);

void setup() {
  Serial.begin(115200);
  while (!Serial && millis() < 2000)
    ;  // Wait for Serial monitor on native USB boards

  Serial.println(F("\n=================================================="));
  Serial.println(F("       CS123x BASIC API & HARDWARE TEST"));
  Serial.println(F("=================================================="));

  // ---------------------------------------------------------------------------
  // 1. INITIALIZATION
  // ---------------------------------------------------------------------------
  Serial.println(F("\n[1] INITIALIZATION TEST (begin)"));
  while (!adc.begin()) {
    Serial.println(F("    [WARN] Initialization failed/timeout. Retrying in 500ms..."));
    delay(500);
  }
  Serial.println(F("    [OK] ADC initialized and configuration verified."));

  // ---------------------------------------------------------------------------
  // 2.  TESTING SETTERS WITH ROLLBACK AND RETURN VALUE CHECK AND GETTERS
  // ---------------------------------------------------------------------------
  Serial.println(F("\n[2] TESTING SETTERS (Hardware Sync & Rollback)..."));

  bool okGain = adc.setGain(CS123X_GAIN_64, true);
  Serial.print(F("setGain(64): "));
  Serial.println(okGain ? F("[OK]") : F("[FAIL]"));

  bool okRate = adc.setRate(CS123X_RATE_40Hz, true);
  Serial.print(F("setRate(40Hz): "));
  Serial.println(okRate ? F("[OK]") : F("[FAIL]"));

  bool okCh = adc.setCh(CS123X_CH_A, true);
  Serial.print(F("setCh(CH_A): "));
  Serial.println(okCh ? F("[OK]") : F("[FAIL]"));

  bool okRef = adc.setRef(CS123X_INT_REF_OFF, true);
  Serial.print(F("setRef(OFF): "));
  Serial.println(okRef ? F("[OK]") : F("[FAIL]"));

  Serial.print(F("Read back -> Gain: "));
  Serial.print(adc.getGain());
  Serial.print(F(" | Rate: "));
  Serial.print(adc.getRate());
  Serial.print(F(" | Ch: "));
  Serial.print(adc.getCh());
  Serial.print(F(" | Ref: "));
  Serial.println(adc.getRef());

  // ---------------------------------------------------------------------------
  // 3. OFFSET, SCALE & CALIBRATION METHODS TEST
  // ---------------------------------------------------------------------------
  Serial.println(F("\n[3] OFFSET, SCALE & CALIBRATION TEST"));
  if (adc.tare(5)) {
    Serial.print(F("[OK] Tare done. Current offset: "));
    Serial.println(adc.getOffset());
  } else {
    Serial.println(F("[FAIL] Tare fail (Timeout)."));
  }

  if (adc.calibrateScale(100.0f, 5)) {  // Scale calibration with a knownWeight of 100g/kg/ecc...
    Serial.print(F("[OK] Calibration done. Scale factor: "));
    Serial.println(adc.getScale());
  } else {
    Serial.println(F("[FAIL] Calibration fail."));
  }

  // ---------------------------------------------------------------------------
  // 4. TEMPERATURE SENSOR TEST
  // ---------------------------------------------------------------------------
  Serial.println(F("\n[4] TEMPERATURE SENSOR TEST"));

  // Overload 1: Manual calibration
  adc.setTempCalibration(25.0f, 123456L);
  Serial.println(F("[OK] manual setTempCalibration done."));

  // Overload 2: Automatic calibration on-the-fly
  bool okTemp1 = adc.setTempCalibration(25.0f);
  Serial.print(F("setTempCalibration(25C): "));
  Serial.println(okTemp1 ? F("[OK]") : F("[FAIL]"));

  // ---------------------------------------------------------------------------
  // 5. POWER MANAGEMENT TEST
  // ---------------------------------------------------------------------------
  Serial.println(F("\n[5] POWER MANAGEMENT TEST"));
  adc.powerDown();
  delay(10);
  adc.powerUp();
  Serial.println(F("[OK] Power Cycle executed."));

  // Final optimal configuration for the main read loop
  adc.setGain(CS123X_GAIN_128, true);
  adc.setRate(CS123X_RATE_10Hz, true);
  adc.setCh(CS123X_CH_A, true);

  Serial.println(F("\n=================================================="));
  Serial.println(F("    SETUP TEST COMPLETED - STARTING READ LOOP"));
  Serial.println(F("==================================================\n"));
}

void loop() {
  // ---------------------------------------------------------------------------
  // 9. CONTINUOUS DATA READING TEST
  // ---------------------------------------------------------------------------

  // Synchronous reads (internally handle isReady polling and timeouts)
  int32_t rawSingle = adc.read();
  int32_t rawAvg = adc.readAverage(3);

  int32_t valueRaw = adc.getValue(3);     // Raw - Offset
  float weightUnits = adc.getUnits(3);  // (Raw - Offset) / Scale

  float temperature = adc.readTemperature(3);

  Serial.print(F("Raw: "));
  Serial.print(rawSingle);
  Serial.print(F(" | Avg: "));
  Serial.print(rawAvg);
  Serial.print(F(" | Value: "));
  Serial.print(valueRaw);
  Serial.print(F(" | Units: "));
  Serial.print(weightUnits, 2);
  Serial.print(F(" | Temp C: "));
  Serial.println(temperature);

  delay(1000);
}
