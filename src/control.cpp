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

    // 1. Cek Saklar Sistem Utama (ON/OFF dari Dashboard)
    if (!state.systemEnabled) {
        s_peltierRunning = false;
        s_currentRampedPct = 0.0f;
        outRawPid = 0.0f;
        outPeltierPct = 0.0f;
        outPump = false;
        return;
    }

    // 2. Evaluasi Input Suhu untuk PID
    if (!state.waterSensorOk || state.sensorDataStale) {
        // Sensor air bermasalah: matikan aktuator
        s_peltierRunning = false;
        s_currentRampedPct = 0.0f;
        outRawPid = 0.0f;
        outPeltierPct = 0.0f;
        outPump = false;
        return;
    }

    // Update Setpoint dan Input ke Controller
    s_pidSetpoint = (double)state.setpoint;
    s_pidInput = (double)state.waterTemp;

    // Jika suhu air sudah lebih dingin atau sama dengan set point, PID output 0 (FR-12)
    if (state.waterTemp <= state.setpoint) {
        s_pidOutput = 0.0;
    } else {
        s_pid.Compute();
    }

    outRawPid = (float)s_pidOutput;

    // 3. Batasi Daya Sesuai Keputusan Modul Safety (Safety trumps PID!)
    float targetPower = outRawPid;
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
        // Mematikan Peltier jika daya turun di bawah 5% (FR-08) atau proteksi darurat
        if (targetPower < PELTIER_OFF_POWER_PCT || safety.forcePeltierOff || safety.maxPeltierAllowedPct <= 0.0f) {
            // Jika proteksi kritis/fault atau force off: matikan seketika tanpa tunda!
            if (safety.forcePeltierOff || safety.maxPeltierAllowedPct <= 0.0f) {
                s_peltierRunning = false;
                s_lastSwitchTimeMs = now;
                targetPower = 0.0f;
            } else {
                // Untuk siklus normal, hormati jeda minimum 30 detik
                if (now - s_lastSwitchTimeMs >= (PELTIER_MIN_SWITCH_SEC * 1000UL)) {
                    s_peltierRunning = false;
                    s_lastSwitchTimeMs = now;
                    targetPower = 0.0f;
                }
            }
        }
    }

    // 5. Soft-Start: Batasi Kenaikan Daya Maks. 5% per Detik (FR-10)
    if (!s_peltierRunning || safety.forcePeltierOff || safety.maxPeltierAllowedPct <= 0.0f) {
        // Jika kondisi darurat atau mati, langsung nolkan
        s_currentRampedPct = 0.0f;
    } else {
        float delta = targetPower - s_currentRampedPct;
        if (delta > PELTIER_SOFT_START_RAMP) {
            s_currentRampedPct += PELTIER_SOFT_START_RAMP;
        } else if (delta < -PELTIER_SOFT_START_RAMP) {
            s_currentRampedPct -= PELTIER_SOFT_START_RAMP;
        } else {
            s_currentRampedPct = targetPower;
        }
    }

    outPeltierPct = s_currentRampedPct;

    // 6. Logika Pompa Air 5V (FR-13 & Proteksi)
    // Pompa aktif jika sistem enabled, diizinkan safety (NORMAL), dan tidak force off
    if (state.systemEnabled && safety.pumpAllowed && !safety.forcePumpOff) {
        outPump = true;
    } else {
        outPump = false;
    }
}

