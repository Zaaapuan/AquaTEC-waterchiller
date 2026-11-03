# PRD: ESP32-Based Water Temperature Control System for Aquascape Using a Thermoelectric Cooler with Overheat Protection

| | |
|---|---|
| **Versi** | 1.3 (draft). Revisi: sensor heatsink pindah ke GPIO 2, SDA LCD pindah ke GPIO 23 |
| **Tanggal** | 3 November 2026 (revisi). Versi awal: 29 Oktober 2026 |
| **Mata kuliah** | Sistem Embedded |
| **Target pembaca** | Coder pemula (belum biasa membuat proyek embedded multi-bagian) |
| **Tag penting** | `[ASUMSI]` = hal yang belum Anda tentukan, saya isi dengan nilai wajar. Boleh diganti. |

---

## 1. Ringkasan

Sistem ini menjaga **suhu air aquascape tetap stabil di angka yang diinginkan (set point)**. Pendinginan memakai **Peltier (thermoelectric cooler)**. Seluruh sistem dikendalikan **ESP32** dengan algoritma **PID**, dan diatur lewat **dashboard web**.

Sisi panas Peltier berbahaya jika terlalu panas. Karena itu ada **proteksi overheat bertahap** yang bisa mematikan pompa dan Peltier secara otomatis.

### Tujuan
1. Suhu air stabil di sekitar set point (target: selisih ≤ ±0,5 °C setelah stabil).
2. Pengguna bisa mengubah set point dan memantau suhu dari dashboard, serta melihat suhu dan set point langsung di LCD pada alat.
3. Sistem aman: tidak ada komponen yang rusak akibat panas berlebih, walaupun sensor atau fan bermasalah.
4. Kode rapi, terbagi per modul, dan mudah dijelaskan saat presentasi mata kuliah.

### Bukan tujuan (di luar cakupan)
- Pemanas air. Sistem **hanya mendinginkan**. Jika air lebih dingin dari set point, sistem diam saja.
- Akses dari internet / cloud. Dashboard hanya lewat WiFi lokal.
- Login dan keamanan tingkat produksi.
- Tombol fisik untuk mengubah set point. LCD hanya menampilkan, set point diubah lewat dashboard.
- Pengontrol pencahayaan, CO₂, atau parameter air lain.

---

## 2. Cara Kerja Sistem

### 2.1 Alur loop utama (berulang tiap 1 detik)

```
 Pengguna isi set point di dashboard
              │
              ▼
   ┌──────────────────────────────┐
   │ 1. Baca suhu air             │
   │    dan suhu heatsink         │
   │ 2. Cek PROTEKSI OVERHEAT     │ ← selalu dicek PERTAMA
   │ 3. error = suhu_air − setpoint│
   │ 4. PID menghitung output 0–100%│
   │ 5. Atur Peltier dan pompa    │
   │ 6. Perbarui dashboard + LCD  │
   └──────────────┬───────────────┘
                  └──── kembali ke langkah 1 (loop tertutup)
```

### 2.2 Skema pendinginan (aliran air dan panas)

```
 Akuarium ──▶ Pompa 5V ──▶ Water block ──▶ kembali ke Akuarium
                               │
                      (sisi DINGIN Peltier)
                           [ PELTIER ]
                      (sisi PANAS Peltier)
                               │
                    Heatsink + Fan 12V  ← DS18B20 #2 (suhu heatsink)
```

Air ditarik dari akuarium, didorong pompa ke water block. Water block menempel di sisi dingin Peltier, jadi air didinginkan lalu kembali ke akuarium. Panas dari sisi panas dibuang heatsink dan fan.

---

## 3. Hardware

### 3.1 Daftar komponen

| Komponen | Spesifikasi | Catatan |
|---|---|---|
| Mikrokontroler | ESP32 DevKitV1 DOIT (WROOM, 30 pin) |  |
| Sensor suhu air | DS18B20 waterproof | Dicelupkan di akuarium |
| Sensor suhu heatsink | DS18B20 (versi probe/waterproof) | Ditempel di heatsink dekat Peltier, pakai pasta termal atau lakban aluminium |
| Peltier | TEC1-12706 (12V, maks ±6A) |  1 buah |
| Driver Peltier | Modul MOSFET switch JZ-MOS #1 | Menerima sinyal PWM dari ESP32. Lihat catatan 3.3 |
| Driver pompa | Modul MOSFET switch JZ-MOS #2 (jenis sama) | Dipakai hanya sebagai switch on/off |
| Layar | LCD karakter 16×2 + modul I2C (PCF8574), alamat 0x27  | Menampilkan suhu air, set point, suhu heatsink, dan status. Daya 5V dari buck |
| Pompa air | DC 5V, **on/off** | Lewat modul JZ-MOS #2, daya dari buck 5V |
| Fan heatsink | DC 12V, **selalu ON** | Langsung ke jalur 12V PSU, tidak dikontrol ESP32 |
| Catu daya | 12V 20A | |
| Step-down (buck) | 12V → 5V, min. 2A | Hanya untuk ESP32 dan pompa. Fan **tidak** lewat buck. **Jangan** ambil 5V dari pin ESP32 untuk pompa |
| Resistor | 2 × 4,7 kΩ | Pull-up untuk masing-masing DS18B20 |
| Sekring | 8A pada jalur 12V | Perlindungan korsleting |

### 3.2 Pemetaan pin ESP32 

| Fungsi | GPIO | Keterangan |
|---|---|---|
| DS18B20 suhu air | 19 | Pull-up 4,7 kΩ ke 3,3V |
| DS18B20 suhu heatsink | 18 | Pull-up 4,7 kΩ ke 3,3V. Dipisah dari sensor air agar tidak perlu mencari alamat sensor. |
| Peltier (PWM) | 25 | Ke input sinyal modul JZ-MOS #1 |
| Pompa (on/off) | 27 | Ke sinyal modul JZ-MOS #2 |
| LCD I2C SDA | 23 | Bukan pin I2C bawaan (bawaan ESP32 adalah 21), jadi harus ditentukan di kode: `Wire.begin(PIN_LCD_SDA, PIN_LCD_SCL)` |
| LCD I2C SCL | 22 | Pin I2C bawaan ESP32. Bersebelahan dengan SDA di header |

Fan tidak memakai GPIO karena langsung ke PSU.


### 3.3 Catatan penting kelistrikan

- **Daya**: perkiraan beban Peltier 6A + fan 0,25A + pompa 0,5A + ESP32/buck 0,3A + LCD 0,05A ≈ **7A**. Aman untuk PSU 10A (sisa ±30%). Jika Peltier lebih dari satu, perlu PSU lebih besar.
- **Ground harus disatukan** antara ESP32, modul JZ-MOS, LCD, PSU, dan buck.
- **Modul JZ-MOS**: modul switch MOSFET yang dikendalikan lewat pin sinyal (bukan potensiometer di papan), jadi cocok untuk ESP32. `[ASUMSI]` Rentang tegangan trigger-nya mencakup 3,3V dan bisa menerima PWM ±5 kHz. **Cek label atau datasheet modul Anda**: (a) tegangan trigger minimum ≤ 3,3V, (b) tegangan beban sesuai (12V untuk Peltier, 5V untuk pompa) dan arus ≥ 10A untuk Peltier, (c) pasang beban dan sinyal sesuai tanda di papan (IN, OUT, trigger/signal).
- **Peltier dan pompa** masing-masing memakai satu modul JZ-MOS (total 2 modul). JZ-MOS #1 menerima sinyal PWM untuk Peltier. JZ-MOS #2 hanya di-ON/OFF untuk pompa 5V. Pastikan JZ-MOS #2 cocok untuk beban 5V dan sinyal 3,3V dari ESP32. Beban tidak boleh disambung langsung ke pin GPIO.
- **Fan 12V** disambung **langsung ke jalur 12V PSU** dan menyala sejak PSU hidup. Jangan lewat buck converter: buck hanya menurunkan tegangan, jadi dari 12V tidak bisa menghasilkan 12V.
- Beri dioda flyback untuk beban motor (pompa) jika modul belum punya.
- **LCD I2C**: ambil daya 5V dari output buck (bukan dari pin 3V3 ESP32). SDA ke GPIO 23, SCL ke GPIO 22. Karena SDA bukan pin bawaan, panggil `Wire.begin(PIN_LCD_SDA, PIN_LCD_SCL)` sebelum LCD diinisialisasi (program I2C scanner juga harus memakai pin ini). Alamat umum 0x27 atau 0x3F: jalankan program *I2C scanner* sekali untuk memastikan. Jika layar hanya menampilkan kotak-kotak atau kosong, putar potensiometer kontras di belakang modul. Perhatian: pada sebagian modul, resistor pull-up SDA/SCL tersambung ke 5V sehingga pin ESP32 (hanya toleran 3,3V) bisa menerima 5V. Jika ada jumper pull-up di modul, lepas dan pakai pull-up 4,7 kΩ ke 3,3V, atau pakai level shifter I2C.
- Jalur daya:

```
PSU 12V 20A ─┬──▶ JZ-MOS #1 ─▶ Peltier (PWM dari ESP32)
             ├─▶ Fan 12V (langsung, selalu menyala)
             └─▶ Buck 12V→5V ─┬─▶ ESP32 (VIN)
                              ├─▶ LCD I2C 16x2 (VCC 5V)
                              └─▶ JZ-MOS #2 ─▶ Pompa 5V (on/off dari ESP32)
```
- Pasang Peltier sesuai arah: sisi dingin ke water block, sisi panas ke heatsink. Pakai pasta termal di kedua sisi.

---

## 4. Software

### 4.1 Framework dan library `[ASUMSI]`

- **PlatformIO** (VS Code) dengan **Arduino framework** untuk ESP32. Arduino IDE juga bisa.
- **FreeRTOS sudah tersedia otomatis** di Arduino-ESP32, tidak perlu instal apa pun.

| Library | Fungsi |
|---|---|
| `OneWire` + `DallasTemperature` | Baca DS18B20 |
| `PID` (br3ttb / PID_v1) | Algoritma PID |
| `ArduinoJson` | Membuat dan membaca JSON untuk dashboard |
| `Wire` (bawaan) + `LiquidCrystal_I2C` | Menampilkan teks di LCD I2C |
| `WiFi`, `WebServer`, `ESPmDNS`, `Preferences` | Bawaan ESP32: WiFi, server web, alamat `aquatec.local`, penyimpanan set point |

Catatan: fungsi PWM di Arduino-ESP32 berbeda antara core versi 2.x (`ledcSetup` + `ledcAttachPin`) dan 3.x (`ledcAttach`). **Kunci versi platform di `platformio.ini`** agar tidak bingung.

### 4.2 Jawaban: apakah FreeRTOS bisa dipakai? Ya.

FreeRTOS memungkinkan beberapa pekerjaan berjalan "bersamaan" sebagai **task**. Manfaatnya di proyek ini: pembacaan sensor yang lambat (DS18B20 butuh ±0,75 detik) dan web server tidak mengganggu kontrol dan proteksi.

#### Skema task

| Task | Prioritas | Core | Periode | Tugas |
|---|---|---|---|---|
| `TaskControl` | **4 (tertinggi)** | 1 | 1000 ms | Cek proteksi → hitung PID → atur Peltier dan pompa. **Satu-satunya yang boleh menulis ke pin output** |
| `TaskSensor` | 3 | 1 | 1000 ms | Minta dan baca kedua DS18B20, validasi hasil |
| `TaskWeb` | 1 | 0 | terus-menerus | Melayani dashboard dan API (WiFi berjalan di core 0) |
| `TaskDisplay` | 1 | 1 | 500 ms | Membaca data dari struct (lewat mutex) dan menulis ke LCD. **Satu-satunya yang boleh memakai bus I2C/LCD** |
| `TaskLogger` (opsional) | 1 | 1 | 5000 ms | Cetak status ke Serial Monitor |

Fungsi `loop()` Arduino dibiarkan kosong (`vTaskDelete(NULL)`), semua kerja ada di task.

#### Berbagi data antar task

Semua data ada di satu struct yang dilindungi **mutex** agar tidak saling menimpa:

```cpp
struct SystemState {
  float waterTemp;       // °C
  float heatsinkTemp;    // °C
  bool  waterSensorOk;
  bool  heatsinkSensorOk;
  float setpoint;        // °C
  bool  systemEnabled;
  float pidOutput;       // 0–100 %
  uint8_t safetyLevel;   // 0=NORMAL, 1=WARNING, 2=CRITICAL, 3=FAULT
  bool  peltierOn, pumpOn;   // fan tidak dikontrol ESP32 (selalu ON)
};
extern SystemState g_state;
extern SemaphoreHandle_t g_mutex;

// Pola pemakaian: ambil → ubah/baca → lepas
xSemaphoreTake(g_mutex, portMAX_DELAY);
g_state.waterTemp = nilaiBaru;
xSemaphoreGive(g_mutex);
```

Aturan emas: **jangan berhenti lama (delay panjang) saat memegang mutex**. Gunakan `vTaskDelay()` dan `vTaskDelayUntil()` (bukan `delay()`) di dalam task.

Khusus `TaskDisplay`: salin data dari struct ke variabel lokal saat memegang mutex, lepaskan mutex, **baru** tulis ke LCD (penulisan I2C relatif lambat).

#### Pengaman tambahan
- **Task Watchdog** (`esp_task_wdt`, 10 detik) pada `TaskControl`. Jika macet, ESP32 restart dan output kontrol kembali LOW (Peltier dan pompa mati). Fan tetap menyala karena langsung ke PSU.
- Jika `TaskSensor` tidak memperbarui data > 5 detik, data dianggap **tidak valid** (lihat 6.3).

### 4.3 Struktur folder proyek

```
aquatec/
├─ platformio.ini
├─ include/
│  ├─ config.h          ← semua angka konfigurasi (pin, batas suhu, PID)
│  └─ secrets.h         ← SSID & password WiFi (jangan di-commit ke Git)
└─ src/
   ├─ main.cpp          ← setup() dan membuat task
   ├─ shared_data.h/.cpp← struct SystemState + mutex
   ├─ sensors.h/.cpp    ← baca DS18B20, validasi
   ├─ safety.h/.cpp     ← state machine proteksi overheat
   ├─ control.h/.cpp    ← PID + keputusan aktuator
   ├─ actuators.h/.cpp  ← PWM Peltier, on/off pompa
   ├─ display.h/.cpp    ← tampilan LCD I2C
   ├─ web_server.h/.cpp ← endpoint API
   └─ dashboard_html.h  ← halaman HTML (disimpan di PROGMEM)
```

Semua angka penting ada di `config.h` agar mudah diubah tanpa mencari di banyak file:

```cpp
// config.h (contoh)
#define PIN_TEMP_WATER      19
#define PIN_TEMP_HEATSINK   18        // pin strapping, lihat PRD 3.2
#define PIN_PELTIER_PWM     25
#define PIN_PUMP            27
#define PUMP_ACTIVE_LEVEL   HIGH      // ganti LOW jika modul pompa active-low

#define PIN_LCD_SDA         23        // bukan pin bawaan, panggil Wire.begin(23, 22)
#define PIN_LCD_SCL         22
#define LCD_I2C_ADDR        0x27      // coba 0x3F jika layar tidak muncul
#define LCD_COLS            16
#define LCD_ROWS            2
#define LCD_REFRESH_MS      500

#define SETPOINT_DEFAULT    28.0f
#define SETPOINT_MIN        20.0f
#define SETPOINT_MAX        30.0f

#define HS_WARNING_ON       60.0f     // tahap 1
#define HS_WARNING_OFF      50.0f
#define HS_CRITICAL_ON      70.0f     // tahap 2
#define HS_CRITICAL_OFF     45.0f
#define WARNING_MAX_SEC     120
#define CRITICAL_MIN_SEC    60

#define PID_KP              25.0f
#define PID_KI              0.05f
#define PID_KD              0.0f
```

---

## 5. Dashboard

### 5.1 Jawaban: pilihan paling mudah dan efisien

**Web server langsung di ESP32 (WiFi lokal).** Alasannya:

| Opsi | Kemudahan | Catatan |
|---|---|---|
| **Web server di ESP32** ✅ | Paling mudah | Tidak butuh internet, akun, atau server lain. Buka `http://aquatec.local` dari HP/laptop di WiFi yang sama |
| MQTT + Node-RED/Grafana | Sedang | Butuh broker dan komputer server. Bagus untuk riwayat data jangka panjang |
| Blynk / Firebase | Mudah, tapi bergantung internet dan layanan pihak ketiga | Batas gratis dan kebijakan bisa berubah |

MQTT bisa ditambahkan nanti sebagai pengembangan (lihat bagian 11).

### 5.2 Fitur dashboard

| # | Fitur | Prioritas |
|---|---|---|
| D1 | Tampil suhu air dan suhu heatsink (diperbarui tiap 2 detik) | Wajib |
| D2 | Input set point (range 20–30 °C, langkah 0,5) + tombol "Simpan" | Wajib |
| D3 | Tampil status Peltier (% daya) dan pompa | Wajib |
| D4 | Tampil level proteksi: NORMAL / WARNING / CRITICAL / FAULT (warna hijau/kuning/merah) | Wajib |
| D5 | Tombol **Sistem ON/OFF** | Wajib |
| D6 | Grafik suhu 1 jam terakhir (ring buffer 360 titik, gambar pakai `<canvas>` tanpa library eksternal) | Sebaiknya ada |
| D7 | Tombol **Reset FAULT** | Sebaiknya ada |
| D8 | Peringatan jika sensor error atau suhu air > 32 °C | Sebaiknya ada |

Halaman web dibuat dengan HTML + JavaScript biasa yang memanggil API (`fetch`) tiap 2 detik. Tidak butuh framework.

### 5.3 API

| Method | Endpoint | Fungsi | Contoh |
|---|---|---|---|
| GET | `/` | Halaman dashboard | |
| GET | `/api/status` | Status lengkap (JSON) | lihat bawah |
| POST | `/api/setpoint` | Ubah set point | `{"value": 27.5}` |
| POST | `/api/power` | Sistem ON/OFF | `{"enabled": true}` |
| POST | `/api/reset-fault` | Reset status FAULT | |
| GET | `/api/history` | Data grafik | `[27.9, 27.8, ...]` |

Contoh respons `/api/status`:

```json
{
  "water_temp": 28.4,
  "heatsink_temp": 41.2,
  "setpoint": 27.5,
  "enabled": true,
  "safety": "NORMAL",
  "peltier_pct": 62,
  "pump": true,
  "sensor_ok": true,
  "uptime_s": 3600
}
```

Server menolak set point di luar 20–30 °C dengan respons HTTP 400.

### 5.4 WiFi
- Mode utama: terhubung ke WiFi rumah/kampus (SSID dan password di `secrets.h`).
- Jika gagal terhubung dalam 15 detik: pindah ke **mode Access Point** bernama `AquaTEC` agar dashboard tetap bisa dibuka.
- Alamat mudah diingat via mDNS: `aquatec.local`.

---

## 6. Kebutuhan Fungsional

### 6.1 Pembacaan sensor
- **FR-01** Sistem membaca suhu air dan suhu heatsink tiap 1 detik, resolusi 12-bit (0,0625 °C).
- **FR-02** Pembacaan memakai mode non-blocking (`setWaitForConversion(false)`): minta konversi → tunggu ±800 ms dengan `vTaskDelay` → baca hasil.
- **FR-03** Data **tidak valid** jika bernilai `-127` (sensor lepas) atau `85.0` (nilai awal sensor setelah reset). Sensor dianggap **error** jika 3 kali berturut-turut tidak valid.

### 6.2 Kontrol PID
- **FR-04** Hitung `error = suhu_air − setpoint`. Output PID 0–100 % dipakai sebagai daya Peltier.
- **FR-05** Gunakan library PID dengan arah **`REVERSE`**. Tips pemula: `DIRECT` itu untuk pemanas (output naik saat suhu di bawah target), sedangkan sistem ini pendingin (output naik saat suhu **di atas** target), jadi pakai `REVERSE`.
- **FR-06** Batas output 0–100 (sekaligus mencegah *integral windup*), sample time 1000 ms.
- **FR-07** Nilai awal PID `[ASUMSI]`: **Kp = 25, Ki = 0,05, Kd = 0**. Ini hanya titik awal dan **harus di-tuning lewat percobaan** (lihat 9.2).
- **FR-08** Peltier baru menyala jika output PID ≥ 15 %, dan baru mati jika output < 5 % (hysteresis agar tidak berkedip). Daya di bawah 15 % tidak efektif.
- **FR-09** Sebaiknya ada jeda minimum 30 detik antar nyala/mati Peltier untuk mengurangi *thermal cycling* yang bisa merusak Peltier dalam jangka panjang.
- **FR-10** Sebaiknya ada *soft start*: kenaikan daya Peltier dibatasi maks. 5 % per detik.
- **FR-11** PWM Peltier: frekuensi ±5 kHz, resolusi 10-bit (0–1023). Frekuensi terlalu rendah membuat Peltier panas-dingin berulang.
- **FR-12** Jika air lebih dingin dari set point, output PID otomatis 0 dan Peltier mati (tidak ada pemanas).

### 6.3 Logika pompa dan fan
- **FR-13** **Pompa**: ON selama sistem aktif dan level proteksi NORMAL. Pompa 5V arusnya kecil dan membantu pencampuran air.
- **FR-14** **Fan**: selalu ON sejak PSU dinyalakan. Fan terhubung langsung ke jalur 12V PSU (tidak lewat ESP32 dan tidak lewat buck) dan **tidak dikontrol software**. Konsekuensinya: (a) fan tetap berputar walaupun ESP32 hang atau restart, (b) ESP32 tidak bisa tahu fan rusak, jadi kerusakan fan hanya terdeteksi lewat kenaikan suhu heatsink (bagian 7), (c) tidak ada pin GPIO untuk fan.

### 6.4 Penyimpanan
- **FR-15** Set point tersimpan di memori flash (`Preferences`) sehingga tetap ada setelah ESP32 dimatikan.
- **FR-16** Saat boot, sistem aktif dengan set point terakhir (default 28 °C jika belum pernah disimpan).

### 6.5 Simulasi untuk pengujian
- **FR-17** Sebaiknya ada perintah Serial untuk **menyuntikkan suhu heatsink palsu** (misal ketik `hs=66`), supaya proteksi bisa diuji **tanpa benar-benar memanaskan** Peltier. Aktifkan hanya dengan flag `#define SIMULATION_MODE` saat debugging.

### 6.6 Tampilan LCD I2C (16×2)
- **FR-18** LCD menampilkan suhu air, set point, suhu heatsink, daya Peltier, dan level proteksi. Diperbarui tiap 500 ms oleh `TaskDisplay`.
- **FR-19** Tata letak layar (16 karakter per baris):

```
Baris 1:  Air:28.4°C SET POINT: 28.0°C
Baris 2:  Status: Normal
```

  `Air` = suhu air (°C), `SP` = set point, `HS` = suhu heatsink (bulat), `P` = daya Peltier, lalu level proteksi disingkat `NORM`, `WARN`, `CRIT`, `FALT`.
- **FR-20** Kondisi khusus: sensor air error → `Air:ERR`; sensor heatsink error → `HS:ERR`; sistem dimatikan dari dashboard → baris 2 menampilkan `SISTEM OFF`.
- **FR-21** Saat boot, LCD menampilkan `AquaTEC` dan alamat IP selama 3 detik, lalu pindah ke tampilan normal.
- **FR-22** Jangan memanggil `lcd.clear()` tiap pembaruan (layar berkedip). Tulis ulang teks di posisi tetap dan isi sisa baris dengan spasi.
- **FR-23** Jika LCD tidak terdeteksi saat boot (alamat salah atau kabel lepas), sistem **tetap berjalan normal**. Kontrol dan proteksi tidak boleh bergantung pada LCD.
- **FR-24** LCD hanya menampilkan data. Set point tetap diubah lewat dashboard.

---

## 7. Proteksi Overheat (Bertahap)

Sensor heatsink dibaca tiap siklus. **Proteksi dijalankan sebelum PID** dan hasilnya selalu menang atas keputusan PID.

### 7.1 Saran perubahan angka (menjawab pertanyaan nomor 8)

Angka awal Anda (65 °C / 75 °C) tetap masuk akal sebagai ide, tapi saya ubah agar lebih aman dan tidak sering mati-nyala:

| | Usulan Anda | Yang dipakai di PRD | Alasan |
|---|---|---|---|
| Tahap 1 | 65 °C | **60 °C** | Sensor ada di heatsink, sedangkan permukaan Peltier bisa lebih panas daripada yang terbaca. Perlu ruang aman |
| Tahap 2 | 75 °C | **70 °C** | Sama. Water block dan selang juga ada di dekat Peltier |
| Histeresis | Belum ada | **Nyala lagi saat turun ke 50 °C / 45 °C** | Selisih 10–15 °C mencegah sistem on-off terus saat suhu naik-turun di sekitar batas |

Semua angka ada di `config.h`. Jika ingin kembali ke 65/75, cukup ubah 4 baris.

### 7.2 Tabel level proteksi

| Level | Kondisi masuk | Peltier | Pompa | Fan |
|---|---|---|---|---|
| **0 NORMAL** | Heatsink < 60 °C | Sesuai PID (0–100 %) | ON | ON (selalu) |
| **1 WARNING** | Heatsink ≥ 60 °C | **Dibatasi maks. 70 %**  | **OFF** | ON (selalu) |
| **2 CRITICAL** | Heatsink ≥ 70 °C, **atau** WARNING berlangsung > 120 detik, **atau** sensor heatsink error | **OFF** | OFF | ON (selalu) |
| **3 FAULT** | CRITICAL terjadi ≥ 3 kali dalam 30 menit | OFF | OFF | ON (selalu) |

Kolom fan selalu ON karena fan langsung ke PSU. Pada level apa pun fan berputar penuh.

### 7.3 Aturan pulih (kembali ke NORMAL)

- **WARNING → NORMAL**: heatsink ≤ 50 °C.
- **CRITICAL → NORMAL**: heatsink ≤ 45 °C **dan** sudah di CRITICAL minimal 60 detik.
- **FAULT → NORMAL**: hanya dengan tombol **Reset FAULT** di dashboard, dan hanya jika heatsink < 45 °C. FAULT berarti ada masalah nyata (fan rusak, heatsink terlalu kecil, pasta termal buruk) yang perlu dicek manual.

### 7.4 Diagram state machine

```
             ≥60°C                     ≥70°C atau >120 s
 NORMAL ───────────▶ WARNING ─────────────────────────────▶ CRITICAL
   ▲                    │                                      │
   │     ≤50°C          │                                      │
   └────────────────────┘                                      │
   ▲                         ≤45°C dan ≥60 s                   │
   └───────────────────────────────────────────────────────────┘
        CRITICAL ≥3 kali / 30 menit  ──▶  FAULT (reset manual)
```

### 7.5 Catatan penting soal tahap 1 (pompa mati, Peltier masih jalan)

Pada alur awal, di tahap 1 pompa dimatikan sementara Peltier tetap menyala. Ini punya **risiko**: tanpa aliran air, sisi dingin terus mendingin dan **air di water block bisa membeku**, sementara sisi panas tetap menerima daya listrik.

Untuk itu, di PRD ini pada tahap 1:
- Peltier **dibatasi 70 %** (bukan 100 %), dan
- Ada **batas waktu 120 detik**. Jika suhu heatsink belum turun, lanjut ke tahap 2 (Peltier mati).

Tujuan Anda tetap tercapai: Peltier tidak mati total, sehingga pemulihannya cepat. Angka 30 % dan 120 detik ada di `config.h` dan bisa diubah. Jika dosen/Anda ingin Peltier tetap 100 % di tahap 1, ubah satu konstanta, tetapi tambahkan pengujian suhu sisi dingin.

### 7.6 Fail-safe lain

| Kejadian | Respons sistem |
|---|---|
| Sensor heatsink error  | Langsung CRITICAL |
| Sensor air error error | Peltier OFF, pompa OFF, dashboard menampilkan peringatan |
| Data sensor > 10 detik tidak diperbarui | Dianggap sensor error |
| ESP32 macet / restart | Watchdog restart, Peltier dan pompa mati (output LOW), fan tetap menyala |
| Fan rusak / kabel fan lepas | Tidak terdeteksi langsung. Terdeteksi lewat kenaikan suhu heatsink sehingga WARNING/CRITICAL aktif |
| Suhu air < 20 °C (batas bawah mutlak) | Peltier OFF (melindungi ikan dan tanaman) |
| Suhu air > 32 °C | Hanya peringatan di dashboard (alarm) |

---

## 8. Kebutuhan Non-Fungsional

- **NFR-01 Respons**: perubahan set point mulai diproses maks. 2 detik.
- **NFR-02 Keandalan**: sistem berjalan 24 jam tanpa restart sendiri dan tanpa data suhu tertahan.
- **NFR-03 Keamanan listrik**: tidak ada kabel 12V terbuka di dekat air. Gunakan sekring. Semua sambungan di dalam kotak tertutup.
- **NFR-04 Kode**: tiap file punya satu tanggung jawab, semua angka konfigurasi di `config.h`, fungsi diberi komentar singkat berbahasa Indonesia.
- **NFR-05 Memori**: halaman dashboard < 20 KB.
- **NFR-06 Versi**: simpan di Git, commit per fitur. `secrets.h` masuk `.gitignore`.

---

## 9. Pengujian

### 9.1 Tabel uji

| ID | Pengujian | Hasil yang diharapkan |
|---|---|---|
| T1 | Baca sensor air dan heatsink, bandingkan dengan termometer | Selisih ≤ 0,5 °C |
| T2 | Cabut sensor air saat berjalan | Peltier OFF, dashboard menampilkan error |
| T3 | Cabut sensor heatsink saat berjalan | Level CRITICAL, Peltier OFF, fan ON |
| T4 | Ubah set point dari dashboard | Nilai berubah dan tetap setelah ESP32 di-restart |
| T5 | Set point di luar 20–30 °C | Ditolak (HTTP 400) |
| T6 | Simulasi heatsink 61 °C (`hs=61`) | WARNING: pompa OFF, Peltier ≤ 30 %, fan ON |
| T7 | Simulasi heatsink 71 °C | CRITICAL: Peltier OFF, pompa OFF, fan ON |
| T8 | Turunkan simulasi ke 49 °C dari WARNING | Kembali NORMAL |
| T9 | Turunkan simulasi ke 44 °C dari CRITICAL setelah ≥ 60 detik | Kembali NORMAL |
| T10 | Simulasikan WARNING bertahan 3 menit | Pindah ke CRITICAL pada detik ke-120 |
| T11 | Picu CRITICAL 3 kali dalam 30 menit | Masuk FAULT, hanya bisa keluar lewat Reset |
| T12 | Lepas kabel fan sementara Peltier jalan (pengawasan penuh!) | Suhu heatsink naik, proteksi aktif bertahap |
| T13 | Kontrol 2 jam dengan air bersuhu awal 31 °C, set point 28 °C | Suhu masuk ±0,5 °C dan stabil |
| T14 | Putus WiFi | Sistem tetap mengontrol suhu, dashboard bisa dibuka lagi setelah WiFi pulih |
| T15 | Uji 24 jam | Tidak ada restart tidak terduga |
| T16 | Nyalakan PSU dengan ESP32 belum aktif / dicabut | Fan tetap berputar, Peltier dan pompa mati |
| T17 | Ubah set point dari dashboard | LCD menampilkan suhu air, set point, suhu heatsink, dan level proteksi sesuai dashboard dalam ≤ 1 detik |
| T18 | Cabut kabel LCD saat sistem berjalan | Kontrol dan proteksi tetap berjalan normal, tidak ada restart |

### 9.2 Cara tuning PID (langkah mudah)

1. Mulai dengan **Kp = 25, Ki = 0, Kd = 0**. Amati suhu.
2. Jika suhu bergoyang di sekitar target: kecilkan Kp. Jika terlalu lambat: besarkan Kp.
3. Jika suhu stabil tapi selalu meleset (misal tertahan di 28,6 °C): naikkan Ki sedikit (0,01 → 0,05 → 0,1).
4. Biarkan Kd = 0 kecuali ada alasan jelas (sistem air lambat dan sensor sedikit berisik, Kd justru memperburuk).
5. Catat grafik tiap percobaan untuk laporan.

---

## 10. Risiko dan Mitigasi

| Risiko | Dampak | Mitigasi |
|---|---|---|
| Heatsink terlalu kecil / fan lemah | Overheat berulang | Proteksi bertahap + FAULT + pasta termal baik |
| Fan rusak / lepas (tidak dipantau ESP32) | Heatsink cepat panas | Proteksi suhu heatsink tetap bekerja. Pastikan fan berputar sebelum setiap uji |
| Pompa mati sementara Peltier jalan | Air beku di water block | Batas daya 30 % dan batas waktu 120 detik (7.5) |
| Spesifikasi modul JZ-MOS tidak sesuai (trigger > 3,3V atau tidak menerima PWM) | Peltier tidak bisa diatur | Cek label/datasheet, uji PWM tanpa beban penuh dulu (3.3) |
| Kabel 12V dekat air | Sengatan / korsleting | Kotak tertutup, sekring, pisahkan jalur air dan listrik |
| Sensor DS18B20 salah baca (85 °C / -127 °C) | Keputusan salah | Validasi nilai (FR-03) |
| Data bentrok antar task | Nilai suhu aneh | Mutex (4.2) |
| WiFi hilang | Dashboard tidak bisa diakses | Kontrol tetap jalan lokal, fallback AP |
| Peltier sering nyala-mati | Umur Peltier turun | Hysteresis (FR-08) dan jeda minimum (FR-09) |
| Alamat I2C LCD salah (0x27 / 0x3F) atau SDA/SCL tertukar | LCD kosong | I2C scanner (3.3). Sistem tetap jalan walau LCD mati (FR-23) |
| Pull-up modul LCD tersambung ke 5V | Pin I2C ESP32 menerima 5V | Lepas jumper pull-up atau pakai level shifter (3.3) |

---

## 11. Rencana Pengerjaan (urutan yang disarankan) `[ASUMSI]`

| Tahap | Pekerjaan | Selesai jika |
|---|---|---|
| 1 | Rakit hardware dasar, tes DS18B20 (air dan heatsink) lewat Serial, jalankan I2C scanner, tampilkan teks di LCD | Dua suhu tampil di Serial dan LCD menyala dengan alamat yang benar |
| 2 | Pasang fan langsung ke PSU 12V, lalu uji Peltier PWM dan pompa (lewat modul JZ-MOS) satu per satu dari kode (tanpa air panas) | Fan berputar otomatis saat PSU hidup, Peltier dan pompa bisa dihidupkan dari kode |
| 3 | Buat struktur FreeRTOS: `TaskSensor`, `TaskControl`, mutex | Data sensor mengalir antar task |
| 4 | Tulis **proteksi overheat dulu** sebelum PID, uji dengan simulasi (T6–T11) | Semua uji proteksi lolos |
| 5 | Tambahkan PID, tuning (9.2) | Suhu stabil (T13) |
| 6 | Buat web server, API, dashboard, dan `TaskDisplay` untuk LCD | Fitur D1–D5 berfungsi dan LCD sinkron dengan dashboard (T17) |
| 7 | Tambahkan grafik, simpan set point, fail-safe lengkap | T1–T18 lolos |
| 8 | Uji 24 jam, dokumentasi, laporan, presentasi | Definition of Done terpenuhi |

Alasan proteksi dibuat sebelum PID: kontrol Peltier tanpa proteksi berisiko merusak komponen saat uji coba.

### Pengembangan lanjutan (opsional)
- MQTT + Grafana untuk riwayat data jangka panjang.
- Notifikasi (Telegram) saat WARNING/FAULT.
- OTA update (upload kode lewat WiFi).
- Sensor arus untuk mengukur daya Peltier.
- Fan PWM atau pompa PWM (fan perlu modul tambahan, tidak lagi langsung ke PSU).

---

## 12. Definition of Done

- [ ] Suhu air stabil ±0,5 °C dari set point setelah 2 jam uji.
- [ ] Semua uji T1–T18 lolos dan hasilnya didokumentasikan.
- [ ] Proteksi overheat terbukti bekerja pada semua level (NORMAL → WARNING → CRITICAL → FAULT).
- [ ] Dashboard fitur D1–D5 (wajib) berfungsi dari HP dan laptop.
- [ ] LCD menampilkan suhu air, set point, suhu heatsink, dan level proteksi, dan sistem tetap berjalan jika LCD dicabut.
- [ ] Kode terstruktur per modul, ada komentar, `config.h` lengkap.
- [ ] Repositori Git rapi dan `secrets.h` tidak ikut ter-upload.
- [ ] Skema rangkaian dan diagram alur masuk laporan.

---

## 13. Glosarium untuk Pemula

| Istilah | Arti sederhana |
|---|---|
| **Set point** | Suhu target yang Anda inginkan |
| **PID** | Rumus yang menentukan "seberapa kuat" mendinginkan berdasarkan seberapa jauh suhu dari target (P), sudah berapa lama meleset (I), dan seberapa cepat suhu berubah (D) |
| **PWM** | Cara mengatur daya dengan menyala-mati sangat cepat. Makin lama menyala, makin besar daya |
| **Peltier / TEC** | Chip yang satu sisinya jadi dingin dan sisi lainnya panas saat dialiri listrik |
| **Water block** | Blok logam berongga tempat air mengalir, menempel di sisi dingin Peltier |
| **Heatsink** | Sirip logam pembuang panas |
| **FreeRTOS / task** | Sistem yang memungkinkan beberapa "pekerjaan" berjalan bergantian dengan sangat cepat sehingga terlihat bersamaan |
| **Mutex** | "Kunci" agar dua task tidak mengubah data yang sama pada saat bersamaan |
| **Histeresis** | Selisih batas nyala dan batas mati, supaya komponen tidak nyala-mati terus di sekitar satu angka |
| **Watchdog** | Pengatur waktu otomatis yang me-restart ESP32 jika program macet |
| **Fail-safe** | Kondisi aman yang diambil sistem ketika ada yang rusak |
| **mDNS** | Fitur yang membuat alamat seperti `aquatec.local` tanpa perlu mengetik angka IP |
| **I2C** | Cara menyambung perangkat dengan dua kabel data (SDA dan SCL) plus daya. Satu jalur bisa dipakai beberapa perangkat dengan alamat berbeda |

---

## 14. Riwayat Revisi

| Versi | Perubahan |
|---|---|
| 1.0 | PRD awal |
| 1.1 | Fan 12V langsung ke PSU dan selalu ON (tanpa GPIO); pompa memakai modul MOSFET #2 (GPIO 27); aturan fan dan tabel proteksi disesuaikan; ditambah uji T16 dan risiko fan tidak terpantau |
| 1.2 | Driver diganti ke modul switch JZ-MOS (2 unit); LCD I2C 16×2 ditambahkan (GPIO 21/22, `TaskDisplay`, FR-18 sampai FR-24); uji T17–T18; risiko baru modul JZ-MOS dan I2C |
| 1.3 | Pin sensor heatsink dipindah dari GPIO 16 ke GPIO 2 (pin strapping, catatan di 3.2); pin SDA LCD dipindah dari GPIO 21 ke GPIO 23 agar bersebelahan dengan SCL (GPIO 22) |
