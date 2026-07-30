/// @file SimpleScale.ino
/// @brief Practical scale tare, calibration, and weight measurement example.
/// @details Demonstrates step-by-step zero-point alignment (tare), scale factor calibration
///          using a known weight, and continuous weight acquisition in physical units.
/// @author FMazz97 (https://github.com/FMazz97)
/// @copyright MIT License
/// @example SimpleScale.ino


#include <Arduino.h>
#include "CS123x.h"

// Hardware Pin Configuration
#define DOUT_PIN 4
#define SCLK_PIN 5

// Reference weight used for calibration (e.g., 100.0 grams, kg, or lbs)
#define KNOWN_WEIGHT 100.0f

CS123x adc(CS123X_TYPE_CS1237, DOUT_PIN, SCLK_PIN);

void setup() {
  Serial.begin(115200);
  while (!Serial && millis() < 2000)
    ;

  Serial.println(F("\n================================================"));
  Serial.println(F("     CS123x SCALE CALIBRATION & MEASUREMENT     "));
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
  // 2. TARE (ZERO CALIBRATION)
  // ---------------------------------------------------------------------------
  Serial.println(F("\n[2] TARE PROCEDURE"));
  Serial.println(F("    Ensure scale platform is completely empty..."));
  delay(2000);  // Time to remove any load

  if (adc.tare(10)) {  // Average across 10 readings
    Serial.print(F("    [OK] Tare successful! Offset saved: "));
    Serial.println(adc.getOffset());
  } else {
    Serial.println(F("    [FAIL] Tare failed due to hardware timeout."));
  }

  // ---------------------------------------------------------------------------
  // 3. SCALE FACTOR CALIBRATION
  // ---------------------------------------------------------------------------
  // Option A: Live automatic calibration on-the-fly
  Serial.println(F("\n[3] SCALE FACTOR CALIBRATION"));
  Serial.print(F("    Place your known weight ("));
  Serial.print(KNOWN_WEIGHT);
  Serial.println(F(" units) on scale..."));
  delay(5000);  // Time to place the weight

  if (adc.calibrateScale(KNOWN_WEIGHT, 10)) {
    Serial.print(F("    [OK] Calibration successful! Scale factor: "));
    Serial.println(adc.getScale());
  } else {
    Serial.println(F("    [FAIL] Calibration failed. Check load cell wiring/weight."));
  }

  /* 
    Option B: Restoring calibration parameters saved in EEPROM/Flash
    adc.setScale(123.4f); 
  */

  Serial.println(F("\n================================================"));
  Serial.println(F("      CALIBRATION COMPLETE - STARTING LOOP      "));
  Serial.println(F("================================================\n"));
}

void loop() {
  // Read weight in calibrated units (averaged over 3 samples)
  int32_t valueRaw = adc.getValue(3);   // Raw - Offset
  float weightUnits = adc.getUnits(3);  // (Raw - Offset) / Scale

  if (isnan(weightUnits)) {
    Serial.println(F("[ERROR] Hardware read timeout!"));
  } else {
    Serial.print(F("Net Counts: "));
    Serial.print(valueRaw, 0);
    Serial.print(F(" | Weight: "));
    Serial.print(weightUnits, 2);
    Serial.println(F(" units"));
  }

  adc.powerDown();
  delay(5000);
  adc.powerUp();
}