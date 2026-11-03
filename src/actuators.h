#ifndef ACTUATORS_H
#define ACTUATORS_H

#include <Arduino.h>
#include "config.h"

// Inisialisasi pin aktuator dan PWM LEDC
void actuatorsInit();

// Mengatur daya Peltier dalam persen (0.0% - 100.0%)
// Menghitung duty cycle 10-bit (0 - 1023) pada frekuensi 5 kHz
void peltierSetPercent(float pct);

// Mengatur status hidup/mati pompa air 5V
void pumpSet(bool on);

// Penghentian darurat semua aktuator (Peltier 0%, Pompa OFF)
void actuatorsEmergencyStop();

#endif // ACTUATORS_H

