/// @file TestExhaustive.ino
/// @brief Comprehensive hardware and API test suite for the CS123x library.
/// @details Exhaustively tests all valid PGA gain settings, data output rates,
///          input channels, internal reference modes, calibration overloads, and polling timeouts.
/// @author FMazz97 (https://github.com/FMazz97)
/// @copyright MIT License
/// @example TestExhaustive.ino

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
  Serial.println(F("    CS123x EXHAUSTIVE API & HARDWARE TEST"));
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
  // 2. EXHAUSTIVE PGA / GAIN TEST (All 4 valid values)
  // ---------------------------------------------------------------------------
  Serial.println(F("\n[2] EXHAUSTIVE PGA / GAIN TEST"));
  const CS123X_Gain pgaList[] = { CS123X_GAIN_1, CS123X_GAIN_2, CS123X_GAIN_64, CS123X_GAIN_128 };
  const char* pgaNames[] = { "GAIN_1", "GAIN_2", "GAIN_64", "GAIN_128" };

  for (uint8_t i = 0; i < 4; i++) {
    bool ok = adc.setGain(pgaList[i], true);  // Test setter with HW verification
    uint8_t readBack = adc.getGain();         // Test getter
    Serial.print(F("    Setting "));
    Serial.print(pgaNames[i]);
    Serial.print(F(" -> HW Sync: "));
    Serial.print(ok ? F("[OK]") : F("[FAIL]"));
    Serial.print(F(" | Readback: "));
    Serial.println(readBack == pgaList[i] ? F("[MATCH]") : F("[MISMATCH]"));
  }

  // ---------------------------------------------------------------------------
  // 3. EXHAUSTIVE DATA RATE TEST (All 4 valid values)
  // ---------------------------------------------------------------------------
  Serial.println(F("\n[3] EXHAUSTIVE DATA RATE TEST"));
  const CS123X_Rate rateList[] = { CS123X_RATE_10Hz, CS123X_RATE_40Hz, CS123X_RATE_640Hz, CS123X_RATE_1280Hz };
  const char* rateNames[] = { "10Hz", "40Hz", "640Hz", "1280Hz" };

  for (uint8_t i = 0; i < 4; i++) {
    bool ok = adc.setRate(rateList[i], true);
    uint8_t readBack = adc.getRate();
    uint32_t timeout = adc.getTimeoutMs();  // Verify dynamic timeout recalculation
    Serial.print(F("    Setting "));
    Serial.print(rateNames[i]);
    Serial.print(F(" -> HW Sync: "));
    Serial.print(ok ? F("[OK]") : F("[FAIL]"));
    Serial.print(F(" | Timeout: "));
    Serial.print(timeout);
    Serial.print(F("ms"));
    Serial.print(F(" | Readback: "));
    Serial.println(readBack == rateList[i] ? F("[MATCH]") : F("[MISMATCH]"));
  }

  // Restore 10Hz to proceed with readings
  if (adc.setRate(CS123X_RATE_10Hz, true)) {
    Serial.println("\n{Correcly restored 10Hz rate.}");
  } else {
    Serial.println("\n{10Hz rate not restored.}");
  }

  // ---------------------------------------------------------------------------
  // 4. EXHAUSTIVE CHANNEL TEST (All 4 valid channels)
  // ---------------------------------------------------------------------------
  Serial.println(F("\n[4] EXHAUSTIVE CHANNEL TEST"));
  const CS123X_Channel chList[] = { CS123X_CH_A, CS123X_CH_B, CS123X_CH_TEMP, CS123X_CH_SHORT };
  const char* chNames[] = { "CH_A", "CH_B", "CH_TEMP", "CH_SHORT" };

  for (uint8_t i = 0; i < 4; i++) {
    bool ok = adc.setCh(chList[i], true);
    uint8_t readBack = adc.getCh();
    Serial.print(F("    Setting "));
    Serial.print(chNames[i]);
    Serial.print(F(" -> Result: "));
    Serial.print(ok ? F("[OK]") : F("[REJECTED/FAIL]"));
    Serial.print(F(" | Readback: "));
    Serial.println(readBack == chList[i] ? F("[MATCH]") : F("[MISMATCH]"));
  }

  // Restore CH_A to proceed with readings
  if (adc.setCh(CS123X_CH_A, true)) {
    Serial.println("\n{Correcly restored channel A.}");
  } else {
    Serial.println("\n{Channel A not restored.}");
  }

  // ---------------------------------------------------------------------------
  // 5. EXHAUSTIVE INT_REF TEST (On / Off)
  // ---------------------------------------------------------------------------
  Serial.println(F("\n[5] EXHAUSTIVE INT_REF TEST"));
  const CS123X_IntRef refList[] = { CS123X_INT_REF_ON, CS123X_INT_REF_OFF };
  const char* refNames[] = { "REF_ON", "REF_OFF" };

  for (uint8_t i = 0; i < 2; i++) {
    bool ok = adc.setRef(refList[i], true);
    uint8_t readBack = adc.getRef();
    Serial.print(F("    Setting "));
    Serial.print(refNames[i]);
    Serial.print(F(" -> HW Sync: "));
    Serial.print(ok ? F("[OK]") : F("[FAIL]"));
    Serial.print(F(" | Readback: "));
    Serial.println(readBack == refList[i] ? F("[MATCH]") : F("[MISMATCH]"));
  }

  // ---------------------------------------------------------------------------
  // 6. OFFSET, SCALE & CALIBRATION METHODS TEST
  // ---------------------------------------------------------------------------
  Serial.println(F("\n[6] OFFSET, SCALE & CALIBRATION TEST"));

  // Test manual Setters / Getters
  adc.setOffset(12345L);
  adc.setScale(420.5f);
  Serial.print(F("    Manual setOffset(12345) -> getOffset: "));
  Serial.println(adc.getOffset());
  Serial.print(F("    Manual setScale(420.5)  -> getScale: "));
  Serial.println(adc.getScale());

  // Test Tare
  bool tareOk = adc.tare(3);
  Serial.print(F("    tare(3 samples) -> "));
  Serial.print(tareOk ? F("[OK]") : F("[FAIL]"));
  Serial.print(F(" | New Offset: "));
  Serial.println(adc.getOffset());

  // Test CalibrateScale
  bool calOk = adc.calibrateScale(100.0f, 3);
  Serial.print(F("    calibrateScale(100g, 3 samples) -> "));
  Serial.print(calOk ? F("[OK]") : F("[FAIL]"));
  Serial.print(F(" | New Scale Factor: "));
  Serial.println(adc.getScale());

  // ---------------------------------------------------------------------------
  // 7. TEMPERATURE SENSOR TEST (Overloads)
  // ---------------------------------------------------------------------------
  Serial.println(F("\n[7] TEMPERATURE SENSOR TEST"));

  // Overload 1: Manual calibration
  adc.setTempCalibration(25.0f, 8388608L);
  Serial.println(F("    setTempCalibration(25.0C, 8388608L) -> [OK]"));

  // Overload 2: Automatic calibration on-the-fly
  bool tCal1 = adc.setTempCalibration(25.0f);
  Serial.print(F("    setTempCalibration(25.0C) -> "));
  Serial.println(tCal1 ? F("[OK]") : F("[FAIL]"));

  // Read temperature value averaged over samples
  float tempVal = adc.readTemperature(3);
  Serial.print(F("    readTemperature(3 samples) -> "));
  Serial.print(tempVal);
  Serial.println(F(" C"));

  // ---------------------------------------------------------------------------
  // 8. POWER MANAGEMENT TEST
  // ---------------------------------------------------------------------------
  Serial.println(F("\n[8] POWER MANAGEMENT TEST"));
  adc.powerDown();
  Serial.println(F("    powerDown() executed."));
  delay(500);
  adc.powerUp();
  Serial.println(F("    powerUp() executed."));

  // Final optimal configuration for the main read loop
  if (adc.getGain() != CS123X_GAIN_128) {
    if (adc.setGain(CS123X_GAIN_128, true)) {
      Serial.println("\n{Correcly restored 128x gain.}");
    } else {
      Serial.println("\n{128x gain rate not restored.}");
    }
  } else {
    Serial.println("\n{128x gain already setted.}");
  }

  if (adc.getRate() != CS123X_RATE_10Hz) {
    if (adc.setRate(CS123X_RATE_10Hz, true)) {
      Serial.println("\n{Correcly restored 10Hz rate.}");
    } else {
      Serial.println("\n{10Hz rate not restored.}");
    }
  } else {
    Serial.println("\n{10Hz rate already setted.}");
  }

  if (adc.getCh() != CS123X_CH_A) {
    if (adc.setCh(CS123X_CH_A, true)) {
      Serial.println("\n{Correcly restored channel A.}");
    } else {
      Serial.println("\n{Channel A not restored.}");
    }
  } else {
    Serial.println("\n{Channel A already setted.}");
  }


  Serial.println(F("\n=================================================="));
  Serial.println(F("    SETUP TEST COMPLETED - STARTING READ LOOP"));
  Serial.println(F("==================================================\n"));

  delay(5000);
}

void loop() {
  // ---------------------------------------------------------------------------
  // 9. CONTINUOUS DATA READING TEST
  // ---------------------------------------------------------------------------

  // Synchronous reads (internally handle isReady polling and timeouts)
  int32_t rawSingle = adc.read();
  int32_t rawAvg = adc.readAverage(3);

  int32_t valueRaw = adc.getValue(3);   // (Raw - Offset)
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