#ifndef CONTROL_H
#define CONTROL_H

#include <Arduino.h>
#include "shared_data.h"
#include "safety.h"
#include "config.h"

// Inisialisasi controller PID dan pelindung Peltier
void controlInit();

// Update tuning parameter PID jika diperlukan
void controlUpdateTunings(float kp, float ki, float kd);

// Eksekusi siklus regulasi kontrol suhu (dipanggil tiap 1000 ms oleh TaskControl)
void controlStep(const SystemState &state, 
                 const SafetyDecision &safety, 
                 float &outRawPid, 
                 float &outPeltierPct, 
                 bool &outPump);

#endif // CONTROL_H

