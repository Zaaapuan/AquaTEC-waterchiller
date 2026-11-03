#ifndef SENSORS_H
#define SENSORS_H

#include <Arduino.h>
#include "config.h"

// Inisialisasi bus OneWire dan sensor DS18B20
void sensorsInit();

// Meminta konversi suhu (non-blocking)
void sensorsRequestTemperatures();

// Membaca hasil konversi suhu setelah jeda konversi
// Mengembalikan suhu dan flag apakah nilai valid
bool sensorsReadWater(float &outTemp);
bool sensorsReadHeatsink(float &outTemp);

// Mode Simulasi (FR-17) untuk pengujian proteksi overheat
#if SIMULATION_MODE
void sensorsSetHeatsinkSimulation(bool enabled, float temp);
bool sensorsIsHeatsinkSimulated();
float sensorsGetSimulatedHeatsinkTemp();
#endif

#endif // SENSORS_H

