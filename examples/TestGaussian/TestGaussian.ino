/// @file TestGaussian.ino
/// @brief Gaussian noise characterization suite for CS123x ADC devices.
/// @details Performs high‑resolution statistical analysis on raw ADC output using
///          Welford’s algorithm for numerically stable mean and variance computation.
///          Reports RMS noise, peak‑to‑peak noise, LSB‑equivalent voltage noise,
///          effective number of bits (ENOB), noise‑free resolution, and channel/gain/rate
///          configuration details.
/// @warning TL431 Current Limitation: Stock modules are current-limited by R1 (1 kΩ).
///          For low-impedance sensors (e.g., 350 Ω load cells), reduce R1 by adding a resistor
///          between DVDD and AVDD. For full details, see the README section
///          "Current Limit for Low-Impedance Sensors":
///          https://github.com/FMazz97/CS123x#%EF%B8%8F-important-current-limit-for-low-impedance-sensors
/// @note Effective Precision Reference:
///          - CS1237: 20.0b @5V / 19.5b @3.3V
///          - CS1238: 20.7b @5V / 20.2b @3.3V
///       Typical peak‑to‑peak noise: 0.180 µV
///       These values correspond to datasheet effective resolution at PGA=128 and 10 Hz.
/// @author FMazz97 (https://github.com/FMazz97)
/// @see CS123x GitHub Repository: https://github.com/FMazz97/CS123x
/// @copyright MIT License
/// @example TestGaussian.ino

#include <Arduino.h>

#include "CS123x.h"
#include "math.h"

// Test pin configuration (adjust according to your hardware setup)
#define DOUT_PIN 4
#define SCLK_PIN 5

// Test parameters
#define VREF_VOLTS 2.5f    // TL431 reference voltage on module
uint16_t nSamples = 1000;  // Default number of samples

// Initial sensor instance configured for CS1237 at defatult configuration
// (Change to CS123X_TYPE_CS1238 if testing a CS1238 chip)
CS123x adc(CS123X_TYPE_CS1237, DOUT_PIN, SCLK_PIN);

// Returns the numeric gain multiplier (1x, 2x, 64x, 128x)
uint16_t getGainMultiplier(CS123X_Gain g) {
    switch (g) {
        case CS123X_GAIN_1:
            return 1;
        case CS123X_GAIN_2:
            return 2;
        case CS123X_GAIN_64:
            return 64;
        case CS123X_GAIN_128:
            return 128;
        default:
            return 1;
    }
}

// Returns the active channel name as text
const char* getChannelName(CS123X_Channel ch) {
    switch (ch) {
        case CS123X_CH_A:
            return "CH_A";
        case CS123X_CH_B:
            return "CH_B";
        case CS123X_CH_TEMP:
            return "CH_TEMP";
        case CS123X_CH_SHORT:
            return "CH_SHORT";
        default:
            return "UNKNOWN";
    }
}

// Returns the sampling rate in Hz
uint16_t getRateHz(CS123X_Rate r) {
    switch (r) {
        case CS123X_RATE_10Hz:
            return 10;
        case CS123X_RATE_40Hz:
            return 40;
        case CS123X_RATE_640Hz:
            return 640;
        case CS123X_RATE_1280Hz:
            return 1280;
        default:
            return 10;
    }
}

void printHelp() {
    CS123X_Config cfg = adc.getConfig();
    Serial.println(F("\n===================================="));
    Serial.println(F("  CS123x Advanced Gaussina Evaluation  "));
    Serial.println(F("===================================="));
    Serial.print(F("Current Config -> Channel: "));
    Serial.print(getChannelName(cfg.channel));
    Serial.print(F(" | Gain: "));
    Serial.print(getGainMultiplier(cfg.gain));
    Serial.print(F("x | Speed: "));
    Serial.print(getRateHz(cfg.rate));
    Serial.print(F(" Hz | Samples: "));
    Serial.println(nSamples);
    Serial.println(F("------------------------------------"));
    Serial.println(F(" [r] -> Run Gaussian Noise Test"));
    Serial.println(F(" [n] -> Set Number of Samples"));
    Serial.println(F(" [c] -> Cycle Channel (A -> B -> Temp -> Short)"));
    Serial.println(F(" [g] -> Cycle Gain (1x -> 2x -> 64x -> 128x)"));
    Serial.println(F(" [f] -> Cycle Rate (10Hz -> 40Hz -> 640Hz -> 1280Hz)"));
    Serial.println(F(" [h] -> Print this menu"));
    Serial.println(F("====================================\n"));
}

void setup() {
    Serial.begin(115200);
    while (!Serial && millis() < 2000);  // Wait for Serial monitor on native USB boards

    // Initialize ADC and retry until configuration is validated
    while (!adc.begin()) {
        Serial.println(F("    [WARN] Initialization failed/timeout. Retrying in 500ms..."));
        delay(500);
    }
    Serial.println(F("    [OK] ADC initialized and configuration verified."));
    printHelp();
}

void runGaussianTest() {
    Serial.println(F("\n[GAUSSIAN TEST] Collecting raw samples..."));

    // Welford’s algorithm accumulators for stable mean/variance
    double mean = 0.0;
    double M2 = 0.0;

    int32_t minVal = INT32_MAX;
    int32_t maxVal = INT32_MIN;

    uint16_t validSamples = 0;
    int8_t lastPercent = -1;

    for (uint16_t i = 1; i <= nSamples; i++) {
        // Non‑blocking wait for ADC readiness with ESC abort and timeout protection
        uint32_t startTime = millis();
        bool timeout = false;

        if (Serial.available() && Serial.read() == 27) {
            Serial.println(F("\n\n[ABORT] Test interrupted by user!"));
            return;
        }

        int32_t raw = adc.read();

        // Skip invalid samples caused by communication errors
        if (raw == CS123X_TIMEOUT_ERROR || raw == CS123X_SWITCH_ERROR) continue;

        validSamples++;

        // Track peak‑to‑peak noise
        if (raw < minVal) minVal = raw;
        if (raw > maxVal) maxVal = raw;

        // Welford incremental mean/variance update
        double delta = raw - mean;
        mean += delta / validSamples;
        double delta2 = raw - mean;
        M2 += delta * delta2;

        // Update progress only when percentage increases
        int currentPercent = (i * 100) / nSamples;
        if (currentPercent > lastPercent) {
            lastPercent = currentPercent;
            Serial.print(F("\rProgress: "));
            Serial.print(currentPercent);
            Serial.print(F("% "));
        }
    }

    if (validSamples == 0) {
        Serial.println(F("\n[ERROR] All samples timed out!"));
        return;
    }

    // Compute variance and standard deviation
    double variance = M2 / validSamples;
    double stddev = sqrt(variance);

    // Peak‑to‑peak noise in LSBs
    int32_t noisePP = maxVal - minVal;

    // Convert noise to millivolts:
    // LSB voltage = Vref / (2^24 * Gain)
    CS123X_Gain gainEnum = adc.getGain();
    uint16_t gainVal = getGainMultiplier(gainEnum);

    double lsbVoltageVolts = (double)VREF_VOLTS / ((double)CS123X_MAX_VALUE * 2 * gainVal);
    double noisePP_uV = (noisePP * lsbVoltageVolts) * 1e6;
    double stdDev_uV = (stddev * lsbVoltageVolts) * 1e6;

    // Effective resolution (ENOB) from RMS noise:
    // ENOB = 24 − log2(RMS_noise_LSB)
    double lostBitsRMS = (stddev > 0) ? log(stddev) / log(2.0) : 0;
    double enobRMS = 24.0 - lostBitsRMS;

    // Noise‑free resolution from peak‑to‑peak noise:
    // NoiseFreeBits = 24 − log2(PP_noise_LSB)
    double lostBitsPP = (noisePP > 0) ? log(noisePP) / log(2.0) : 0;
    double noiseFreeBits = 24.0 - lostBitsPP;

    // Print statistical report
    Serial.println(F("\n\n=== Gaussian Noise Statistics ==="));
    Serial.print(F("Valid Samples:   "));
    Serial.println(validSamples);
    Serial.print(F("Mean (Offset):   "));
    Serial.println(mean, 2);
    Serial.print(F("Std Dev (RMS):   "));
    Serial.print(stddev, 2);
    Serial.print(F(" LSB  ("));
    Serial.print(stdDev_uV, 3);
    Serial.println(F(" uV)"));
    Serial.print(F("Variance:        "));
    Serial.println(variance, 2);
    Serial.print(F("Min Raw:         "));
    Serial.println(minVal);
    Serial.print(F("Max Raw:         "));
    Serial.println(maxVal);
    Serial.print(F("Noise P-P:       "));
    Serial.print(noisePP);
    Serial.print(F(" LSB  ("));
    Serial.print(noisePP_uV, 3);
    Serial.println(F(" uV) [yp: 0.180 uV @ 128x,10Hz]"));
    Serial.println(F("-----------------------------------"));
    Serial.print(F("ENOB (RMS):      "));
    Serial.print(enobRMS, 2);
    Serial.println(F(" bits [Typ: 20.0b-5V / 19.5b-3.3V (CS1237) | 20.7b-5V / 20.2b-3.3V (CS1238) @128x,10Hz]"));
    Serial.print(F("Noise-Free Resolution: "));
    Serial.print(noiseFreeBits, 2);
    Serial.println(F(" bits"));
    Serial.println(F("===================================\n"));
}

void loop() {
    if (!Serial.available()) return;

    char cmd = Serial.read();

    // -------------------------
    // RUN GAUSSIAN NOISE TEST
    // -------------------------
    if (cmd == 'r' || cmd == 'R') {
        runGaussianTest();
    }

    // -------------------------
    // Set number of samples
    // -------------------------
    else if (cmd == 'n' || cmd == 'N') {
        Serial.println(F("\nInsert new sample count (e.g. 500, 2000):"));
        while (!Serial.available()) yield();  // Attende l'input
        long val = Serial.parseInt();

        if (val >= 10 && val <= 65535) {
            nSamples = (uint16_t)val;
            Serial.print(F("[OK] nSamples set to: "));
            Serial.println(nSamples);
        } else {
            Serial.println(F("[ERROR] Invalid range! Value must be between 10 and 65535."));
        }
    }

    // -------------------------
    // Cycle through input channels
    // -------------------------
    else if (cmd == 'c' || cmd == 'C') {
        CS123X_Channel nextCh;
        switch (adc.getCh()) {
            case CS123X_CH_A:
                nextCh = CS123X_CH_B;
                break;
            case CS123X_CH_B:
                nextCh = CS123X_CH_TEMP;
                break;
            case CS123X_CH_TEMP:
                nextCh = CS123X_CH_SHORT;
                break;
            default:
                nextCh = CS123X_CH_A;
                break;
        }

        if (adc.setCh(nextCh)) {
            Serial.print(F("[OK] Channel set to: "));
            Serial.println(getChannelName(nextCh));
        } else if (nextCh == CS123X_CH_B) {
            adc.setCh(CS123X_CH_TEMP);
            Serial.println(F("[OK] CS1237 detected (No CH_B). Switched to CH_TEMP."));
        }
    }

    // -------------------------
    // Cycle through gain settings
    // -------------------------
    else if (cmd == 'g' || cmd == 'G') {
        CS123X_Gain nextGain;
        switch (adc.getGain()) {
            case CS123X_GAIN_1:
                nextGain = CS123X_GAIN_2;
                break;
            case CS123X_GAIN_2:
                nextGain = CS123X_GAIN_64;
                break;
            case CS123X_GAIN_64:
                nextGain = CS123X_GAIN_128;
                break;
            default:
                nextGain = CS123X_GAIN_1;
                break;
        }
        if (adc.setGain(nextGain)) {
            Serial.print(F("[OK] Gain set to: "));
            Serial.print(getGainMultiplier(nextGain));
            Serial.println(F("x"));
        }
    }

    // -------------------------
    // Cycle through sampling rates
    // -------------------------
    else if (cmd == 'f' || cmd == 'F') {
        CS123X_Rate nextRate;
        switch (adc.getRate()) {
            case CS123X_RATE_10Hz:
                nextRate = CS123X_RATE_40Hz;
                break;
            case CS123X_RATE_40Hz:
                nextRate = CS123X_RATE_640Hz;
                break;
            case CS123X_RATE_640Hz:
                nextRate = CS123X_RATE_1280Hz;
                break;
            default:
                nextRate = CS123X_RATE_10Hz;
                break;
        }
        if (adc.setRate(nextRate)) {
            Serial.print(F("[OK] Sampling Rate set to: "));
            Serial.print(getRateHz(nextRate));
            Serial.println(F(" Hz"));
        }
    }

    // -------------------------
    // Print help menu
    // -------------------------
    else if (cmd == 'h' || cmd == 'H') {
        printHelp();
    }
}