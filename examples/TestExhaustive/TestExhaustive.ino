/// @file TestExhaustive.ino
/// @brief Comprehensive hardware and API test suite for the CS123x library.
/// @details Exhaustively tests all valid PGA gain settings, data output rates,
///          input channels, internal reference modes, batch configuration,
///          dual-channel reads, voltage conversion, calibration overloads,
///          and polling timeouts.
/// @warning TL431 Current Limitation: Stock modules are current-limited by R1 (1 kΩ).
///          For low-impedance sensors (e.g., 350 Ω load cells), reduce R1 by adding a resistor
///          between DVDD and AVDD. For full details, see the README section
///          "Current Limit for Low-Impedance Sensors":
///          https://github.com/FMazz97/CS123x#%EF%B8%8F-important-current-limit-for-low-impedance-sensors
/// @author FMazz97 (https://github.com/FMazz97)
/// @see CS123x GitHub Repository: https://github.com/FMazz97/CS123x
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
CS123X_Config defaultConf = {CS123X_CH_A, CS123X_GAIN_128, CS123X_RATE_10Hz, CS123X_INT_REF_OFF};

void setup() {
    Serial.begin(115200);
    while (!Serial && millis() < 2000);  // Wait for Serial monitor on native USB boards

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
    const CS123X_Gain pgaList[] = {CS123X_GAIN_1, CS123X_GAIN_2, CS123X_GAIN_64, CS123X_GAIN_128};
    const char* pgaNames[] = {"GAIN_1", "GAIN_2", "GAIN_64", "GAIN_128"};

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

    // Restore default confing to proceed with readings
    if (adc.getConfig() != defaultConf) {
      Serial.println(adc.setConfig(defaultConf) ? F("\n{Correcly restored default configuration.}") : F("\n{Default configuration not restored.}"));
    } else {
        Serial.println(F("\n{Default configuration already setted.}"));
    }

    // ---------------------------------------------------------------------------
    // 3. EXHAUSTIVE DATA RATE TEST (All 4 valid values)
    // ---------------------------------------------------------------------------
    Serial.println(F("\n[3] EXHAUSTIVE DATA RATE TEST"));
    const CS123X_Rate rateList[] = {CS123X_RATE_10Hz, CS123X_RATE_40Hz, CS123X_RATE_640Hz, CS123X_RATE_1280Hz};
    const char* rateNames[] = {"10Hz", "40Hz", "640Hz", "1280Hz"};

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

    // Restore default confing to proceed with readings
    if (adc.getConfig() != defaultConf) {
      Serial.println(adc.setConfig(defaultConf) ? F("\n{Correcly restored default configuration.}") : F("\n{Default configuration not restored.}"));
    } else {
        Serial.println(F("\n{Default configuration already setted.}"));
    }

    // ---------------------------------------------------------------------------
    // 4. EXHAUSTIVE CHANNEL TEST (All 4 valid channels)
    // ---------------------------------------------------------------------------
    Serial.println(F("\n[4] EXHAUSTIVE CHANNEL TEST"));
    const CS123X_Channel chList[] = {CS123X_CH_A, CS123X_CH_B, CS123X_CH_TEMP, CS123X_CH_SHORT};
    const char* chNames[] = {"CH_A", "CH_B", "CH_TEMP", "CH_SHORT"};

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

    // Restore default confing to proceed with readings
    if (adc.getConfig() != defaultConf) {
      Serial.println(adc.setConfig(defaultConf) ? F("\n{Correcly restored default configuration.}") : F("\n{Default configuration not restored.}"));
    } else {
        Serial.println(F("\n{Default configuration already setted.}"));
    }

    // ---------------------------------------------------------------------------
    // 5. EXHAUSTIVE INT_REF TEST (On / Off)
    // ---------------------------------------------------------------------------
    Serial.println(F("\n[5] EXHAUSTIVE INT_REF TEST"));
    const CS123X_IntRef refList[] = {CS123X_INT_REF_ON, CS123X_INT_REF_OFF};
    const char* refNames[] = {"REF_ON", "REF_OFF"};

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

    // Restore default confing to proceed with readings
    if (adc.getConfig() != defaultConf) {
      Serial.println(adc.setConfig(defaultConf) ? F("\n{Correcly restored default configuration.}") : F("\n{Default configuration not restored.}"));
    } else {
        Serial.println(F("\n{Default configuration already setted.}"));
    }

    // ---------------------------------------------------------------------------
    // 6. BATCH CONFIGURATION TEST (setConfig / getConfig)
    // ---------------------------------------------------------------------------
    Serial.println(F("\n[6] BATCH CONFIGURATION TEST"));

    bool batchOk = adc.setConfig(CS123X_CH_SHORT, CS123X_GAIN_64, CS123X_RATE_40Hz, true);
    Serial.print(F("    setConfig(CH_SHORT, GAIN_64, 40Hz) -> "));
    Serial.println(batchOk ? F("[OK]") : F("[FAIL]"));

    CS123X_Config snapshot = adc.getConfig();
    Serial.print(F("    getConfig() snapshot -> ch: "));
    Serial.print(snapshot.channel);
    Serial.print(F(" | gain: "));
    Serial.print(snapshot.gain);
    Serial.print(F(" | rate: "));
    Serial.print(snapshot.rate);
    Serial.print(F(" | intRef: "));
    Serial.println(snapshot.intRef);

    // Restore default confing to proceed with readings
    if (adc.getConfig() != defaultConf) {
      Serial.println(adc.setConfig(defaultConf) ? F("\n{Correcly restored default configuration.}") : F("\n{Default configuration not restored.}"));
    } else {
        Serial.println(F("\n{Default configuration already setted.}"));
    }

    // ---------------------------------------------------------------------------
    // 7. DUAL-CHANNEL READ TEST (CH_A / CH_TEMP — works on both CS1237 & CS1238)
    // ---------------------------------------------------------------------------
    Serial.println(F("\n[7] DUAL-CHANNEL READ TEST"));

    CS123X_DualReading dual = adc.readDualChannel(CS123X_CH_A, CS123X_CH_TEMP, true);
    Serial.print(F("    readDualChannel(CH_A, CH_TEMP) -> ch1: "));
    Serial.print(dual.ch1);
    Serial.print(F(" | ch2: "));
    Serial.println(dual.ch2);
    Serial.print(F("    Chip left on channel: "));
    Serial.println(adc.getCh());

    // Restore default confing to proceed with readings
    if (adc.getConfig() != defaultConf) {
      Serial.println(adc.setConfig(defaultConf) ? F("\n{Correcly restored default configuration.}") : F("\n{Default configuration not restored.}"));
    } else {
        Serial.println(F("\n{Default configuration already setted.}"));
    }

    // ---------------------------------------------------------------------------
    // 8. OFFSET, SCALE & CALIBRATION METHODS TEST
    // ---------------------------------------------------------------------------
    Serial.println(F("\n[8] OFFSET, SCALE & CALIBRATION TEST"));

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
    // 9. TEMPERATURE SENSOR TEST (Overloads)
    // ---------------------------------------------------------------------------
    Serial.println(F("\n[9] TEMPERATURE SENSOR TEST"));

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
    // 10. POWER MANAGEMENT TEST
    // ---------------------------------------------------------------------------
    Serial.println(F("\n[10] POWER MANAGEMENT TEST"));
    adc.powerDown();
    Serial.println(F("    powerDown() executed."));
    delay(500);
    adc.powerUp();
    Serial.println(F("    powerUp() executed."));

    // Restore default confing to proceed with readings
    if (adc.getConfig() != defaultConf) {
      Serial.println(adc.setConfig(defaultConf) ? F("\n{Correcly restored default configuration.}") : F("\n{Default configuration not restored.}"));
    } else {
        Serial.println(F("\n{Default configuration already setted.}"));
    }

    Serial.println(F("\n=================================================="));
    Serial.println(F("    SETUP TEST COMPLETED"));
    Serial.println(F("==================================================\n"));

    delay(500);

    Serial.println(F("\n=================================================="));
    Serial.println(F("    STARTING READ LOOP"));
    Serial.println(F("==================================================\n"));
}

void loop() {
    // ---------------------------------------------------------------------------
    // 11. CONTINUOUS DATA READING TEST
    // ---------------------------------------------------------------------------

    // Synchronous reads (internally handle isReady polling and timeouts)
    int32_t rawSingle = adc.read();
    int32_t rawAvg = adc.readAverage(3);

    int32_t valueRaw = adc.getValue(3);   // (Raw - Offset)
    float weightUnits = adc.getUnits(3);  // (Raw - Offset) / Scale
    float voltage = adc.readVoltage();    // Differential input voltage (VREF default: 2.5V)

    float temperature = adc.readTemperature(3);

    Serial.print(F("Raw: "));
    Serial.print(rawSingle);
    Serial.print(F(" | Avg: "));
    Serial.print(rawAvg);
    Serial.print(F(" | Value: "));
    Serial.print(valueRaw);
    Serial.print(F(" | Units: "));
    Serial.print(weightUnits, 2);
    Serial.print(F(" | Voltage: "));
    Serial.print(voltage, 6);
    Serial.print(F(" V | Temp C: "));
    Serial.println(temperature);

    delay(1000);
}