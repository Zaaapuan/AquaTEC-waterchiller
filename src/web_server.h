#ifndef WEB_SERVER_H
#define WEB_SERVER_H

#include <Arduino.h>
#include "shared_data.h"

// Inisialisasi koneksi WiFi (Station / SoftAP fallback), mDNS, NVS Preferences, dan REST API
void webServerInit();

// Menangani permintaan klien HTTP (dipanggil berulang dalam TaskWeb di Core 0)
void webServerProcess();

// Mencatat data riwayat suhu air ke dalam ring buffer (dipanggil berkala tiap 10 detik)
void webServerRecordHistory(float waterTemp);

#endif // WEB_SERVER_H

