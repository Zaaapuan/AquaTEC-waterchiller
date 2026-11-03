#include "safety.h"

static SafetyLevel s_currentLevel = SAFETY_NORMAL;
static uint32_t s_warningEnteredMs = 0;
static uint32_t s_criticalEnteredMs = 0;

// Riwayat waktu kejadian CRITICAL untuk deteksi FAULT (3x dalam 30 menit)
#define MAX_CRITICAL_HISTORY 10
static uint32_t s_criticalTimestamps[MAX_CRITICAL_HISTORY];
static uint8_t s_criticalCount = 0;

void safetyInit() {
    s_currentLevel = SAFETY_NORMAL;
    s_warningEnteredMs = 0;
    s_criticalEnteredMs = 0;
    s_criticalCount = 0;
    for (int i = 0; i < MAX_CRITICAL_HISTORY; i++) {
        s_criticalTimestamps[i] = 0;
    }
}

SafetyLevel safetyGetCurrentLevel() {
    return s_currentLevel;
}

static void registerCriticalOccurrence(uint32_t now) {
    if (s_criticalCount < MAX_CRITICAL_HISTORY) {
        s_criticalTimestamps[s_criticalCount++] = now;
    } else {
        // Geser ke kiri jika buffer penuh
        for (int i = 0; i < MAX_CRITICAL_HISTORY - 1; i++) {
            s_criticalTimestamps[i] = s_criticalTimestamps[i + 1];
        }
        s_criticalTimestamps[MAX_CRITICAL_HISTORY - 1] = now;
    }
}

static uint8_t countRecentCriticals(uint32_t now) {
    uint8_t count = 0;
    uint32_t windowMs = FAULT_WINDOW_SEC * 1000UL;
    for (uint8_t i = 0; i < s_criticalCount; i++) {
        if (now >= s_criticalTimestamps[i] && (now - s_criticalTimestamps[i]) <= windowMs) {
            count++;
        }
    }
    return count;
}

SafetyDecision safetyEvaluate(float heatsinkTemp, bool heatsinkOk, 
                              float waterTemp, bool waterOk, 
                              bool isSensorStale) {
    uint32_t now = millis();

    // =========================================================================
    // 1. STATE MACHINE TRANSITIONS (PRD Bagian 7)
    // =========================================================================
    switch (s_currentLevel) {
        case SAFETY_NORMAL:
            // Masuk CRITICAL jika sensor heatsink rusak atau suhu >= 70°C
            if (!heatsinkOk || heatsinkTemp >= HS_CRITICAL_ON) {
                s_currentLevel = SAFETY_CRITICAL;
                s_criticalEnteredMs = now;
                registerCriticalOccurrence(now);
                if (countRecentCriticals(now) >= CRITICAL_FAULT_COUNT) {
                    s_currentLevel = SAFETY_FAULT;
                }
            }
            // Masuk WARNING jika suhu heatsink >= 60°C
            else if (heatsinkTemp >= HS_WARNING_ON) {
                s_currentLevel = SAFETY_WARNING;
                s_warningEnteredMs = now;
            }
            break;

        case SAFETY_WARNING:
            // Pindah ke CRITICAL jika:
            // a. Sensor heatsink error
            // b. Suhu heatsink >= 70°C
            // c. Berada di WARNING > 120 detik (panas tidak kunjung turun)
            if (!heatsinkOk || heatsinkTemp >= HS_CRITICAL_ON || 
                (now - s_warningEnteredMs > (WARNING_MAX_SEC * 1000UL))) {
                s_currentLevel = SAFETY_CRITICAL;
                s_criticalEnteredMs = now;
                registerCriticalOccurrence(now);
                if (countRecentCriticals(now) >= CRITICAL_FAULT_COUNT) {
                    s_currentLevel = SAFETY_FAULT;
                }
            }
            // Pulih ke NORMAL jika heatsink <= 50°C
            else if (heatsinkTemp <= HS_WARNING_OFF) {
                s_currentLevel = SAFETY_NORMAL;
            }
            break;

        case SAFETY_CRITICAL:
            // Cek kondisi FAULT jika terjadi berulang
            if (countRecentCriticals(now) >= CRITICAL_FAULT_COUNT) {
                s_currentLevel = SAFETY_FAULT;
                break;
            }

            // Pulih ke NORMAL jika:
            // a. Sensor heatsink normal
            // b. Suhu heatsink <= 45°C
            // c. Sudah bertahan di CRITICAL minimal 60 detik (mencegah fluktuasi cepat)
            if (heatsinkOk && heatsinkTemp <= HS_CRITICAL_OFF && 
                (now - s_criticalEnteredMs >= (CRITICAL_MIN_SEC * 1000UL))) {
                s_currentLevel = SAFETY_NORMAL;
            }
            break;

        case SAFETY_FAULT:
            // FAULT adalah kondisi latching permanen (hanya bisa keluar lewat reset manual)
            break;
    }

    // =========================================================================
    // 2. KELUARAN PROTEKSI SESUAI LEVEL (PRD Tabel 7.2)
    // =========================================================================
    SafetyDecision decision;
    decision.level = s_currentLevel;
    decision.forcePeltierOff = false;
    decision.forcePumpOff = false;

    switch (s_currentLevel) {
        case SAFETY_NORMAL:
            decision.maxPeltierAllowedPct = 100.0f;
            decision.pumpAllowed = true;
            break;

        case SAFETY_WARNING:
            // Tahap 1: Batasi Peltier maks 70%, matikan pompa
            decision.maxPeltierAllowedPct = HS_WARNING_PELTIER_MAX;
            decision.pumpAllowed = false;
            break;

        case SAFETY_CRITICAL:
        case SAFETY_FAULT:
        default:
            // Tahap 2 & 3: Matikan Peltier dan Pompa total
            decision.maxPeltierAllowedPct = 0.0f;
            decision.pumpAllowed = false;
            break;
    }

    // =========================================================================
    // 3. FAIL-SAFE TAMBAHAN (PRD Bagian 7.6)
    // =========================================================================
    // Jika sensor air error atau data sensor macet > 5 detik: matikan aktuator
    if (!waterOk || isSensorStale) {
        decision.forcePeltierOff = true;
        decision.forcePumpOff = true;
    }

    // Jika suhu air < 20°C (batas bawah mutlak): matikan Peltier untuk lindungi biota
    if (waterOk && waterTemp < WATER_TEMP_MIN_SAFE) {
        decision.forcePeltierOff = true;
    }

    return decision;
}

bool safetyResetFault(float currentHeatsinkTemp) {
    // Reset hanya diizinkan jika sistem sedang FAULT dan heatsink sudah dingin (< 45°C)
    if (s_currentLevel == SAFETY_FAULT) {
        if (currentHeatsinkTemp < FAULT_RECOVER_MAX_TEMP) {
            s_currentLevel = SAFETY_NORMAL;
            s_criticalCount = 0; // Bersihkan riwayat error
            return true;
        }
    }
    return false;
}

