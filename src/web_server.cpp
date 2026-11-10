#include "web_server.h"
#include <WiFi.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <Preferences.h>
#include <ArduinoJson.h>
#include "secrets.h"
#include "config.h"
#include "dashboard_html.h"
#include "devmode_html.h"
#include "safety.h"
#include "sensors.h"

static WebServer s_server(80);
static Preferences s_prefs;

// Struktur titik riwayat telemetri
struct HistoryPoint {
    float waterTemp;
    float heatsinkTemp;
    float peltierPct;
    float pidOutput;
    uint8_t safetyLevel;
};

// Ring buffer untuk riwayat grafik suhu & telemetri (360 titik = 1 jam terakhir)
static HistoryPoint s_history[HISTORY_CAPACITY];
static uint16_t s_historyHead = 0;
static uint16_t s_historyCount = 0;

void webServerRecordHistory(float waterTemp, float heatsinkTemp, float peltierPct, float pidOutput, uint8_t safetyLevel) {
    bool validWater = (!isnan(waterTemp) && waterTemp >= -40.0f && waterTemp <= 100.0f);
    bool validHs = (!isnan(heatsinkTemp) && heatsinkTemp >= -40.0f && heatsinkTemp <= 125.0f);

    if (!validWater && !validHs) {
        return;
    }

    // Ambil nilai sebelumnya jika salah satu sensor sempat gagal baca sekilas
    float prevWater = (s_historyCount > 0) ? s_history[(s_historyHead + HISTORY_CAPACITY - 1) % HISTORY_CAPACITY].waterTemp : 25.0f;
    float prevHs = (s_historyCount > 0) ? s_history[(s_historyHead + HISTORY_CAPACITY - 1) % HISTORY_CAPACITY].heatsinkTemp : 30.0f;

    s_history[s_historyHead].waterTemp = validWater ? waterTemp : prevWater;
    s_history[s_historyHead].heatsinkTemp = validHs ? heatsinkTemp : prevHs;
    s_history[s_historyHead].peltierPct = peltierPct;
    s_history[s_historyHead].pidOutput = pidOutput;
    s_history[s_historyHead].safetyLevel = safetyLevel;

    s_historyHead = (s_historyHead + 1) % HISTORY_CAPACITY;
    if (s_historyCount < HISTORY_CAPACITY) {
        s_historyCount++;
    }
}

// Endpoint: GET /
static void handleRoot() {
    s_server.send_P(200, "text/html", DASHBOARD_HTML);
}

// Endpoint: GET /api/status (D1, D3, D4)
static void handleStatus() {
    SystemState st = sharedDataGet();

    JsonDocument doc;
    doc["water_temp"] = st.waterTemp;
    doc["heatsink_temp"] = st.heatsinkTemp;
    doc["setpoint"] = st.setpoint;
    doc["enabled"] = st.systemEnabled;
    doc["safety"] = safetyLevelToString(st.safetyLevel);
    doc["peltier_pct"] = st.peltierActualPct;
    doc["pid_output"] = st.pidOutput;
    doc["pump"] = st.pumpActive;
    doc["sensor_ok"] = (st.waterSensorOk && st.heatsinkSensorOk && !st.sensorDataStale);
    doc["heatsink_ok"] = st.heatsinkSensorOk;
    doc["water_ok"] = st.waterSensorOk;
    doc["heatsink_grace"] = safetyIsHeatsinkInGrace();
    doc["heatsink_grace_rem"] = safetyGetHeatsinkGraceRemainingSec();
    doc["water_grace"] = safetyIsWaterInGrace();
    doc["water_grace_rem"] = safetyGetWaterGraceRemainingSec();
    doc["uptime_s"] = st.uptimeSeconds;
    doc["ip"] = st.ipAddress;
    doc["is_ap"] = st.isApMode;

    String jsonResponse;
    serializeJson(doc, jsonResponse);
    s_server.send(200, "application/json", jsonResponse);
}

// Endpoint: POST /api/setpoint (D2)
static void handleSetpoint() {
    float newSp = 0.0f;
    bool parsed = false;

    if (s_server.hasArg("plain")) {
        JsonDocument doc;
        DeserializationError err = deserializeJson(doc, s_server.arg("plain"));
        if (!err && !doc["value"].isNull()) {
            newSp = doc["value"].as<float>();
            parsed = true;
        }
    } else if (s_server.hasArg("value")) {
        newSp = s_server.arg("value").toFloat();
        parsed = true;
    }

    if (!parsed || newSp < SETPOINT_MIN || newSp > SETPOINT_MAX) {
        // HTTP 400 Bad Request jika di luar 20.0 - 30.0 °C (PRD 5.3 & T5)
        s_server.send(400, "application/json", "{\"error\":\"Nilai setpoint harus antara 20.0 dan 30.0 C\"}");
        return;
    }

    // Perbarui state bersama
    sharedDataSetpoint(newSp);

    // Simpan permanen di Flash NVS (FR-15)
    s_prefs.putFloat("setpoint", newSp);

    JsonDocument res;
    res["success"] = true;
    res["setpoint"] = newSp;
    String resStr;
    serializeJson(res, resStr);
    s_server.send(200, "application/json", resStr);
}

// Endpoint: POST /api/power (D5)
static void handlePower() {
    bool enabled = true;
    bool parsed = false;

    if (s_server.hasArg("plain")) {
        JsonDocument doc;
        DeserializationError err = deserializeJson(doc, s_server.arg("plain"));
        if (!err && !doc["enabled"].isNull()) {
            enabled = doc["enabled"].as<bool>();
            parsed = true;
        }
    } else if (s_server.hasArg("enabled")) {
        enabled = (s_server.arg("enabled") == "true" || s_server.arg("enabled") == "1");
        parsed = true;
    }

    if (!parsed) {
        s_server.send(400, "application/json", "{\"error\":\"Parameter 'enabled' tidak ditemukan\"}");
        return;
    }

    sharedDataSetPower(enabled);

    JsonDocument res;
    res["success"] = true;
    res["enabled"] = enabled;
    String resStr;
    serializeJson(res, resStr);
    s_server.send(200, "application/json", resStr);
}

// Endpoint: POST /api/reset-fault (D7)
static void handleResetFault() {
    SystemState st = sharedDataGet();
    bool success = safetyResetFault(st.heatsinkTemp);

    JsonDocument res;
    res["success"] = success;
    if (!success) {
        res["message"] = "Gagal me-reset FAULT: Suhu heatsink masih >= 45.0 C atau sistem tidak dalam status FAULT.";
    }
    String resStr;
    serializeJson(res, resStr);
    s_server.send(200, "application/json", resStr);
}

// Endpoint: GET /api/history (D6) - Menyajikan riwayat suhu air dan heatsink
static void handleHistory() {
    if (s_server.hasArg("legacy") || s_server.hasArg("water_only")) {
        // Kompatibilitas mundur jika dipanggil format array tunggal
        String json = "[";
        if (s_historyCount > 0) {
            uint16_t startIndex = (s_historyCount < HISTORY_CAPACITY) ? 0 : s_historyHead;
            for (uint16_t i = 0; i < s_historyCount; i++) {
                uint16_t idx = (startIndex + i) % HISTORY_CAPACITY;
                json += String(s_history[idx].waterTemp, 2);
                if (i < s_historyCount - 1) json += ",";
            }
        }
        json += "]";
        s_server.send(200, "application/json", json);
        return;
    }

    // Format lengkap: objek JSON berisi array suhu air & heatsink
    String json = "{\"water\":[";
    if (s_historyCount > 0) {
        uint16_t startIndex = (s_historyCount < HISTORY_CAPACITY) ? 0 : s_historyHead;
        for (uint16_t i = 0; i < s_historyCount; i++) {
            uint16_t idx = (startIndex + i) % HISTORY_CAPACITY;
            json += String(s_history[idx].waterTemp, 2);
            if (i < s_historyCount - 1) json += ",";
        }
    }
    json += "],\"heatsink\":[";
    if (s_historyCount > 0) {
        uint16_t startIndex = (s_historyCount < HISTORY_CAPACITY) ? 0 : s_historyHead;
        for (uint16_t i = 0; i < s_historyCount; i++) {
            uint16_t idx = (startIndex + i) % HISTORY_CAPACITY;
            json += String(s_history[idx].heatsinkTemp, 2);
            if (i < s_historyCount - 1) json += ",";
        }
    }
    json += "]}";
    s_server.send(200, "application/json", json);
}

// =============================================================================
// ENDPOINT DEVELOPER MODE (ANALISIS GRAFIK & SIMULASI PROTEKSI)
// =============================================================================

// GET /dev
static void handleDevRoot() {
    s_server.send_P(200, "text/html", DEVMODE_HTML);
}

// GET /api/dev/state
static void handleDevState() {
    SystemState st = sharedDataGet();
    SafetyDebugInfo dbg = safetyGetDebugInfo();
    SimulationState sim = sensorsGetSimState();

    JsonDocument doc;
    doc["water_temp"] = st.waterTemp;
    doc["heatsink_temp"] = st.heatsinkTemp;
    doc["setpoint"] = st.setpoint;
    doc["enabled"] = st.systemEnabled;
    doc["safety"] = safetyLevelToString(st.safetyLevel);
    doc["peltier_pct"] = st.peltierActualPct;
    doc["pid_output"] = st.pidOutput;
    doc["pump"] = st.pumpActive;
    doc["uptime_s"] = st.uptimeSeconds;

    // Safety Debug Timers & Counters
    doc["critical_count_window"] = dbg.criticalCountInWindow;
    doc["warning_dur_s"] = dbg.warningDurationSec;
    doc["critical_dur_s"] = dbg.criticalDurationSec;
    doc["heatsink_grace"] = dbg.heatsinkInGrace;
    doc["heatsink_grace_rem"] = dbg.heatsinkGraceRemSec;
    doc["water_grace"] = dbg.waterInGrace;
    doc["water_grace_rem"] = dbg.waterGraceRemSec;

    // Simulation Diagnostics
    doc["sim_hs_enabled"] = sim.heatsinkSimEnabled;
    doc["sim_hs_temp"] = sim.heatsinkSimTemp;
    doc["sim_water_enabled"] = sim.waterSimEnabled;
    doc["sim_water_temp"] = sim.waterSimTemp;
    doc["sim_hs_fail"] = sim.heatsinkForceError;
    doc["sim_water_fail"] = sim.waterForceError;

    String jsonStr;
    serializeJson(doc, jsonStr);
    s_server.send(200, "application/json", jsonStr);
}

// GET /api/dev/history/full
static void handleDevHistoryFull() {
    String json;
    json.reserve(s_historyCount * 36 + 128);
    json = "{\"water\":[";
    uint16_t startIndex = (s_historyCount < HISTORY_CAPACITY) ? 0 : s_historyHead;

    for (uint16_t i = 0; i < s_historyCount; i++) {
        uint16_t idx = (startIndex + i) % HISTORY_CAPACITY;
        json += String(s_history[idx].waterTemp, 1);
        if (i < s_historyCount - 1) json += ",";
    }
    json += "],\"heatsink\":[";
    for (uint16_t i = 0; i < s_historyCount; i++) {
        uint16_t idx = (startIndex + i) % HISTORY_CAPACITY;
        json += String(s_history[idx].heatsinkTemp, 1);
        if (i < s_historyCount - 1) json += ",";
    }
    json += "],\"peltier\":[";
    for (uint16_t i = 0; i < s_historyCount; i++) {
        uint16_t idx = (startIndex + i) % HISTORY_CAPACITY;
        json += String(s_history[idx].peltierPct, 1);
        if (i < s_historyCount - 1) json += ",";
    }
    json += "],\"pid\":[";
    for (uint16_t i = 0; i < s_historyCount; i++) {
        uint16_t idx = (startIndex + i) % HISTORY_CAPACITY;
        json += String(s_history[idx].pidOutput, 1);
        if (i < s_historyCount - 1) json += ",";
    }
    json += "],\"safety\":[";
    for (uint16_t i = 0; i < s_historyCount; i++) {
        uint16_t idx = (startIndex + i) % HISTORY_CAPACITY;
        json += String(s_history[idx].safetyLevel);
        if (i < s_historyCount - 1) json += ",";
    }
    json += "]}";

    s_server.send(200, "application/json", json);
}

// GET /api/dev/export/csv
static void handleDevExportCsv() {
    String csv = "index,sec_ago,water_temp_c,heatsink_temp_c,peltier_pct,pid_output_pct,safety_level\r\n";
    if (s_historyCount > 0) {
        uint16_t startIndex = (s_historyCount < HISTORY_CAPACITY) ? 0 : s_historyHead;
        for (uint16_t i = 0; i < s_historyCount; i++) {
            uint16_t idx = (startIndex + i) % HISTORY_CAPACITY;
            uint32_t secAgo = (s_historyCount - 1 - i) * 10;
            csv += String(i);
            csv += ",";
            csv += String(secAgo);
            csv += ",";
            csv += String(s_history[idx].waterTemp, 2);
            csv += ",";
            csv += String(s_history[idx].heatsinkTemp, 2);
            csv += ",";
            csv += String(s_history[idx].peltierPct, 1);
            csv += ",";
            csv += String(s_history[idx].pidOutput, 1);
            csv += ",";
            csv += String(s_history[idx].safetyLevel);
            csv += "\r\n";
        }
    }
    s_server.sendHeader("Content-Disposition", "attachment; filename=aquatec_history.csv");
    s_server.send(200, "text/csv", csv);
}

// POST /api/dev/sim/heatsink
static void handleDevSimHeatsink() {
    bool enabled = true;
    float temp = 25.0f;
    if (s_server.hasArg("plain")) {
        JsonDocument doc;
        DeserializationError err = deserializeJson(doc, s_server.arg("plain"));
        if (!err) {
            if (!doc["enabled"].isNull()) enabled = doc["enabled"].as<bool>();
            if (!doc["temp"].isNull()) temp = doc["temp"].as<float>();
        }
    } else {
        if (s_server.hasArg("enabled")) enabled = (s_server.arg("enabled") == "true" || s_server.arg("enabled") == "1");
        if (s_server.hasArg("temp")) temp = s_server.arg("temp").toFloat();
    }
    sensorsSetHeatsinkSimulation(enabled, temp);
    s_server.send(200, "application/json", "{\"success\":true}");
}

// POST /api/dev/sim/water
static void handleDevSimWater() {
    bool enabled = true;
    float temp = 25.0f;
    if (s_server.hasArg("plain")) {
        JsonDocument doc;
        DeserializationError err = deserializeJson(doc, s_server.arg("plain"));
        if (!err) {
            if (!doc["enabled"].isNull()) enabled = doc["enabled"].as<bool>();
            if (!doc["temp"].isNull()) temp = doc["temp"].as<float>();
        }
    } else {
        if (s_server.hasArg("enabled")) enabled = (s_server.arg("enabled") == "true" || s_server.arg("enabled") == "1");
        if (s_server.hasArg("temp")) temp = s_server.arg("temp").toFloat();
    }
    sensorsSetWaterSimulation(enabled, temp);
    s_server.send(200, "application/json", "{\"success\":true}");
}

// POST /api/dev/sim/sensor-fail
static void handleDevSimSensorFail() {
    bool failHs = false;
    bool failWater = false;
    if (s_server.hasArg("plain")) {
        JsonDocument doc;
        DeserializationError err = deserializeJson(doc, s_server.arg("plain"));
        if (!err) {
            if (!doc["heatsink"].isNull()) failHs = doc["heatsink"].as<bool>();
            if (!doc["water"].isNull()) failWater = doc["water"].as<bool>();
        }
    } else {
        if (s_server.hasArg("heatsink")) failHs = (s_server.arg("heatsink") == "true" || s_server.arg("heatsink") == "1");
        if (s_server.hasArg("water")) failWater = (s_server.arg("water") == "true" || s_server.arg("water") == "1");
    }
    sensorsSetForceError(failHs, failWater);
    s_server.send(200, "application/json", "{\"success\":true}");
}

// POST /api/dev/sim/reset
static void handleDevSimReset() {
    sensorsResetAllSimulation();
    s_server.send(200, "application/json", "{\"success\":true}");
}

// POST /api/dev/safety/force
static void handleDevSafetyForce() {
    String lvlStr = "";
    if (s_server.hasArg("plain")) {
        JsonDocument doc;
        DeserializationError err = deserializeJson(doc, s_server.arg("plain"));
        if (!err && !doc["level"].isNull()) lvlStr = doc["level"].as<String>();
    } else if (s_server.hasArg("level")) {
        lvlStr = s_server.arg("level");
    }

    lvlStr.toUpperCase();
    if (lvlStr == "NORMAL") {
        safetyForceState(SAFETY_NORMAL);
        sensorsResetAllSimulation(); // Reset simulasi agar suhu tidak nyangkut di nilai simulasi
    }
    else if (lvlStr == "WARNING") safetyForceState(SAFETY_WARNING);
    else if (lvlStr == "CRITICAL") safetyForceState(SAFETY_CRITICAL);
    else if (lvlStr == "FAULT") safetyForceState(SAFETY_FAULT);

    s_server.send(200, "application/json", "{\"success\":true}");
}

void webServerInit() {
    // 1. Baca setpoint tersimpan dari NVS Flash (Preferences)
    s_prefs.begin("aquatec", false);
    float savedSetpoint = s_prefs.getFloat("setpoint", SETPOINT_DEFAULT);
    if (savedSetpoint < SETPOINT_MIN || savedSetpoint > SETPOINT_MAX) {
        savedSetpoint = SETPOINT_DEFAULT;
    }
    sharedDataSetpoint(savedSetpoint);

    // 2. Hubungkan ke Jaringan WiFi (Timeout 15 Detik)
    Serial.printf("[WiFi] Mencoba terhubung ke SSID: %s...\n", WIFI_SSID_DEFAULT);
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID_DEFAULT, WIFI_PASSWORD_DEFAULT);

    uint32_t startAttempt = millis();
    bool connected = false;
    while (millis() - startAttempt < (WIFI_CONNECT_TIMEOUT_S * 1000UL)) {
        if (WiFi.status() == WL_CONNECTED) {
            connected = true;
            break;
        }
        delay(500);
        Serial.print(".");
    }
    Serial.println();

    String currentIp;
    bool isAp = false;

    if (connected) {
        currentIp = WiFi.localIP().toString();
        Serial.printf("[WiFi] Berhasil terhubung! IP: %s\n", currentIp.c_str());
    } else {
        // Fallback otomatis ke Mode Access Point (PRD 5.4)
        Serial.println("[WiFi] Gagal terhubung ke Station. Berpindah ke Mode SoftAP...");
        WiFi.mode(WIFI_AP);
        WiFi.softAP(AP_SSID, AP_PASSWORD);
        currentIp = WiFi.softAPIP().toString();
        isAp = true;
        Serial.printf("[WiFi] SoftAP aktif! SSID: '%s', IP: %s\n", AP_SSID, currentIp.c_str());
    }

    sharedDataSetNetwork(currentIp.c_str(), isAp);

    // 3. Konfigurasi mDNS (aquatec.local)
    if (MDNS.begin(MDNS_HOSTNAME)) {
        Serial.printf("[mDNS] Layanan aktif di http://%s.local\n", MDNS_HOSTNAME);
    }

    // 4. Daftarkan Rute Endpoint Web Server
    s_server.on("/", HTTP_GET, handleRoot);
    s_server.on("/api/status", HTTP_GET, handleStatus);
    s_server.on("/api/setpoint", HTTP_POST, handleSetpoint);
    s_server.on("/api/power", HTTP_POST, handlePower);
    s_server.on("/api/reset-fault", HTTP_POST, handleResetFault);
    s_server.on("/api/history", HTTP_GET, handleHistory);

    // Rute Developer Mode (Analisis Matplotlib & Simulasi Proteksi)
    s_server.on("/dev", HTTP_GET, handleDevRoot);
    s_server.on("/api/dev/state", HTTP_GET, handleDevState);
    s_server.on("/api/dev/history/full", HTTP_GET, handleDevHistoryFull);
    s_server.on("/api/dev/export/csv", HTTP_GET, handleDevExportCsv);
    s_server.on("/api/dev/sim/heatsink", HTTP_POST, handleDevSimHeatsink);
    s_server.on("/api/dev/sim/water", HTTP_POST, handleDevSimWater);
    s_server.on("/api/dev/sim/sensor-fail", HTTP_POST, handleDevSimSensorFail);
    s_server.on("/api/dev/sim/reset", HTTP_POST, handleDevSimReset);
    s_server.on("/api/dev/safety/force", HTTP_POST, handleDevSafetyForce);

    s_server.begin();
    Serial.println("[WebServer] Server HTTP port 80 telah siap menerima permintaan.");
}

void webServerProcess() {
    s_server.handleClient();
}

