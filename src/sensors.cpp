#include "sensors.h"
#include <OneWire.h>
#include <DallasTemperature.h>
#include <math.h>

// Bus OneWire terpisah untuk sensor air dan sensor heatsink
// Hal ini memudahkan identifikasi tanpa perlu memindai dan menyimpan alamat ROM 64-bit
static OneWire oneWireWater(PIN_TEMP_WATER);
static DallasTemperature dallasWater(&oneWireWater);

static OneWire oneWireHeatsink(PIN_TEMP_HEATSINK);
static DallasTemperature dallasHeatsink(&oneWireHeatsink);

// Penghitung error berurutan
static uint8_t s_waterConsecutiveErrors = 0;
static uint8_t s_heatsinkConsecutiveErrors = 0;
static float s_lastValidWaterTemp = 25.0f;
static float s_lastValidHeatsinkTemp = 30.0f;

// Variabel status simulasi & pengujian proteksi
static bool  s_simHeatsinkEnabled = false;
static float s_simHeatsinkTemp = 25.0f;
static bool  s_simWaterEnabled = false;
static float s_simWaterTemp = 25.0f;
static bool  s_simHeatsinkForceError = false;
static bool  s_simWaterForceError = false;

void sensorsInit() {
    dallasWater.begin();
    dallasHeatsink.begin();

    // Mode non-blocking (FR-02) agar tidak menahan prosesor selama ~750ms
    dallasWater.setWaitForConversion(false);
    dallasHeatsink.setWaitForConversion(false);

    // Resolusi 12-bit (0.0625 °C)
    dallasWater.setResolution(12);
    dallasHeatsink.setResolution(12);

    s_waterConsecutiveErrors = 0;
    s_heatsinkConsecutiveErrors = 0;
}

void sensorsRequestTemperatures() {
    dallasWater.requestTemperatures();
    dallasHeatsink.requestTemperatures();
}

static bool isReadingValid(float temp) {
    // Nilai tidak valid sesuai FR-03:
    // -127.0 °C (sensor terlepas / kabel putus)
    // 85.0 °C (nilai default register DS18B20 saat baru reset sebelum konversi selesai)
    if (temp <= -120.0f || fabs(temp - 85.0f) < 0.05f || temp > 125.0f) {
        return false;
    }
    return true;
}

bool sensorsReadWater(float &outTemp) {
    if (s_simWaterForceError) {
        s_waterConsecutiveErrors = SENSOR_MAX_ERRORS;
        outTemp = -127.0f;
        return false;
    }

    if (s_simWaterEnabled) {
        s_waterConsecutiveErrors = 0;
        s_lastValidWaterTemp = s_simWaterTemp;
        outTemp = s_simWaterTemp;
        return true;
    }

    float raw = dallasWater.getTempCByIndex(0);

    if (isReadingValid(raw)) {
        s_waterConsecutiveErrors = 0;
        s_lastValidWaterTemp = raw;
        outTemp = raw;
        return true;
    } else {
        s_waterConsecutiveErrors++;
        if (s_waterConsecutiveErrors >= SENSOR_MAX_ERRORS) {
            // Sensor resmi dianggap ERROR jika 3x berturut-turut invalid
            outTemp = raw;
            return false;
        }
        // Glitch sementara (1-2x), pertahankan nilai valid terakhir
        outTemp = s_lastValidWaterTemp;
        return true;
    }
}

bool sensorsReadHeatsink(float &outTemp) {
    if (s_simHeatsinkForceError) {
        s_heatsinkConsecutiveErrors = SENSOR_MAX_ERRORS;
        outTemp = -127.0f;
        return false;
    }

    if (s_simHeatsinkEnabled) {
        s_heatsinkConsecutiveErrors = 0;
        s_lastValidHeatsinkTemp = s_simHeatsinkTemp;
        outTemp = s_simHeatsinkTemp;
        return true;
    }

    float raw = dallasHeatsink.getTempCByIndex(0);

    if (isReadingValid(raw)) {
        s_heatsinkConsecutiveErrors = 0;
        s_lastValidHeatsinkTemp = raw;
        outTemp = raw;
        return true;
    } else {
        s_heatsinkConsecutiveErrors++;
        if (s_heatsinkConsecutiveErrors >= SENSOR_MAX_ERRORS) {
            outTemp = raw;
            return false;
        }
        outTemp = s_lastValidHeatsinkTemp;
        return true;
    }
}

void sensorsSetHeatsinkSimulation(bool enabled, float temp) {
    s_simHeatsinkEnabled = enabled;
    s_simHeatsinkTemp = temp;
}

bool sensorsIsHeatsinkSimulated() {
    return s_simHeatsinkEnabled;
}

float sensorsGetSimulatedHeatsinkTemp() {
    return s_simHeatsinkTemp;
}

void sensorsSetWaterSimulation(bool enabled, float temp) {
    s_simWaterEnabled = enabled;
    s_simWaterTemp = temp;
}

bool sensorsIsWaterSimulated() {
    return s_simWaterEnabled;
}

float sensorsGetSimulatedWaterTemp() {
    return s_simWaterTemp;
}

void sensorsSetForceError(bool heatsinkErr, bool waterErr) {
    s_simHeatsinkForceError = heatsinkErr;
    s_simWaterForceError = waterErr;
}

SimulationState sensorsGetSimState() {
    SimulationState st;
    st.heatsinkSimEnabled = s_simHeatsinkEnabled;
    st.heatsinkSimTemp = s_simHeatsinkTemp;
    st.waterSimEnabled = s_simWaterEnabled;
    st.waterSimTemp = s_simWaterTemp;
    st.heatsinkForceError = s_simHeatsinkForceError;
    st.waterForceError = s_simWaterForceError;
    return st;
}

void sensorsResetAllSimulation() {
    s_simHeatsinkEnabled = false;
    s_simWaterEnabled = false;
    s_simHeatsinkForceError = false;
    s_simWaterForceError = false;
}

