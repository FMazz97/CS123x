/// @file SimpleTemperature.ino
/// @brief Internal temperature sensor calibration and reading example.
/// @details Demonstrates live single-point ambient temperature calibration and 
///          continuous temperature reading in °C using automatic channel switching.
/// @author FMazz97 (https://github.com/FMazz97)
/// @copyright MIT License
/// @example SimpleTemperature.ino

#include <Arduino.h>
#include "CS123x.h"

// Hardware Pin Configuration
#define DOUT_PIN 4
#define SCLK_PIN 5

// Known ambient room temperature during calibration (in °C)
#define CURRENT_ROOM_TEMP 25.0f

CS123x adc(CS123X_TYPE_CS1237, DOUT_PIN, SCLK_PIN);

void setup() {
  Serial.begin(115200);
  while (!Serial && millis() < 2000)
    ;

  Serial.println(F("\n================================================"));
  Serial.println(F("   CS123x TEMPERATURE SENSOR CALIBRATION        "));
  Serial.println(F("================================================"));

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
  // 2. TEMPERATURE SENSOR CALIBRATION
  // ---------------------------------------------------------------------------
  Serial.println(F("\n[2] TEMPERATURE CALIBRATION"));
  Serial.print(F("    Calibrating sensor at current ambient temperature ("));
  Serial.print(CURRENT_ROOM_TEMP);
  Serial.println(F(" C)..."));

  // Option A: Live automatic calibration on-the-fly
  if (adc.setTempCalibration(CURRENT_ROOM_TEMP)) {
    Serial.println(F("    [OK] Live temperature calibration succeeded!"));
  } else {
    Serial.println(F("    [FAIL] Temperature calibration failed (Timeout)."));
  }

  /* 
    Option B: Restoring calibration parameters saved in EEPROM/Flash
    adc.setTempCalibration(25.0f, 8388608L); 
  */

  Serial.println(F("\n================================================"));
  Serial.println(F("        SETUP COMPLETED - STARTING LOOP        "));
  Serial.println(F("================================================\n"));
}

void loop() {
  // Read temperature in °C (averaged over 5 samples for noise reduction)
  // Note: readTemperature() handles channel switching and PGA adjustment automatically
  float tempC = adc.readTemperature(5);

  if (isnan(tempC)) {
    Serial.println(F("[ERROR] Failed to read temperature (Timeout or uncalibrated)."));
  } else {
    Serial.print(F("Internal Temperature: "));
    Serial.print(tempC, 2);
    Serial.println(F(" °C"));
  }

  delay(1000);
}