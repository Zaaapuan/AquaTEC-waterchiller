#include "shared_data.h"
#include <string.h>

SemaphoreHandle_t g_stateMutex = NULL;
SystemState g_systemState;

bool sharedDataInit() {
    g_stateMutex = xSemaphoreCreateMutex();
    if (g_stateMutex == NULL) {
        return false;
    }

    // Inisialisasi nilai awal state
    g_systemState.waterTemp = 0.0f;
    g_systemState.heatsinkTemp = 0.0f;
    g_systemState.waterSensorOk = false;
    g_systemState.heatsinkSensorOk = false;
    g_systemState.sensorDataReady = false;
    g_systemState.lastSensorUpdateMs = 0;

    g_systemState.setpoint = SETPOINT_DEFAULT;
    g_systemState.systemEnabled = true;

    g_systemState.pidOutput = 0.0f;
    g_systemState.peltierActualPct = 0.0f;
    g_systemState.pumpActive = false;
    g_systemState.safetyLevel = SAFETY_NORMAL;

    g_systemState.alarmHighTemp = false;
    g_systemState.sensorDataStale = false;
    strncpy(g_systemState.ipAddress, "0.0.0.0", sizeof(g_systemState.ipAddress) - 1);
    g_systemState.isApMode = false;
    g_systemState.uptimeSeconds = 0;

    return true;
}

SystemState sharedDataGet() {
    SystemState copy;
    if (g_stateMutex != NULL && xSemaphoreTake(g_stateMutex, portMAX_DELAY) == pdTRUE) {
        copy = g_systemState;
        xSemaphoreGive(g_stateMutex);
    } else {
        // Fallback darurat jika mutex belum siap
        copy = g_systemState;
    }
    return copy;
}

void sharedDataUpdateSensors(float water, bool waterOk, float heatsink, bool heatsinkOk) {
    if (g_stateMutex != NULL && xSemaphoreTake(g_stateMutex, portMAX_DELAY) == pdTRUE) {
        g_systemState.waterTemp = water;
        g_systemState.waterSensorOk = waterOk;
        g_systemState.heatsinkTemp = heatsink;
        g_systemState.heatsinkSensorOk = heatsinkOk;
        g_systemState.sensorDataReady = true;
        g_systemState.lastSensorUpdateMs = millis();
        g_systemState.alarmHighTemp = (waterOk && water > WATER_TEMP_MAX_ALARM);
        g_systemState.sensorDataStale = false;
        xSemaphoreGive(g_stateMutex);
    }
}

void sharedDataUpdateControl(float rawPid, float peltierPct, bool pump, SafetyLevel safety) {
    if (g_stateMutex != NULL && xSemaphoreTake(g_stateMutex, portMAX_DELAY) == pdTRUE) {
        g_systemState.pidOutput = rawPid;
        g_systemState.peltierActualPct = peltierPct;
        g_systemState.pumpActive = pump;
        g_systemState.safetyLevel = safety;

        // Cek apakah data sensor usang (> SENSOR_TIMEOUT_SEC)
        if (millis() - g_systemState.lastSensorUpdateMs > (SENSOR_TIMEOUT_SEC * 1000UL)) {
            g_systemState.sensorDataStale = true;
        }
        xSemaphoreGive(g_stateMutex);
    }
}

bool sharedDataSetpoint(float newSp) {
    if (newSp < SETPOINT_MIN || newSp > SETPOINT_MAX) {
        return false;
    }
    if (g_stateMutex != NULL && xSemaphoreTake(g_stateMutex, portMAX_DELAY) == pdTRUE) {
        g_systemState.setpoint = newSp;
        xSemaphoreGive(g_stateMutex);
        return true;
    }
    return false;
}

void sharedDataSetPower(bool enabled) {
    if (g_stateMutex != NULL && xSemaphoreTake(g_stateMutex, portMAX_DELAY) == pdTRUE) {
        g_systemState.systemEnabled = enabled;
        xSemaphoreGive(g_stateMutex);
    }
}

void sharedDataSetNetwork(const char* ip, bool isAp) {
    if (g_stateMutex != NULL && xSemaphoreTake(g_stateMutex, portMAX_DELAY) == pdTRUE) {
        strncpy(g_systemState.ipAddress, ip, sizeof(g_systemState.ipAddress) - 1);
        g_systemState.ipAddress[sizeof(g_systemState.ipAddress) - 1] = '\0';
        g_systemState.isApMode = isAp;
        xSemaphoreGive(g_stateMutex);
    }
}

void sharedDataUpdateUptime(uint32_t uptimeSec) {
    if (g_stateMutex != NULL && xSemaphoreTake(g_stateMutex, portMAX_DELAY) == pdTRUE) {
        g_systemState.uptimeSeconds = uptimeSec;
        xSemaphoreGive(g_stateMutex);
    }
}

const char* safetyLevelToString(SafetyLevel level) {
    switch (level) {
        case SAFETY_NORMAL:   return "NORMAL";
        case SAFETY_WARNING:  return "WARNING";
        case SAFETY_CRITICAL: return "CRITICAL";
        case SAFETY_FAULT:    return "FAULT";
        default:              return "UNKNOWN";
    }
}
