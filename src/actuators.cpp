#include "actuators.h"
#include <esp_arduino_version.h>

void actuatorsInit() {
    // 1. Inisialisasi Pin Pompa (Digital Output)
    pinMode(PIN_PUMP, OUTPUT);
    digitalWrite(PIN_PUMP, (PUMP_ACTIVE_LEVEL == HIGH) ? LOW : HIGH); // Default: OFF

    // 2. Inisialisasi PWM Peltier (LEDC ESP32)
#if defined(ESP_ARDUINO_VERSION_MAJOR) && ESP_ARDUINO_VERSION_MAJOR >= 3
    // Core ESP32 versi 3.x
    ledcAttach(PIN_PELTIER_PWM, PELTIER_PWM_FREQ, PELTIER_PWM_RES_BITS);
    ledcWrite(PIN_PELTIER_PWM, 0);
#else
    // Core ESP32 versi 2.x (espressif32 @ 6.x)
    ledcSetup(PELTIER_PWM_CHANNEL, PELTIER_PWM_FREQ, PELTIER_PWM_RES_BITS);
    ledcAttachPin(PIN_PELTIER_PWM, PELTIER_PWM_CHANNEL);
    ledcWrite(PELTIER_PWM_CHANNEL, 0);
#endif

    // Catatan Hardware:
    // Fan 12V terhubung langsung ke jalur 12V PSU dan selalu menyala penuh sejak PSU ON.
    // Tidak ada inisialisasi GPIO untuk Fan di mikrokontroler.
}

void peltierSetPercent(float pct) {
    // Batasi nilai rentang 0.0 - 100.0%
    if (pct < 0.0f) pct = 0.0f;
    if (pct > 100.0f) pct = 100.0f;

    // Hitung duty cycle 10-bit (0 - 1023)
    uint32_t duty = (uint32_t)((pct / 100.0f) * (float)PELTIER_PWM_MAX_DUTY);
    if (duty > PELTIER_PWM_MAX_DUTY) {
        duty = PELTIER_PWM_MAX_DUTY;
    }

#if defined(ESP_ARDUINO_VERSION_MAJOR) && ESP_ARDUINO_VERSION_MAJOR >= 3
    ledcWrite(PIN_PELTIER_PWM, duty);
#else
    ledcWrite(PELTIER_PWM_CHANNEL, duty);
#endif
}

void pumpSet(bool on) {
    if (on) {
        digitalWrite(PIN_PUMP, (PUMP_ACTIVE_LEVEL == HIGH) ? HIGH : LOW);
    } else {
        digitalWrite(PIN_PUMP, (PUMP_ACTIVE_LEVEL == HIGH) ? LOW : HIGH);
    }
}

void actuatorsEmergencyStop() {
    peltierSetPercent(0.0f);
    pumpSet(false);
}

