#ifndef WEB_SERVER_H
#define WEB_SERVER_H

#include <Arduino.h>
#include "shared_data.h"

// Inisialisasi koneksi WiFi (Station / SoftAP fallback), mDNS, NVS Preferences, dan REST API
void webServerInit();

// Menangani permintaan klien HTTP (dipanggil berulang dalam TaskWeb di Core 0)
void webServerProcess();

// Mencatat data riwayat suhu air, heatsink, aktuator, dan safety ke dalam ring buffer
void webServerRecordHistory(float waterTemp, float heatsinkTemp = NAN,
                            float peltierPct = 0.0f, float pidOutput = 0.0f,
                            uint8_t safetyLevel = 0);

#endif // WEB_SERVER_H

