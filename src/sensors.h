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

// Mode Simulasi (FR-17 & Developer Mode) untuk pengujian proteksi overheat
struct SimulationState {
    bool  heatsinkSimEnabled;
    float heatsinkSimTemp;
    bool  waterSimEnabled;
    float waterSimTemp;
    bool  heatsinkForceError;
    bool  waterForceError;
};

void sensorsSetHeatsinkSimulation(bool enabled, float temp);
bool sensorsIsHeatsinkSimulated();
float sensorsGetSimulatedHeatsinkTemp();

void sensorsSetWaterSimulation(bool enabled, float temp);
bool sensorsIsWaterSimulated();
float sensorsGetSimulatedWaterTemp();

void sensorsSetForceError(bool heatsinkErr, bool waterErr);
SimulationState sensorsGetSimState();
void sensorsResetAllSimulation();

#endif // SENSORS_H

