#ifndef SAFETY_H
#define SAFETY_H

#include <Arduino.h>
#include "shared_data.h"
#include "config.h"

// Hasil evaluasi modul proteksi overheat
struct SafetyDecision {
    SafetyLevel level;          // Level proteksi aktif
    float maxPeltierAllowedPct; // Batas daya maksimal Peltier yang diizinkan (0 - 100%)
    bool pumpAllowed;           // Apakah pompa diizinkan berputar
    bool forcePeltierOff;       // Apakah Peltier dipaksa mati (misal air < 20°C atau sensor error)
    bool forcePumpOff;          // Apakah pompa dipaksa mati
};

// Inisialisasi state machine proteksi
void safetyInit();

// Evaluasi kondisi termal dan sensor untuk menentukan status proteksi
SafetyDecision safetyEvaluate(float heatsinkTemp, bool heatsinkOk, 
                              float waterTemp, bool waterOk, 
                              bool isSensorStale);

// Reset manual status FAULT (hanya bisa berhasil jika heatsink < 45°C)
bool safetyResetFault(float currentHeatsinkTemp);

// Mendapatkan level proteksi saat ini
SafetyLevel safetyGetCurrentLevel();

// Fungsi pemantau status grace period sensor
bool safetyIsHeatsinkInGrace();
uint32_t safetyGetHeatsinkGraceRemainingSec();
bool safetyIsWaterInGrace();
uint32_t safetyGetWaterGraceRemainingSec();

// Informasi diagnostik Developer Mode
struct SafetyDebugInfo {
    SafetyLevel currentLevel;
    uint32_t warningDurationSec;
    uint32_t criticalDurationSec;
    uint8_t  criticalCountInWindow;
    bool     heatsinkInGrace;
    uint32_t heatsinkGraceRemSec;
    bool     waterInGrace;
    uint32_t waterGraceRemSec;
};

SafetyDebugInfo safetyGetDebugInfo();
void safetyForceState(SafetyLevel level);

#endif // SAFETY_H

