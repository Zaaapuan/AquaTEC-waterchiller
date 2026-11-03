# AquaTEC: ESP32 Aquascape Water Temperature Control System

AquaTEC adalah sistem pengatur suhu air aquascape berbasis mikrokontroler **ESP32** menggunakan elemen pendingin **Thermoelectric Cooler (Peltier TEC1-12706)**, algoritma **PID Controller**, dan **Proteksi Overheat Bertahap Multi-Tingkat**. Sistem ini dilengkapi antarmuka ganda: layar **LCD Karakter 16×2 I2C** langsung pada alat dan **Web Dashboard Modern Responsif** (Wi-Fi lokal) bergaya estetika Generative AI (Dark Theme + AI Teal `#00A67E`).

---

## 1. Fitur Utama

- **FreeRTOS Dual-Core Architecture**:
  - `TaskControl` (Core 1, Prioritas 4): Mengatur proteksi overheat, PID, dan output aktuator fisik. Dilindungi Watchdog Timer (WDT) 10 detik.
  - `TaskSensor` (Core 1, Prioritas 3): Membaca 2 sensor DS18B20 dengan pola non-blocking (konversi 12-bit ~750ms tanpa memblokir CPU) dan memfilter pembacaan invalid (`-127°C` dan `85°C`).
  - `TaskDisplay` (Core 1, Prioritas 1): Pembaruan tampilan LCD I2C 16×2 tanpa kedip (*anti-flicker*, tanpa `lcd.clear()`).
  - `TaskWeb` (Core 0, Prioritas 1): Server HTTP, REST API JSON, fallback otomatis ke SoftAP jika Wi-Fi rumah putus, dan penyimpanan NVS Flash (`Preferences`).
  - `TaskLogger` (Core 1, Prioritas 1): Telemetri serial dan penerimaan perintah simulasi suhu.
- **Proteksi Overheat Bertahap (Safety-First)**:
  - **Level 0 (NORMAL)**: Heatsink < 60°C &rarr; Peltier beroperasi sesuai output PID (0–100%), Pompa ON, Fan ON.
  - **Level 1 (WARNING)**: Heatsink &ge; 60°C &rarr; Daya Peltier dibatasi maks. 70%, Pompa OFF (mencegah beban sirkulasi), Fan ON. Otomatis pulih ke NORMAL saat suhu &le; 50°C.
  - **Level 2 (CRITICAL)**: Heatsink &ge; 70°C, atau WARNING > 120 detik, atau sensor heatsink rusak &rarr; Peltier OFF (0%), Pompa OFF, Fan ON. Pulih ke NORMAL hanya jika heatsink &le; 45°C DAN telah bertahan di CRITICAL minimal 60 detik.
  - **Level 3 (FAULT)**: Terjadi CRITICAL &ge; 3 kali dalam kurun waktu 30 menit &rarr; Sistem terkunci (latching) OFF. Hanya bisa dipulihkan lewat tombol "Reset FAULT" di dashboard dan suhu heatsink < 45°C.
- **Pelindung Elemen Peltier (Anti Thermal Stress)**:
  - Histeresis daya: Peltier baru ON jika kebutuhan PID &ge; 15%, dan OFF jika < 5%.
  - Jeda minimum *anti-thermal cycling*: jeda 30 detik antar transisi ON/OFF.
  - *Soft-start*: laju perubahan daya dibatasi maksimum 5% per detik.
  - *Freeze protection*: Peltier otomatis mati jika suhu air < 20°C.
- **Web Dashboard Mandiri (Single Page Application)**:
  - Disimpan langsung di dalam memori Flash ESP32 (`PROGMEM`, ukuran < 15 KB).
  - Grafik riwayat suhu 1 jam terakhir menggunakan HTML5 `<canvas>` murni (tanpa ketergantungan internet/CDN eksternal).
  - Kontrol set point (20.0 - 30.0 °C), tombol ON/OFF sistem, dan tombol Reset FAULT.

---

## 2. Diagram Skematik & Pemetaan Pin

### 2.1 Tabel Wiring Pin ESP32

| Fungsi | Pin ESP32 | Rangkaian Eksternal |
|---|---|---|
| **Sensor Suhu Air (DS18B20 #1)** | **GPIO 19** | Data line, resistor pull-up 4.7 kΩ ke 3.3V |
| **Sensor Suhu Heatsink (DS18B20 #2)** | **GPIO 18** | Data line, resistor pull-up 4.7 kΩ ke 3.3V |
| **Peltier PWM Output** | **GPIO 25* * | Input trigger modul MOSFET JZ-MOS #1 (PWM 5 kHz, 10-bit) |
| **Pompa Air (ON/OFF)** | **GPIO 27** | Input trigger modul MOSFET JZ-MOS #2 (Active HIGH) |
| **LCD I2C SDA** | **GPIO 23** | Pin data I2C (`Wire.begin(23, 22)`) |
| **LCD I2C SCL** | **GPIO 22** | Pin clock I2C |
| **Fan Heatsink 12V** | *Tanpa GPIO* | **Langsung ke PSU 12V (Selalu ON, hardware-level safety)** |

### 2.2 Jalur Distribusi Daya

```
PSU 12V 20A ──┬──▶ Sekring 8A ──▶ JZ-MOS #1 ──▶ Peltier TEC1-12706 (PWM dari GPIO 25)
             ├──▶ Fan 12V (Langsung ke 12V PSU, selalu menyala penuh)
             └──▶ Buck Converter 12V ──▶ 5V ──┬──▶ ESP32 (Pin VIN)
                                              ├──▶ LCD 16x2 I2C (Pin VCC)
                                              └──▶ JZ-MOS #2 ──▶ Pompa Air 5V (GPIO 27)
* Catatan Kritis: Seluruh jalur GROUND (GND) dari PSU, Buck, ESP32, LCD, dan MOSFET wajib disatukan (Common Ground)!
```

---

## 3. Struktur Proyek

```
Project Embeded/
├── platformio.ini              # Konfigurasi PlatformIO & library dependencies
├── .gitignore                  # Melindungi file secrets.h dan direktori build
├── README.md                   # Dokumentasi lengkap sistem
├── include/
│   ├── config.h                # Seluruh konstanta konfigurasi, pinout, batas suhu, PID
│   └── secrets.h               # Kredensial Wi-Fi rumah (jangan di-commit ke publik)
└── src/
    ├── main.cpp                # Setup, inisialisasi task FreeRTOS, Task WDT, serial command
    ├── shared_data.h / .cpp    # SystemState struct, Mutex, thread-safe accessors
    ├── actuators.h / .cpp      # Abstraksi driver MOSFET JZ-MOS (Peltier PWM 5kHz & Pompa)
    ├── sensors.h / .cpp        # Driver DS18B20 non-blocking + filter validasi FR-03
    ├── safety.h / .cpp         # State machine proteksi overheat 4 tingkat
    ├── control.h / .cpp        # PID Controller (REVERSE), histeresis, soft-start
    ├── display.h / .cpp        # Pengendali LCD 16x2 non-flicker, status/error badge
    ├── web_server.h / .cpp     # REST API endpoint, Wi-Fi Station & SoftAP fallback, NVS Flash
    └── dashboard_html.h        # Web dashboard SPA dalam PROGMEM (HTML5 Canvas + Inter/Mono font)
```

---

## 4. Panduan Kompilasi & Upload

### Prasyarat
1. Pasang [Visual Studio Code](https://code.visualstudio.com/).
2. Pasang ekstensi **PlatformIO IDE** dari marketplace VS Code.

### Langkah-langkah
1. Buka folder `Project Embeded` di VS Code.
2. Buka file [include/secrets.h](file:///c:/Users/Noval/Downloads/Project%20Embeded/include/secrets.h) dan sesuaikan SSID serta Password Wi-Fi Anda:
   ```cpp
   #define WIFI_SSID_DEFAULT     "Nama_WiFi_Anda"
   #define WIFI_PASSWORD_DEFAULT "Password_WiFi_Anda"
   ```
3. Hubungkan ESP32 DevKit ke komputer lewat kabel USB data.
4. Klik tombol **PlatformIO: Build** (tanda centang di baris bawah) untuk kompilasi.
5. Klik tombol **PlatformIO: Upload** (tanda panah kanan) untuk mengunggah program ke ESP32.
6. Buka **Serial Monitor** pada baud rate `115200`.

---

## 5. Cara Mengakses Web Dashboard

1. **Mode Station (Tersambung ke Wi-Fi Rumah)**:
   - LCD akan menampilkan alamat IP lokal saat boot (misal `192.168.1.50`).
   - Buka browser di ponsel atau laptop yang terhubung ke Wi-Fi yang sama, lalu akses:
     - `http://aquatec.local` (lewat mDNS), atau
     - `http://192.168.1.50` (ganti dengan IP yang tampil di LCD).
2. **Mode SoftAP (Fallback jika Wi-Fi Rumah tidak ada / gagal terhubung dalam 15 detik)**:
   - ESP32 akan memancarkan hotspot Wi-Fi sendiri bernama: **`AquaTEC`** (tanpa password).
   - Sambungkan ponsel/laptop ke Wi-Fi `AquaTEC`.
   - Buka browser dan buka alamat: **`http://192.168.4.1`**.

---

## 6. Prosedur Uji Proteksi Overheat (Simulasi Serial Tanpa Pemanasan Riil)

Untuk menguji keandalan sistem tanpa risiko merusak Peltier atau membakar heatsink, gunakan fitur **Mode Simulasi Serial** (`SIMULATION_MODE`):

Buka Serial Monitor (115200 baud) dan ketik perintah berikut:

| Uji | Perintah Serial | Respon Sistem yang Diharapkan |
|---|---|---|
| **Uji WARNING (T6)** | `hs=61` | Status berubah ke **WARNING**, Pompa OFF, daya Peltier dibatasi &le; 70%, Fan tetap ON. LCD baris 2 menampilkan `WARN`. |
| **Uji CRITICAL (T7)** | `hs=71` | Status berubah ke **CRITICAL**, Peltier OFF (0%), Pompa OFF. LCD menampilkan `CRIT`. |
| **Uji Pulih WARNING (T8)** | `hs=49` | Dari WARNING, suhu turun &le; 50°C &rarr; Sistem kembali **NORMAL**, Pompa ON kembali. |
| **Uji Pulih CRITICAL (T9)** | `hs=44` | Dari CRITICAL, suhu turun &le; 45°C setelah bertahan &ge; 60 detik &rarr; Sistem kembali **NORMAL**. |
| **Uji Timeout WARNING (T10)**| `hs=61` (tahan > 120 detik) | Status WARNING otomatis naik menjadi **CRITICAL** pada detik ke-120. |
| **Uji Latching FAULT (T11)**| Picu CRITICAL 3 kali dalam 30 menit | Sistem masuk ke status **FAULT**. Peltier & Pompa terkunci mati. Tombol "Reset FAULT" muncul di dashboard. |
| **Reset FAULT** | Klik tombol Reset di Web Dashboard | Berhasil keluar dari FAULT jika heatsink < 45°C. |
| **Kembali ke Sensor Fisik** | `hs=reset` | Simulasi dimatikan, sistem kembali membaca sensor fisik DS18B20 heatsink. |

---

## 7. Format Tampilan Layar LCD 16×2

Sesuai spesifikasi PRD FR-19 (tepat 16 karakter per baris):
```
Baris 1: Air:28.4 SP:28.0
Baris 2: HS41 P: 62% NORM
```
- `Air`: Suhu air terukur (°C). Menampilkan `Air:ERR ` jika sensor terputus.
- `SP`: Target suhu (*set point*).
- `HS`: Suhu heatsink pembulatan (°C) atau `HS:ER` jika sensor heatsink putus.
- `P`: Persentase daya aktual PWM Peltier.
- `NORM` / `WARN` / `CRIT` / `FALT`: Singkatan status proteksi aktif.
- Jika sistem dimatikan dari dashboard, baris 2 akan menampilkan: `SISTEM OFF      `.

