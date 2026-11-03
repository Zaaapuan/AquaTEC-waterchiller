#include "display.h"
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include "config.h"

static LiquidCrystal_I2C s_lcd(LCD_I2C_ADDR, LCD_COLS, LCD_ROWS);
static bool s_lcdReady = false;
static uint8_t s_detectedAddress = LCD_I2C_ADDR;

// Bitmap custom character untuk derajat Celsius (°), 5x8 pixel
static const uint8_t s_degreeChar[8] = {
    0b00110,
    0b01001,
    0b01001,
    0b00110,
    0b00000,
    0b00000,
    0b00000,
    0b00000
};

// State perputaran / rotasi tampilan section LCD
static uint32_t s_lastSectionSwitchMs = 0;
static uint8_t  s_currentSection = 0; // 0: Section 1, 1: Section 2

bool displayInit() {
    // Inisialisasi bus Wire dengan pin I2C non-bawaan (GPIO 23 SDA, GPIO 22 SCL)
    Wire.begin(PIN_LCD_SDA, PIN_LCD_SCL);

    // Deteksi keberadaan LCD (coba alamat 0x27, jika gagal coba 0x3F)
    Wire.beginTransmission(LCD_I2C_ADDR);
    if (Wire.endTransmission() == 0) {
        s_detectedAddress = LCD_I2C_ADDR;
        s_lcdReady = true;
    } else {
        Wire.beginTransmission(0x3F);
        if (Wire.endTransmission() == 0) {
            s_detectedAddress = 0x3F;
            s_lcd = LiquidCrystal_I2C(0x3F, LCD_COLS, LCD_ROWS);
            s_lcdReady = true;
        } else {
            // LCD tidak terpasang atau alamat tidak sesuai.
            // Sesuai FR-23: Sistem tetap jalan normal tanpa LCD!
            s_lcdReady = false;
            Serial.println("[LCD] Peringatan: Layar LCD I2C tidak terdeteksi pada 0x27 atau 0x3F.");
            return false;
        }
    }

    s_lcd.init();
    s_lcd.backlight();
    s_lcd.createChar(1, (uint8_t*)s_degreeChar);
    s_lcd.clear();
    return true;
}

void displayShowBoot(const char* ipStr) {
    if (!s_lcdReady) return;

    char line1[17];
    char line2[17];

    snprintf(line1, sizeof(line1), "   AquaTEC v1.4 ");
    snprintf(line2, sizeof(line2), "%-16.16s", (ipStr != NULL && strlen(ipStr) > 0) ? ipStr : "Menghubungkan...");

    s_lcd.setCursor(0, 0);
    s_lcd.print(line1);
    s_lcd.setCursor(0, 1);
    s_lcd.print(line2);
}

void displayUpdate(const SystemState &state) {
    if (!s_lcdReady) return;

    // Logika pergantian (looping) section secara periodik
    uint32_t now = millis();
    if (s_lastSectionSwitchMs == 0) {
        s_lastSectionSwitchMs = now;
        s_currentSection = 0;
    } else if (now - s_lastSectionSwitchMs >= LCD_SECTION_INTERVAL_MS) {
        s_lastSectionSwitchMs = now;
        s_currentSection = (s_currentSection == 0) ? 1 : 0;
    }

    char line1[17];
    char line2[17];

    if (s_currentSection == 0) {
        // =====================================================================
        // SECTION 1 (16 Karakter per baris):
        // Baris 1: Suhu Air: 20.0°C  (atau Suhu Air: ERROR )
        // Baris 2: Setpoint: 20.0°C
        // =====================================================================
        if (!state.waterSensorOk || state.sensorDataStale) {
            snprintf(line1, sizeof(line1), "Suhu Air: ERROR ");
        } else {
            float wTemp = state.waterTemp;
            if (wTemp > 99.9f) wTemp = 99.9f;
            if (wTemp < -9.9f) wTemp = -9.9f;
            snprintf(line1, sizeof(line1), "Suhu Air: %4.1f%cC", wTemp, (char)1);
        }

        float sp = state.setpoint;
        if (sp > 99.9f) sp = 99.9f;
        if (sp < 0.0f) sp = 0.0f;
        snprintf(line2, sizeof(line2), "Setpoint: %4.1f%cC", sp, (char)1);

    } else {
        // =====================================================================
        // SECTION 2 (16 Karakter per baris):
        // Baris 1: Heatsink: 40.0°C (Suhu Heatsink, muat pas 16 karakter)
        // Baris 2: Status: NORMAL / WARNING / CRITICAL / FAULT / OFF
        // Catatan revisi: Persentase daya PID tidak ditampilkan di LCD
        // =====================================================================
        if (!state.heatsinkSensorOk) {
            snprintf(line1, sizeof(line1), "Heatsink: ERROR ");
        } else {
            float hsTemp = state.heatsinkTemp;
            if (hsTemp > 99.9f) hsTemp = 99.9f;
            if (hsTemp < -9.9f) hsTemp = -9.9f;
            snprintf(line1, sizeof(line1), "Heatsink: %4.1f%cC", hsTemp, (char)1);
        }

        const char* statusStr = "NORMAL";
        if (!state.systemEnabled) {
            statusStr = "OFF";
        } else {
            switch (state.safetyLevel) {
                case SAFETY_WARNING:  statusStr = "WARNING";  break;
                case SAFETY_CRITICAL: statusStr = "CRITICAL"; break;
                case SAFETY_FAULT:    statusStr = "FAULT";    break;
                case SAFETY_NORMAL:
                default:              statusStr = "NORMAL";   break;
            }
        }
        snprintf(line2, sizeof(line2), "Status: %-8.8s", statusStr);
    }

    // Tulis ke LCD di posisi absolut tanpa lcd.clear() agar anti-flicker (FR-22)
    s_lcd.setCursor(0, 0);
    s_lcd.print(line1);
    s_lcd.setCursor(0, 1);
    s_lcd.print(line2);
}
