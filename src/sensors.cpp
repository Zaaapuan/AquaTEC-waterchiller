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

#if SIMULATION_MODE
static bool s_simHeatsinkEnabled = false;
static float s_simHeatsinkTemp = 25.0f;
#endif

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
#if SIMULATION_MODE
    if (s_simHeatsinkEnabled) {
        outTemp = s_simHeatsinkTemp;
        return true;
    }
#endif

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

#if SIMULATION_MODE
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
#endif

