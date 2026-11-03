#include "web_server.h"
#include <WiFi.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <Preferences.h>
#include <ArduinoJson.h>
#include "secrets.h"
#include "config.h"
#include "dashboard_html.h"
#include "safety.h"

static WebServer s_server(80);
static Preferences s_prefs;

// Ring buffer untuk riwayat grafik suhu (360 titik = 1 jam terakhir)
static float s_historyBuffer[HISTORY_CAPACITY];   // Riwayat Suhu Air
static float s_heatsinkHistory[HISTORY_CAPACITY]; // Riwayat Suhu Heatsink
static uint16_t s_historyHead = 0;
static uint16_t s_historyCount = 0;

void webServerRecordHistory(float waterTemp, float heatsinkTemp) {
    bool validWater = (!isnan(waterTemp) && waterTemp >= -40.0f && waterTemp <= 100.0f);
    bool validHs = (!isnan(heatsinkTemp) && heatsinkTemp >= -40.0f && heatsinkTemp <= 125.0f);

    if (!validWater && !validHs) {
        return;
    }

    // Ambil nilai sebelumnya jika salah satu sensor sempat gagal baca sekilas
    float prevWater = (s_historyCount > 0) ? s_historyBuffer[(s_historyHead + HISTORY_CAPACITY - 1) % HISTORY_CAPACITY] : 25.0f;
    float prevHs = (s_historyCount > 0) ? s_heatsinkHistory[(s_historyHead + HISTORY_CAPACITY - 1) % HISTORY_CAPACITY] : 30.0f;

    s_historyBuffer[s_historyHead] = validWater ? waterTemp : prevWater;
    s_heatsinkHistory[s_historyHead] = validHs ? heatsinkTemp : prevHs;

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
                json += String(s_historyBuffer[idx], 2);
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
            json += String(s_historyBuffer[idx], 2);
            if (i < s_historyCount - 1) json += ",";
        }
    }
    json += "],\"heatsink\":[";
    if (s_historyCount > 0) {
        uint16_t startIndex = (s_historyCount < HISTORY_CAPACITY) ? 0 : s_historyHead;
        for (uint16_t i = 0; i < s_historyCount; i++) {
            uint16_t idx = (startIndex + i) % HISTORY_CAPACITY;
            json += String(s_heatsinkHistory[idx], 2);
            if (i < s_historyCount - 1) json += ",";
        }
    }
    json += "]}";
    s_server.send(200, "application/json", json);
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

    s_server.begin();
    Serial.println("[WebServer] Server HTTP port 80 telah siap menerima permintaan.");
}

void webServerProcess() {
    s_server.handleClient();
}

