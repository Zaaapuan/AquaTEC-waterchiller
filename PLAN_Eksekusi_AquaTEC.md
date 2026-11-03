# Planning Eksekusi: AquaTEC (ESP32 Water Temperature Control)

Dokumen ini adalah pasangan dari `PRD_AquaTEC_Water_Temperature_Control.md` (v1.2). PRD menjelaskan **apa** yang dibangun, dokumen ini menjelaskan **urutan mengerjakannya** supaya bisa dicicil per sesi.

---

## Cara pakai

1. Kerjakan **satu seksi per sesi**. Setiap seksi dibuat agar selesai dalam 1 sampai 3 jam.
2. Jangan lanjut ke seksi berikutnya sebelum bagian **Selesai jika** terpenuhi.
3. Di akhir setiap seksi, lakukan `git commit` (pesan contoh ada di tiap seksi). Kalau ada yang rusak di seksi berikutnya, Anda bisa kembali ke titik aman.
4. Centang kotak `[ ]` di bawah saat selesai. Estimasi waktu adalah perkiraan saya `[ASUMSI]` untuk pemula, jadi sesuaikan dengan kecepatan Anda.
5. Rujukan seperti **FR-08** atau **T6** menunjuk ke nomor di PRD.

### Urutan dan gambaran besar

| # | Seksi | Estimasi | Hasil akhir | Selesai |
|---|---|---|---|---|
### SECTION 1 (MANDIRI)
| 0 | Persiapan proyek | 1 jam | Proyek kosong bisa di-upload ke ESP32 | [x] |
### SECTION 2
| 1 | Sensor DS18B20 | 1–2 jam | Dua suhu tampil di Serial Monitor | [x] |
| 2 | LCD I2C | 1–2 jam | Suhu tampil di LCD | [x] |
| 3 | Aktuator (pompa dan Peltier) | 2–3 jam | Pompa dan Peltier bisa dikontrol dari kode | [x] |
| 4 | Kerangka FreeRTOS | 2–3 jam | Task jalan bersamaan, data dibagi lewat mutex | [x] |
| 5 | Proteksi overheat | 3 jam | State machine proteksi lolos uji simulasi | [x] |
| 6 | Kontrol PID | 3 jam + waktu tuning | Suhu air stabil di set point | [x] |
### SECTION 3
| 7 | WiFi dan dashboard | 3–4 jam | Set point bisa diubah dari HP | [x] |
| 8 | Fitur lengkap dan fail-safe | 2–3 jam | Grafik, reset FAULT, penanganan error | [x] |
### SECTION 4 (MANDIRI)
| 9 | Integrasi dan uji akhir | 2 hari (sebagian menunggu) | Semua uji T1–T18 lolos | [ ] |
| 10 | Dokumentasi dan presentasi | 1–2 hari | Laporan, README, dan bahan demo siap | [x] |


### Aturan keselamatan yang berlaku sepanjang proyek

- **Fan harus berputar sebelum Peltier dinyalakan.** Peltier tanpa pembuangan panas rusak dalam hitungan detik.
- **Peltier tidak boleh dinyalakan tanpa heatsink terpasang** (dan pasta termal di kedua sisi).
- **Pompa tidak boleh jalan kering** (tanpa air) lebih dari beberapa detik.
- **Uji awal Peltier singkat**: maksimal 10 detik, daya kecil, sambil memegang heatsink untuk merasakan panasnya.
- Jangan ada kabel 12V terbuka di dekat air. Pasang sekring 8A di jalur 12V sebelum menyalakan apa pun.
- Saat uji proteksi dengan **suhu simulasi**, tidak perlu memanaskan Peltier sungguhan.

---

## Seksi 0: Persiapan proyek

**Tujuan:** lingkungan kerja siap, kerangka folder sesuai PRD (bagian 4.3).
**Butuh:** laptop, ESP32 + kabel USB. Belum perlu komponen lain.

### Langkah
- [x] Pasang VS Code dan ekstensi **PlatformIO** (atau pakai Arduino IDE jika lebih nyaman).
- [x] Buat proyek baru: board `esp32dev`, framework `arduino`.
- [x] **Kunci versi platform** di `platformio.ini` (misalnya `platform = espressif32@6.x` atau 3.x). Catat versinya. Cara PWM berbeda antara core 2.x dan 3.x (lihat seksi 3).
- [x] Tambahkan library di `platformio.ini`:
  ```ini
  lib_deps =
      paulstoffregen/OneWire
      milesburton/DallasTemperature
      br3ttb/PID
      bblanchon/ArduinoJson
      marcoschwartz/LiquidCrystal_I2C
  monitor_speed = 115200
  ```
- [x] Buat folder dan file kosong sesuai PRD 4.3: `config.h`, `secrets.h`, `shared_data`, `sensors`, `safety`, `control`, `actuators`, `display`, `web_server`.
- [x] Isi `config.h` dengan semua angka dari PRD 4.3 (pin, batas suhu, PID).
- [x] Buat `.gitignore` dan masukkan `secrets.h` ke dalamnya. Lalu `git init`.
- [x] Upload program `Serial.println("Halo AquaTEC");` untuk membuktikan alur upload dan monitor jalan.

### Selesai jika
Tulisan "Halo AquaTEC" muncul di Serial Monitor dan repositori Git sudah ada commit pertama.

**Commit:** `chore: kerangka proyek dan config.h`

---

## Seksi 1: Sensor DS18B20

**Tujuan:** membaca suhu air dan suhu heatsink dengan benar (PRD FR-01 sampai FR-03).
**Butuh:** ESP32, 2 × DS18B20, 2 × resistor 4,7 kΩ, breadboard.

### Wiring
| DS18B20 | Ke ESP32 |
|---|---|
| Merah (VCC) | 3V3 |
| Hitam (GND) | GND |
| Kuning (DATA) sensor air | GPIO 4, plus 4,7 kΩ ke 3V3 |
| Kuning (DATA) sensor heatsink | GPIO 2, plus 4,7 kΩ ke 3V3 (pin strapping, lihat catatan di bawah) |

Setiap sensor punya resistor pull-up sendiri.

**Catatan GPIO 2:** jika upload gagal dengan *"Wrong boot mode"*, cabut kabel DATA sensor heatsink dari GPIO 2 selama upload, lalu pasang lagi. Alternatif: pindahkan ke GPIO 17, 18, atau 19 dengan mengubah `PIN_TEMP_HEATSINK` di `config.h`.

### Langkah
- [x] Buat program tes kecil (di `main.cpp` sementara) untuk membaca sensor air saja, cetak ke Serial tiap detik.
- [x] Tambahkan sensor heatsink di pin terpisah.
- [x] Buat fungsi `readTemp(sensor)` di `sensors.cpp` yang mengembalikan nilai dan status valid/tidak.
- [x] Terapkan validasi FR-03: nilai `-127` dan `85.0` dianggap tidak valid, error jika 3 kali berturut-turut.
- [x] Pakai mode non-blocking `setWaitForConversion(false)` (FR-02): minta konversi, tunggu sekitar 800 ms, lalu baca.
- [ ] Bandingkan dengan termometer lain, celupkan sensor air ke air hangat dan dingin.

### Selesai jika
- Dua suhu stabil di Serial dan selisih dengan termometer ≤ 0,5 °C (**T1**).
- Mencabut sensor memunculkan status error, bukan angka aneh.

**Jebakan umum:** nilai selalu `-127` biasanya karena pull-up lupa dipasang atau kabel DATA salah. Nilai `85` terbaca jika membaca sebelum konversi selesai.

**Commit:** `feat: pembacaan DS18B20 dengan validasi`

---

## Seksi 2: LCD I2C

**Tujuan:** LCD menampilkan suhu dengan tata letak PRD FR-19.
**Butuh:** LCD 16×2 + modul I2C, daya 5V.

### Wiring
| LCD I2C | Ke |
|---|---|
| VCC | 5V (dari buck atau pin VIN/5V saat uji di meja) |
| GND | GND (satukan dengan ESP32) |
| SDA | GPIO 23 |
| SCL | GPIO 22 |

**Catatan:** SDA di GPIO 23 bukan pin I2C bawaan (bawaan GPIO 21), jadi di kode wajib memanggil `Wire.begin(PIN_LCD_SDA, PIN_LCD_SCL)`. Program I2C scanner juga harus memakai `Wire.begin(23, 22)`. Jika tidak, LCD tidak akan ditemukan.

### Langkah
- [x] Jalankan program **I2C scanner** (contoh banyak di internet) untuk mencari alamat. Biasanya 0x27 atau 0x3F. Masukkan hasilnya ke `LCD_I2C_ADDR` di `config.h`.
- [ ] Jika layar kosong atau hanya kotak-kotak: putar potensiometer kontras di belakang modul.
- [x] Tampilkan "Halo AquaTEC" untuk memastikan layar hidup.
- [ ] Cek jumper pull-up di modul LCD. Jika SDA/SCL ditarik ke 5V, lepas jumper dan pakai pull-up 4,7 kΩ ke 3,3V (PRD 3.3).
- [x] Tulis fungsi `showMain(waterTemp, setpoint, hsTemp, peltierPct, levelText)` di `display.cpp`:
  ```
  Air:28.4 SP:28.0
  HS41 P: 62% NORM
  ```
- [x] **Jangan** pakai `lcd.clear()` di setiap pembaruan (FR-22). Tulis di posisi tetap dan isi sisa baris dengan spasi.
- [x] Hubungkan dengan data sensor dari seksi 1 (set point sementara 28.0, daya 0 %, level `NORM`).
- [x] Tambahkan kasus khusus FR-20: `Air:ERR`, `HS:ERR`, `SISTEM OFF`.
- [x] Tambahkan tampilan boot FR-21 (nama dan IP, IP-nya diisi nanti di seksi 7).

### Selesai jika
LCD menampilkan suhu air asli dan suhu heatsink asli, tidak berkedip, dan menampilkan `Air:ERR` saat sensor air dicabut.

**Commit:** `feat: tampilan LCD I2C`

---

## Seksi 3: Aktuator (pompa dan Peltier)

**Tujuan:** bisa menyalakan pompa (on/off) dan Peltier (PWM) dari kode (PRD FR-11, FR-13).
**Butuh:** PSU 12V 10A + sekring 8A, 2 × modul JZ-MOS, buck 12V→5V, pompa 5V, fan 12V, Peltier + heatsink + water block, wadah air.

### Langkah A: kelistrikan dasar (tanpa Peltier dulu)
- [ ] Baca label modul JZ-MOS: cek tegangan trigger minimum, tegangan beban, dan arti terminal (PRD 3.3). Foto labelnya untuk laporan.
- [ ] Sambungkan **fan 12V langsung ke PSU** (bukan lewat buck). Pastikan fan berputar begitu PSU hidup.
- [ ] Setel buck ke 5,0V **sebelum** disambung ke ESP32 dan LCD. Ukur dengan multimeter.
- [ ] Satukan GND: PSU, buck, ESP32, kedua modul JZ-MOS.

### Langkah B: pompa
- [ ] Sambungkan pompa 5V ke JZ-MOS #2 (daya dari buck), sinyal ke GPIO 27.
- [x] Tulis `pumpSet(bool on)` di `actuators.cpp` memakai `PUMP_ACTIVE_LEVEL`. Tes nyala/mati tiap 3 detik, dengan pompa tercelup air.

### Langkah C: Peltier (hati-hati, ikuti aturan keselamatan)
- [ ] **Uji PWM dulu dengan beban pengganti**, misalnya lampu 12V kecil atau LED strip, supaya modul JZ-MOS terbukti menerima PWM dari ESP32. Ukur redupnya.
- [ ] Pasang Peltier: sisi dingin ke water block, sisi panas ke heatsink, pasta termal di kedua sisi, fan sudah berputar, pompa sudah mengalirkan air.
- [x] Tulis `peltierSetPercent(float pct)` dengan PWM ±5 kHz, 10-bit:
  - Core Arduino-ESP32 2.x: `ledcSetup(ch, 5000, 10); ledcAttachPin(PIN_PELTIER_PWM, ch); ledcWrite(ch, duty);`
  - Core 3.x: `ledcAttach(PIN_PELTIER_PWM, 5000, 10); ledcWrite(PIN_PELTIER_PWM, duty);`
- [ ] Uji **10 detik** pada 20 %, rasakan sisi heatsink hangat dan air di water block dingin. Naikkan bertahap (40 %, 60 %) dengan jeda dan awasi arus/suhu.
- [x] Pastikan saat ESP32 di-reset, Peltier dan pompa **mati** (pin default LOW).

### Selesai jika
Pompa on/off dan Peltier PWM berfungsi dari kode, fan berputar otomatis saat PSU hidup (**T16**), dan tidak ada komponen yang terlalu panas selama uji.

**Commit:** `feat: kontrol pompa dan Peltier PWM`

---

## Seksi 4: Kerangka FreeRTOS

**Tujuan:** struktur multi-task dan data bersama yang aman (PRD 4.2).

### Langkah
- [x] Di `shared_data.h/.cpp`: definisikan `struct SystemState` dan `g_mutex` (contoh ada di PRD 4.2). Buat `setup()` membuat mutex dengan `xSemaphoreCreateMutex()`.
- [x] Buat helper kecil agar tidak lupa melepas mutex, misalnya `stateRead()` dan `stateWrite()` yang menyalin struct dengan aman.
- [x] Pindahkan pembacaan sensor dari seksi 1 menjadi `TaskSensor` (prioritas 3, core 1, tiap 1000 ms). Task menulis hasil ke struct.
- [x] Buat `TaskDisplay` (prioritas 1, core 1, tiap 500 ms): salin data dari struct, lepaskan mutex, **baru** tulis ke LCD.
- [x] Buat `TaskControl` (prioritas 4, core 1, tiap 1000 ms) yang untuk sementara hanya membaca struct dan mencetak ke Serial. Pakai `vTaskDelayUntil()`.
- [x] Kosongkan `loop()`, isi dengan `vTaskDelete(NULL)`.
- [x] Aktifkan **Task Watchdog** (10 detik) untuk `TaskControl`.
- [x] Aturan: hanya `TaskControl` yang boleh menulis ke pin output Peltier dan pompa, hanya `TaskDisplay` yang menyentuh LCD.
- [x] Opsional: `TaskLogger` yang mencetak status tiap 5 detik.

### Selesai jika
Tiga task jalan bersamaan, LCD terus diperbarui, tidak ada nilai suhu aneh, tidak ada restart sendiri selama 30 menit.

**Jebakan umum:** memanggil `delay()` panjang di dalam task atau menahan mutex sambil menulis ke LCD akan membuat task lain tersendat.

**Commit:** `feat: kerangka FreeRTOS dan shared state`

---

## Seksi 5: Proteksi overheat

**Tujuan:** state machine proteksi bekerja dan lolos uji **sebelum** PID dibuat (PRD bagian 7).
**Alasan dahulukan:** kontrol Peltier tanpa proteksi berisiko merusak komponen saat uji.

### Langkah
- [x] Tambahkan **SIMULATION_MODE** (FR-17): perintah Serial `hs=66` menimpa suhu heatsink dengan nilai palsu. Dengan ini proteksi bisa diuji tanpa memanaskan apa pun.
- [x] Tulis `safety.cpp` sebagai state machine dengan 4 level: NORMAL, WARNING, CRITICAL, FAULT.
- [x] Tuliskan aturan masuk:
  - NORMAL → WARNING: heatsink ≥ 60 °C
  - WARNING → CRITICAL: heatsink ≥ 70 °C, **atau** WARNING lebih dari 120 detik, **atau** sensor heatsink error
  - CRITICAL ≥ 3 kali dalam 30 menit → FAULT
- [x] Tuliskan aturan pulih (PRD 7.3):
  - WARNING → NORMAL: heatsink ≤ 50 °C
  - CRITICAL → NORMAL: heatsink ≤ 45 °C **dan** sudah di CRITICAL minimal 60 detik
  - FAULT → NORMAL: hanya lewat Reset (fungsi `safetyResetFault()` dulu, tombol dashboard menyusul di seksi 8)
- [x] Tentukan keluaran proteksi per level (tabel PRD 7.2): batas daya Peltier (NORMAL 100 %, WARNING 30 %, CRITICAL/FAULT 0 %) dan status pompa (ON hanya di NORMAL).
- [x] Hubungkan ke `TaskControl`: proteksi **dijalankan sebelum** keputusan lain dan hasilnya selalu menang. Untuk sementara Peltier diberi daya tetap 50 % sebagai pengganti PID, hanya untuk menguji proteksi.
- [x] Tambahkan penanganan: sensor air error → Peltier dan pompa OFF; suhu air < 20 °C → Peltier OFF; data sensor lebih dari 5 detik tidak diperbarui → dianggap error (PRD 7.6).
- [x] Tampilkan level proteksi di LCD (`NORM`, `WARN`, `CRIT`, `FALT`).
- [x] Simpan semua batas di `config.h`. Jangan angka ajaib di dalam fungsi.

### Uji (semua lewat simulasi)
| Uji | Cara | Hasil |
|---|---|---|
| T6 | `hs=61` | WARNING: pompa OFF, Peltier ≤ 30 % |
| T7 | `hs=71` | CRITICAL: Peltier OFF, pompa OFF |
| T8 | dari WARNING `hs=49` | kembali NORMAL |
| T9 | dari CRITICAL `hs=44` setelah ≥ 60 detik | kembali NORMAL |
| T10 | `hs=61` dan biarkan 3 menit | pindah CRITICAL di detik ke-120 |
| T11 | picu CRITICAL 3 kali dalam 30 menit | FAULT, hanya reset yang bisa keluar |
| T3 | cabut sensor heatsink | CRITICAL |
| T2 | cabut sensor air | Peltier OFF |

**Tip:** untuk uji T10 dan T11 tidak perlu menunggu 30 menit sungguhan. Buat konstanta waktu di `config.h` yang bisa diperkecil sementara (misalnya `FAULT_WINDOW_SEC`) lalu kembalikan setelah uji.

### Selesai jika
Semua uji di tabel lolos dan hasilnya dicatat (foto Serial/LCD atau tabel kecil). Tulis juga hasilnya untuk laporan.

**Commit:** `feat: proteksi overheat bertahap dengan simulasi`

---

## Seksi 6: Kontrol PID

**Tujuan:** suhu air stabil di set point (PRD 6.2).

### Langkah A: logika PID
- [x] Buat objek PID dengan arah **`REVERSE`** (pendingin), sample time 1000 ms, batas keluaran 0–100 (FR-04 sampai FR-06).
- [x] Nilai awal: Kp 25, Ki 0,05, Kd 0 (FR-07). Untuk tuning awal pakai **Ki = 0** dulu.
- [x] Keluaran PID masuk ke batas proteksi: `daya = min(pid, batasProteksi)`.
- [x] Jika air lebih dingin dari set point, keluaran 0 (FR-12).

### Langkah B: pelindung Peltier
- [x] **Hysteresis** (FR-08): Peltier ON jika PID ≥ 15 %, OFF jika < 5 %.
- [x] **Jeda minimum 30 detik** antar nyala/mati (FR-09).
- [x] **Soft start**: kenaikan daya maksimal 5 % per detik (FR-10).
- [x] Pompa ON selama sistem aktif dan level NORMAL (FR-13).
- [x] Sistem OFF dari tombol (nanti dari dashboard): Peltier dan pompa mati.

### Langkah C: tuning (PRD 9.2)
- [ ] Siapkan wadah air 5 sampai 10 liter dengan sirkulasi pompa, sensor air tercelup, sensor heatsink terpasang. Isi dengan air sedikit di atas set point (misalnya 31 °C untuk set point 28 °C).
- [ ] Jalankan dengan **Kp 25, Ki 0, Kd 0**. Rekam suhu tiap 10 detik (Serial → salin ke spreadsheet).
- [ ] Jika suhu bergoyang di sekitar target: kecilkan Kp. Jika terlalu lambat: besarkan Kp.
- [ ] Jika suhu stabil tapi tertahan di atas target: naikkan Ki sedikit (0,01 → 0,05 → 0,1).
- [ ] Biarkan Kd = 0.
- [ ] Catat setiap percobaan: Kp, Ki, waktu mencapai target, overshoot. Ini bahan laporan.
- [ ] Simpan nilai terbaik di `config.h`.

### Selesai jika
Suhu air masuk ±0,5 °C dari set point dan stabil minimal 30 menit (uji penuh 2 jam di seksi 9). Proteksi tetap bekerja selama uji.

**Jebakan umum:** kalau Peltier malah terus menyala padahal air sudah dingin, arah PID masih `DIRECT`. Tukar ke `REVERSE`.

**Commit:** `feat: kontrol PID dan pelindung Peltier`

---

## Seksi 7: WiFi dan dashboard

**Tujuan:** set point dan ON/OFF bisa diubah dari HP, data tampil di browser (PRD bagian 5).

### Langkah
- [x] Isi `secrets.h` dengan SSID dan password WiFi. Pastikan `.gitignore` sudah menyembunyikannya.
- [x] Sambungkan ke WiFi (timeout 15 detik). Jika gagal, pindah ke **mode Access Point** bernama `AquaTEC` (PRD 5.4).
- [x] Aktifkan mDNS (`aquatec.local`). Tampilkan IP di LCD saat boot (FR-21).
- [x] Buat `web_server.cpp` dengan endpoint:
  - `GET /api/status` (JSON seperti contoh PRD 5.3)
  - `POST /api/setpoint` (tolak di luar 20–30 °C dengan HTTP 400)
  - `POST /api/power`
- [x] Buat halaman `dashboard_html.h` (HTML dan JavaScript biasa, simpan di PROGMEM) yang memanggil `/api/status` tiap 2 detik:
  - D1: suhu air dan heatsink
  - D2: input set point + tombol Simpan
  - D3: status Peltier (%) dan pompa
  - D4: level proteksi berwarna (hijau, kuning, merah)
  - D5: tombol Sistem ON/OFF
- [x] Simpan set point dengan `Preferences` (FR-15, FR-16) supaya tetap setelah ESP32 mati.
- [x] Jalankan web server di `TaskWeb` (core 0). Baca dan tulis data hanya lewat mutex.
- [ ] Uji dari HP dan laptop pada WiFi yang sama.

### Selesai jika
- Ubah set point di HP → LCD ikut berubah ≤ 1 detik (**T17**).
- Set point tetap setelah ESP32 di-restart (**T4**).
- Set point 35 atau 10 ditolak (**T5**).

**Jebakan umum:** `aquatec.local` kadang tidak terbuka di Android. Pakai alamat IP yang tampil di LCD.

**Commit:** `feat: WiFi dan dashboard dasar`

---

## Seksi 8: Fitur lengkap dan fail-safe

**Tujuan:** menutup semua fitur "sebaiknya ada" dan memastikan penanganan error.

### Langkah
- [x] **D7**: tombol Reset FAULT di dashboard (`POST /api/reset-fault`), hanya berhasil jika heatsink < 45 °C.
- [x] **D8**: peringatan di dashboard jika sensor error atau suhu air > 32 °C.
- [x] **D6**: ring buffer 360 titik (suhu air tiap 10 detik = 1 jam), endpoint `/api/history`, grafik dengan `<canvas>`.
- [x] Tambahkan `uptime_s` dan `sensor_ok` di `/api/status`.
- [x] Rapikan tampilan LCD dan dashboard untuk semua kondisi error di FR-20 dan PRD 7.6.
- [ ] Uji **T14**: putuskan WiFi saat sistem berjalan. Kontrol harus tetap jalan dan dashboard kembali setelah WiFi pulih.
- [ ] Uji **T18**: cabut LCD saat sistem berjalan. Tidak boleh ada restart atau gangguan kontrol (FR-23).
- [x] Rapikan komentar kode berbahasa Indonesia (NFR-04).

### Selesai jika
D1 sampai D8 berfungsi, T14 dan T18 lolos.

**Commit:** `feat: grafik, reset FAULT, penanganan error`

---

## Seksi 9: Integrasi dan uji akhir

**Tujuan:** membuktikan sistem memenuhi PRD (Definition of Done, PRD bagian 12).

### Langkah
- [ ] Rakit permanen: kotak tertutup untuk elektronik, sekring terpasang, kabel 12V tidak dekat air (NFR-03), kabel dirapikan.
- [ ] Jalankan seluruh daftar uji **T1 sampai T18** (PRD 9.1) dan catat hasil pada tabel: ID, tanggal, hasil, catatan, foto.
- [ ] **T12 (hati-hati)**: lepas kabel fan saat Peltier berjalan **dengan pengawasan penuh dan siap mencabut daya**. Pastikan proteksi aktif bertahap.
- [ ] **T13**: uji 2 jam dengan air awal 31 °C, set point 28 °C. Catat grafik suhu.
- [ ] **T15**: uji 24 jam tanpa restart tidak terduga. Sebaiknya dijalankan saat Anda berada di rumah.
- [ ] Perbaiki bug yang ditemukan. Untuk setiap perbaikan ulangi uji yang terkait.

### Selesai jika
Semua kotak di Definition of Done PRD (bagian 12) tercentang.

**Commit:** `test: hasil uji akhir` lalu beri tag `v1.0`.

---

## Seksi 10: Dokumentasi dan presentasi

**Tujuan:** bahan laporan dan demo (PRD DoD terakhir).

### Langkah
- [ ] Gambar skema rangkaian (misalnya dengan Fritzing atau draw.io): jalur 12V, buck 5V, JZ-MOS #1 dan #2, LCD, sensor.
- [x] Salin diagram alur loop (PRD 2.1), skema pendinginan (2.2), diagram state machine (7.4), dan skema task FreeRTOS (4.2).
- [ ] Susun tabel hasil uji T1 sampai T18.
- [ ] Susun grafik tuning PID (percobaan Kp/Ki dan hasil akhir) dan grafik uji 2 jam.
- [x] Tulis `README.md` di repositori: cara rakit, cara build dan upload, cara akses dashboard.
- [x] Siapkan demo 5 menit:
  1. Tunjukkan LCD dan dashboard.
  2. Ubah set point dan tunjukkan sistem merespons.
  3. Demo proteksi dengan `hs=61` dan `hs=71` (suhu simulasi).
  4. Jelaskan pembagian task FreeRTOS dan alasan memakainya.
- [x] Siapkan jawaban untuk pertanyaan dosen yang mungkin muncul:
  - Mengapa proteksi dijalankan sebelum PID?
  - Mengapa PID memakai `REVERSE`?
  - Mengapa ada histeresis di batas suhu?
  - Mengapa fan tidak dikontrol ESP32?
  - Apa risiko pompa mati sementara Peltier jalan, dan bagaimana mitigasinya?

### Selesai jika
Laporan, README, dan demo siap, serta repositori rapi tanpa `secrets.h`.

---

## Jika terhambat

| Gejala | Cek dulu |
|---|---|
| Suhu selalu `-127` | Pull-up 4,7 kΩ, kabel DATA, nomor pin |
| Suhu `85` | Terbaca sebelum konversi selesai |
| LCD kosong | Alamat I2C (0x27 / 0x3F), kontras, SDA/SCL tertukar |
| ESP32 restart sendiri | Watchdog (ada task macet), arus dari buck kurang, tegangan turun saat Peltier menyala |
| Peltier tidak bereaksi | Modul JZ-MOS tidak menerima trigger 3,3V, GND belum disatukan |
| PID seperti terbalik | Arah PID harus `REVERSE` |
| Dashboard tidak terbuka | Pakai IP dari LCD, pastikan HP dan ESP32 di WiFi yang sama |
| Error kompilasi `ledcSetup` / `ledcAttach` | Versi core Arduino-ESP32 tidak cocok dengan kode PWM (lihat seksi 3) |

## Pengembangan setelah selesai (opsional)

MQTT dan Grafana untuk riwayat jangka panjang, notifikasi Telegram saat WARNING/FAULT, OTA update, sensor arus Peltier, dan PWM untuk fan atau pompa (lihat PRD bagian 11).
