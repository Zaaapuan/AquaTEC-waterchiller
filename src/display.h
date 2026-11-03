#ifndef DISPLAY_H
#define DISPLAY_H

#include <Arduino.h>
#include "shared_data.h"

// Inisialisasi layar LCD 16x2 I2C pada SDA 23 dan SCL 22
// Mengembalikan false jika LCD tidak terdeteksi (sistem tetap jalan tanpa LCD)
bool displayInit();

// Menampilkan layar splash boot awal (Nama dan IP)
void displayShowBoot(const char* ipStr);

// Memperbarui tampilan data operasional (anti-flicker tanpa lcd.clear())
void displayUpdate(const SystemState &state);

#endif // DISPLAY_H

