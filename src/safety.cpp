#include "safety.h"

static SafetyLevel s_currentLevel = SAFETY_NORMAL;
static uint32_t s_warningEnteredMs = 0;
static uint32_t s_criticalEnteredMs = 0;

// Variabel pelacak Grace Period untuk toleransi sensor terlepas / tersenggol (30 detik)
static uint32_t s_heatsinkGraceStartMs = 0;
static bool     s_heatsinkInGrace = false;
static uint32_t s_waterGraceStartMs = 0;
static bool     s_waterInGrace = false;

// Riwayat waktu kejadian CRITICAL untuk deteksi FAULT (3x dalam 30 menit)
#define MAX_CRITICAL_HISTORY 10
static uint32_t s_criticalTimestamps[MAX_CRITICAL_HISTORY];
static uint8_t s_criticalCount = 0;

void safetyInit() {
    s_currentLevel = SAFETY_NORMAL;
    s_warningEnteredMs = 0;
    s_criticalEnteredMs = 0;
    s_criticalCount = 0;
    s_heatsinkGraceStartMs = 0;
    s_heatsinkInGrace = false;
    s_waterGraceStartMs = 0;
    s_waterInGrace = false;
    for (int i = 0; i < MAX_CRITICAL_HISTORY; i++) {
        s_criticalTimestamps[i] = 0;
    }
}

SafetyLevel safetyGetCurrentLevel() {
    return s_currentLevel;
}

bool safetyIsHeatsinkInGrace() {
    return s_heatsinkInGrace;
}

uint32_t safetyGetHeatsinkGraceRemainingSec() {
    if (!s_heatsinkInGrace) return 0;
    uint32_t elapsed = (millis() - s_heatsinkGraceStartMs) / 1000UL;
    if (elapsed >= SENSOR_GRACE_PERIOD_SEC) return 0;
    return SENSOR_GRACE_PERIOD_SEC - elapsed;
}

bool safetyIsWaterInGrace() {
    return s_waterInGrace;
}

uint32_t safetyGetWaterGraceRemainingSec() {
    if (!s_waterInGrace) return 0;
    uint32_t elapsed = (millis() - s_waterGraceStartMs) / 1000UL;
    if (elapsed >= SENSOR_GRACE_PERIOD_SEC) return 0;
    return SENSOR_GRACE_PERIOD_SEC - elapsed;
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
    // 1. PEMANTAUAN TIMER GRACE PERIOD MASING-MASING SENSOR (30 Detik Mandiri)
    // =========================================================================
    bool heatsinkTimedOut = false;
    bool waterTimedOut = false;

    // Grace timer berjalan secara mandiri untuk masing-masing sensor selama belum CRITICAL/FAULT
    if (s_currentLevel != SAFETY_CRITICAL && s_currentLevel != SAFETY_FAULT) {
        // a. Timer Mandiri Sensor Heatsink
        if (!heatsinkOk) {
            if (!s_heatsinkInGrace) {
                s_heatsinkInGrace = true;
                s_heatsinkGraceStartMs = now;
            } else if ((now - s_heatsinkGraceStartMs) >= (SENSOR_GRACE_PERIOD_SEC * 1000UL)) {
                heatsinkTimedOut = true;
            }
        } else {
            s_heatsinkInGrace = false;
        }

        // b. Timer Mandiri Sensor Air
        if (!waterOk || isSensorStale) {
            if (!s_waterInGrace) {
                s_waterInGrace = true;
                s_waterGraceStartMs = now;
            } else if ((now - s_waterGraceStartMs) >= (SENSOR_GRACE_PERIOD_SEC * 1000UL)) {
                waterTimedOut = true;
            }
        } else {
            s_waterInGrace = false;
        }
    } else {
        // Jika sudah resmi berada di level CRITICAL atau FAULT, matikan mode grace period
        s_heatsinkInGrace = false;
        s_waterInGrace = false;
    }

    // =========================================================================
    // 2. STATE MACHINE TRANSITIONS (PRD Bagian 7 & Fail-Safe Mandiri 1 Sensor)
    // =========================================================================
    switch (s_currentLevel) {
        case SAFETY_NORMAL:
            // Kondisi 1: Overheat fisik nyata pada heatsink (>= 70°C) -> Langsung CRITICAL
            if (heatsinkTemp >= HS_CRITICAL_ON) {
                s_currentLevel = SAFETY_CRITICAL;
                s_criticalEnteredMs = now;
                s_heatsinkInGrace = false;
                s_waterInGrace = false;
                registerCriticalOccurrence(now);
                if (countRecentCriticals(now) >= CRITICAL_FAULT_COUNT) {
                    s_currentLevel = SAFETY_FAULT;
                }
            }
            // Kondisi 2: SALAH SATU sensor (Heatsink ATAU Air) time out (> 30s) -> Langsung CRITICAL!
            else if (heatsinkTimedOut || waterTimedOut) {
                s_currentLevel = SAFETY_CRITICAL;
                s_criticalEnteredMs = now;
                s_heatsinkInGrace = false;
                s_waterInGrace = false;
                registerCriticalOccurrence(now);
                if (countRecentCriticals(now) >= CRITICAL_FAULT_COUNT) {
                    s_currentLevel = SAFETY_FAULT;
                }
            }
            // Kondisi 3: Masuk WARNING jika suhu heatsink >= 60°C (hanya jika sensor heatsink valid)
            else if (heatsinkOk && heatsinkTemp >= HS_WARNING_ON) {
                s_currentLevel = SAFETY_WARNING;
                s_warningEnteredMs = now;
            }
            break;

        case SAFETY_WARNING:
            // Kondisi 1: Overheat heatsink >= 70°C ATAU berada di WARNING > 120 detik
            if (heatsinkTemp >= HS_CRITICAL_ON || 
                (now - s_warningEnteredMs > (WARNING_MAX_SEC * 1000UL))) {
                s_currentLevel = SAFETY_CRITICAL;
                s_criticalEnteredMs = now;
                s_heatsinkInGrace = false;
                s_waterInGrace = false;
                registerCriticalOccurrence(now);
                if (countRecentCriticals(now) >= CRITICAL_FAULT_COUNT) {
                    s_currentLevel = SAFETY_FAULT;
                }
            }
            // Kondisi 2: SALAH SATU sensor (Heatsink ATAU Air) time out (> 30s) -> Langsung CRITICAL!
            else if (heatsinkTimedOut || waterTimedOut) {
                s_currentLevel = SAFETY_CRITICAL;
                s_criticalEnteredMs = now;
                s_heatsinkInGrace = false;
                s_waterInGrace = false;
                registerCriticalOccurrence(now);
                if (countRecentCriticals(now) >= CRITICAL_FAULT_COUNT) {
                    s_currentLevel = SAFETY_FAULT;
                }
            }
            // Kondisi 3: Pulih ke NORMAL jika heatsink <= 50°C (dan sensor heatsink valid)
            else if (heatsinkOk && heatsinkTemp <= HS_WARNING_OFF) {
                s_currentLevel = SAFETY_NORMAL;
            }
            break;

        case SAFETY_CRITICAL:
            s_heatsinkInGrace = false;
            s_waterInGrace = false;

            // Cek eskalasi ke FAULT jika kejadian CRITICAL terjadi berulang (3x dalam 30 menit)
            if (countRecentCriticals(now) >= CRITICAL_FAULT_COUNT) {
                s_currentLevel = SAFETY_FAULT;
                break;
            }

            // Pulih ke NORMAL HANYA jika:
            // a. KEDUA sensor normal (heatsinkOk && waterOk && !isSensorStale)
            // b. Suhu heatsink sudah dingin (<= 45°C)
            // c. Sudah bertahan di CRITICAL minimal 60 detik (mencegah fluktuasi cepat)
            if (heatsinkOk && waterOk && !isSensorStale && 
                heatsinkTemp <= HS_CRITICAL_OFF && 
                (now - s_criticalEnteredMs >= (CRITICAL_MIN_SEC * 1000UL))) {
                s_currentLevel = SAFETY_NORMAL;
            }
            break;

        case SAFETY_FAULT:
            s_heatsinkInGrace = false;
            s_waterInGrace = false;
            // FAULT adalah kondisi latching permanen (hanya bisa keluar lewat reset manual)
            break;
    }

    // =========================================================================
    // 3. KELUARAN PROTEKSI SESUAI LEVEL (PRD Tabel 7.2 & Revisi Pengguna)
    // =========================================================================
    SafetyDecision decision;
    decision.level = s_currentLevel;
    decision.forcePeltierOff = false;
    decision.forcePumpOff = false;

    switch (s_currentLevel) {
        case SAFETY_NORMAL:
            decision.maxPeltierAllowedPct = 100.0f;
            decision.pumpAllowed = true;

            // Jika ada salah satu sensor sedang dalam grace period (terlepas < 30 detik):
            // Batasi daya Peltier ke 70% sebagai pencegahan, tetapi pompa TETAP aktif 100%
            if (s_heatsinkInGrace || s_waterInGrace) {
                decision.maxPeltierAllowedPct = HS_WARNING_PELTIER_MAX;
                decision.pumpAllowed = true;
            }
            break;

        case SAFETY_WARNING:
            // Tahap 1: Batasi Peltier maks 70%, matikan pompa
            decision.maxPeltierAllowedPct = HS_WARNING_PELTIER_MAX;
            decision.pumpAllowed = false;
            break;

        case SAFETY_CRITICAL:
        case SAFETY_FAULT:
        default:
            // Tahap 2 & 3: Matikan Peltier dan Pompa total seketika
            decision.maxPeltierAllowedPct = 0.0f;
            decision.pumpAllowed = false;
            decision.forcePeltierOff = true;
            decision.forcePumpOff = true;
            break;
    }

    // =========================================================================
    // 4. FAIL-SAFE TAMBAHAN
    // =========================================================================
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
            s_heatsinkInGrace = false;
            s_waterInGrace = false;
            return true;
        }
    }
    return false;
}

SafetyDebugInfo safetyGetDebugInfo() {
    uint32_t now = millis();
    SafetyDebugInfo info;
    info.currentLevel = s_currentLevel;
    
    info.warningDurationSec = (s_currentLevel == SAFETY_WARNING && s_warningEnteredMs > 0) 
        ? ((now - s_warningEnteredMs) / 1000UL) : 0;
        
    info.criticalDurationSec = (s_currentLevel == SAFETY_CRITICAL && s_criticalEnteredMs > 0)
        ? ((now - s_criticalEnteredMs) / 1000UL) : 0;
        
    info.criticalCountInWindow = countRecentCriticals(now);
    info.heatsinkInGrace = s_heatsinkInGrace;
    info.heatsinkGraceRemSec = safetyGetHeatsinkGraceRemainingSec();
    info.waterInGrace = s_waterInGrace;
    info.waterGraceRemSec = safetyGetWaterGraceRemainingSec();
    
    return info;
}

void safetyForceState(SafetyLevel level) {
    uint32_t now = millis();
    s_currentLevel = level;
    if (level == SAFETY_FAULT) {
        s_criticalCount = CRITICAL_FAULT_COUNT;
        s_heatsinkInGrace = false;
        s_waterInGrace = false;
    } else if (level == SAFETY_CRITICAL) {
        s_criticalEnteredMs = now;
        s_heatsinkInGrace = false;
        s_waterInGrace = false;
    } else if (level == SAFETY_WARNING) {
        s_warningEnteredMs = now;
    } else if (level == SAFETY_NORMAL) {
        s_criticalCount = 0;
        s_warningEnteredMs = 0;
        s_criticalEnteredMs = 0;
        s_heatsinkInGrace = false;
        s_waterInGrace = false;
    }
}

