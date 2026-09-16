# Pengujian BAIK-ESP32

Dokumen ini menjelaskan empat cara menguji proyek ini, dari yang paling cepat
sampai yang paling mendekati kenyataan:

| # | Metode | Butuh papan? | Kecepatan | Menguji apa |
|---|---|---|---|---|
| 1 | [Uji host](#1-uji-host-di-pc) | tidak | detik | bahasa BAIK + logika skrip `.ina` |
| 2 | [Build firmware](#2-build-firmware) | tidak | ~15 detik | apakah kode ESP32 bisa dikompilasi |
| 3 | [QEMU](#3-qemu-emulator) | tidak | ~1 menit | boot, bootloader, awal firmware |
| 4 | [Papan sungguhan](#4-papan-sungguhan) | ya | menit | semuanya, termasuk WiFi & GPIO |

Aturan praktis: **kerjakan 1 dan 2 setiap kali mengubah kode.** Nomor 3 hanya
berguna untuk masalah boot. Nomor 4 wajib sebelum merilis.

---

## 1. Uji host (di PC)

Cara tercepat dan paling penting. Interpreter BAIK (modul di `src/baik/`) dikompilasi
dengan gcc biasa lalu dipakai menjalankan skrip `.ina` di PC — tanpa papan,
tanpa emulator.

```bash
test/host/run_tests.sh
```

Keluaran:

```
==> Membangun penjalan BAIK untuk PC...
==> Menjalankan 30 kasus uji

  01-aritmetika                LULUS
  ...
==================== RINGKASAN UJI HOST ====================
  Total  : 30
  LULUS  : 30
  GAGAL  : 0
============================================================
```

Kode keluar 0 bila semua lulus, 1 bila ada yang gagal — cocok untuk CI.

### Pemakaian lain

```bash
test/host/run_tests.sh json gpio    # hanya kasus yang namanya mengandung itu
test/host/run_tests.sh --verbose    # tampilkan keluaran penuh tiap kasus
test/host/run_tests.sh --rekam      # REKAM ULANG berkas .expected
```

> `--rekam` menimpa berkas `.expected` dengan keluaran saat ini. **Selalu baca
> hasil `git diff` setelah memakainya** — kalau tidak, bug diam-diam berubah
> menjadi "perilaku yang diharapkan".

### Menjalankan satu skrip sendiri

```bash
make -C test/host                       # bangun sekali
test/host/build/baik-host berkas.ina    # jalankan berkas
test/host/build/baik-host -e 'tulis(1+2);'
test/host/build/baik-host -n berkas.ina # tanpa shim ESP32 (bahasa murni)
```

### Susunan berkas

```
test/host/
├── Makefile              # kompilasi src/baik/*.c + runner + shim -> build/baik-host
├── runner.c              # main() sendiri: jalankan .ina, cetak galat, kode keluar
├── run_tests.sh          # bangun + jalankan semua kasus + bandingkan .expected
├── shim/
│   ├── e32_shim.h
│   └── e32_shim.c        # tiruan API ESP32 untuk PC
└── cases/
    ├── NN-nama.ina       # skrip uji
    └── NN-nama.expected  # keluaran yang diharapkan (stdout+stderr+kode keluar)
```

### Kenapa `runner.c` menulis `main()` sendiri?

`src/baik/baik_repl.c` punya blok `#ifdef BAIK_MAIN` yang berisi `main()` dan REPL, tapi
REPL itu memakai GNU readline (`readline()`, `add_history()`, `rl_bind_key()`).
Itu menambah ketergantungan pustaka yang tidak perlu, dan REPL-nya interaktif
sehingga tidak bisa dipakai dalam uji otomatis. Karena itu `BAIK_MAIN` **tidak**
didefinisikan; `test/host/runner.c` menyediakan `main()` yang non-interaktif.

### Shim ESP32 (`test/host/shim/`)

Supaya skrip yang memakai `pinMode`, `digitalWrite`, `millis`, dan kawan-kawan
bisa dijalankan di PC, shim menyediakan fungsi dengan **nama dan semantik yang
sama persis** seperti di papan (lihat kontrak API). Setiap aksi dicatat ke
stdout dengan format yang stabil:

```
[GPIO] mode pin 2 <- OUTPUT
[GPIO] pin 2 <- HIGH
[GPIO] pin 4 -> LOW
[ADC] pin 34 -> 2048
[PWM] pin 16 <- 128/255
[WAKTU] delay 100 ms (millis=100)
[GALAT] digitalWrite: pin 34 tidak punya kapabilitas OUTPUT
```

Shim ini **mandiri**: ia tidak menyertakan apa pun dari `src/esp32/` (berkas di
sana butuh `Arduino.h`). Yang ditiru:

- GPIO: `pinMode`, `digitalWrite`, `digitalRead`, `digitalToggle` + alias
  Indonesia (`modePin`, `tulisDigital`, `bacaDigital`)
- Analog: `analogRead`, `analogReadMilliVolts`, `analogWrite`, `ledcAttach`,
  `ledcWrite`, `touchRead`, `tone`, `noTone`
- Waktu: `delay`, `delayMicroseconds`, `millis`, `micros`, `yield`
- Matematika: `map`, `constrain`, `min`, `max`, `abs`, `pow`, `sqrt`, `sin`,
  `cos`, `tan`, `floor`, `ceil`, `round`, `log`, `exp`, `random`, `randomSeed`
- Introspeksi: `pinInfo`, `pinCaps`, `pinPunya`, `daftarPin`, `pinout`
- Object `Serial` dan `ESP`
- Konstanta: `HIGH`, `LOW`, `INPUT`, `OUTPUT`, `LED_BUILTIN`, `A0`..`A19`,
  `GPIO0`..`GPIO39`, `CAP_*`, `PI`, `BOARD`, `CHIP`, dst.

Dua keputusan penting supaya hasil uji **deterministik** (selalu sama persis,
sehingga bisa dibandingkan dengan `.expected`):

1. **Jam virtual.** `millis()`/`micros()` mulai dari 0 dan hanya maju ketika
   `delay()`/`delayMicroseconds()` dipanggil. Di papan, jam berjalan sungguhan.
2. **PRNG tetap.** `random()` memakai LCG dengan benih tetap, bukan `esp_random()`.

### Fungsi khusus uji (hanya ada di PC)

Fungsi-fungsi ini **tidak ada di papan**. Pakai hanya di dalam `test/host/cases/`:

| Fungsi | Guna |
|---|---|
| `shimSetDigital(pin, nilai)` | suntik level yang akan dibaca `digitalRead` |
| `shimSetAnalog(pin, nilai)` | suntik nilai yang akan dibaca `analogRead` |
| `shimSetTouch(pin, nilai)` | suntik nilai `touchRead` |
| `shimGetLevel(pin)` | baca level pin tanpa mencetak catatan |
| `shimGetPwm(pin)` | baca duty PWM terakhir |
| `shimLog(benar/salah)` | nyalakan/matikan catatan aksi |
| `shimReset()` | kembalikan semua state ke awal |

### Menambah kasus uji

1. Tulis `test/host/cases/31-namamu.ina`.
2. Jalankan `test/host/run_tests.sh --rekam 31-namamu`.
3. **Baca `test/host/cases/31-namamu.expected` dan pastikan isinya memang benar.**
4. Jalankan `test/host/run_tests.sh` untuk memastikan lulus.

Berkas `.expected` berisi stdout + stderr digabung, ditutup baris
`[keluar=N]` berisi kode keluar.

---

## Batasan bahasa BAIK yang ditemukan lewat uji

Semua di bawah ini **sudah dipastikan dengan menjalankan interpreter**, bukan
tebakan. Kasus uji `14-batasan-bahasa.ina` sampai `19-negatif-delete.ina`
mengunci perilaku ini supaya tidak berubah diam-diam.

### Kata kunci yang BELUM diimplementasi

Dikenali lexer tetapi ditolak parser dengan `[<kata>] tidak terimplementasi`:

| Salah | Benar |
|---|---|
| `var x = 1;` | `isi x = 1;` |
| `pilih (x) { sama 1: ... }` | rantai `jika` / `lainnya jika` / `lainnya` |
| `kerjakan { ... } ulang (c);` | `ulang (benar) { ... jika (c) { berhenti; } }` |
| `delete o.a;` | (tidak ada padanan) |

Juga belum ada: `sama`, `catch`, `instanceof`, `new`, `throw`, `try`, `void`,
`with`.

### Perbedaan lain dari JavaScript

| Hal | Perilaku BAIK |
|---|---|
| `==` dan `!=` | **ditolak**: `Use ===, not ==`. Pakai `===` / `!==`. |
| `.length` | tidak ada — properti panjang bernama **`panjang`** |
| `"teks" + 1` | **galat** "konversi tipe implisit dilarang" |
| `tulis([1,2,3])` | mencetak `<untaian>`, bukan isinya — pakai `JSON.stringify` |
| `tulis({a:1})` | mencetak `<objek>` |
| `tulis(...)` | memisahkan argumen dengan spasi dan menambah satu spasi di akhir |
| angka pecahan | dicetak dengan 6 desimal (`3.500000`); bilangan bulat tanpa koma |
| `s.at(0)` | mengembalikan **kode** karakter, bukan hurufnya |
| closure | fungsi dalam fungsi **tidak** menangkap variabel luar |
| `tipe` | mengembalikan `angka`, `huruf`, `boolean`, `kosong`, `untaian`, `objek` |

Cara menggabungkan angka ke string:

```
tulis("nilai=" + JSON.stringify(n));   /* atau */
tulis("nilai=", n);                    /* argumen terpisah */
```

### JSON: dua bug yang sudah diperbaiki

Sampai sebelum perbaikan ini, JSON di BAIK rusak di kedua arah. Keduanya
berakar pada satu kesalahan yang sama: literal JSON ikut diterjemahkan ke
bahasa Indonesia, padahal JSON adalah format pertukaran data, bukan teks
untuk dibaca manusia.

1. **`JSON.stringify` menghasilkan JSON yang tidak sah.** Boolean dan null
   ditulis sebagai kata Indonesia: `JSON.stringify({ya: benar})` menghasilkan
   `{"ya":benar}`.

2. **`JSON.parse` tidak pernah bisa membaca `true`/`false`/`null`.** Ini yang
   lebih parah. Pengurai memilih cabang berdasarkan huruf pertama `'n'`,
   `'t'`, `'f'`, lalu membandingkan kata itu dengan `"kosong"`, `"benar"`,
   `"salah"` — perbandingan yang tidak mungkin cocok. Akibatnya setiap JSON
   yang memuat ketiga nilai itu ditolak, **termasuk JSON dari API luar lewat
   `HTTP.getJSON()`**.

Sekarang keduanya benar:

```
JSON.stringify({ya: benar})        ->  {"ya":true}
JSON.parse('{"ok":true,"n":null}') ->  bisa dibaca
JSON.parse(JSON.stringify(x))      ->  pulang-pergi berhasil
```

Yang **tidak** berubah: `tulis()` tetap mencetak `benar`, `salah`, `kosong`
dalam bahasa Indonesia. Pemisahan itu disengaja — tampilan untuk manusia
memakai bahasa Indonesia, serialisasi JSON memakai literal JSON.

Perilaku ini dikunci tiga kasus uji: `29-json-nilai-dasar`,
`30-json-boolean` (termasuk regresi bahwa `tulis()` tetap Indonesia),
`38-json-bolak-balik`, dan `39-json-parse-luar` (JSON dari luar).

Satu hal yang perlu diketahui saat membaca `38-json-bolak-balik`: urutan
properti objek **terbalik** setiap kali melewati `stringify`/`parse`, jadi
membandingkan teks hasil pulang-pergi objek menghasilkan `salah` walau
datanya identik. Untaian tidak kena ini.

### Catatan untuk `src/baik.h`

Makro `BAIK_UNDEFINED` dan `BAIK_NULL` di `src/baik.h` mengembang menjadi
`BAIK_TAG_UNDEFINED` / `BAIK_TAG_NULL` yang **hanya terdefinisi di dalam
`src/baik/`**. Jadi kedua makro itu tidak bisa dipakai dari berkas lain.
Pakai `baik_mk_undefined()` / `baik_mk_null()`, atau `0` sebagai penanda
"belum diisi" (itulah yang dilakukan `runner.c`).

---

## 2. Build firmware

```bash
tools/build.sh                      # bangun kedua papan
tools/build.sh esp32doit-devkit-v1  # satu papan saja
tools/build.sh --clean              # bersihkan dulu
```

Log tersimpan di `.pio/build-logs/build-<env>.log`.

### Menyiapkan PlatformIO

PlatformIO dipasang di virtualenv tersendiri supaya tidak mengotori Python
sistem:

```bash
python3 -m venv ~/.baik-pio-venv
~/.baik-pio-venv/bin/pip install -U platformio
```

`tools/build.sh` otomatis memakai venv itu. Kalau `pio` milikmu ada di tempat
lain:

```bash
PIO=/path/ke/pio tools/build.sh
```

Toolchain Xtensa (~200 MB) diunduh otomatis saat build pertama.

### Hasil build

Artefak ada di `.pio/build/<env>/`:

| Berkas | Offset flash (ESP32) | Isi |
|---|---|---|
| `bootloader.bin` | `0x1000` (S3: `0x0`) | bootloader tahap dua |
| `partitions.bin` | `0x8000` | tabel partisi |
| `boot_app0.bin` | `0xe000` | penanda partisi OTA |
| `firmware.bin` | `0x10000` | aplikasi |
| `spiffs.bin` | dari tabel partisi | isi direktori `data/` |

---

## 3. QEMU (emulator)

> **Status jujur:** QEMU berhasil dipasang dan menjalankan firmware sampai
> masuk ke `setup()`, lalu **panik di blok WiFi**. REPL BAIK **belum** bisa
> diuji lewat QEMU. Rinciannya di bawah.

### Memasang QEMU

QEMU untuk Xtensa **tidak ada di apt Ubuntu** dan **tidak ada di registry
PlatformIO**. Paket `platformio/tool-qemu-xtensa` yang sering disebut orang
**tidak pernah ada** — registry hanya punya `platformio/tool-qemu-riscv`
terbitan 2019. Opsi `board_build.qemu` juga bukan fitur PlatformIO resmi.
Satu-satunya jalan adalah fork resmi Espressif:

```bash
tools/qemu.sh --pasang
```

Skrip itu mengunduh rilis prebuilt dari
<https://github.com/espressif/qemu/releases>, memasangnya di `~/.baik-qemu/`,
lalu melengkapi pustaka pendukung (SDL2, slirp, samplerate, Xss, decor) ke
`~/.baik-qemu/lib` **tanpa mengubah sistem** (paket `.deb` diekstrak lokal,
bukan dipasang). Verifikasi:

```bash
LD_LIBRARY_PATH=~/.baik-qemu/lib ~/.baik-qemu/bin/qemu-system-xtensa --version
# QEMU emulator version 9.2.2 (esp_develop_9.2.2_20260417)
```

Mesin yang tersedia: `esp32` dan `esp32s3`.

### Menjalankan

```bash
tools/qemu.sh                          # env esp32doit-devkit-v1
tools/qemu.sh -e esp32-s3-devkitc-1
tools/qemu.sh --image-saja             # hanya rakit flash.bin
tools/qemu.sh --gdb                    # tunggu gdb di :1234
```

Keluar dari QEMU: **Ctrl-A lalu X**.

Skrip menggabungkan bootloader + tabel partisi + `boot_app0` + firmware +
SPIFFS menjadi satu image flash 4 MB, dengan offset SPIFFS **dibaca dari tabel
partisi yang sebenarnya** (bukan ditebak), lalu menjalankannya dengan serial
tersambung ke stdio.

### Sejauh mana QEMU berhasil

Yang **berhasil**:

- ROM bootloader jalan (`rst:0x1 (POWERON_RESET)`)
- bootloader tahap dua memuat segmen aplikasi (`entry 0x400805e4`)
- aplikasi Arduino mulai, SPIFFS terpasang, NVS/Preferences terbaca
- firmware sampai ke `setup()` di `src/main.cpp`

Yang **gagal** — `setup()` menyalakan WiFi, dan QEMU tidak mengemulasikan
radio/PHY ESP32, jadi blob PHY tertutup milik Espressif menabrak register yang
tidak ada:

```
Guru Meditation Error: Core 0 panic'ed (LoadStorePIFAddrError).
EXCVADDR: 0x60033c00
Backtrace: register_chipv7_phy <- esp_phy_load_cal_and_init <- esp_phy_enable
           <- esp_phy_enable_wrapper <- wifi_hw_start <- wifi_start_process
           <- ieee80211_ioctl_process <- ppTask
```

Papan virtual lalu boot-loop. Karena `src/main.cpp` **selalu** memanggil
`WiFi.softAP()` (bila belum ada kredensial) atau `WiFi.begin()`, REPL BAIK tidak
pernah sempat jalan di QEMU.

**Saran perbaikan** (untuk pemilik `src/main.cpp`): bungkus blok WiFi dengan
penjaga waktu kompilasi, misalnya

```cpp
#ifndef BAIK_TANPA_WIFI
    /* ... seluruh blok WiFi + AsyncWebServer ... */
#endif
```

lalu QEMU bisa dijalankan dengan `-DBAIK_TANPA_WIFI` dan REPL bisa diuji.

### Opsi `--tanpa-wifi` (eksperimental, belum berhasil)

`tools/qemu.sh --tanpa-wifi` mencoba menyiasati itu: ia menukar `src/main.cpp`
dengan `test/qemu/main_qemu.cpp` (versi tanpa WiFi dan tanpa server web) lewat
`PLATFORMIO_BUILD_SRC_FILTER`, di direktori build terpisah `.pio/build-qemu/`,
**tanpa mengubah `platformio.ini` maupun `src/`**.

Hasil sejauh ini:

- firmware **berhasil dikompilasi** (RAM 16.9%, Flash 40.1%)
- di QEMU 9.2.2 firmware itu **ter-reset watchdog timer-group**
  (`TG1WDT_SYS_RESET` / `TG0WDT_SYS_RESET`) berulang-ulang **sebelum**
  `setup()` sempat mencetak apa pun

Sudah dicoba dan **tidak** mengubah hasil: menyertakan/menghilangkan SPIFFS di
image, dan memakai sketsa minimal (hanya `Serial.println` di `setup`/`loop`).
Build kontrol dengan `src/main.cpp` asli dan pipeline yang persis sama tetap
boot sampai `setup()`, jadi perakitan image flash-nya sendiri sudah benar.
**Penyebabnya belum ditemukan.**

### Batasan QEMU yang memang nyata

- **WiFi tidak diemulasikan.** `WiFi.*`, `HTTP.*`, `NTP.*`, `MDNS.*` tidak bisa
  diuji di QEMU — harus di papan sungguhan.
- **GPIO tidak terhubung ke apa pun.** Tidak ada LED, tombol, atau sensor.
  `digitalWrite` jalan tanpa galat tetapi tidak ada yang bisa diamati. Untuk
  menguji logika skrip `.ina`, pakai uji host — jauh lebih cepat dan punya
  catatan aksi per pin.
- **ADC, DAC, sentuh, PWM** tidak disimulasikan secara fisik.
- **ESP32-S3 jauh lebih muda dukungannya** daripada ESP32 klasik. Bila S3 tidak
  mau boot, coba `esp32doit-devkit-v1` dulu.
- **Waktu tidak real-time.** `millis()` di QEMU tidak sepadan dengan papan.

Kesimpulan praktis: QEMU berguna untuk masalah **boot dan bootloader**. Untuk
menguji bahasa BAIK, uji host jauh lebih efektif.

---

## 4. Papan sungguhan

Satu-satunya cara menguji WiFi, GPIO nyata, dan waktu sesungguhnya.

### Flash firmware

```bash
~/.baik-pio-venv/bin/pio run -e esp32doit-devkit-v1 -t upload
```

Kalau port tidak terdeteksi otomatis:

```bash
~/.baik-pio-venv/bin/pio device list
~/.baik-pio-venv/bin/pio run -e esp32doit-devkit-v1 -t upload --upload-port /dev/ttyUSB0
```

Papan yang tidak mau masuk mode unduh: tahan tombol **BOOT**, tekan sebentar
**EN/RST**, lepas **BOOT**.

### Unggah SPIFFS (isi direktori `data/`)

Firmware dan SPIFFS **terpisah**. Setelah mengubah apa pun di `data/`
(termasuk `data/baik.ina`), unggah ulang:

```bash
~/.baik-pio-venv/bin/pio run -e esp32doit-devkit-v1 -t uploadfs
```

### Monitor serial

```bash
~/.baik-pio-venv/bin/pio device monitor -e esp32doit-devkit-v1
```

Kecepatan 115200 (dari `platformio.ini`), dengan
`monitor_filters = esp32_exception_decoder` sehingga alamat pada backtrace
langsung diterjemahkan ke nama fungsi dan nomor baris. Keluar: **Ctrl-C**.

Gabungan yang praktis:

```bash
~/.baik-pio-venv/bin/pio run -e esp32doit-devkit-v1 -t upload -t uploadfs \
  && ~/.baik-pio-venv/bin/pio device monitor -e esp32doit-devkit-v1
```

### Menguji di REPL

Setelah papan menyala, prompt `Baik> ` muncul di monitor serial. Coba:

```
tulis(1+2);
pinout
api
pinMode(2, OUTPUT); digitalWrite(2, HIGH);
run /baik.ina
```

Kasus uji di `test/host/cases/` bisa disalin ke `data/`, diunggah dengan
`uploadfs`, lalu dijalankan dengan `run /namaberkas.ina` — hasilnya harus sama
dengan berkas `.expected`, **kecuali**:

- baris `[GPIO] ...`, `[ADC] ...`, `[WAKTU] ...` (itu buatan shim PC saja)
- nilai `millis()`/`micros()` (di papan jam berjalan sungguhan)
- nilai `random()` (di papan memakai penghasil acak perangkat keras)
- fungsi `shim*` (tidak ada di papan — kasus yang memakainya akan galat)

Kasus `01`–`19` dan `29`–`30` tidak memakai `shim*` sama sekali, jadi paling
cocok dipakai membandingkan perilaku PC vs papan.

### Batasan menguji di papan

- Butuh perangkat keras dan kabel data (banyak kabel USB hanya untuk mengecas).
- Siklus uji lambat: kompilasi + flash + reboot.
- Sulit diotomatiskan di CI.
- Kegagalan bisa berasal dari perangkat keras (catu daya kurang, kabel jelek),
  bukan dari kode.
- Pin `6`–`11` terpakai SPI flash: **jangan** dipakai.
- Pin `34`–`39` hanya input: tidak punya output dan tidak punya pull-up internal.

---

## Ringkasan batasan tiap metode

| Metode | Tidak bisa menguji |
|---|---|
| Uji host | kode `src/esp32/*.cpp` yang sesungguhnya, perilaku perangkat keras, WiFi, waktu nyata, batas memori ESP32 |
| Build firmware | apa pun saat runtime — hanya membuktikan kode bisa dikompilasi dan ditautkan |
| QEMU | WiFi (panik), GPIO/ADC/DAC/sentuh fisik, waktu nyata; dukungan S3 masih lemah; REPL belum jalan |
| Papan sungguhan | (bisa semuanya, tapi lambat dan sulit diotomatiskan) |
