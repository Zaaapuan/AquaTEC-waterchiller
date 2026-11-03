#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

// =============================================================================
// KONFIGURASI PIN ESP32 (Sesuai PRD v1.3 & PLAN Seksi 0-3)
// =============================================================================
#define PIN_TEMP_WATER          19      // DS18B20 Suhu Air (pull-up 4.7kΩ ke 3.3V)
#define PIN_TEMP_HEATSINK       18      // DS18B20 Suhu Heatsink (pull-up 4.7kΩ ke 3.3V)
#define PIN_PELTIER_PWM         25      // Output PWM ke JZ-MOS #1 (Peltier TEC1-12706)
#define PIN_PUMP                27      // Output Digital ON/OFF ke JZ-MOS #2 (Pompa 5V)
#define PUMP_ACTIVE_LEVEL       HIGH    // Modul MOSFET active-HIGH

// Pin I2C untuk LCD Karakter 16x2
#define PIN_LCD_SDA             23      // Pin SDA non-default, panggil Wire.begin(23, 22)
#define PIN_LCD_SCL             22      // Pin SCL ESP32
#define LCD_I2C_ADDR            0x27    // Alamat I2C PCF8574 (bisa 0x27 atau 0x3F)
#define LCD_COLS                16
#define LCD_ROWS                2
#define LCD_REFRESH_MS          500     // Interval perbaruan LCD (ms)
#define LCD_SECTION_INTERVAL_MS 3000    // Durasi tiap section sebelum berganti (ms)

// Catatan Keras: Fan 12V tersambung langsung ke jalur 12V PSU (Selalu ON),
// tidak menggunakan pin GPIO ESP32 (hardware safety fail-safe).

// =============================================================================
// KONFIGURASI SETPOINT SUHU AIR
// =============================================================================
#define SETPOINT_DEFAULT        28.0f   // Setpoint awal (°C)
#define SETPOINT_MIN            20.0f   // Batas minimum aman (°C)
#define SETPOINT_MAX            30.0f   // Batas maksimum (°C)
#define SETPOINT_STEP           0.5f    // Langkah perubahan (°C)

// =============================================================================
// KONFIGURASI PROTEKSI OVERHEAT BERTAHAP (PRD Bagian 7)
// =============================================================================
// Tahap 1: WARNING
#define HS_WARNING_ON           60.0f   // Heatsink >= 60°C -> Masuk WARNING
#define HS_WARNING_OFF          50.0f   // Heatsink <= 50°C -> Kembali ke NORMAL
#define HS_WARNING_PELTIER_MAX  70.0f   // Daya maksimal Peltier saat WARNING (%)
#define WARNING_MAX_SEC         120     // Jika WARNING > 120 detik -> Lanjut CRITICAL

// Tahap 2: CRITICAL
#define HS_CRITICAL_ON          70.0f   // Heatsink >= 70°C -> Masuk CRITICAL
#define HS_CRITICAL_OFF         45.0f   // Heatsink <= 45°C untuk syarat pemulihan
#define CRITICAL_MIN_SEC        60      // Wajib berada di CRITICAL min. 60 detik sebelum pulih

// Tahap 3: FAULT (Latching Protection)
#define CRITICAL_FAULT_COUNT    3       // Jika CRITICAL terjadi 3 kali...
#define FAULT_WINDOW_SEC        1800    // ...dalam kurun 30 menit (1800 detik) -> FAULT
#define FAULT_RECOVER_MAX_TEMP  45.0f   // Syarat suhu untuk reset FAULT manual

// =============================================================================
// BATAS FAIL-SAFE TAMBAHAN
// =============================================================================
#define WATER_TEMP_MIN_SAFE     20.0f   // Jika suhu air < 20°C, Peltier OFF (proteksi beku)
#define WATER_TEMP_MAX_ALARM    32.0f   // Suhu air > 32°C memicu alarm visual di dashboard
#define SENSOR_TIMEOUT_SEC      5       // Data > 5 detik tanpa pembaruan dianggap invalid
#define SENSOR_MAX_ERRORS       3       // 3 kegagalan baca berurutan menandai sensor rusak

// =============================================================================
// PARAMETER KONTROL PID & PROTEKSI PELTIER (PRD Bagian 6.2)
// =============================================================================
#define PID_KP                  25.0f   // Titik awal Proportional gain
#define PID_KI                  0.05f   // Titik awal Integral gain
#define PID_KD                  0.0f    // Titik awal Derivative gain
#define PID_SAMPLE_TIME_MS      1000    // Periode kalkulasi PID (1000 ms)

#define PELTIER_MIN_POWER_PCT   15.0f   // Histeresis: Peltier ON jika PID >= 15%
#define PELTIER_OFF_POWER_PCT   5.0f    // Histeresis: Peltier OFF jika PID < 5%
#define PELTIER_MIN_SWITCH_SEC  30      // Jeda minimum antar status ON/OFF (anti-thermal cycling)
#define PELTIER_SOFT_START_RAMP 5.0f    // Batas kenaikan daya maks. 5% per detik

// =============================================================================
// KONFIGURASI PWM PELTIER (LEDC ESP32)
// =============================================================================
#define PELTIER_PWM_CHANNEL     0       // Channel PWM LEDC
#define PELTIER_PWM_FREQ        5000    // Frekuensi 5 kHz (mencegah siklus panas-dingin cepat)
#define PELTIER_PWM_RES_BITS    10      // Resolusi 10-bit (nilai rentang 0 - 1023)
#define PELTIER_PWM_MAX_DUTY    1023    // (1 << PELTIER_PWM_RES_BITS) - 1

// =============================================================================
// KONFIGURASI JARINGAN & WEB SERVER
// =============================================================================
#define AP_SSID                 "AquaTEC"
#define AP_PASSWORD             ""      // Mode Access Point terbuka (tanpa password)
#define WIFI_CONNECT_TIMEOUT_S  15      // Timeout koneksi Station sebelum fallback ke AP
#define MDNS_HOSTNAME           "aquatec"// http://aquatec.local

// Riwayat Suhu untuk Grafik Dashboard (D6)
#define HISTORY_CAPACITY        360     // 360 data point (1 sampel tiap 10 detik = 1 jam)
#define HISTORY_INTERVAL_MS     10000   // 10 detik sekali

// Watchdog Timer (ESP Task WDT)
#define WDT_TIMEOUT_SECONDS     10

#endif // CONFIG_H

