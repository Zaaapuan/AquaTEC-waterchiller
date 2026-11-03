#ifndef SHARED_DATA_H
#define SHARED_DATA_H

#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include "config.h"

// Level Proteksi Sistem Overheat (PRD Bagian 7)
enum SafetyLevel {
    SAFETY_NORMAL   = 0,
    SAFETY_WARNING  = 1,
    SAFETY_CRITICAL = 2,
    SAFETY_FAULT    = 3
};

// Struktur Data Bersama Antar-Task FreeRTOS
struct SystemState {
    // Pembacaan Sensor Suhu
    float waterTemp;            // Suhu air (°C)
    float heatsinkTemp;         // Suhu heatsink Peltier (°C)
    bool  waterSensorOk;        // Status keabsahan sensor air
    bool  heatsinkSensorOk;     // Status keabsahan sensor heatsink
    uint32_t lastSensorUpdateMs;// Timestamp pembaruan sensor terakhir

    // Konfigurasi Pengguna
    float setpoint;             // Target suhu (°C)
    bool  systemEnabled;        // Tombol ON/OFF sistem (true = aktif)

    // Status Kontrol & Aktuator
    float pidOutput;            // Output murni PID (0 - 100 %)
    float peltierActualPct;     // Daya aktual Peltier setelah proteksi & ramp (0 - 100 %)
    bool  pumpActive;           // Status on/off pompa air 5V
    SafetyLevel safetyLevel;    // Level proteksi aktif

    // Peringatan & Status Jaringan
    bool  alarmHighTemp;        // Peringatan jika suhu air > 32°C
    bool  sensorDataStale;      // Flag jika pembacaan sensor macet > 5 detik
    char  ipAddress[24];        // Alamat IP (Station atau AP)
    bool  isApMode;             // True jika berjalan di mode SoftAP fallback
    uint32_t uptimeSeconds;     // Waktu sistem aktif (detik)
};

// Handle Mutex Eksternal
extern SemaphoreHandle_t g_stateMutex;
extern SystemState g_systemState;

// Fungsi-Fungsi Pengakses Data yang Aman (Thread-Safe)
bool sharedDataInit();
SystemState sharedDataGet();
void sharedDataUpdateSensors(float water, bool waterOk, float heatsink, bool heatsinkOk);
void sharedDataUpdateControl(float rawPid, float peltierPct, bool pump, SafetyLevel safety);
bool sharedDataSetpoint(float newSp);
void sharedDataSetPower(bool enabled);
void sharedDataSetNetwork(const char* ip, bool isAp);
void sharedDataUpdateUptime(uint32_t uptimeSec);
const char* safetyLevelToString(SafetyLevel level);

#endif // SHARED_DATA_H

