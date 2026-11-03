#include <Arduino.h>
#include <esp_task_wdt.h>
#include <esp_idf_version.h>

#include "config.h"
#include "shared_data.h"
#include "sensors.h"
#include "safety.h"
#include "control.h"
#include "actuators.h"
#include "display.h"
#include "web_server.h"

// Forward Declaration Task FreeRTOS
void TaskControl(void *pvParameters);
void TaskSensor(void *pvParameters);
void TaskDisplay(void *pvParameters);
void TaskWeb(void *pvParameters);
void TaskLogger(void *pvParameters);

// Fungsi pembantu parsing perintah Serial (Mode Simulasi FR-17)
void processSerialCommands();

void setup() {
    Serial.begin(115200);
    delay(1000);
    Serial.println("\n==============================================");
    Serial.println("   AquaTEC - ESP32 Water Temperature Control  ");
    Serial.println("==============================================");

    // 1. Inisialisasi Mutex & State Bersama
    if (!sharedDataInit()) {
        Serial.println("[ERROR] Gagal membuat Mutex FreeRTOS!");
        while (1) { delay(1000); }
    }

    // 2. Inisialisasi Driver Aktuator (Semua output default LOW/OFF)
    actuatorsInit();
    Serial.println("[Aktuator] Peltier PWM & Pompa air telah diinisialisasi.");

    // 3. Inisialisasi Sensor DS18B20
    sensorsInit();
    Serial.println("[Sensor] DS18B20 Suhu Air & Heatsink telah siap.");

    // 4. Inisialisasi Modul Safety & Kontrol PID
    safetyInit();
    controlInit();
    Serial.println("[Kontrol] State machine safety dan PID REVERSE telah aktif.");

    // 5. Inisialisasi Layar LCD I2C 16x2
    bool lcdOk = displayInit();
    if (lcdOk) {
        displayShowBoot("Menghubungkan...");
    }

    // 6. Inisialisasi Jaringan WiFi & Web Server
    webServerInit();

    // Perbarui info IP ke LCD setelah WiFi tersambung
    SystemState st = sharedDataGet();
    if (lcdOk) {
        displayShowBoot(st.ipAddress);
        delay(2000); // Tampilkan splash boot selama 2 detik sebelum masuk menu utama
    }

    // 7. Konfigurasi Task Watchdog Timer (WDT) 10 Detik untuk TaskControl
#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 0, 0)
    esp_task_wdt_config_t twdt_config = {
        .timeout_ms = WDT_TIMEOUT_SECONDS * 1000,
        .idle_core_mask = 0,
        .trigger_panic = true
    };
    esp_task_wdt_init(&twdt_config);
#else
    esp_task_wdt_init(WDT_TIMEOUT_SECONDS, true);
#endif

    // 8. Pembuatan Task FreeRTOS (PRD Bagian 4.2)
    // TaskControl: Prioritas 4 (Tertinggi), Core 1, Periode 1000 ms
    xTaskCreatePinnedToCore(
        TaskControl, "TaskControl", 4096, NULL, 4, NULL, 1
    );

    // TaskSensor: Prioritas 3, Core 1, Periode 1000 ms
    xTaskCreatePinnedToCore(
        TaskSensor, "TaskSensor", 4096, NULL, 3, NULL, 1
    );

    // TaskDisplay: Prioritas 1, Core 1, Periode 500 ms
    xTaskCreatePinnedToCore(
        TaskDisplay, "TaskDisplay", 3072, NULL, 1, NULL, 1
    );

    // TaskWeb: Prioritas 1, Core 0 (Isolasi Jaringan WiFi pada Core 0)
    xTaskCreatePinnedToCore(
        TaskWeb, "TaskWeb", 8192, NULL, 1, NULL, 0
    );

    // TaskLogger: Prioritas 1, Core 1 (Serial Monitor & Simulasi)
    xTaskCreatePinnedToCore(
        TaskLogger, "TaskLogger", 3072, NULL, 1, NULL, 1
    );

    Serial.println("[FreeRTOS] Seluruh task berhasil dijalankan!");
}

void loop() {
    // Sesuai PRD 4.2: fungsi loop() dihapus karena semua proses dijalankan oleh FreeRTOS task
    vTaskDelete(NULL);
}

// =============================================================================
// TASK 1: TaskControl (Core 1, Prioritas 4, 1000 ms)
// Satu-satunya task yang berhak mengendalikan pin fisik aktuator!
// =============================================================================
void TaskControl(void *pvParameters) {
    esp_task_wdt_add(NULL); // Daftarkan task ini ke Watchdog Timer

    TickType_t xLastWakeTime = xTaskGetTickCount();
    const TickType_t xFrequency = pdMS_TO_TICKS(1000);

    for (;;) {
        // Reset Watchdog Timer tiap siklus
        esp_task_wdt_reset();

        // 1. Ambil salinan state sistem terbaru secara aman
        SystemState state = sharedDataGet();

        // 2. Evaluasi Proteksi Overheat setelah pembacaan sensor pertama tersedia.
        // Selama startup, aktuator tetap OFF tanpa mengubah status menjadi CRITICAL.
        SafetyDecision safety;
        if (state.sensorDataReady) {
            safety = safetyEvaluate(
                state.heatsinkTemp,
                state.heatsinkSensorOk,
                state.waterTemp,
                state.waterSensorOk,
                state.sensorDataStale
            );
        } else {
            safety.level = SAFETY_NORMAL;
            safety.maxPeltierAllowedPct = 0.0f;
            safety.pumpAllowed = false;
            safety.forcePeltierOff = true;
            safety.forcePumpOff = true;
        }

        // 3. Hitung Algoritma Kontrol PID dan Batasan Fisik Peltier
        float rawPid = 0.0f;
        float actualPeltierPct = 0.0f;
        bool pumpTarget = false;

        controlStep(state, safety, rawPid, actualPeltierPct, pumpTarget);

        // 4. Eksekusi ke Pin Fisik Aktuator
        peltierSetPercent(actualPeltierPct);
        pumpSet(pumpTarget);

        // 5. Perbarui state bersama dengan hasil aktuasi
        sharedDataUpdateControl(rawPid, actualPeltierPct, pumpTarget, safety.level);

        // Tunggu siklus berikutnya persis 1000 ms
        vTaskDelayUntil(&xLastWakeTime, xFrequency);
    }
}

// =============================================================================
// TASK 2: TaskSensor (Core 1, Prioritas 3, 1000 ms)
// Pembacaan sensor DS18B20 dengan pola non-blocking
// =============================================================================
void TaskSensor(void *pvParameters) {
    TickType_t xLastWakeTime = xTaskGetTickCount();
    const TickType_t xFrequency = pdMS_TO_TICKS(1000);
    uint32_t lastHistoryRecordMs = 0;

    for (;;) {
        // Minta konversi suhu pada kedua sensor
        sensorsRequestTemperatures();

        // Tunggu konversi 12-bit selesai (~750 ms) tanpa memblokir CPU
        vTaskDelay(pdMS_TO_TICKS(800));

        // Ambil data hasil konversi
        float waterTemp = 0.0f;
        float heatsinkTemp = 0.0f;
        bool waterOk = sensorsReadWater(waterTemp);
        bool heatsinkOk = sensorsReadHeatsink(heatsinkTemp);

        // Tulis data ke shared state
        sharedDataUpdateSensors(waterTemp, waterOk, heatsinkTemp, heatsinkOk);

        // Catat riwayat suhu tiap 10 detik untuk grafik canvas di dashboard
        if (millis() - lastHistoryRecordMs >= HISTORY_INTERVAL_MS) {
            lastHistoryRecordMs = millis();
            if (waterOk || heatsinkOk) {
                webServerRecordHistory(waterTemp, heatsinkTemp);
            }
        }

        vTaskDelayUntil(&xLastWakeTime, xFrequency);
    }
}

// =============================================================================
// TASK 3: TaskDisplay (Core 1, Prioritas 1, 500 ms)
// Mengelola tampilan LCD 16x2 tanpa menahan bus I2C atau mutex
// =============================================================================
void TaskDisplay(void *pvParameters) {
    const TickType_t xFrequency = pdMS_TO_TICKS(LCD_REFRESH_MS);

    for (;;) {
        // Salin state secepat mungkin lewat mutex
        SystemState state = sharedDataGet();

        // Tulis ke layar (mutex sudah dilepas sehingga tidak mengganggu task lain)
        displayUpdate(state);

        vTaskDelay(xFrequency);
    }
}

// =============================================================================
// TASK 4: TaskWeb (Core 0, Prioritas 1)
// Menangani server HTTP, API REST, dan manajemen jaringan
// =============================================================================
void TaskWeb(void *pvParameters) {
    uint32_t lastUptimeUpdateMs = 0;

    for (;;) {
        // Tangani permintaan HTTP
        webServerProcess();

        // Perbarui uptime tiap detik
        if (millis() - lastUptimeUpdateMs >= 1000) {
            lastUptimeUpdateMs = millis();
            sharedDataUpdateUptime(millis() / 1000UL);
        }

        vTaskDelay(pdMS_TO_TICKS(5)); // Jeda singkat untuk yield FreeRTOS
    }
}

// =============================================================================
// TASK 5: TaskLogger & Serial Command (Core 1, Prioritas 1)
// Mencetak log diagnostik tiap 5 detik dan menerima perintah simulasi
// =============================================================================
void TaskLogger(void *pvParameters) {
    uint32_t lastLogMs = 0;

    for (;;) {
        // Cek apakah ada input Serial dari pengguna
        if (Serial.available()) {
            processSerialCommands();
        }

        // Cetak log periodik tiap 5 detik
        if (millis() - lastLogMs >= 5000) {
            lastLogMs = millis();
            SystemState st = sharedDataGet();

            Serial.printf("[LOG] Air: %4.1f C | HS: %4.1f C | SP: %4.1f C | PID: %3.0f%% | Peltier: %3.0f%% | Pompa: %s | Safety: %s\n",
                st.waterTemp,
                st.heatsinkTemp,
                st.setpoint,
                st.pidOutput,
                st.peltierActualPct,
                st.pumpActive ? "ON" : "OFF",
                safetyLevelToString(st.safetyLevel)
            );
        }

        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

// =============================================================================
// PEMROSES PERINTAH SERIAL (FR-17: SIMULASI PENGUJIAN)
// Format perintah:
// - hs=66       -> Injeksi suhu heatsink 66 °C (Uji WARNING)
// - hs=72       -> Injeksi suhu heatsink 72 °C (Uji CRITICAL)
// - hs=reset    -> Matikan simulasi heatsink, kembali ke sensor riil
// - setpoint=27 -> Ubah setpoint ke 27.0 °C
// - power=on/off-> Hidupkan/matikan sistem
// =============================================================================
void processSerialCommands() {
    String cmd = Serial.readStringUntil('\n');
    cmd.trim();
    if (cmd.length() == 0) return;

#if SIMULATION_MODE
    if (cmd.startsWith("hs=")) {
        String valStr = cmd.substring(3);
        if (valStr.equalsIgnoreCase("reset")) {
            sensorsSetHeatsinkSimulation(false, 0.0f);
            Serial.println("[SIMULASI] Simulasi heatsink DIMATIKAN. Membaca sensor riil.");
        } else {
            float simT = valStr.toFloat();
            sensorsSetHeatsinkSimulation(true, simT);
            Serial.printf("[SIMULASI] Suhu heatsink disimulasikan: %.1f C\n", simT);
        }
        return;
    }
#endif

    if (cmd.startsWith("setpoint=")) {
        float sp = cmd.substring(9).toFloat();
        if (sharedDataSetpoint(sp)) {
            Serial.printf("[KONTROL] Setpoint diubah ke: %.1f C\n", sp);
        } else {
            Serial.println("[ERROR] Setpoint tidak valid! (Rentang: 20.0 - 30.0 C)");
        }
    } else if (cmd.equalsIgnoreCase("power=on")) {
        sharedDataSetPower(true);
        Serial.println("[KONTROL] Sistem DIAKTIFKAN.");
    } else if (cmd.equalsIgnoreCase("power=off")) {
        sharedDataSetPower(false);
        Serial.println("[KONTROL] Sistem DINONAKTIFKAN.");
    } else if (cmd.equalsIgnoreCase("help")) {
        Serial.println("\n--- DAFTAR PERINTAH SERIAL AQUATEC ---");
        Serial.println("hs=<nilai>     : Injeksi simulasi suhu heatsink (contoh: hs=66)");
        Serial.println("hs=reset       : Kembali ke pembacaan sensor fisik heatsink");
        Serial.println("setpoint=<val> : Ubah target suhu air (20.0 - 30.0)");
        Serial.println("power=on/off   : Hidupkan atau matikan sistem");
        Serial.println("help           : Tampilkan panduan ini");
        Serial.println("---------------------------------------\n");
    } else {
        Serial.printf("[SERIAL] Perintah tidak dikenal: '%s'. Ketik 'help' untuk panduan.\n", cmd.c_str());
    }
}
