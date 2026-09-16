# Dokumentasi BAIK × ESP32

Indeks seluruh dokumen proyek. Kembali ke [halaman depan](../README.md).

## Daftar dokumen

| Dokumen | Satu kalimat |
|---|---|
| [MULAI-CEPAT.md](MULAI-CEPAT.md) | Panduan dari kardus sampai LED berkedip dalam ~10 menit, lengkap dengan bagian pemecahan masalah yang panjang. |
| [BAHASA.md](BAHASA.md) | Rujukan bahasa BAIK: tipe, literal, operator dan presedensinya, kata kunci yang jalan dan yang ditolak parser, fungsi, array, objek, serta batasannya yang nyata. |
| [API.md](API.md) | Daftar lengkap fungsi ESP32 yang bisa dipanggil dari skrip BAIK — GPIO, ADC, PWM, I2C, SPI, UART, Wi-Fi, HTTP, SPIFFS, NVS, *deep sleep* — beserta alias bahasa Indonesianya. |
| [PINOUT.md](PINOUT.md) | Peta pin ESP32 dan ESP32-S3: kapabilitas tiap GPIO, pin yang harus dihindari, dan konstanta yang tersedia. |
| [ARSITEKTUR.md](ARSITEKTUR.md) | Cara kerja firmware di dalam — alur boot, peran tiap berkas, pendaftaran API, penanganan interupsi — plus panduan menambah fungsi API baru. |
| [PENGUJIAN.md](PENGUJIAN.md) | Cara membangun kedua papan, menjalankan uji di host dan QEMU, serta memverifikasi di papan asli. |
| [examples/](../examples/) | Kumpulan skrip `.ina` siap pakai untuk disalin dan diubah. |

## Mulai dari mana?

### Kamu baru pertama kali memakai mikrokontroler

Jangan mulai dari daftar fungsi — mulai dari papan yang menyala.

1. **[MULAI-CEPAT.md](MULAI-CEPAT.md)** — ikuti dari langkah 1 sampai selesai. Setelah
   LED-mu berkedip karena perintah yang kamu ketik sendiri, sisanya jauh lebih masuk akal.
2. **[BAHASA.md](BAHASA.md)** bagian *Bentuk program*, *Tipe data*, dan *Percabangan
   dan perulangan*. Cukup itu dulu; lewati bagian operator dan keterbatasan.
3. **[examples/](../examples/)** — ambil satu contoh, ubah satu angka, lihat apa yang
   terjadi. Ini cara belajar tercepat dengan REPL.
4. Kalau ada yang macet, semua gejala umum (port tidak terdeteksi, layar penuh karakter
   sampah, editor web kosong) sudah dibahas di bagian
   [Pemecahan masalah](MULAI-CEPAT.md#pemecahan-masalah).

**Satu hal yang perlu diingat sejak awal:** variabel dideklarasikan dengan `isi`, bukan
`var`.

### Kamu sudah biasa dengan Arduino

Kabar baiknya: nama fungsinya sama persis dengan yang sudah kamu hafal.

1. **[BAHASA.md](BAHASA.md)** bagian
   [*Kalau kamu tahu JavaScript*](BAHASA.md#kalau-kamu-tahu-javascript) dan
   [*Kata kunci*](BAHASA.md#kata-kunci). Lima menit di sini menghemat berjam-jam
   kebingungan — terutama soal `isi` (bukan `var`), `===` (bukan `==`), tidak adanya
   konversi tipe implisit, dan `.panjang` (bukan `.length`).
2. **[API.md](API.md)** — `pinMode`, `digitalWrite`, `analogRead`, `ledcWrite`,
   `Wire.begin`, `WiFi.begin` semuanya ada dengan nama yang sama. Yang berubah hanyalah
   cara memanggilnya: dari REPL, bukan dari `loop()`.
3. **[PINOUT.md](PINOUT.md)** — atau ketik `pinout` di REPL untuk peta papan yang sedang
   menyala.
4. **[BAHASA.md → Keterbatasan yang nyata](BAHASA.md#keterbatasan-yang-nyata)** — baca
   ini sebelum merancang sesuatu yang besar. Tidak ada `setup()`/`loop()`, tidak ada
   timer asinkron, dan callback interupsi dilayani belakangan (bukan dari dalam ISR).

Perbedaan mental terbesar: di Arduino kamu menulis program lalu mem-flash-nya; di sini
kamu berbicara dengan papan yang sudah menyala, lalu menyimpan percakapan yang berhasil
sebagai berkas `.ina`.

### Kamu ingin ikut mengembangkan

1. **[ARSITEKTUR.md](ARSITEKTUR.md)** — baca seluruhnya. Di situ ada alur boot, peran
   tiap berkas di `src/` dan `src/esp32/`, cara API didaftarkan ke interpreter, dan
   alasan mengapa interupsi ditunda.
2. **[ARSITEKTUR.md → Cara menambah fungsi API baru](ARSITEKTUR.md#cara-menambah-fungsi-api-baru)**
   — contoh lengkap berikut potongan kodenya.
3. **[PENGUJIAN.md](PENGUJIAN.md)** — bangun kedua environment dan jalankan uji sebelum
   mengirim PR.
4. **[BAHASA.md](BAHASA.md)** — supaya contoh dan dokumen yang kamu tulis memakai kata
   kunci yang benar-benar didukung, bukan yang sekadar dicadangkan.

Aturan yang tidak boleh dilanggar: modul di `src/baik/` dan `src/baik.h` tidak diedit untuk
keperluan ESP32, nama fungsi perangkat keras identik dengan Arduino-ESP32, dan setiap
fungsi yang menerima nomor pin wajib memanggil `e32_pin_ok()` lebih dulu.

## Rujukan cepat

- Kata kunci yang jalan: `isi`, `fungsi`, `balik`, `jika`, `lainnya`, `untuk`, `ulang`,
  `berhenti`, `teruskan`, `benar`, `salah`, `kosong`, `takterdefinisi`, `tipe`, `this`,
  dan `in` (khusus di dalam `untuk`).
- Kata kunci yang **ditolak** parser: `var`, `pilih`, `sama`, `kerjakan`, `try`,
  `catch`, `throw`, `new`, `delete`, `instanceof`, `void`, `with`.
- Perintah konsol: `help`, `api`, `pinout`, `run`, `ls`, `cat`, `sysinfo`, `meminfo`,
  `restart`, `history`, `clear`.
- Berkas autorun: `/baik.ina` di SPIFFS.
- Editor web: `http://<ip-papan>/` — pengaturan Wi-Fi: `http://<ip-papan>/ap`.
- Access Point bawaan: **ESP32_AP** / **12345678**.
- Baud monitor serial: **115200**.
