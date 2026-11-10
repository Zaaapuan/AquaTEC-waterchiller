#include "control.h"
#include <PID_v1.h>
#include <math.h>

// Variabel double untuk library PID_v1
static double s_pidInput = 28.0;
static double s_pidOutput = 0.0;
static double s_pidSetpoint = 28.0;

// Inisialisasi obyek PID dengan arah REVERSE (Pendingin)
static PID s_pid(&s_pidInput, &s_pidOutput, &s_pidSetpoint, 
                 PID_KP, PID_KI, PID_KD, REVERSE);

// Pelindung Siklus Termal & Soft Start Peltier
static bool s_peltierRunning = false;
static uint32_t s_lastSwitchTimeMs = 0;
static float s_currentRampedPct = 0.0f;

void controlInit() {
    s_pid.SetMode(AUTOMATIC);
    s_pid.SetSampleTime(PID_SAMPLE_TIME_MS);
    s_pid.SetOutputLimits(0.0, 100.0);

    s_peltierRunning = false;
    s_lastSwitchTimeMs = 0;
    s_currentRampedPct = 0.0f;
}

void controlUpdateTunings(float kp, float ki, float kd) {
    s_pid.SetTunings(kp, ki, kd);
}

void controlStep(const SystemState &state, 
                 const SafetyDecision &safety, 
                 float &outRawPid, 
                 float &outPeltierPct, 
                 bool &outPump) {
    uint32_t now = millis();

    // 1. Cek Saklar Sistem Utama dan Kondisi Kritis Darurat
    // HANYA dalam kondisi CRITICAL, FAULT, atau sistem dinonaktifkan manual,
    // seluruh aktuator direset ke 0% seketika untuk proteksi keselamatan mutlak.
    bool isCritical = (safety.level == SAFETY_CRITICAL || safety.level == SAFETY_FAULT);
    if (!state.systemEnabled || isCritical) {
        s_peltierRunning = false;
        s_currentRampedPct = 0.0f;
        s_lastSwitchTimeMs = now;
        outRawPid = 0.0f;
        outPeltierPct = 0.0f;
        outPump = false;
        return;
    }

    // 2. Evaluasi Input Suhu untuk PID & Target Daya Peltier
    float targetPower = 0.0f;

    if (!state.waterSensorOk || state.sensorDataStale) {
        // Sensor air terlepas/senggol atau data sensor macet
        if (safetyIsWaterInGrace()) {
            // Masih dalam batas toleransi / grace period (kabel tersenggol < 30 detik):
            // JANGAN langsung reset ke 0%! Pertahankan daya aktual yang sedang berjalan
            targetPower = s_currentRampedPct;
            outRawPid = s_currentRampedPct;
        } else {
            // Grace period habis (> 30 detik): arahkan target ke 0%, turun bertahap lewat soft-ramp
            targetPower = 0.0f;
            outRawPid = 0.0f;
        }
    } else {
        // Sensor air normal: Update Setpoint dan Input ke Controller
        s_pidSetpoint = (double)state.setpoint;
        s_pidInput = (double)state.waterTemp;

        // Jika suhu air sudah lebih dingin atau sama dengan set point, PID output 0 (FR-12)
        if (state.waterTemp <= state.setpoint) {
            s_pidOutput = 0.0;
        } else {
            s_pid.Compute();
        }

        outRawPid = (float)s_pidOutput;
        targetPower = outRawPid;
    }

    // 3. Batasi Daya Sesuai Keputusan Modul Safety (Safety trumps PID!)
    if (targetPower > safety.maxPeltierAllowedPct) {
        targetPower = safety.maxPeltierAllowedPct;
    }
    if (safety.forcePeltierOff) {
        targetPower = 0.0f;
    }

    // 4. Histeresis Peltier (FR-08) & Jeda Minimum (FR-09)
    if (!s_peltierRunning) {
        // Hanya menyala jika permintaan daya >= 15% (FR-08)
        if (targetPower >= PELTIER_MIN_POWER_PCT) {
            // Cek apakah sudah memenuhi jeda 30 detik sejak mati terakhir
            if (s_lastSwitchTimeMs == 0 || (now - s_lastSwitchTimeMs >= (PELTIER_MIN_SWITCH_SEC * 1000UL))) {
                s_peltierRunning = true;
                s_lastSwitchTimeMs = now;
            } else {
                targetPower = 0.0f; // Tahan dulu sampai batas jeda terpenuhi
            }
        } else {
            targetPower = 0.0f;
        }
    } else {
        // Jika daya turun di bawah 5% (FR-08) atau proteksi meminta nonaktif:
        // Arahkan targetPower ke 0.0f, TETAPI JANGAN langsung nolkan s_currentRampedPct seketika!
        // Biarkan proses Soft-Ramp di Langkah 5 menurunkan daya perlahan (tidak langsung reset ke 0%).
        if (targetPower < PELTIER_OFF_POWER_PCT || safety.forcePeltierOff || safety.maxPeltierAllowedPct <= 0.0f) {
            targetPower = 0.0f;
        }
    }

    // 5. Soft-Start & Soft-Ramp Down (Peltier tidak langsung reset ke 0% selagi tidak CRITICAL)
    // Batasi laju perubahan daya maks. 5% per detik (PELTIER_SOFT_START_RAMP)
    float delta = targetPower - s_currentRampedPct;
    if (delta > PELTIER_SOFT_START_RAMP) {
        s_currentRampedPct += PELTIER_SOFT_START_RAMP;
    } else if (delta < -PELTIER_SOFT_START_RAMP) {
        s_currentRampedPct -= PELTIER_SOFT_START_RAMP;
    } else {
        s_currentRampedPct = targetPower;
    }

    // Hanya jika daya sudah benar-benar ramp-down mencapai 0.0%, matikan status running
    if (s_currentRampedPct <= 0.0f && targetPower <= 0.0f) {
        s_currentRampedPct = 0.0f;
        if (s_peltierRunning) {
            s_peltierRunning = false;
            s_lastSwitchTimeMs = now;
        }
    }

    outPeltierPct = s_currentRampedPct;

    // 6. Logika Pompa Air 5V (FR-13 & Revisi Pengguna)
    // Jika status safety NORMAL: Pompa TETAP berada di 100% (ON) selama sistem aktif!
    // Pada level selain NORMAL: patuhi izin modul safety
    if (state.systemEnabled) {
        if (safety.level == SAFETY_NORMAL) {
            outPump = true; // Selalu 100% saat status safety NORMAL
        } else if (safety.pumpAllowed && !safety.forcePumpOff) {
            outPump = true;
        } else {
            outPump = false;
        }
    } else {
        outPump = false;
    }
}

