# Rujukan API BAIK untuk ESP32

Dokumen ini adalah rujukan lengkap seluruh fungsi, objek, dan konstanta yang
tersedia di dalam bahasa **BAIK** saat dijalankan pada papan ESP32 / ESP32-S3.

> **Nama fungsi sengaja dibuat identik dengan Arduino-ESP32.** `pinMode`,
> `digitalWrite`, `ledcWrite`, `WiFi.begin`, dan seterusnya dieja persis seperti
> di Arduino. Siapa pun yang sudah pernah menulis sketsa Arduino bisa langsung
> menulis skrip BAIK tanpa belajar nama baru. **Alias bahasa Indonesia**
> disediakan untuk fungsi yang sering dipakai (mis. `tulisDigital` untuk
> `digitalWrite`). Alias adalah fungsi native yang **sama persis**, bukan
> pembungkus, sehingga tidak ada biaya tambahan saat dipanggil — silakan pilih
> gaya mana pun, atau campur keduanya dalam satu skrip.

---

## Daftar isi

- [1. Sebelum mulai](#1-sebelum-mulai)
  - [1.1 Menjalankan skrip](#11-menjalankan-skrip)
  - [1.2 Sintaks singkat bahasa BAIK](#12-sintaks-singkat-bahasa-baik)
- [1.3 Bantuan dan versi](#13-bantuan-dan-versi)
- [2. Konstanta](#2-konstanta)
  - [2.1 Tabel konstanta](#21-tabel-konstanta)
  - [2.2 Fungsi introspeksi pinout](#22-fungsi-introspeksi-pinout)
- [3. GPIO, Analog, dan PWM](#3-gpio-analog-dan-pwm)
  - [3.1 GPIO digital](#31-gpio-digital)
  - [3.2 LED bawaan dan LED RGB (WS2812)](#32-led-bawaan-dan-led-rgb-ws2812)
  - [3.3 ADC (analog masuk)](#33-adc-analog-masuk)
  - [3.4 PWM sederhana (analogWrite)](#34-pwm-sederhana-analogwrite)
  - [3.5 DAC (analog keluar)](#35-dac-analog-keluar)
  - [3.6 Sensor sentuh](#36-sensor-sentuh)
  - [3.7 LEDC (PWM tingkat lanjut)](#37-ledc-pwm-tingkat-lanjut)
  - [3.8 Nada / buzzer](#38-nada--buzzer)
  - [3.9 Interupsi](#39-interupsi)
  - [3.10 Pewaktuan pulsa dan shift register](#310-pewaktuan-pulsa-dan-shift-register)
- [4. Sistem dan waktu](#4-sistem-dan-waktu)
  - [4.1 Tunda dan pencacah waktu](#41-tunda-dan-pencacah-waktu)
  - [4.2 Matematika dan bilangan acak](#42-matematika-dan-bilangan-acak)
  - [4.3 Objek `Serial`](#43-objek-serial)
  - [4.4 Objek `ESP` (informasi chip)](#44-objek-esp-informasi-chip)
  - [4.5 Frekuensi CPU dan suhu](#45-frekuensi-cpu-dan-suhu)
  - [4.6 Tidur (sleep) dan sebab bangun](#46-tidur-sleep-dan-sebab-bangun)
  - [4.7 Memori RTC](#47-memori-rtc)
  - [4.8 Watchdog dan FreeRTOS](#48-watchdog-dan-freertos)
- [5. Bus: I2C, SPI, UART](#5-bus-i2c-spi-uart)
  - [5.1 I2C — objek `Wire`](#51-i2c--objek-wire)
  - [5.2 SPI — objek `SPI`](#52-spi--objek-spi)
  - [5.3 UART — objek `Serial1` / `Serial2`](#53-uart--objek-serial1--serial2)
- [6. Jaringan](#6-jaringan)
  - [6.1 Objek `WiFi`](#61-objek-wifi)
  - [6.2 Objek `HTTP`](#62-objek-http)
  - [6.3 Waktu jaringan — objek `NTP`](#63-waktu-jaringan--objek-ntp)
  - [6.4 mDNS](#64-mdns)
- [7. Berkas dan penyimpanan](#7-berkas-dan-penyimpanan)
  - [7.1 Objek `FS` (SPIFFS)](#71-objek-fs-spiffs)
  - [7.2 Objek `NVS` (Preferences)](#72-objek-nvs-preferences)
  - [7.3 Menjalankan skrip lain](#73-menjalankan-skrip-lain)
- [8. Perbedaan ESP32 vs ESP32-S3](#8-perbedaan-esp32-vs-esp32-s3)
- [9. Galat umum dan artinya](#9-galat-umum-dan-artinya)
- [10. Yang BELUM didukung](#10-yang-belum-didukung)

---

## 1. Sebelum mulai

### 1.1 Menjalankan skrip

Ada tiga cara menjalankan kode BAIK di papan:

| Cara | Langkah |
|---|---|
| **REPL serial** | Sambungkan papan, buka monitor serial pada 115200 baud, lalu ketik satu baris kode di belakang prompt `Baik> ` dan tekan Enter. |
| **Berkas `.ina` di SPIFFS** | Unggah berkas lewat editor web papan, lalu panggil `jalankan("/nama.ina")` (alias `run`). |
| **Otomatis saat boot** | Berkas bernama `/baik.ina` dijalankan sendiri setiap kali papan menyala, sebelum prompt REPL muncul. |

REPL memproses **satu baris tiap kali Enter**. Blok bertingkat (`jika { ... }`,
`untuk { ... }`, `fungsi { ... }`) yang ditulis pada beberapa baris karena itu
lebih aman disimpan sebagai berkas `.ina` dan dijalankan dengan `jalankan()`.

#### Perintah konsol

Selain kode BAIK, prompt menerima sejumlah perintah konsol. Perintah ditulis
**dipisah spasi** (tanpa kurung), dan inilah daftar lengkapnya:

| Kelompok | Perintah |
|---|---|
| Bantuan | `help`, `api`, `pinout` |
| Skrip | `run` |
| Berkas | `ls`, `cat`, `cd`, `pwd`, `mv`, `cp`, `rm`, `rmdir`, `edit` |
| Sistem | `sysinfo`, `meminfo`, `restart` |
| Jaringan | `ping`, `ipconfig` |
| Terminal | `history`, `clear` |

Apa pun selain itu diperlakukan sebagai kode BAIK dan diserahkan ke interpreter.

> **GPIO dikerjakan lewat fungsi BAIK saja.** Versi firmware terdahulu juga
> punya perintah konsol `pinMode`, `digitalRead`, `digitalWrite`, dan
> `analogRead` yang ditulis dipisah spasi (mis. `digitalWrite 2 1`). Perintah
> itu **sudah dihapus**, karena namanya persis sama dengan fungsi BAIK sehingga
> satu nama punya dua perilaku yang berbeda tergantung ada tidaknya kurung —
> dan yang berbahaya, jalur perintah konsol **melewati validasi pin**, sehingga
> `digitalWrite 34 1` pada pin input-only lolos begitu saja. Tulislah
> `digitalWrite(2, HIGH)` dengan kurung, seperti seluruh dokumen ini.

### 1.2 Sintaks singkat bahasa BAIK

BAIK bergaya JavaScript dengan kata kunci bahasa Indonesia. Tabel padanan lengkap ada di
`docs/BAHASA.md`; ini ringkasan yang cukup untuk membaca contoh di dokumen ini.

| BAIK | Padanan JavaScript | Catatan |
|---|---|---|
| `isi` | `let` / `var` | **Satu-satunya** kata kunci deklarasi yang jalan hari ini. |
| `fungsi` | `function` | |
| `balik` | `return` | |
| `jika` / `lainnya` | `if` / `else` | `lainnya jika` untuk rantai. |
| `untuk` | `for` | `untuk (isi i = 0; i < n; i++)` dan `untuk (isi k in obj)`. |
| `ulang` | `while` | |
| `berhenti` / `teruskan` | `break` / `continue` | |
| `benar` / `salah` | `true` / `false` | |
| `kosong` / `takterdefinisi` | `null` / `undefined` | |
| `tipe` | `typeof` | Balik `"angka"`, `"huruf"`, `"boolean"`, `"untaian"`, `"kosong"`, `"takterdefinisi"`, `"objek"`, `"fungsi"`. |
| `tulis(...)` | `console.log(...)` | Argumen dipisah spasi, diakhiri baris baru. |
| `.panjang` | `.length` | Berlaku untuk string maupun array. |

Tiga aturan yang sering menjebak pendatang dari JavaScript:

1. Gunakan `===` dan `!==`. Operator `==` dan `!=` ditolak dengan galat
   `Use ===, not ==`.
2. **Tidak ada konversi tipe implisit.** `"suhu: " + 25` adalah galat. Cetak
   dengan beberapa argumen: `tulis("suhu:", 25);`, atau ubah dulu dengan
   `JSON.stringify(25)`.
3. Deklarasikan variabel dengan `isi`, bukan `var`. Karena `isi` adalah kata
   kunci, ia **tidak boleh dipakai sebagai nama variabel** — `isi isi = 5;`
   adalah galat sintaks. Hal yang sama berlaku untuk setiap kata tercadang
   (lihat [10.6](#106-kata-kunci-yang-dikenali-lexer-tetapi-ditolak-parser)).

```baik
isi pin = 2;
pinMode(pin, OUTPUT);
digitalWrite(pin, HIGH);
tulis("LED pada GPIO", pin, "menyala");
```

### 1.3 Bantuan dan versi

Dua fungsi ini membantu kamu berorientasi langsung dari prompt REPL, tanpa perlu
membuka dokumen apa pun.

---

#### `bantuan()` — alias `help_baik`

Cetak ringkasan seluruh API BAIK-ESP32 ke konsol: daftar fungsi per modul beserta
keterangan singkatnya.

**Parameter:** tidak ada. **Nilai balik:** `takterdefinisi`.
**Alias:** `help_baik` · **Padanan Arduino:** tidak ada (khas BAIK).
**Catatan:** keluarannya sama dengan perintah konsol `api`. Karena daftarnya
panjang, gulirkan terminalmu ke atas setelah memanggilnya.

```baik
bantuan();
```

---

#### `versi()`

Cetak identitas firmware yang sedang berjalan: versi BAIK-ESP32, nama papan dan
chip, versi core arduino-esp32, versi ESP-IDF, serta tanggal dan jam kompilasi.

**Parameter:** tidak ada.
**Nilai balik:** **string** versi BAIK-ESP32 (selain dicetak ke konsol).
**Alias:** — · **Padanan Arduino:** tidak ada (khas BAIK).
**Catatan:** sebutkan keluaran fungsi ini saat melaporkan masalah — versi core
arduino-esp32 (2.x atau 3.x) menentukan perilaku beberapa fungsi, terutama LEDC
dan `hallRead()`.

```baik
versi();

// nilai baliknya bisa dipakai di dalam skrip
isi v = versi();
tulis("firmware:", v);
```

Contoh keluarannya:

```
BAIK-ESP32 versi 1.0.0
  Papan        : esp32doit-devkit-v1
  Chip         : ESP32-D0WD
  arduino-esp32: 2.0.17
  ESP-IDF      : v4.4.7
  Dibangun     : Sep 16 2026 06:30:00
```

---

## 2. Konstanta

Seluruh konstanta di bawah ini terdaftar sebagai variabel global, jadi langsung
bisa dipakai tanpa awalan apa pun. Nilainya angka, kecuali `BOARD` dan `CHIP`
yang berupa string.

### 2.1 Tabel konstanta

| Kelompok | Konstanta | Nilai / arti |
|---|---|---|
| Level logika | `HIGH`, `LOW` | `1`, `0` |
| Mode pin | `INPUT`, `OUTPUT`, `INPUT_PULLUP`, `INPUT_PULLDOWN`, `OUTPUT_OPEN_DRAIN`, `ANALOG` | `0x01`, `0x03`, `0x05`, `0x09`, `0x13`, `0xC0` |
| Penyusun mode pin | `DISABLED`, `PULLUP`, `PULLDOWN`, `OPEN_DRAIN` | `0x00`, `0x04`, `0x08`, `0x10`. Bit penyusun yang digabung `|` dengan `INPUT`/`OUTPUT`; mis. `INPUT_PULLUP` = `INPUT | PULLUP`. Umumnya pakai konstanta gabungan di baris atas saja. |
| Pemicu interupsi | `RISING`, `FALLING`, `CHANGE`, `ONLOW`, `ONHIGH`, `ONLOW_WE`, `ONHIGH_WE` | `1`, `2`, `3`, `4`, `5`, … |
| Nilai matematika | `PI`, `HALF_PI`, `TWO_PI`, `DEG_TO_RAD`, `RAD_TO_DEG`, `EULER` | Sama seperti Arduino |
| Urutan bit | `LSBFIRST`, `MSBFIRST` | `0`, `1` |
| Redaman ADC | `ADC_0db`, `ADC_2_5db`, `ADC_6db`, `ADC_11db` | `0`..`3` |
| Papan | `LED_BUILTIN`, `BUILTIN_LED`, `BOOT_BUTTON`, `TX`, `RX`, `SDA`, `SCL`, `MOSI`, `MISO`, `SCK`, `SS` | Nomor GPIO sesuai papan aktif |
| Pin sablon | `D0`, `D2`, `D4`, … | Sesuai sablon devkit. Pada ESP32 DevKit V1 **tidak ada `D1` dan `D3`** — sablon papan menamainya `TX0` dan `RX0` karena dipakai konsol serial. |
| Semua GPIO | `GPIO0` … `GPIO48` | Hanya GPIO yang benar-benar ada pada chip |
| Pin ADC | `A0`, `A3`, `A4`, … | ESP32: `A0`=36, `A3`=39, `A4`=32, `A5`=33, `A6`=34, `A7`=35, `A10`=4, `A11`=0, `A12`=2, `A13`=15, `A14`=13, `A15`=12, `A16`=14, `A17`=27, `A18`=25, `A19`=26. **`A1` dan `A2` tidak ada** di ESP32: GPIO37/38 tidak dibonding pada modul WROOM-32. ESP32-S3: `A0`=1 … `A9`=10 (ADC1). |
| Pin sentuh | `T0`..`T9` (ESP32), `T1`..`T14` (ESP32-S3) | Nomor GPIO pin sentuh |
| Bangun dari sleep | `WAKEUP_ALL_LOW`, `WAKEUP_ANY_HIGH` | Mode `ext1` |
| Kapabilitas pin | `CAP_DIGITAL`, `CAP_INPUT`, `CAP_OUTPUT`, `CAP_PULL`, `CAP_ADC1`, `CAP_ADC2`, `CAP_DAC`, `CAP_TOUCH`, `CAP_RTC`, `CAP_PWM`, `CAP_STRAP`, `CAP_FLASH`, `CAP_USB`, `CAP_UART0`, `CAP_I2C`, `CAP_SPI`, `CAP_BOOT`, `CAP_LED` | Bitmask satu-bit, dipakai `pinPunya()` dan `daftarPin()` |
| Kapabilitas gabungan | `CAP_ADC` = `CAP_ADC1｜CAP_ADC2` (ADC unit mana pun), `CAP_IO` = `CAP_DIGITAL｜CAP_INPUT｜CAP_OUTPUT` (GPIO serbaguna penuh) | Pintasan untuk kombinasi yang sering dipakai |
| Status WiFi | `WL_NO_SHIELD`(255), `WL_IDLE_STATUS`(0), `WL_NO_SSID_AVAIL`(1), `WL_SCAN_COMPLETED`(2), `WL_CONNECTED`(3), `WL_CONNECT_FAILED`(4), `WL_CONNECTION_LOST`(5), `WL_DISCONNECTED`(6) | Lihat [6.1](#61-objek-wifi) |
| Mode WiFi | `WIFI_STA`, `WIFI_AP`, `WIFI_AP_STA`, `WIFI_OFF` | Lihat [6.1](#61-objek-wifi) |
| Mode SPI | `SPI_MODE0` … `SPI_MODE3`, `VSPI`, `HSPI`, `FSPI` | Lihat [5.2](#52-spi--objek-spi) |
| LED bawaan | `RGB_BUILTIN` | Nomor GPIO LED RGB pintar (WS2812) bawaan papan, bila ada. Lihat [3.2](#32-led-bawaan-dan-led-rgb-ws2812). |
| Identitas | `BOARD`, `CHIP` | String, mis. `"esp32doit-devkit-v1"` dan `"ESP32-D0WD"` |

> ### ⚠ Jebakan nomor satu bagi pengguna ESP32-S3: `LED_BUILTIN`
>
> Di ESP32-S3-DevKitC-1, `LED_BUILTIN` bernilai **48**, tetapi LED di pin itu
> **bukan LED biasa** — ia sebuah **WS2812** (LED RGB "pintar") yang dikendalikan
> lewat protokol satu kabel, bukan lewat level logika. Akibatnya:
>
> ```baik
> digitalWrite(LED_BUILTIN, HIGH);   // DI ESP32 menyala; DI ESP32-S3 TIDAK terjadi apa-apa
> ```
>
> Kode itu tidak memberi galat — LED-nya hanya diam, sehingga mudah disangka
> papan atau skripnya rusak. Yang benar di ESP32-S3:
>
> ```baik
> ledBawaan(benar);                  // cara portabel: jalan di KEDUA papan
> rgbLedWrite(LED_BUILTIN, 16, 16, 16);  // atau kendalikan WS2812 langsung
> ```
>
> **Saran:** pakai `ledBawaan()` di skrip apa pun yang ingin jalan di kedua
> papan. Lihat [3.2](#32-led-bawaan-dan-led-rgb-ws2812).

```baik
tulis("Papan:", BOARD, "Chip:", CHIP);
tulis("LED bawaan ada di GPIO", LED_BUILTIN);
pinMode(LED_BUILTIN, OUTPUT);
```

### 2.2 Fungsi introspeksi pinout

Kelima fungsi berikut membaca tabel pin bawaan firmware, sehingga jawabannya
selalu cocok dengan papan yang sedang dipakai.

---

#### `pinInfo(gpio)`

Kembalikan seluruh keterangan satu pin.

| Parameter | Tipe | Arti |
|---|---|---|
| `gpio` | angka | Nomor GPIO yang ditanyakan |

**Nilai balik:** objek, atau `kosong` bila GPIO itu tidak ada pada chip ini.

| Field | Tipe | Arti |
|---|---|---|
| `gpio` | angka | Nomor GPIO |
| `label` | huruf | Label sablon pada papan, mis. `"D23"`, `"VP"`, `"TX0"` |
| `adc.unit` | angka | Unit ADC (`1` atau `2`), atau `-1` bila pin bukan ADC |
| `adc.channel` | angka | Nomor channel ADC, atau `-1` |
| `touch` | angka | Nomor channel sentuh (`T0`..`T14`), atau `-1` |
| `dac` | angka | Channel DAC (`1` atau `2`), atau `-1` (selalu `-1` di ESP32-S3) |
| `rtc` | angka | Nomor RTC GPIO, atau `-1` bila pin tidak bisa membangunkan dari deep sleep |
| `caps` | untaian | Daftar nama kapabilitas, mis. `["DIGITAL","INPUT","OUTPUT","ADC1"]` |
| `capsBit` | angka | Kapabilitas yang sama dalam bentuk bitmask, sama dengan `pinCaps(gpio)` |
| `note` | huruf | Catatan atau peringatan pemakaian; string kosong bila tidak ada |

**Alias:** — · **Padanan Arduino:** tidak ada (khas BAIK).

```baik
isi info = pinInfo(34);
jika (info !== kosong) {
  tulis("Label:", info.label, "ADC unit:", info.adc.unit);
  tulis("Kapabilitas:", JSON.stringify(info.caps));
  tulis("Catatan:", info.note);
}
```

---

#### `pinCaps(gpio)`

Kembalikan bitmask kapabilitas mentah sebuah pin.

| Parameter | Tipe | Arti |
|---|---|---|
| `gpio` | angka | Nomor GPIO |

**Nilai balik:** angka bitmask (gabungan `CAP_*`); `0` bila pin tidak ada.
**Alias:** — · **Padanan Arduino:** tidak ada.

```baik
isi m = pinCaps(25);
tulis("bitmask:", m, "punya DAC?", (m & CAP_DAC) !== 0);
```

---

#### `pinPunya(gpio, capBitmask)` — alias `pinHas`

Periksa apakah sebuah pin memiliki **semua** kapabilitas pada bitmask.

| Parameter | Tipe | Arti |
|---|---|---|
| `gpio` | angka | Nomor GPIO |
| `capBitmask` | angka | Satu `CAP_*` atau gabungannya dengan `|` |

**Nilai balik:** `benar` / `salah`.
**Alias:** `pinHas` · **Padanan Arduino:** tidak ada.
**Catatan papan:** `pinPunya(25, CAP_DAC)` bernilai `benar` di ESP32 dan
`salah` di ESP32-S3.

```baik
jika (pinPunya(4, CAP_TOUCH)) {
  tulis("GPIO4 bisa dipakai sensor sentuh");
}
```

---

#### `daftarPin([capBitmask])` — alias `pinList`

Kembalikan daftar GPIO yang punya kapabilitas tertentu.

| Parameter | Tipe | Arti |
|---|---|---|
| `capBitmask` | angka, opsional | Kapabilitas yang dicari. Bila dikosongkan, semua GPIO dikembalikan. |

**Nilai balik:** array berisi nomor GPIO (angka).
**Alias:** `pinList` · **Padanan Arduino:** tidak ada.

```baik
isi adc = daftarPin(CAP_ADC1);
untuk (isi i = 0; i < adc.panjang; i++) {
  tulis("ADC1 tersedia di GPIO", adc[i]);
}

// Kapabilitas gabungan: CAP_ADC = ADC unit mana pun,
// CAP_IO = GPIO serbaguna (digital + input + output).
tulis("pin ber-ADC apa pun :", daftarPin(CAP_ADC).panjang);
tulis("pin serbaguna penuh :", daftarPin(CAP_IO).panjang);
```

---

#### `pinout()`

Cetak tabel pinout lengkap papan aktif ke konsol (GPIO, label, kapabilitas,
catatan).

**Parameter:** tidak ada. **Nilai balik:** `takterdefinisi`.
**Alias:** — · **Padanan Arduino:** tidak ada.

```baik
pinout();
```

---

## 3. GPIO, Analog, dan PWM

Setiap fungsi yang menerima nomor pin memvalidasi pin itu lebih dulu. Bila pin
tidak ada, atau ada tetapi tidak mampu melakukan yang diminta, firmware mencetak
pesan berbahasa Indonesia yang menyebut nama fungsi dan pin alternatif, lalu
fungsi tersebut mengembalikan `takterdefinisi` tanpa menyentuh perangkat keras.

> **Pin SPI flash / PSRAM SELALU ditolak.** Pin yang bertanda `CAP_FLASH`
> — **GPIO6–GPIO11 di ESP32**, dan **GPIO26–GPIO32 di ESP32-S3** — dipakai chip
> untuk berbicara dengan flash dan PSRAM-nya sendiri. Menyentuhnya akan
> menggantung atau me-reset papan seketika, jadi **semua** fungsi pin menolaknya
> tanpa kecuali, apa pun yang kamu minta. Ini disengaja dan tidak bisa
> dilewati. Periksa daftarnya dengan `daftarPin(CAP_FLASH)`.

### 3.1 GPIO digital

---

#### `pinMode(pin, mode)` — alias `modePin`

Tentukan arah dan pull-resistor sebuah pin.

| Parameter | Tipe | Arti |
|---|---|---|
| `pin` | angka | Nomor GPIO |
| `mode` | angka | `INPUT`, `OUTPUT`, `INPUT_PULLUP`, `INPUT_PULLDOWN`, `OUTPUT_OPEN_DRAIN`, atau `ANALOG` |

**Nilai balik:** `takterdefinisi`.
**Alias:** `modePin` · **Padanan Arduino:** `pinMode()`.
**Catatan papan:** GPIO34–39 di ESP32 hanya bisa jadi input dan tidak punya
pull-resistor internal; `INPUT_PULLUP` pada pin itu akan ditolak.

```baik
pinMode(2, OUTPUT);
pinMode(0, INPUT_PULLUP);
```

---

#### `digitalWrite(pin, nilai)` — alias `tulisDigital`

Set level logika pin keluaran.

| Parameter | Tipe | Arti |
|---|---|---|
| `pin` | angka | Nomor GPIO (harus mampu OUTPUT) |
| `nilai` | angka | `HIGH` (1) atau `LOW` (0) |

**Nilai balik:** `takterdefinisi`.
**Alias:** `tulisDigital` · **Padanan Arduino:** `digitalWrite()`.

```baik
pinMode(LED_BUILTIN, OUTPUT);
digitalWrite(LED_BUILTIN, HIGH);
delay(300);
tulisDigital(LED_BUILTIN, LOW);
```

---

#### `digitalRead(pin)` — alias `bacaDigital`

Baca level logika pin masukan.

| Parameter | Tipe | Arti |
|---|---|---|
| `pin` | angka | Nomor GPIO (harus mampu INPUT) |

**Nilai balik:** `0` atau `1`; `takterdefinisi` bila pin tidak sah.
**Alias:** `bacaDigital` · **Padanan Arduino:** `digitalRead()`.

```baik
pinMode(0, INPUT_PULLUP);
jika (digitalRead(0) === LOW) {
  tulis("Tombol BOOT ditekan");
}
```

---

#### `digitalToggle(pin)`

Balik level pin keluaran: baca nilainya sekarang lalu tulis kebalikannya.

| Parameter | Tipe | Arti |
|---|---|---|
| `pin` | angka | Nomor GPIO (harus mampu OUTPUT) |

**Nilai balik:** level baru (`0`/`1`).
**Alias:** `balikDigital` · **Padanan Arduino:** tidak ada padanan langsung
(setara `digitalWrite(pin, !digitalRead(pin))`).

```baik
pinMode(2, OUTPUT);
untuk (isi i = 0; i < 6; i++) { digitalToggle(2); delay(200); }
```

### 3.2 LED bawaan dan LED RGB (WS2812)

Kedua papan punya LED bawaan, tetapi **jenisnya berbeda**: ESP32 DevKit V1
memakai LED biasa yang dikendalikan level logika, sedangkan ESP32-S3-DevKitC-1
memakai WS2812. Dua fungsi berikut menjembatani perbedaan itu.

---

#### `ledBawaan(nyala)` — alias `builtinLed`

Nyalakan atau matikan LED bawaan papan **dengan cara yang benar untuk papan itu**.
Di papan berLED biasa ia memakai `pinMode()` + `digitalWrite()`; di papan berLED
RGB ia memakai driver WS2812 dengan warna putih redup.

| Parameter | Tipe | Arti |
|---|---|---|
| `nyala` | boolean/angka | `benar`/`HIGH`/`1` menyalakan, `salah`/`LOW`/`0` mematikan |

**Nilai balik:** boolean status yang dipasang.
**Alias:** `builtinLed` · **Padanan Arduino:** **tidak ada** — ini fungsi
kenyamanan khas BAIK.
**Catatan papan:** inilah satu-satunya cara memakai LED bawaan yang dijamin
jalan di ESP32 **dan** ESP32-S3. Pakai ini di skrip yang ingin portabel.

```baik
untuk (isi i = 0; i < 6; i++) {
  ledBawaan(benar);
  delay(250);
  ledBawaan(salah);
  delay(250);
}
```

---

#### `rgbLedWrite(pin, r, g, b)` — alias `neopixelWrite`, `tulisLedRgb`

Kirim satu warna ke LED RGB pintar (WS2812 / NeoPixel).

| Parameter | Tipe | Arti |
|---|---|---|
| `pin` | angka | GPIO tempat LED tersambung (di ESP32-S3-DevKitC-1: `LED_BUILTIN` = 48) |
| `r`, `g`, `b` | angka | Komponen merah, hijau, biru, masing-masing `0`..`255` |

**Nilai balik:** `takterdefinisi`.
**Alias:** `neopixelWrite`, `tulisLedRgb` · **Padanan Arduino:**
`rgbLedWrite()` (arduino-esp32 3.x) / `neopixelWrite()` (2.x).
**Catatan papan:** ESP32-S3-DevKitC-1 punya WS2812 bawaan di GPIO48. ESP32
DevKit V1 **tidak punya** LED RGB bawaan, tetapi fungsi ini tetap bekerja pada
strip WS2812 eksternal yang kamu sambungkan sendiri ke GPIO mana pun yang mampu
OUTPUT.

> **Hati-hati soal kecerahan.** WS2812 sangat terang. `255,255,255` menyilaukan
> dan menarik arus cukup besar; untuk sekadar indikator, nilai `16` sudah lebih
> dari cukup.

```baik
rgbLedWrite(LED_BUILTIN, 32, 0, 0);   // merah redup
delay(400);
rgbLedWrite(LED_BUILTIN, 0, 32, 0);   // hijau redup
delay(400);
rgbLedWrite(LED_BUILTIN, 0, 0, 0);    // mati
```

### 3.3 ADC (analog masuk)

---

#### `analogRead(pin)` — alias `bacaAnalog`

Baca tegangan pada pin sebagai angka mentah.

| Parameter | Tipe | Arti |
|---|---|---|
| `pin` | angka | GPIO yang punya kapabilitas ADC |

**Nilai balik:** angka `0`..`4095` pada resolusi bawaan 12 bit.
**Alias:** `bacaAnalog` · **Padanan Arduino:** `analogRead()`.
**Catatan papan:** ADC2 tidak bisa dibaca sementara WiFi aktif — pada kedua
papan. Pakai pin ADC1 (`CAP_ADC1`) bila skrip juga memakai WiFi.

```baik
isi mentah = analogRead(34);
tulis("nilai ADC:", mentah);
```

---

#### `analogReadMilliVolts(pin)` — alias `bacaMiliVolt`

Baca ADC dan langsung ubah ke milivolt memakai kalibrasi pabrik eFuse.

| Parameter | Tipe | Arti |
|---|---|---|
| `pin` | angka | GPIO ber-ADC |

**Nilai balik:** angka milivolt.
**Alias:** `bacaMiliVolt` · **Padanan Arduino:** `analogReadMilliVolts()`.

```baik
tulis("tegangan:", analogReadMilliVolts(34), "mV");
```

---

#### `analogReadResolution(bits)`

Ubah lebar hasil `analogRead()` untuk semua pin.

| Parameter | Tipe | Arti |
|---|---|---|
| `bits` | angka | `9`..`12` |

**Nilai balik:** `takterdefinisi`. **Alias:** — ·
**Padanan Arduino:** `analogReadResolution()`.

```baik
analogReadResolution(10);
tulis("sekarang 0..1023:", analogRead(34));
```

---

#### `analogSetAttenuation(atten)`

Set redaman ADC global, yang menentukan jangkauan tegangan yang terukur.

| Parameter | Tipe | Arti |
|---|---|---|
| `atten` | angka | `ADC_0db` (±0,8 V), `ADC_2_5db`, `ADC_6db`, `ADC_11db` (±3,3 V, bawaan) |

**Nilai balik:** `takterdefinisi`. **Alias:** — ·
**Padanan Arduino:** `analogSetAttenuation()`.

```baik
analogSetAttenuation(ADC_11db);
```

---

#### `analogSetPinAttenuation(pin, atten)`

Sama seperti di atas, tetapi hanya untuk satu pin.

| Parameter | Tipe | Arti |
|---|---|---|
| `pin` | angka | GPIO ber-ADC |
| `atten` | angka | `ADC_0db`..`ADC_11db` |

**Nilai balik:** `takterdefinisi`. **Alias:** — ·
**Padanan Arduino:** `analogSetPinAttenuation()`.

```baik
analogSetPinAttenuation(35, ADC_6db);
tulis(analogReadMilliVolts(35));
```

### 3.4 PWM sederhana (analogWrite)

---

#### `analogWrite(pin, nilai[, max])` — alias `tulisAnalog`

PWM gaya Arduino. Kanal LEDC dialokasikan otomatis pada pemanggilan pertama.

| Parameter | Tipe | Arti |
|---|---|---|
| `pin` | angka | GPIO yang mampu PWM |
| `nilai` | angka | Duty `0`..`max` |
| `max` | angka, opsional | Nilai duty tertinggi, bawaan `255` |

**Nilai balik:** `takterdefinisi`.
**Alias:** `tulisAnalog` · **Padanan Arduino:** `analogWrite()`.
**Catatan papan:** jumlah pin PWM yang bisa aktif bersamaan dibatasi jumlah
kanal LEDC — 16 di ESP32, 8 di ESP32-S3.

```baik
analogWrite(2, 128);
delay(500);
tulisAnalog(2, 1023, 1023);
```

---

#### `analogWriteResolution(pin, bits)`

Ubah resolusi duty `analogWrite()` untuk satu pin.

| Parameter | Tipe | Arti |
|---|---|---|
| `pin` | angka | GPIO PWM |
| `bits` | angka | Lebar duty, lazimnya `8`..`14` |

**Nilai balik:** `takterdefinisi`. **Alias:** — ·
**Padanan Arduino:** `analogWriteResolution()`.

```baik
analogWriteResolution(2, 12);
analogWrite(2, 4095, 4095);
```

---

#### `analogWriteFrequency(pin, hz)`

Ubah frekuensi PWM untuk satu pin.

| Parameter | Tipe | Arti |
|---|---|---|
| `pin` | angka | GPIO PWM |
| `hz` | angka | Frekuensi dalam hertz |

**Nilai balik:** `takterdefinisi`. **Alias:** — ·
**Padanan Arduino:** `analogWriteFrequency()`.

```baik
analogWriteFrequency(2, 20000);
analogWrite(2, 100);
```

### 3.5 DAC (analog keluar)

---

#### `dacWrite(pin, nilai0_255)` — alias `tulisDac`

Keluarkan tegangan analog sungguhan dari DAC 8 bit.

| Parameter | Tipe | Arti |
|---|---|---|
| `pin` | angka | GPIO25 atau GPIO26 |
| `nilai0_255` | angka | `0` = 0 V, `255` ≈ 3,3 V |

**Nilai balik:** `takterdefinisi`.
**Alias:** `tulisDac` · **Padanan Arduino:** `dacWrite()`.
**Catatan papan:** **DAC tidak tersedia di ESP32-S3.** Pada papan itu fungsi
tetap terdaftar tetapi mencetak galat `DAC tidak tersedia pada ESP32-S3` dan
tidak melakukan apa pun.

```baik
jika (pinPunya(25, CAP_DAC)) {
  dacWrite(25, 128);
} lainnya {
  tulis("Papan ini tidak punya DAC");
}
```

---

#### `dacDisable(pin)`

Matikan keluaran DAC dan lepaskan pin.

| Parameter | Tipe | Arti |
|---|---|---|
| `pin` | angka | GPIO25 / GPIO26 |

**Nilai balik:** `takterdefinisi`. **Alias:** — ·
**Padanan Arduino:** `dacDisable()`. **Catatan papan:** ESP32 saja.

```baik
dacWrite(26, 200);
delay(500);
dacDisable(26);
```

### 3.6 Sensor sentuh

---

#### `touchRead(pin)` — alias `bacaSentuh`

Baca nilai sensor sentuh kapasitif.

| Parameter | Tipe | Arti |
|---|---|---|
| `pin` | angka | GPIO ber-`CAP_TOUCH`, atau konstanta `T0`..`T14` |

**Nilai balik:** angka mentah.
**Alias:** `bacaSentuh` · **Padanan Arduino:** `touchRead()`.
**Catatan papan:** arah nilainya berbeda! Di **ESP32** nilai **turun** saat
disentuh (ambang = "kurang dari"); di **ESP32-S3** nilai **naik** saat
disentuh (ambang = "lebih dari").

```baik
isi n = touchRead(T0);
tulis("sentuh:", n);
```

---

#### `touchSetCycles(ukur, tidur)`

Atur lamanya siklus pengukuran sensor sentuh (memengaruhi sensitivitas).

| Parameter | Tipe | Arti |
|---|---|---|
| `ukur` | angka | Siklus pengukuran |
| `tidur` | angka | Siklus jeda |

**Nilai balik:** `takterdefinisi`. **Alias:** — ·
**Padanan Arduino:** `touchSetCycles()`.

```baik
touchSetCycles(0x1000, 0x1000);
tulis(touchRead(T0));
```

---

#### `touchAttachInterrupt(pin, fn, ambang)` — alias `sentuhInterupsi`

Panggil fungsi BAIK ketika pembacaan sentuh melewati ambang.

| Parameter | Tipe | Arti |
|---|---|---|
| `pin` | angka | GPIO / `T0`..`T14` |
| `fn` | fungsi | Fungsi yang ditulis dengan kata kunci `fungsi`. **Fungsi bawaan ditolak.** |
| `ambang` | angka | Nilai ambang pemicu |

**Nilai balik:** `takterdefinisi`.
**Alias:** `sentuhInterupsi` · **Padanan Arduino:** `touchAttachInterrupt()`.
**Catatan penting:** `fn` **wajib** fungsi BAIK (ditulis dengan kata kunci
`fungsi`); fungsi bawaan ditolak. `fn` juga **tidak** dijalankan di dalam ISR —
interupsi hanya dicatat, lalu `fn` dipanggil saat `serviceInterrupts()`
berjalan. Lihat [3.9](#39-interupsi).

```baik
fungsi kena() { tulis("tersentuh!"); }
sentuhInterupsi(T0, kena, 40);
serviceInterrupts();
```

---

#### `touchDetachInterrupt(pin)`

Lepaskan interupsi sentuh dari sebuah pin.

| Parameter | Tipe | Arti |
|---|---|---|
| `pin` | angka | GPIO / `T0`..`T14` |

**Nilai balik:** `takterdefinisi`. **Alias:** — ·
**Padanan Arduino:** `touchDetachInterrupt()`.

```baik
touchDetachInterrupt(T0);
```

### 3.7 LEDC (PWM tingkat lanjut)

LEDC adalah pengendali PWM bawaan ESP32. Ada dua gaya API, dan keduanya
tersedia di BAIK apa pun versi arduino-esp32 yang dipakai: gaya **2.x**
(kanal dulu, lalu pin ditempelkan ke kanal) dan gaya **3.x** (langsung per pin).

---

#### `ledcSetup(kanal, freq, bits)`

Siapkan sebuah kanal LEDC.

| Parameter | Tipe | Arti |
|---|---|---|
| `kanal` | angka | `0`..`15` (ESP32) atau `0`..`7` (ESP32-S3) |
| `freq` | angka | Frekuensi hertz |
| `bits` | angka | Resolusi duty, `1`..`20` |

**Nilai balik:** frekuensi nyata yang berhasil dipasang (angka); `0` bila gagal.
**Alias:** — · **Padanan Arduino:** `ledcSetup()` di 2.x; pada 3.x diemulasi di
atas `ledcAttachChannel()`.

```baik
isi nyata = ledcSetup(0, 5000, 8);
tulis("frekuensi nyata:", nyata);
```

---

#### `ledcAttachPin(pin, kanal)`

Sambungkan sebuah GPIO ke kanal LEDC yang sudah disiapkan.

| Parameter | Tipe | Arti |
|---|---|---|
| `pin` | angka | GPIO mampu PWM |
| `kanal` | angka | Nomor kanal |

**Nilai balik:** `takterdefinisi`. **Alias:** — ·
**Padanan Arduino:** `ledcAttachPin()` (2.x) / `ledcAttachChannel()` (3.x).

```baik
ledcSetup(0, 5000, 8);
ledcAttachPin(2, 0);
ledcWrite(0, 200);
```

---

#### `ledcAttach(pin, freq, bits)`

Gaya 3.x: siapkan kanal **dan** tempelkan pin dalam satu panggilan.

| Parameter | Tipe | Arti |
|---|---|---|
| `pin` | angka | GPIO mampu PWM |
| `freq` | angka | Frekuensi hertz |
| `bits` | angka | Resolusi duty |

**Nilai balik:** `benar` / `salah`. **Alias:** — ·
**Padanan Arduino:** `ledcAttach()` (3.x); pada 2.x diemulasi dengan
`ledcSetup()` + `ledcAttachPin()`.

```baik
ledcAttach(4, 1000, 10);
ledcWrite(4, 512);
```

---

#### `ledcWrite(kanal_atau_pin, duty)` — alias `tulisPwm`

Set duty cycle. Argumen pertama boleh nomor kanal (gaya 2.x) atau nomor pin
(gaya 3.x); firmware mengenali keduanya.

> **Bagaimana angka itu ditafsirkan.** Nomor pin dan nomor kanal sama-sama
> bilangan bulat kecil, jadi `ledcWrite(2, 128)` bisa berarti "pin 2" atau
> "kanal 2". Firmware memutuskannya dengan urutan tetap berikut:
>
> 1. Bila angkanya adalah **pin yang sedang terpasang** ke LEDC → dianggap pin (gaya 3.x).
> 2. Bila bukan, dan angkanya adalah **kanal yang sudah dikonfigurasi** → dianggap kanal (gaya 2.x).
> 3. Bila bukan keduanya → dianggap pin mentah.
>
> Urutan ini membuat dua idiom paling umum tetap benar. Tetapi bila nomor kanal
> kebetulan **sama** dengan nomor pin yang sedang terpasang, aturan 1 menang dan
> yang dimaksud adalah pinnya. Untuk menghindari kebingungan, **pilih satu gaya
> dan konsisten** dalam satu skrip — gaya pin (`ledcAttach` + `ledcWrite(pin, …)`)
> adalah yang paling tidak ambigu.

| Parameter | Tipe | Arti |
|---|---|---|
| `kanal_atau_pin` | angka | Kanal LEDC atau GPIO yang sudah di-`ledcAttach` |
| `duty` | angka | `0` .. `2^bits - 1` |

**Nilai balik:** `takterdefinisi`.
**Alias:** `tulisPwm` · **Padanan Arduino:** `ledcWrite()`.

```baik
ledcAttach(2, 5000, 8);
untuk (isi d = 0; d <= 255; d += 15) { tulisPwm(2, d); delay(30); }
```

---

#### `ledcRead(kanal_atau_pin)`

Baca duty yang sedang berlaku.

| Parameter | Tipe | Arti |
|---|---|---|
| `kanal_atau_pin` | angka | Kanal atau GPIO |

**Nilai balik:** angka duty. **Alias:** — ·
**Padanan Arduino:** `ledcRead()`.

```baik
ledcWrite(2, 77);
tulis("duty sekarang:", ledcRead(2));
```

---

#### `ledcReadFreq(kanal_atau_pin)`

Baca frekuensi yang sedang berlaku.

| Parameter | Tipe | Arti |
|---|---|---|
| `kanal_atau_pin` | angka | Kanal atau GPIO |

**Nilai balik:** angka hertz. **Alias:** — ·
**Padanan Arduino:** `ledcReadFreq()`.

```baik
tulis("Hz:", ledcReadFreq(2));
```

---

#### `ledcWriteTone(kanal, freq)`

Keluarkan gelombang persegi duty 50 % pada frekuensi tertentu.

| Parameter | Tipe | Arti |
|---|---|---|
| `kanal` | angka | Kanal (atau pin) LEDC |
| `freq` | angka | Hertz; `0` mematikan nada |

**Nilai balik:** frekuensi nyata (angka). **Alias:** — ·
**Padanan Arduino:** `ledcWriteTone()`.

```baik
ledcAttach(15, 1000, 10);
ledcWriteTone(15, 440);
delay(400);
ledcWriteTone(15, 0);
```

---

#### `ledcWriteNote(kanal, nada, oktaf)`

Mainkan nada musik berdasarkan namanya.

| Parameter | Tipe | Arti |
|---|---|---|
| `kanal` | angka | Kanal (atau pin) LEDC |
| `nada` | string | `"C"`, `"C#"`, `"D"`, `"D#"`, `"E"`, `"F"`, `"F#"`, `"G"`, `"G#"`, `"A"`, `"A#"`, `"B"` |
| `oktaf` | angka | Nomor oktaf, mis. `4` |

**Nilai balik:** frekuensi nyata (angka). **Alias:** — ·
**Padanan Arduino:** `ledcWriteNote()`.

```baik
ledcAttach(15, 1000, 10);
ledcWriteNote(15, "A", 4);
delay(500);
ledcWriteTone(15, 0);
```

---

#### `ledcAttachChannel(pin, freq, bits, kanal)`

Gaya 3.x dengan kanal yang **dipilih sendiri**, bukan dialokasikan otomatis.

| Parameter | Tipe | Arti |
|---|---|---|
| `pin` | angka | GPIO mampu PWM |
| `freq` | angka | Frekuensi hertz |
| `bits` | angka | Resolusi duty |
| `kanal` | angka | Nomor kanal yang diminta (`0`..`15` ESP32 / `0`..`7` S3) |

**Nilai balik:** `benar` / `salah`. **Alias:** — ·
**Padanan Arduino:** `ledcAttachChannel()` (3.x).
**Catatan:** berguna bila beberapa pin harus berbagi satu timer agar
frekuensinya terkunci sama, mis. menggerakkan motor dua arah.

```baik
ledcAttachChannel(18, 1000, 10, 0);
ledcAttachChannel(19, 1000, 10, 0);   // kanal yang sama -> frekuensi terkunci
```

---

#### `ledcChangeFrequency(kanal_atau_pin, freq, bits)`

Ubah frekuensi (dan resolusi) LEDC yang sudah berjalan, tanpa melepas pin.

| Parameter | Tipe | Arti |
|---|---|---|
| `kanal_atau_pin` | angka | Ditafsirkan dengan urutan yang sama seperti `ledcWrite` |
| `freq` | angka | Frekuensi baru dalam hertz |
| `bits` | angka | Resolusi duty baru |

**Nilai balik:** frekuensi nyata yang tercapai (angka); `0` bila gagal.
**Alias:** — · **Padanan Arduino:** `ledcChangeFrequency()` (3.x).

```baik
ledcAttach(2, 1000, 10);
tulis("nyata:", ledcChangeFrequency(2, 20000, 8));
```

---

#### `ledcDetach(pin)` dan `ledcDetachPin(pin)`

Lepaskan pin dari LEDC dan kembalikan ke GPIO biasa.

| Parameter | Tipe | Arti |
|---|---|---|
| `pin` | angka | GPIO |

Kedua nama melakukan hal yang sama dan keduanya tersedia apa pun versi
arduino-esp32 yang dipakai.

| Parameter | Tipe | Arti |
|---|---|---|
| `pin` | angka | GPIO yang akan dilepas dari LEDC |

**Nilai balik:** `takterdefinisi`. **Alias:** — ·
**Padanan Arduino:** `ledcDetachPin()` (2.x) / `ledcDetach()` (3.x).

```baik
ledcDetach(2);      // sama saja dengan ledcDetachPin(2)
pinMode(2, OUTPUT); // pin kembali bisa dipakai digitalWrite
```

### 3.8 Nada / buzzer

---

#### `tone(pin, freq[, durasi])` — alias `bunyi`

Bunyikan buzzer pasif pada frekuensi tertentu.

| Parameter | Tipe | Arti |
|---|---|---|
| `pin` | angka | GPIO mampu PWM |
| `freq` | angka | Hertz |
| `durasi` | angka, opsional | Milidetik; bila dikosongkan nada terus berbunyi sampai `noTone()` |

**Nilai balik:** `takterdefinisi`.
**Alias:** `bunyi` · **Padanan Arduino:** `tone()`.

```baik
bunyi(15, 880, 200);
delay(250);
bunyi(15, 660, 200);
```

---

#### `noTone(pin)` — alias `diam`

Hentikan nada pada sebuah pin.

| Parameter | Tipe | Arti |
|---|---|---|
| `pin` | angka | GPIO yang sedang berbunyi |

**Nilai balik:** `takterdefinisi`.
**Alias:** `diam` · **Padanan Arduino:** `noTone()`.

```baik
tone(15, 440);
delay(300);
diam(15);
```

### 3.9 Interupsi

> **Aturan emas 1 — callback WAJIB fungsi BAIK.** Callback yang diberikan ke
> `attachInterrupt()` dan `touchAttachInterrupt()` harus fungsi yang kamu tulis
> sendiri dengan kata kunci `fungsi`. **Fungsi bawaan tidak boleh**, karena
> interpreter tidak bisa memanggil fungsi native lewat jalur ini dan papan akan
> reset tanpa pesan apa pun. Firmware karena itu menolaknya lebih dulu dengan
> pesan yang jelas:
>
> ```baik
> pasangInterupsi(0, digitalToggle, RISING);   // DITOLAK: itu fungsi bawaan
> fungsi kedip(pin) { digitalToggle(2); }
> pasangInterupsi(0, kedip, RISING);           // BENAR
> ```
>
> Kalau yang kamu inginkan memang menjalankan fungsi bawaan, bungkus saja di
> dalam `fungsi` seperti contoh di atas.

> **Aturan emas 2 — penanganan bersifat TERTUNDA, bukan real-time.**
> Interpreter BAIK tidak reentrant, jadi callback **tidak pernah** dijalankan
> dari dalam ISR. Yang sebenarnya terjadi:
>
> 1. ISR (kode C) hanya menaikkan pencacah per pin. Sangat cepat, tanpa alokasi.
> 2. Callback BAIK baru dipanggil belakangan dari loop REPL, atau saat skrip
>    memanggil `serviceInterrupts()` sendiri.
>
> Konsekuensi yang harus kamu terima:
>
> - **Jangan mengharapkan respons berskala mikrodetik.** Jeda antara pinggiran
>   sinyal dan jalannya callback bergantung pada seberapa sering
>   `serviceInterrupts()` sempat jalan — lazimnya orde milidetik, dan jauh lebih
>   lama bila skrip sedang sibuk atau tertahan di `delay()` yang panjang.
> - **Paling banyak 8 interupsi per pin dilayani tiap putaran.** Bila sebuah pin
>   memicu lebih cepat daripada yang sempat dilayani, kelebihannya menumpuk di
>   pencacah dan dilayani pada putaran berikutnya. Sinyal yang sangat cepat
>   (mis. pulsa enkoder putaran tinggi) **akan** kehilangan hitungan.
> - Untuk mengukur waktu secara presisi, pakai `pulseIn()` yang mengukur di
>   dalam kode C, bukan interupsi + callback BAIK.

---

#### `attachInterrupt(pin, fnBAIK, mode)` — alias `pasangInterupsi`

Daftarkan fungsi BAIK sebagai penangan interupsi sebuah pin.

| Parameter | Tipe | Arti |
|---|---|---|
| `pin` | angka | GPIO mampu INPUT |
| `fnBAIK` | fungsi | Fungsi yang ditulis dengan kata kunci `fungsi`, menerima nomor pin sebagai argumen. **Fungsi bawaan ditolak.** |
| `mode` | angka | `RISING`, `FALLING`, `CHANGE`, `ONLOW`, `ONHIGH` |

**Nilai balik:** `takterdefinisi`.
**Alias:** `pasangInterupsi` · **Padanan Arduino:** `attachInterrupt()`.

```baik
fungsi ditekan(pin) { tulis("tombol di GPIO", pin, "ditekan"); }
pinMode(0, INPUT_PULLUP);
pasangInterupsi(0, ditekan, FALLING);
serviceInterrupts();
```

---

#### `detachInterrupt(pin)` — alias `lepasInterupsi`

Cabut penangan interupsi sebuah pin.

| Parameter | Tipe | Arti |
|---|---|---|
| `pin` | angka | GPIO |

**Nilai balik:** `takterdefinisi`.
**Alias:** `lepasInterupsi` · **Padanan Arduino:** `detachInterrupt()`.

```baik
lepasInterupsi(0);
```

---

#### `interruptCount(pin)`

Berapa interupsi pin itu yang tercatat tetapi callback-nya belum dijalankan.

| Parameter | Tipe | Arti |
|---|---|---|
| `pin` | angka | GPIO |

**Nilai balik:** angka. **Alias:** `jumlahInterupsi` ·
**Padanan Arduino:** tidak ada.

```baik
tulis("tertahan:", interruptCount(0));
```

---

#### `serviceInterrupts()`

Jalankan sekarang semua callback interupsi yang tertahan.

**Parameter:** tidak ada. **Nilai balik:** angka callback yang dijalankan.
**Alias:** `layaniInterupsi` · **Padanan Arduino:** tidak ada (khas BAIK).
**Catatan:** melayani paling banyak **8 interupsi tertahan per pin** dalam satu
panggilan; sisanya menunggu panggilan berikutnya.

```baik
untuk (isi i = 0; i < 100; i++) { serviceInterrupts(); delay(50); }
```

---

#### `interrupts()` dan `noInterrupts()`

Aktifkan / matikan interupsi global. Pakai berpasangan dan sesingkat mungkin.

**Parameter:** tidak ada. **Nilai balik:** `takterdefinisi`. **Alias:** — ·
**Padanan Arduino:** `interrupts()` / `noInterrupts()` (`sei`/`cli`).

```baik
noInterrupts();
isi salinan = interruptCount(0);
interrupts();
tulis(salinan);
```

#### `hallRead()` — alias `bacaHall`

Baca sensor efek Hall yang tertanam di dalam chip ESP32 klasik.

**Parameter:** tidak ada. **Nilai balik:** angka mentah (tanpa satuan; positif
atau negatif bergantung arah medan magnet).
**Alias:** `bacaHall` · **Padanan Arduino:** `hallRead()`.

> **Ketersediaan sangat terbatas.** Fungsi ini **hanya** bekerja di **ESP32
> klasik yang dibangun dengan arduino-esp32 2.x**. Di luar itu ia tetap
> terdaftar tetapi mencetak galat dan mengembalikan `takterdefinisi`:
> - **ESP32-S3 tidak punya sensor Hall sama sekali.**
> - **arduino-esp32 3.x sudah menghapus `hallRead()`** dari core-nya, bahkan
>   untuk ESP32 klasik.
>
> Sensornya juga sangat tidak peka dan ikut terpengaruh suhu — hanya cocok untuk
> percobaan, bukan pengukuran sungguhan.

```baik
isi h = hallRead();
jika (h !== takterdefinisi) { tulis("sensor Hall:", h); }
```

---

### 3.10 Pewaktuan pulsa dan shift register

---

#### `pulseIn(pin, nilai[, timeoutUs])` — alias `bacaPulsa`

Ukur panjang satu pulsa. Berguna untuk sensor jarak ultrasonik.

| Parameter | Tipe | Arti |
|---|---|---|
| `pin` | angka | GPIO mampu INPUT |
| `nilai` | angka | `HIGH` atau `LOW` — level pulsa yang diukur |
| `timeoutUs` | angka, opsional | Batas tunggu mikrodetik, bawaan 1 000 000 |

**Nilai balik:** panjang pulsa dalam mikrodetik; `0` bila kehabisan waktu.
**Alias:** `bacaPulsa` · **Padanan Arduino:** `pulseIn()`.

```baik
pinMode(18, INPUT);
isi us = bacaPulsa(18, HIGH, 30000);
tulis("jarak kira-kira", us / 58, "cm");
```

---

#### `shiftOut(dataPin, clockPin, urutan, nilai)`

Geser keluar satu byte, bit demi bit (mis. ke 74HC595).

| Parameter | Tipe | Arti |
|---|---|---|
| `dataPin` | angka | GPIO data (OUTPUT) |
| `clockPin` | angka | GPIO clock (OUTPUT) |
| `urutan` | angka | `MSBFIRST` atau `LSBFIRST` |
| `nilai` | angka | Byte `0`..`255` |

**Nilai balik:** `takterdefinisi`. **Alias:** — ·
**Padanan Arduino:** `shiftOut()`.

```baik
pinMode(13, OUTPUT); pinMode(14, OUTPUT);
shiftOut(13, 14, MSBFIRST, 0xA5);
```

---

#### `shiftIn(dataPin, clockPin, urutan)`

Geser masuk satu byte, bit demi bit (mis. dari 74HC165).

| Parameter | Tipe | Arti |
|---|---|---|
| `dataPin` | angka | GPIO data (INPUT) |
| `clockPin` | angka | GPIO clock (OUTPUT) |
| `urutan` | angka | `MSBFIRST` atau `LSBFIRST` |

**Nilai balik:** byte `0`..`255`. **Alias:** — ·
**Padanan Arduino:** `shiftIn()`.

```baik
pinMode(13, INPUT); pinMode(14, OUTPUT);
tulis("byte masuk:", shiftIn(13, 14, MSBFIRST));
```

---

## 4. Sistem dan waktu

### 4.1 Tunda dan pencacah waktu

---

#### `delay(ms)` — alias `tunggu`

Tunda eksekusi. Diterapkan lewat `vTaskDelay`, jadi task FreeRTOS lain
(WiFi, web server) tetap berjalan selama papan menunggu.

| Parameter | Tipe | Arti |
|---|---|---|
| `ms` | angka | Milidetik |

**Nilai balik:** `takterdefinisi`.
**Alias:** `tunggu` · **Padanan Arduino:** `delay()`.

```baik
tulis("mulai");
tunggu(1000);
tulis("satu detik kemudian");
```

---

#### `delayMicroseconds(us)` — alias `tungguMikro`

Tunda singkat dengan busy-wait. **Memblokir CPU**, jadi jangan lebih dari
beberapa milidetik.

| Parameter | Tipe | Arti |
|---|---|---|
| `us` | angka | Mikrodetik |

**Nilai balik:** `takterdefinisi`.
**Alias:** `tungguMikro` · **Padanan Arduino:** `delayMicroseconds()`.

```baik
digitalWrite(5, HIGH);
tungguMikro(10);
digitalWrite(5, LOW);
```

---

#### `millis()` — alias `milidetik`

Waktu sejak papan menyala, dalam milidetik.

**Parameter:** tidak ada. **Nilai balik:** angka.
**Alias:** `milidetik` · **Padanan Arduino:** `millis()`.
**Beda dari Arduino:** pencacahnya **64 bit**, jadi **tidak melilit** setelah
~49 hari seperti `millis()` Arduino yang 32 bit. Perbandingan `millis() - t0`
aman dipakai berapa lama pun papan menyala.

```baik
isi t0 = millis();
delay(250);
tulis("selisih:", millis() - t0, "ms");
```

---

#### `micros()` — alias `mikrodetik`

Waktu sejak papan menyala, dalam mikrodetik.

**Parameter:** tidak ada. **Nilai balik:** angka.
**Alias:** `mikrodetik` · **Padanan Arduino:** `micros()`.
**Beda dari Arduino:** **64 bit**, jadi tidak melilit setelah ~71 menit seperti
`micros()` Arduino. Perlu diingat semua angka BAIK adalah `double`, sehingga
ketelitian bilangan bulat terjamin sampai 2^53 mikrodetik (sekitar 285 tahun).

```baik
isi a = micros();
isi b = micros();
tulis("biaya satu panggilan:", b - a, "us");
```

---

#### `yield()`

Serahkan giliran CPU ke task lain tanpa menunda.

**Parameter:** tidak ada. **Nilai balik:** `takterdefinisi`.
**Alias:** — · **Padanan Arduino:** `yield()`.

```baik
untuk (isi i = 0; i < 10000; i++) { yield(); }
```

### 4.2 Matematika dan bilangan acak

---

#### `random(maks)` / `random(min, maks)` — alias `acak`

Bilangan bulat acak dari pembangkit acak perangkat keras (`esp_random`).

| Parameter | Tipe | Arti |
|---|---|---|
| `maks` | angka | Bentuk satu argumen: hasil `0` .. `maks-1` |
| `min`, `maks` | angka | Bentuk dua argumen: hasil `min` .. `maks-1` |

**Nilai balik:** angka bulat.
**Alias:** `acak` · **Padanan Arduino:** `random()`.

```baik
tulis("dadu:", acak(1, 7));
tulis("0..99:", random(100));
```

---

#### `randomSeed(n)`

Set benih pembangkit acak perangkat lunak.

| Parameter | Tipe | Arti |
|---|---|---|
| `n` | angka | Benih |

**Nilai balik:** `takterdefinisi`. **Alias:** — ·
**Padanan Arduino:** `randomSeed()`.

```baik
randomSeed(millis());
tulis(random(10));
```

---

#### `map(x, in_min, in_max, out_min, out_max)` — alias `peta`

Petakan sebuah nilai dari satu jangkauan ke jangkauan lain.

| Parameter | Tipe | Arti |
|---|---|---|
| `x` | angka | Nilai masukan |
| `in_min`, `in_max` | angka | Jangkauan asal |
| `out_min`, `out_max` | angka | Jangkauan tujuan |

**Nilai balik:** angka hasil pemetaan.
**Alias:** `peta` · **Padanan Arduino:** `map()`.

> **Beda penting dari Arduino.** `map()` di BAIK memakai aritmetika **pecahan
> (double)**, bukan bilangan bulat seperti Arduino yang memotong sisa bagi.
> Jadi:
>
> ```baik
> tulis(map(5, 0, 9, 0, 100));   // BAIK   -> 55.5556
> // Arduino C++ untuk masukan yang sama -> 55
> ```
>
> Ini biasanya **lebih** akurat, tetapi bila kamu butuh bilangan bulat — mis.
> untuk duty PWM — bulatkan sendiri: `round(map(...))`.
> Bila `in_min === in_max`, fungsi mencetak galat dan mengembalikan `out_min`.

```baik
isi mentah = analogRead(34);
tulis("persen:", peta(mentah, 0, 4095, 0, 100));
```

---

#### `constrain(x, a, b)` — alias `batas`

Jepit nilai supaya tidak keluar dari jangkauan `a`..`b`.

| Parameter | Tipe | Arti |
|---|---|---|
| `x` | angka | Nilai |
| `a`, `b` | angka | Batas bawah dan batas atas |

**Nilai balik:** angka.
**Alias:** `batas` · **Padanan Arduino:** `constrain()`.

```baik
tulis(batas(300, 0, 255));
```

---

#### Fungsi matematika dasar

| Fungsi | Arti | Catatan |
|---|---|---|
| `min(a, b)`, `max(a, b)` | Nilai terkecil / terbesar dari dua angka | |
| `abs(x)` | Nilai mutlak | |
| `pow(a, b)` | `a` pangkat `b` | |
| `sqrt(x)` | Akar kuadrat | |
| `sin(x)`, `cos(x)`, `tan(x)` | Fungsi trigonometri | Sudut dalam **radian**. Ubah dari derajat dengan `x * DEG_TO_RAD`. |
| `atan(x)` | Arkus tangen satu argumen | Hasil dalam radian, jangkauan −π/2 … π/2 |
| `atan2(y, x)` | Arkus tangen dua argumen | Hasil −π … π, **kuadrannya benar**. Perhatikan urutannya: `y` dulu, baru `x`. Inilah yang dipakai untuk mencari sudut sebuah vektor. |
| `floor(x)`, `ceil(x)` | Pembulatan ke bawah / ke atas | |
| `round(x)` | Pembulatan ke bilangan terdekat | Setengah dibulatkan **menjauhi nol**: `round(-2.5)` = `-3` |
| `log(x)` | Logaritma natural (basis e) | |
| `log10(x)` | Logaritma basis 10 | Mengembalikan `takterdefinisi` bila `x <= 0` |
| `exp(x)` | e pangkat `x` | |

Semuanya menerima dan mengembalikan angka.

**Alias:** — · **Padanan Arduino / C:** nama yang sama.
**Catatan:** BAIK **belum punya objek `Math`**, jadi tulis `sqrt(2)`, bukan
`Math.sqrt(2)`.

```baik
tulis(sqrt(2), pow(2, 10), round(3.6));
tulis(sin(PI / 2), max(3, 9), abs(-4));

// atan2 memberi sudut sebuah vektor dengan kuadran yang benar
isi sudut = atan2(1, -1) * RAD_TO_DEG;
tulis("sudut:", sudut, "derajat");         // 135, bukan -45

// log10 berguna untuk desibel
tulis("dB:", 20 * log10(2));
```

### 4.3 Objek `Serial`

Objek `Serial` adalah **UART0 — port yang sama tempat REPL BAIK hidup**. Apa pun
yang dicetak akan muncul bercampur dengan prompt `Baik> `.

> ### `Serial.begin()` dan `Serial.end()` sengaja tidak melakukan apa-apa
>
> Keduanya **tidak menyentuh perangkat keras**; keduanya hanya mencetak catatan
> penjelas lalu mengembalikan `takterdefinisi`. Ini disengaja:
>
> - `Serial.begin(9600)` dari skrip akan mengubah baud rate **di bawah kaki
>   konsolnya sendiri**, dan terminalmu langsung berubah jadi sampah.
> - `Serial.end()` akan **mematikan satu-satunya jalur** yang kamu pakai untuk
>   mengetik. Papan tampak menggantung dan hanya pulih dengan menekan reset.
>
> Namanya tetap disediakan supaya kode yang disalin dari tutorial Arduino tidak
> langsung galat.
>
> **Di BAIK, `Serial.begin()` memang tidak diperlukan.** Firmware sudah membuka
> konsol pada **115200 baud** sebelum skrip mana pun berjalan — begitu skripmu
> jalan, `Serial` sudah siap pakai. Untuk berbicara dengan UART lain, pakai
> [`Serial1` / `Serial2`](#53-uart--objek-serial1--serial2), yang tidak dipakai
> konsol dan `begin()`-nya berfungsi penuh.
>
> Sisa fungsinya — `print`, `println`, `printf`, `available`, `read`,
> `readString`, `write`, `flush` — **bekerja normal**.

| Fungsi | Parameter | Nilai balik | Padanan Arduino |
|---|---|---|---|
| `Serial.begin(baud)` | `baud` angka | `takterdefinisi` — **diabaikan**, hanya mencetak catatan | `Serial.begin()` |
| `Serial.end()` | — | `takterdefinisi` — **diabaikan**, hanya mencetak catatan | `Serial.end()` |
| `Serial.print(x)` | `x` string/angka | `takterdefinisi` | `Serial.print()` |
| `Serial.println(x)` | `x` string/angka | `takterdefinisi` | `Serial.println()` |
| `Serial.printf(fmt, ...)` | `fmt` string format gaya C | `takterdefinisi` | `Serial.printf()` |
| `Serial.available()` | — | angka byte tersedia | `Serial.available()` |
| `Serial.read()` | — | angka byte, `-1` bila kosong | `Serial.read()` |
| `Serial.readString()` | — | string | `Serial.readString()` |
| `Serial.write(b)` | `b` angka byte | angka byte tertulis | `Serial.write()` |
| `Serial.flush()` | — | `takterdefinisi` | `Serial.flush()` |

**Alias:** — · **Catatan:** untuk mencetak dari skrip, `tulis()` lebih praktis
karena menerima banyak argumen dan tidak menuntut konversi tipe eksplisit.

```baik
// Tidak perlu Serial.begin() - konsol sudah terbuka pada 115200.
Serial.printf("suhu %d C\n", 30);
jika (Serial.available() > 0) {
  tulis("masuk:", Serial.readString());
}
```

### 4.4 Objek `ESP` (informasi chip)

Semua fungsi di bawah tanpa parameter kecuali disebutkan lain.

| Fungsi | Nilai balik | Arti |
|---|---|---|
| `ESP.restart()` | tidak pernah kembali | Boot ulang papan. Alias `mulaiUlang`. |
| `ESP.getFreeHeap()` | angka byte | Heap yang masih bebas sekarang |
| `ESP.getMinFreeHeap()` | angka byte | Titik terendah heap sejak boot |
| `ESP.getHeapSize()` | angka byte | Total heap |
| `ESP.getMaxAllocHeap()` | angka byte | Blok bersambung terbesar yang bisa dialokasikan |
| `ESP.getPsramSize()` | angka byte | Total PSRAM; `0` bila tidak ada |
| `ESP.getFreePsram()` | angka byte | PSRAM bebas; `0` bila tidak ada |
| `ESP.getChipModel()` | string | mis. `"ESP32-D0WDQ6"`, `"ESP32-S3"` |
| `ESP.getChipRevision()` | angka | Revisi silikon |
| `ESP.getChipCores()` | angka | Jumlah inti CPU |
| `ESP.getCpuFreqMHz()` | angka MHz | Frekuensi CPU sekarang |
| `ESP.getSdkVersion()` | string | Versi ESP-IDF |
| `ESP.getFlashChipSize()` | angka byte | Kapasitas flash |
| `ESP.getFlashChipSpeed()` | angka Hz | Kecepatan bus flash |
| `ESP.getEfuseMac()` | angka | MAC pabrik sebagai angka |
| `ESP.getSketchSize()` | angka byte | Ukuran firmware terpasang |
| `ESP.getFreeSketchSpace()` | angka byte | Ruang OTA yang tersisa |

**Alias:** `mulaiUlang` untuk `ESP.restart()`.
**Padanan Arduino:** nama yang sama pada objek `ESP`.
**Catatan papan:** PSRAM biasanya `0` di esp32doit-devkit-v1 dan sekitar 8 MB
di varian esp32-s3-devkitc-1 N8R8.

```baik
tulis(ESP.getChipModel(), "rev", ESP.getChipRevision());
tulis("heap bebas:", ESP.getFreeHeap(), "byte");
tulis("PSRAM:", ESP.getPsramSize());
```

### 4.5 Frekuensi CPU dan suhu

| Fungsi | Parameter | Nilai balik | Padanan |
|---|---|---|---|
| `setCpuFrequencyMhz(mhz)` | `mhz` angka: 240/160/80/40/20/10 | `benar`/`salah` | `setCpuFrequencyMhz()` |
| `getCpuFrequencyMhz()` | — | angka MHz | `getCpuFrequencyMhz()` |
| `getXtalFrequencyMhz()` | — | angka MHz | `getXtalFrequencyMhz()` |
| `getApbFrequency()` | — | angka Hz | `getApbFrequency()` |

**Catatan:** menurunkan frekuensi CPU di bawah 80 MHz membuat UART dan WiFi
tidak dapat diandalkan. Turunkan hanya saat radio mati.

```baik
tulis("sekarang", getCpuFrequencyMhz(), "MHz");
setCpuFrequencyMhz(80);
tulis("sesudah", getCpuFrequencyMhz(), "MHz");
```

---

#### `temperatureRead()` — alias `bacaSuhu`

Baca sensor suhu internal chip.

**Parameter:** tidak ada. **Nilai balik:** angka derajat Celsius.
**Alias:** `bacaSuhu` · **Padanan Arduino:** `temperatureRead()`.
**Catatan:** ini suhu **die** chip, bukan suhu ruangan — nilainya selalu lebih
panas beberapa derajat dan tidak terkalibrasi. Jangan dipakai sebagai termometer
lingkungan.

```baik
tulis("suhu chip:", bacaSuhu(), "C");
```

### 4.6 Tidur (sleep) dan sebab bangun

| Fungsi | Alias / bentuk singkat | Parameter | Nilai balik |
|---|---|---|---|
| `esp_sleep_enable_timer_wakeup(us)` | `sleepTimer(us)` | `us` angka mikrodetik | `takterdefinisi` |
| `esp_sleep_enable_ext0_wakeup(pin, level)` | `sleepExt0(pin, level)` | `pin` GPIO ber-`CAP_RTC`, `level` `0`/`1` | `takterdefinisi` |
| `esp_sleep_enable_ext1_wakeup(mask, mode)` | `sleepExt1(mask, mode)` | `mask` bitmask GPIO, `mode` `WAKEUP_ALL_LOW`/`WAKEUP_ANY_HIGH` | `takterdefinisi` |
| `esp_sleep_enable_touchpad_wakeup()` | — | — | `takterdefinisi` |
| `esp_sleep_disable_wakeup_source(sumber)` | — | `sumber` angka kode ESP-IDF; bila dikosongkan berarti **semua** sumber | angka kode hasil |
| `esp_deep_sleep_start()` | `tidurDalam(us)` | `us` opsional: set timer lalu tidur | tidak pernah kembali |
| `esp_light_sleep_start()` | `tidurRingan(us)` | `us` opsional | `takterdefinisi` (eksekusi lanjut) |
| `esp_sleep_get_wakeup_cause()` | — | — | **angka** kode ESP-IDF |
| `sebabBangun()` | — | — | **string** Indonesia/ringkas: `"TIMER"`, `"EXT0"`, `"EXT1"`, `"TOUCH"`, `"ULP"`, `"GPIO"`, `"UART"`, `"UNDEFINED"` |
| `esp_reset_reason()` | — | — | **angka** kode ESP-IDF |
| `sebabReset()` | — | — | **string**: `"POWERON"`, `"SW"`, `"PANIC"`, `"INT_WDT"`, `"TASK_WDT"`, `"DEEPSLEEP"`, `"BROWNOUT"`, … |

> **Perhatikan tipe nilai baliknya.** Fungsi bernama panjang (`esp_*`) setia pada
> ESP-IDF dan mengembalikan **angka** mentah. Yang mengembalikan **string** yang
> enak dibaca adalah pasangan berbahasa Indonesia, `sebabBangun()` dan
> `sebabReset()`. Jadi `jika (sebabBangun() === "TIMER")` benar, sedangkan
> `jika (esp_sleep_get_wakeup_cause() === "TIMER")` **tidak akan pernah** benar
> karena membandingkan angka dengan string.

**Padanan ESP-IDF:** nama panjangnya persis sama dengan fungsi ESP-IDF.
**Catatan:** setelah **deep sleep** papan melakukan boot ulang penuh — seluruh
variabel skrip hilang dan REPL dimulai dari awal. Pakai `rtcSet()`/`rtcGet()`
atau `NVS` bila ada nilai yang perlu bertahan. Setelah **light sleep** eksekusi
lanjut dari baris berikutnya dan RAM tetap utuh.
**Catatan papan:** pin yang mampu `ext0`/`ext1` berbeda antar papan; periksa
dengan `daftarPin(CAP_RTC)`.

`ext1` mengawasi **beberapa pin sekaligus** lewat bitmask, sedangkan `ext0`
hanya satu pin. Bitmask-nya dibentuk dengan menggeser bit ke posisi nomor GPIO:

```baik
// bangun bila GPIO2 ATAU GPIO4 menjadi HIGH
sleepExt1((1 << 2) | (1 << 4), WAKEUP_ANY_HIGH);
```

`esp_sleep_disable_wakeup_source()` membatalkan sumber bangun yang sudah
dipasang. Dipanggil tanpa argumen, ia membatalkan **semuanya** — berguna untuk
membersihkan keadaan sebelum memasang kombinasi baru:

```baik
esp_sleep_disable_wakeup_source();   // hapus semua sumber bangun
sleepTimer(5 * 1000 * 1000);         // lalu pasang yang baru
```

```baik
tulis("bangun karena:", sebabBangun());
tulis("tidur 5 detik...");
tidurDalam(5000000);
```

### 4.7 Memori RTC

Nilai yang disimpan di sini bertahan melewati deep sleep (tetapi hilang bila
daya dicabut atau papan di-reset keras). Tersedia maksimal **16 slot**.

---

#### `rtcSet(nama, nilai)`

| Parameter | Tipe | Arti |
|---|---|---|
| `nama` | string | Nama slot |
| `nilai` | angka | Nilai yang disimpan |

**Nilai balik:** `benar` bila berhasil, `salah` bila 16 slot sudah penuh.

---

#### `rtcGet(nama)`

| Parameter | Tipe | Arti |
|---|---|---|
| `nama` | string | Nama slot |

**Nilai balik:** angka yang tersimpan, atau `takterdefinisi` bila slot belum ada.

```baik
isi n = rtcGet("hitung");
jika (n === takterdefinisi) { n = 0; }
rtcSet("hitung", n + 1);
tulis("ini bangun ke-", n + 1);
```

### 4.8 Watchdog dan FreeRTOS

| Fungsi | Parameter | Nilai balik | Arti |
|---|---|---|---|
| `watchdogEnable(ms)` | `ms` angka | `takterdefinisi` | Nyalakan task watchdog **atas task konsol** dengan batas `ms` milidetik |
| `watchdogReset()` | — | `takterdefinisi` | "Beri makan" watchdog supaya papan tidak di-reset |
| `watchdogDisable()` | — | `takterdefinisi` | Matikan watchdog |
| `getTaskCount()` | — | angka | Jumlah task FreeRTOS yang hidup |
| `getTaskHighWaterMark()` | — | angka **byte** | Sisa stack terendah task konsol sejak start. Arduino/FreeRTOS mentah melaporkan angka ini dalam *word*; BAIK sudah mengubahnya ke **byte**. |

**Padanan ESP-IDF:** `esp_task_wdt_init` / `esp_task_wdt_reset` /
`esp_task_wdt_deinit`, `uxTaskGetNumberOfTasks`, `uxTaskGetStackHighWaterMark`.
> **Peringatan.** `watchdogEnable()` memasang watchdog pada **task konsol** —
> task yang sama yang menjalankan skrip BAIK-mu. Bila skrip lupa memanggil
> `watchdogReset()` secara berkala (termasuk saat tertahan di `delay()` yang
> lebih lama dari batas watchdog), **papan akan me-reset dirinya sendiri**.
> Selalu pasangkan `watchdogEnable()` dengan `watchdogReset()` di setiap putaran
> loop, dan panggil `watchdogDisable()` setelah selesai.

```baik
watchdogEnable(8000);
untuk (isi i = 0; i < 20; i++) { watchdogReset(); delay(300); }
watchdogDisable();
tulis("task hidup:", getTaskCount());
```

---

## 5. Bus: I2C, SPI, UART

### 5.1 I2C — objek `Wire`

Objek `Wire` adalah bus I2C pertama; alias globalnya `I2C`. ESP32 dan ESP32-S3
sama-sama punya dua kontroler I2C, jadi `Wire1` juga tersedia dengan API yang
persis sama.

| Fungsi | Parameter | Nilai balik | Padanan Arduino |
|---|---|---|---|
| `Wire.begin([sda, scl, freq])` | `sda`/`scl` GPIO, `freq` Hz (bawaan 100000) | `benar`/`salah` | `Wire.begin()` |
| `Wire.setClock(hz)` | `hz` angka | `takterdefinisi` | `Wire.setClock()` |
| `Wire.beginTransmission(addr)` | `addr` angka 7 bit | `takterdefinisi` | `Wire.beginTransmission()` |
| `Wire.write(data)` | angka `0`..`255`, string, **atau array angka** | angka byte antre | `Wire.write()` |
| `Wire.endTransmission([stop])` | `stop` boolean, bawaan `benar` | angka: `0`=OK, `2`=NACK alamat, `3`=NACK data, `4`=galat lain | `Wire.endTransmission()` |
| `Wire.requestFrom(addr, n[, stop])` | `addr`, `n` jumlah byte, `stop` boolean | angka byte diterima | `Wire.requestFrom()` |
| `Wire.available()` | — | angka byte siap dibaca | `Wire.available()` |
| `Wire.read()` | — | angka `0`..`255`, `-1` bila habis | `Wire.read()` |
| `Wire.readBytes(n)` | `n` angka | array angka | `Wire.readBytes()` |
| `Wire.end()` | — | `takterdefinisi` | `Wire.end()` |
| `Wire.scan()` | — | array alamat yang menjawab | tidak ada (khas BAIK) |
| `Wire.writeTo(addr, arrayByte)` | `addr`, array angka | kode `endTransmission` | gabungan praktis |
| `Wire.readFrom(addr, n)` | `addr`, `n` | array angka | gabungan praktis |
| `Wire.writeReg(addr, reg, nilai)` | tiga angka | kode `endTransmission` | pola tulis-register |
| `Wire.readReg(addr, reg[, n])` | `addr`, `reg`, `n` opsional | angka bila `n` dikosongkan, array bila `n` diisi | pola baca-register |

**Alias:** `Wire.scan()` juga tersedia sebagai fungsi global `i2cScan()` dan
`pindaiI2C()`. Objek `Wire` juga bernama `I2C`.
**Catatan papan:** pin `SDA`/`SCL` bawaan berbeda antar papan — pakai konstanta
`SDA` dan `SCL`, jangan menuliskan nomornya langsung.

`Wire.write()` menerima tiga bentuk argumen — satu byte, sebuah string, atau
**sebuah array angka** sekaligus:

```baik
Wire.beginTransmission(0x3C);
Wire.write(0x00);                    // satu byte
Wire.write("halo");                  // string
Wire.write([0xAE, 0xA8, 0x3F]);      // array byte sekaligus
tulis("kode:", Wire.endTransmission());
```

```baik
Wire.begin(SDA, SCL, 400000);
isi alamat = pindaiI2C();
tulis("perangkat ditemukan:", alamat.panjang);
Wire.writeReg(0x68, 0x6B, 0x00);
tulis("WHO_AM_I =", Wire.readReg(0x68, 0x75));
```

### 5.2 SPI — objek `SPI`

| Fungsi | Parameter | Nilai balik | Padanan Arduino |
|---|---|---|---|
| `SPI.begin([sck, miso, mosi, ss])` | GPIO; bila dikosongkan pakai pin bawaan papan | `takterdefinisi` | `SPI.begin()` |
| `SPI.end()` | — | `takterdefinisi` | `SPI.end()` |
| `SPI.setFrequency(hz)` | `hz` angka | `takterdefinisi` | `SPI.setFrequency()` |
| `SPI.setDataMode(mode)` | `SPI_MODE0`..`SPI_MODE3` | `takterdefinisi` | `SPI.setDataMode()` |
| `SPI.setBitOrder(urutan)` | `MSBFIRST`/`LSBFIRST` | `takterdefinisi` | `SPI.setBitOrder()` |
| `SPI.beginTransaction(hz, urutan, mode)` | angka, angka, angka | `takterdefinisi` | `SPI.beginTransaction(SPISettings(...))` |
| `SPI.endTransaction()` | — | `takterdefinisi` | `SPI.endTransaction()` |
| `SPI.transfer(byte)` | `byte` angka `0`..`255` | angka byte balasan | `SPI.transfer()` |
| `SPI.transfer16(word)` | `word` angka `0`..`65535` | angka 16 bit balasan | `SPI.transfer16()` |
| `SPI.transferBytes(array)` | array angka | array angka balasan, panjang sama | `SPI.transferBytes()` |
| `SPI.write(byte)` | `byte` angka | `takterdefinisi` | `SPI.write()` |
| `SPI.writeBytes(array)` | array angka | `takterdefinisi` | `SPI.writeBytes()` |

**Konstanta:** `SPI_MODE0`, `SPI_MODE1`, `SPI_MODE2`, `SPI_MODE3`, dan nomor bus
`VSPI`, `HSPI`, `FSPI`.
**Catatan papan:** ESP32 menamai bus yang bebas dipakai `VSPI` dan `HSPI`;
ESP32-S3 menamainya `FSPI` (dan `HSPI`). Pemilihan pin chip-select tetap urusan
skrip — `SS` hanyalah nilai bawaan yang disarankan.

```baik
SPI.begin(SCK, MISO, MOSI, SS);
SPI.beginTransaction(1000000, MSBFIRST, SPI_MODE0);
pinMode(SS, OUTPUT); digitalWrite(SS, LOW);
tulis("balasan:", SPI.transfer(0x9F));
digitalWrite(SS, HIGH); SPI.endTransaction();
```

### 5.3 UART — objek `Serial1` / `Serial2`

| Fungsi | Parameter | Nilai balik | Padanan Arduino |
|---|---|---|---|
| `SerialN.begin(baud[, rx, tx])` | `baud` angka; `rx`/`tx` GPIO opsional | `takterdefinisi` | `SerialN.begin()` |
| `SerialN.available()` | — | angka byte | `available()` |
| `SerialN.read()` | — | angka byte, `-1` bila kosong | `read()` |
| `SerialN.readString()` | — | string | `readString()` |
| `SerialN.write(b)` | `b` angka byte | angka byte tertulis | `write()` |
| `SerialN.print(x)` | string/angka | `takterdefinisi` | `print()` |
| `SerialN.println(x)` | string/angka | `takterdefinisi` | `println()` |
| `SerialN.flush()` | — | `takterdefinisi` | `flush()` |
| `SerialN.end()` | — | `takterdefinisi` | `end()` |

Pin `rx` dan `tx` divalidasi: `rx` harus mampu INPUT, `tx` harus mampu OUTPUT.

**Catatan papan:** **kedua papan punya tiga UART** (`SOC_UART_NUM` = 3), jadi
`Serial1` **dan** `Serial2` tersedia di ESP32 maupun ESP32-S3. `Serial` (UART0)
dipakai REPL, jadi jangan di-`end()` kalau kamu masih ingin mengetik perintah.

Yang berbeda adalah **letak pin UART0**-nya: di ESP32 ada di GPIO1 (TX) dan
GPIO3 (RX), sedangkan di **ESP32-S3 ada di GPIO43 (TX) dan GPIO44 (RX)**.
Pakailah konstanta `TX` dan `RX` daripada menuliskan nomornya langsung.
`Serial1`/`Serial2` sendiri tidak punya pin tetap — tentukan sendiri lewat
argumen `rx` dan `tx`.

```baik
Serial1.begin(9600, 16, 17);
Serial1.println("halo perangkat");
delay(100);
jika (Serial1.available() > 0) { tulis("balasan:", Serial1.readString()); }
```

---

## 6. Jaringan

### 6.1 Objek `WiFi`

| Fungsi | Parameter | Nilai balik | Padanan Arduino |
|---|---|---|---|
| `WiFi.begin(ssid, sandi)` | dua string | `takterdefinisi` | `WiFi.begin()` |
| `WiFi.disconnect()` | — | `takterdefinisi` | `WiFi.disconnect()` |
| `WiFi.status()` | — | angka, lihat tabel status di bawah | `WiFi.status()` |

**Nilai status WiFi.** Semuanya tersedia sebagai konstanta global dan juga
sebagai properti object `WiFi` (mis. `WL_CONNECTED` dan `WiFi.WL_CONNECTED`
bernilai sama).

| Konstanta | Nilai | Arti |
|---|---|---|
| `WL_NO_SHIELD` | 255 | Perangkat keras WiFi tidak tersedia |
| `WL_IDLE_STATUS` | 0 | Menganggur, belum ada usaha menyambung |
| `WL_NO_SSID_AVAIL` | 1 | SSID yang dituju tidak ditemukan |
| `WL_SCAN_COMPLETED` | 2 | Pemindaian jaringan selesai |
| `WL_CONNECTED` | 3 | Tersambung dan siap dipakai |
| `WL_CONNECT_FAILED` | 4 | Gagal menyambung, biasanya sandi salah |
| `WL_CONNECTION_LOST` | 5 | Sempat tersambung lalu putus |
| `WL_DISCONNECTED` | 6 | Tidak tersambung |

Pakai `WiFi.statusTeks()` bila yang dibutuhkan penjelasan siap cetak dalam
bahasa Indonesia, bukan angkanya.
| `WiFi.statusText()` | — | string bahasa Indonesia, mis. `"tersambung"` | khas BAIK |
| `WiFi.isConnected()` | — | `benar`/`salah` | `WiFi.isConnected()` |
| `WiFi.tungguKoneksi(timeoutMs)` | `timeoutMs` angka | `benar` bila tersambung sebelum batas waktu | khas BAIK; alias `WiFi.waitConnected` |
| `WiFi.localIP()` | — | string, mis. `"192.168.1.20"` | `WiFi.localIP()` |
| `WiFi.gatewayIP()` | — | string | `WiFi.gatewayIP()` |
| `WiFi.subnetMask()` | — | string | `WiFi.subnetMask()` |
| `WiFi.dnsIP()` | — | string | `WiFi.dnsIP()` |
| `WiFi.macAddress()` | — | string `"AA:BB:CC:DD:EE:FF"` | `WiFi.macAddress()` |
| `WiFi.RSSI()` | — | angka dBm (negatif) | `WiFi.RSSI()` |
| `WiFi.SSID()` | — | string nama jaringan | `WiFi.SSID()` |
| `WiFi.channel()` | — | angka kanal | `WiFi.channel()` |
| `WiFi.mode(m)` | `WIFI_STA`/`WIFI_AP`/`WIFI_AP_STA`/`WIFI_OFF` | `benar`/`salah` | `WiFi.mode()` |
| `WiFi.setHostname(nama)` | `nama` string | `benar`/`salah` | `WiFi.setHostname()` |
| `WiFi.softAP(ssid[, sandi[, kanal]])` | string, string opsional, angka opsional | `benar`/`salah` | `WiFi.softAP()` |
| `WiFi.softAPIP()` | — | string | `WiFi.softAPIP()` |
| `WiFi.softAPgetStationNum()` | — | angka klien tersambung | `WiFi.softAPgetStationNum()` |
| `WiFi.scanNetworks()` | — | array objek `{ssid, rssi, kanal, enkripsi, bssid}` | `WiFi.scanNetworks()` |
| `WiFi.sleep(bool)` | `benar`/`salah` | `takterdefinisi` | `WiFi.setSleep()` |
| `WiFi.setTxPower(dbm)` | `dbm` angka | `benar`/`salah` | `WiFi.setTxPower()` |

**Catatan penting:** firmware ini sudah menyambung ke WiFi sendiri saat boot bila
kredensial tersimpan di NVS lewat halaman konfigurasi web. Memanggil
`WiFi.begin()` dari skrip akan memutus sambungan itu dan, bila papan sedang
melayani editor web, menghentikan editornya sampai sambungan baru berhasil.
**Catatan papan:** keduanya hanya mendukung **WiFi 2,4 GHz**; jaringan 5 GHz
tidak akan muncul pada hasil `scanNetworks()`. Selama radio hidup, pin **ADC2**
tidak dapat dibaca.

```baik
WiFi.begin("NamaWiFi", "sandiku");
jika (WiFi.tungguKoneksi(15000)) {
  tulis("IP:", WiFi.localIP(), "RSSI:", WiFi.RSSI());
} lainnya {
  tulis("gagal:", WiFi.statusText());
}
```

Gaya Indonesia sepenuhnya, memakai alias-alias di atas — hasilnya identik:

```baik
jika (WiFi.tersambung()) {                 // = WiFi.isConnected()
  tulis("status:", WiFi.statusTeks());     // = WiFi.statusText()
}
isi daftar = WiFi.pindaiJaringan();        // = WiFi.scanNetworks()
tulis("jaringan terlihat:", daftar.panjang);
```

### 6.2 Objek `HTTP`

| Fungsi | Parameter | Nilai balik | Padanan Arduino |
|---|---|---|---|
| `HTTP.get(url)` | `url` string | string isi balasan, atau `kosong` bila gagal | `HTTPClient.GET()` |
| `HTTP.post(url, body[, contentType])` | `url`, `body` string, `contentType` string opsional (bawaan `"application/x-www-form-urlencoded"`) | string isi balasan, atau `kosong` | `HTTPClient.POST()` |
| `HTTP.getJSON(url)` | `url` string | hasil `JSON.parse()` dari balasan, atau `kosong` | — |
| `HTTP.statusTerakhir()` | — | angka kode HTTP terakhir, mis. `200`; negatif bila galat koneksi | alias `HTTP.lastStatus` |

**Nilai balik saat gagal:** `HTTP.get()`, `HTTP.post()`, dan `HTTP.getJSON()`
sama-sama mengembalikan **`kosong`**. Periksa `HTTP.statusTerakhir()` untuk tahu
sebabnya: angka positif adalah kode HTTP sungguhan dari server (mis. `404`),
sedangkan angka **negatif** berarti gagal di tingkat koneksi (mis. `-1` = tidak
bisa menyambung).

**Catatan memori:** seluruh balasan dimuat ke heap sekaligus. Balasan besar (di
atas beberapa puluh kilobyte) bisa membuat papan kehabisan memori — periksa
`ESP.getFreeHeap()` bila ragu.

> ### ⚠ Keamanan HTTPS: sertifikat TIDAK diverifikasi
>
> Untuk URL `https://`, firmware memakai `WiFiClientSecure` dengan
> **`setInsecure()`**. Artinya:
>
> - Sambungan **memang terenkripsi** — data tidak terbaca oleh penyadap pasif.
> - Tetapi **sertifikat server sama sekali tidak diperiksa**. Papan akan dengan
>   senang hati berbicara kepada siapa pun yang mengaku sebagai server tujuan,
>   sehingga sambungan **rentan terhadap serangan man-in-the-middle**.
>
> Pilihan ini diambil supaya skrip BAIK tidak perlu menanam sertifikat CA di
> dalam flash. **Jangan mengirim kata sandi, token, atau data pribadi** lewat
> `HTTP.*` di jaringan yang tidak kamu percayai. Untuk keperluan yang menuntut
> keamanan sungguhan, tanam sertifikat CA dan bangun kliennya di sisi C.

```baik
isi badan = HTTP.get("http://example.com/data.txt");
tulis("status:", HTTP.statusTerakhir());
jika (badan !== kosong) { tulis(badan); }
```

### 6.3 Waktu jaringan — objek `NTP`

| Fungsi | Parameter | Nilai balik |
|---|---|---|
| `NTP.begin([server, offsetDetik, offsetDst])` | `server` string (bawaan `"pool.ntp.org"`), `offsetDetik` angka zona waktu (WIB = `25200`), `offsetDst` angka | `takterdefinisi` |
| `NTP.sync(timeoutMs)` | `timeoutMs` angka | `benar` bila waktu berhasil didapat |
| `NTP.epoch()` | — | angka detik sejak 1 Januari 1970 |
| `NTP.format(fmt)` | `fmt` string `strftime`, mis. `"%Y-%m-%d %H:%M:%S"` | string |
| `NTP.jam()` | — | angka `0`..`23` |
| `NTP.menit()` | — | angka `0`..`59` |
| `NTP.detik()` | — | angka `0`..`59` |
| `NTP.tanggal()` | — | angka `1`..`31` |
| `NTP.bulan()` | — | angka `1`..`12` |
| `NTP.tahun()` | — | angka, mis. `2026` |

**Padanan ESP-IDF/Arduino:** `configTime()` + `getLocalTime()` + `strftime()`.
**Catatan:** WiFi harus sudah tersambung sebelum `NTP.sync()`.

```baik
NTP.begin("pool.ntp.org", 25200, 0);
jika (NTP.sync(10000)) {
  tulis("sekarang:", NTP.format("%Y-%m-%d %H:%M:%S"));
  tulis("jam:", NTP.jam());
}
```

### 6.4 mDNS

| Fungsi | Parameter | Nilai balik | Padanan Arduino |
|---|---|---|---|
| `MDNS.begin(nama)` | `nama` string tanpa akhiran `.local` | `benar`/`salah` | `MDNS.begin()` |
| `MDNS.addService(svc, proto, port)` | `svc` mis. `"http"`, `proto` `"tcp"`/`"udp"`, `port` angka | `takterdefinisi` | `MDNS.addService()` |

**Catatan:** setelah `MDNS.begin("baik")`, papan bisa dibuka di
`http://baik.local/` dari komputer yang mendukung mDNS/Bonjour.

```baik
jika (MDNS.begin("baik")) {
  MDNS.addService("http", "tcp", 80);
  tulis("buka http://baik.local/");
}
```

---

## 7. Berkas dan penyimpanan

### 7.1 Objek `FS` (SPIFFS)

Alias objek: `Berkas`.

| Fungsi | Parameter | Nilai balik | Padanan Arduino |
|---|---|---|---|
| `FS.begin([format])` | `format` boolean: format otomatis bila gagal dipasang | `benar`/`salah` | `SPIFFS.begin()` |
| `FS.exists(path)` | `path` string | `benar`/`salah` | `SPIFFS.exists()` |
| `FS.read(path)` | `path` string | string isi berkas, atau `kosong` | `SPIFFS.open()` + `readString()` |
| `FS.write(path, teks)` | `path`, `teks` string | `benar`/`salah` | mode `"w"` |
| `FS.append(path, teks)` | `path`, `teks` string | `benar`/`salah` | mode `"a"` |
| `FS.remove(path)` | `path` string | `benar`/`salah` | `SPIFFS.remove()` |
| `FS.rename(lama, baru)` | dua string | `benar`/`salah` | `SPIFFS.rename()` |
| `FS.size(path)` | `path` string | angka byte, `-1` bila tidak ada | `file.size()` |
| `FS.list([dir])` | `dir` string, bawaan `"/"` | array objek `{nama, ukuran, direktori}` | `SPIFFS.open(dir)` + `openNextFile()` |
| `FS.mkdir(path)` | `path` string | **selalu `salah`** | `SPIFFS.mkdir()` |
| `FS.rmdir(path)` | `path` string | **selalu `salah`** | `SPIFFS.rmdir()` |
| `FS.totalBytes()` | — | angka byte | `SPIFFS.totalBytes()` |
| `FS.usedBytes()` | — | angka byte | `SPIFFS.usedBytes()` |
| `FS.freeBytes()` | — | angka byte (`total - used`) | — |
| `FS.format()` | — | `benar`/`salah` | `SPIFFS.format()` |

**Catatan penting tentang path.** API `FS.*` memakai path bergaya SPIFFS yang
diawali garis miring: `"/data.txt"`. Sementara itu builtin `muat()` memakai
`FILE*` POSIX, sehingga path yang sama harus ditulis dengan awalan titik
kait VFS: `"/spiffs/data.txt"`. Keduanya menunjuk berkas yang sama.

**SPIFFS itu datar — tidak punya direktori sungguhan.** Karena itu `FS.mkdir()`
dan `FS.rmdir()` **selalu mengembalikan `salah`**; keduanya tetap terdaftar hanya
demi kelengkapan API. Nama berkas boleh mengandung `/` (mis.
`"/data/suhu.txt"`), tetapi garis miring itu sekadar bagian dari nama, bukan
penanda folder. `FS.list()` karena itu juga tidak pernah benar-benar bersarang.

### Nilai balik saat galat — tidak seragam, perhatikan baik-baik

Karena BAIK tidak punya exception, kegagalan dilaporkan lewat nilai balik. Sayangnya
nilai penandanya **berbeda-beda antar fungsi**, jadi periksalah yang tepat:

| Fungsi | Nilai saat gagal | Cara memeriksa |
|---|---|---|
| `FS.read(path)` | `kosong` | `jika (hasil === kosong)` |
| `FS.list(dir)` | `kosong` | `jika (hasil === kosong)` |
| `FS.size(path)` | `-1` | `jika (hasil < 0)` |
| `FS.begin`, `write`, `append`, `remove`, `rename`, `mkdir`, `rmdir`, `format` | `salah` | `jika (hasil === salah)` |
| Semua `NVS.*` yang mengembalikan status | `salah` | `jika (hasil === salah)` |
| `HTTP.get`, `HTTP.post`, `HTTP.getJSON` | `kosong` | `jika (hasil === kosong)` |

Perhatikan bahwa `FS.size()` memakai `-1`, **bukan** `salah` — dan karena `0`
adalah ukuran yang sah untuk berkas kosong, memeriksanya dengan
`jika (FS.size(p))` saja tidak cukup.

```baik
isi teks = FS.read("/tidak-ada.txt");
jika (teks === kosong) { tulis("gagal membaca"); }

isi uk = FS.size("/tidak-ada.txt");
jika (uk < 0) { tulis("berkas tidak ada"); }
```

```baik
FS.begin(benar);
FS.write("/catatan.txt", "baris pertama\n");
FS.append("/catatan.txt", "baris kedua\n");
tulis(FS.read("/catatan.txt"));
tulis("terpakai", FS.usedBytes(), "dari", FS.totalBytes());
```

### 7.2 Objek `NVS` (Preferences)

Alias objek: `Simpan`. NVS menyimpan nilai di partisi flash khusus yang bertahan
melewati reset, deep sleep, maupun pemutusan daya.

| Fungsi | Parameter | Nilai balik | Padanan Arduino |
|---|---|---|---|
| `NVS.begin(namespace[, readOnly])` | `namespace` string maks. 15 karakter, `readOnly` boolean | `benar`/`salah` | `Preferences.begin()` |
| `NVS.end()` | — | `takterdefinisi` | `Preferences.end()` |
| `NVS.putInt(k, v)` | `k` string kunci, `v` angka | angka byte tertulis | `putInt()` |
| `NVS.getInt(k[, def])` | `k` string, `def` angka bawaan | angka | `getInt()` |
| `NVS.putFloat(k, v)` | `k` string, `v` angka | angka byte tertulis | `putFloat()` |
| `NVS.getFloat(k[, def])` | `k` string, `def` angka | angka | `getFloat()` |
| `NVS.putString(k, v)` | `k`, `v` string | angka byte tertulis | `putString()` |
| `NVS.getString(k[, def])` | `k` string, `def` string | string | `getString()` |
| `NVS.putBool(k, v)` | `k` string, `v` boolean | angka byte tertulis | `putBool()` |
| `NVS.getBool(k[, def])` | `k` string, `def` boolean | `benar`/`salah` | `getBool()` |
| `NVS.remove(k)` | `k` string | `benar`/`salah` | `remove()` |
| `NVS.clear()` | — | `benar`/`salah` | `clear()` |
| `NVS.isKey(k)` | `k` string | `benar`/`salah` | `isKey()` |
| `NVS.freeEntries()` | — | angka slot tersisa | `freeEntries()` |

**Catatan:** kunci maupun nama namespace maksimal 15 karakter.

> **Namespace `"wifi"` DITOLAK.** Firmware memakai namespace itu untuk menyimpan
> SSID dan sandi papan (lihat `src/main.cpp`). Kalau skrip boleh menulis ke sana,
> konfigurasi jaringan bisa rusak dan papan tidak bisa tersambung lagi — dan
> pada papan yang hanya dijangkau lewat WiFi, itu berarti kehilangan akses.
> Karena itu `NVS.begin("wifi")` mengembalikan `salah` dan mencetak galat.
> Pakailah nama namespace lain untuk data skripmu.

Namespace sebelumnya ditutup otomatis saat kamu memanggil `NVS.begin()` dengan
nama lain, jadi handle NVS tidak bocor ketika skrip berpindah namespace.

```baik
NVS.begin("app", salah);
isi n = NVS.getInt("boot", 0);
NVS.putInt("boot", n + 1);
tulis("boot ke-", n + 1);
NVS.end();
```

### 7.3 Menjalankan skrip lain

---

#### `jalankan(path)` — alias `run`

Baca sebuah berkas `.ina` dari SPIFFS lalu jalankan isinya di interpreter yang
sedang hidup. Variabel global yang dibuat skrip itu tetap ada setelah selesai.

| Parameter | Tipe | Arti |
|---|---|---|
| `path` | string | Path bergaya `FS`, mis. `"/01-halo.ina"` |

**Nilai balik:** nilai ekspresi terakhir skrip, atau `takterdefinisi`.
**Alias:** `run` · **Padanan Arduino:** tidak ada.

```baik
jalankan("/02-kedip-led.ina");
```

---

#### `muat(path)`

Builtin bawaan bahasa BAIK. Fungsinya sama dengan `jalankan()`, tetapi membuka
berkas dengan `FILE*` POSIX sehingga **path harus memakai awalan titik kait**
`"/spiffs/"`.

| Parameter | Tipe | Arti |
|---|---|---|
| `path` | string | Path VFS, mis. `"/spiffs/01-halo.ina"` |

**Nilai balik:** nilai ekspresi terakhir skrip.
**Alias:** — · **Padanan JavaScript:** mirip `load()`.

```baik
muat("/spiffs/01-halo.ina");
```

---

## 8. Perbedaan ESP32 vs ESP32-S3

| Hal | ESP32 (`esp32doit-devkit-v1`) | ESP32-S3 (`esp32-s3-devkitc-1`) |
|---|---|---|
| **DAC** | Ada: 2 kanal 8 bit di GPIO25 dan GPIO26. `dacWrite()`/`dacDisable()` bekerja. | **Tidak ada.** `dacWrite()` tetap terdaftar tetapi mencetak galat `DAC tidak tersedia pada ESP32-S3`. |
| **Kanal LEDC** | 16 kanal (`0`..`15`), 4 timer, resolusi sampai 20 bit. | 8 kanal (`0`..`7`), 4 timer, resolusi sampai 14 bit. |
| **Jumlah UART** | 3: `Serial` (UART0, REPL), `Serial1`, `Serial2`. UART0 di GPIO1 (TX) / GPIO3 (RX). | **Juga 3** (`SOC_UART_NUM` = 3): `Serial`, `Serial1`, `Serial2`. Yang berbeda hanya letak UART0: **GPIO43 (TX) / GPIO44 (RX)**. |
| **Pin sentuh** | 10 kanal: `T0`..`T9`. Nilai **turun** saat disentuh. | 14 kanal: `T1`..`T14`. Nilai **naik** saat disentuh (versi 2 peripheral sentuh). |
| **Pin ADC** | ADC1: GPIO32–39. ADC2: GPIO0, 2, 4, 12–15, 25–27. GPIO34–39 input-only. | ADC1: GPIO1–10. ADC2: GPIO11–20. Semua pin ADC juga bisa jadi output. |
| **USB** | Tidak ada USB native; koneksi lewat jembatan UART (CP2102/CH340) di GPIO1/GPIO3. | USB-OTG **dan** USB Serial/JTAG bawaan pada GPIO19/GPIO20 (`CAP_USB`). Papan punya dua konektor USB. |
| **LED bawaan** | LED biasa. `digitalWrite(LED_BUILTIN, HIGH)` menyalakannya. | **WS2812 (LED RGB pintar) di GPIO48.** `digitalWrite()` **tidak berpengaruh** — pakai `ledBawaan()` atau `rgbLedWrite()`. Lihat [3.2](#32-led-bawaan-dan-led-rgb-ws2812). |
| **`hallRead()`** | Ada, **tetapi hanya bila dibangun dengan arduino-esp32 2.x**. Pada 3.x core-nya sudah menghapus fungsi itu, jadi BAIK mencetak galat. | **Tidak ada sensor Hall sama sekali**; selalu galat. |
| **Pin ADC yang hilang** | `A1`/`A2` tidak ada (GPIO37/38 tidak dibonding di WROOM-32). Sablon juga tidak punya `D1`/`D3` — di situ tertulis `TX0`/`RX0`. | Semua `A0`..`A9` ada. |
| **Jumlah GPIO** | GPIO0–GPIO39 (GPIO20, 24, 28–31 tidak dikeluarkan). **GPIO6–11 dipakai SPI flash** (`CAP_FLASH`) dan selalu ditolak. | GPIO0–GPIO48. **GPIO26–32 dipakai SPI flash/PSRAM** (`CAP_FLASH`) dan selalu ditolak. |
| **PSRAM** | Umumnya tidak ada; `ESP.getPsramSize()` balik `0`. | Varian N8R8 punya 8 MB PSRAM. |
| **Inti CPU** | 2 inti, hingga 240 MHz. | 2 inti, hingga 240 MHz, plus instruksi vektor untuk AI. |

Cara paling aman menulis skrip yang jalan di kedua papan adalah menanyakan
kapabilitas, bukan menghardcode nomor pin:

```baik
tulis("Papan:", BOARD);

ledBawaan(benar);                  // jalan di kedua papan, apa pun jenis LED-nya

jika (pinPunya(25, CAP_DAC)) { dacWrite(25, 128); }

isi p = daftarPin(CAP_TOUCH);
jika (p.panjang > 0) { tulis("sentuh pertama di GPIO", p[0]); }
```

---

## 9. Galat umum dan artinya

Galat dilaporkan sebagai teks ke konsol. Bahasa BAIK tidak punya exception, jadi
sebuah galat **menghentikan skrip yang sedang berjalan** dan mengembalikan
kendali ke prompt REPL.

### 9.1 Galat bahasa (dari interpreter)

| Pesan | Arti | Cara memperbaiki |
|---|---|---|
| `[var] tidak terimplementasi` | Kata kunci itu tercadang tetapi parser belum mendukungnya. | Ganti `var` dengan `isi`. Untuk `pilih`, `kerjakan`, `try`, `throw`, `new`, `delete`, `void`, `with`, `instanceof`: tulis ulang dengan `jika`/`ulang`. |
| `[namaFungsi] tidak terdefinisikan` | `REFERENCE_ERROR`: nama itu tidak ada. | Periksa ejaannya (huruf besar-kecil berpengaruh) dan pastikan modulnya memang tersedia di papan ini. |
| `Use ===, not ==` | Operator `==` sengaja dilarang. | Pakai `===`. Begitu pula `!==` menggantikan `!=`. |
| `galat : konversi tipe implisit dilarang` | Mencoba `"teks" + 5` atau operasi campur tipe lain. | Cetak dengan beberapa argumen `tulis("teks", 5)`, atau ubah eksplisit dengan `JSON.stringify(5)`. |
| `conversion from object to string is not supported` | Sebuah objek/array dipakai di tempat yang menuntut string. | Bungkus dengan `JSON.stringify(obj)`. |
| `calling non-callable` | Yang dipanggil bukan fungsi. | Biasanya salah ketik nama, atau memanggil properti yang bernilai `takterdefinisi`. |
| `invalid operand for ++` | `++`/`--` dipakai pada yang bukan angka. | Pastikan variabelnya sudah diberi nilai angka. |
| `GALAT : index harus angka` | Mengindeks nilai `foreign` dengan string. | Gunakan indeks angka. |
| `misplaced 'break'` / `misplaced 'continue'` | `berhenti`/`teruskan` di luar badan loop. | Pindahkan ke dalam `untuk`/`ulang`. |
| `parser stack overflow` | Ekspresi atau blok terlalu dalam bersarang. | Pecah menjadi beberapa fungsi. |
| `OUT_OF_MEMORY` | Heap habis. | Panggil `gc(benar)`, kurangi ukuran array/string, periksa `ESP.getFreeHeap()`. |
| `SYNTAX_ERROR` | Parser gagal di posisi tertentu; nomor baris dicetak di atas pesan. | Periksa kurung, tanda titik koma, dan kutip yang tidak berpasangan. |

### 9.2 Galat perangkat keras (dari lapisan ESP32)

Setiap fungsi yang menerima nomor pin memvalidasi pin itu lebih dulu dan
mencetak pesan berbahasa Indonesia yang menyebut nama fungsi, pin yang salah,
dan pin alternatif yang benar. Bentuk umumnya:

```
GALAT digitalWrite: GPIO 34 tidak bisa dipakai sebagai OUTPUT.
       Pin yang bisa: 2, 4, 5, 12, 13, ...
```

Setelah itu fungsi mengembalikan `takterdefinisi` dan perangkat keras **tidak**
disentuh sama sekali. Beberapa sebab yang paling sering:

| Gejala | Sebab | Solusi |
|---|---|---|
| `tidak bisa dipakai sebagai OUTPUT` | GPIO34–39 di ESP32 hanya input. | Pakai pin lain; cari dengan `daftarPin(CAP_OUTPUT)`. |
| `bukan pin ADC` | Pin itu tidak punya kanal ADC. | `daftarPin(CAP_ADC1)`. |
| `DAC tidak tersedia pada ESP32-S3` | Chip ini memang tidak punya DAC. | Pakai PWM (`ledcWrite`) + tapis RC sebagai pengganti. |
| `analogRead` selalu `4095` | Pin ADC2 dibaca sementara WiFi aktif. | Pindah ke pin ADC1. |
| Papan reset berulang saat skrip jalan | Loop panjang tanpa `delay()`/`yield()` memicu task watchdog. | Sisipkan `delay(1)` atau `yield()` di badan loop. |
| `pin dipakai SPI flash` | Menyentuh GPIO6–11 (ESP32) atau GPIO26–32 (ESP32-S3). | Jangan dipakai; pin itu bertanda `CAP_FLASH` dan **selalu** ditolak. |
| LED bawaan ESP32-S3 tidak menyala, tanpa galat | `digitalWrite(LED_BUILTIN, …)` tidak berpengaruh pada WS2812. | Pakai `ledBawaan(benar)` atau `rgbLedWrite(48, 16, 16, 16)`. |
| `callback harus fungsi BAIK, bukan fungsi bawaan` | Fungsi bawaan diberikan ke `attachInterrupt`/`touchAttachInterrupt`. | Bungkus di dalam `fungsi(pin) { … }`. |
| `namespace "wifi" tidak boleh dipakai` | `NVS.begin("wifi")`. | Pakai nama namespace lain. |
| `hallRead` mencetak galat | Papan ESP32-S3, atau ESP32 yang dibangun dengan arduino-esp32 3.x. | Tidak tersedia; pakai sensor magnet luar. |
| Papan reset walau skrip tampak wajar | `watchdogEnable()` aktif tetapi `watchdogReset()` tidak dipanggil cukup sering. | Panggil `watchdogReset()` tiap putaran, atau `watchdogDisable()`. |
| Papan tidak mau boot setelah kabel dipasang | Menarik strapping pin (`CAP_STRAP`) ke level yang salah saat reset. | Lepaskan beban dari GPIO0/2/12/15 saat boot. |

---

## 10. Yang BELUM didukung

Bagian ini sengaja ditulis apa adanya supaya tidak ada waktu yang terbuang
mencari fitur yang memang belum ada.

### 10.1 `try` / `catch` / `throw` belum diimplementasi

Kata kunci `try`, `catch`, `finally`, dan `throw` sudah tercadang di pemindai
token, tetapi parser menolaknya dengan `[try] tidak terimplementasi`. **Tidak
ada mekanisme exception apa pun di BAIK.**

Konvensi penggantinya: setiap fungsi melaporkan kegagalan lewat **nilai balik** —
`salah`, `-1`, atau `kosong` — dan mencetak keterangan ke konsol. Jadi periksalah
nilai baliknya.

```baik
isi teks = FS.read("/tidak-ada.txt");
jika (teks === kosong) {
  tulis("berkas tidak terbaca, pakai nilai bawaan");
  teks = "";
}
```

### 10.2 Tidak ada `class`

BAIK tidak punya `class`, `extends`, `new`, maupun `instanceof` (`new` dan
`instanceof` ditolak parser). Susun data dengan objek literal dan fungsi biasa.

```baik
fungsi buatLed(pin) {
  pinMode(pin, OUTPUT);
  balik { pin: pin,
          nyala: fungsi() { digitalWrite(this.pin, HIGH); },
          mati:  fungsi() { digitalWrite(this.pin, LOW); } };
}
isi led = buatLed(2);
led.nyala();
```

### 10.3 Tidak ada objek `Math`

Fungsi matematika terdaftar sebagai **fungsi global**, bukan metode `Math`.
`Math.sqrt(2)` menghasilkan `[Math] tidak terdefinisikan`; yang benar `sqrt(2)`.
Konstanta `PI` juga global. Tidak ada `Math.random()` — pakai `random()`/`acak()`.

### 10.4 FFI dimatikan pada build ini

Builtin `ffi()` dan `ffi_cb_free()` dinonaktifkan di firmware ESP32 (baris
pendaftarannya dikomentari di `baik_init_builtin`). Memanggil `ffi` menghasilkan
`[ffi] tidak terdefinisikan`. Untuk memanggil kode C baru, tambahkan fungsi
native di modul `src/esp32/` lalu kompilasi ulang firmware.

### 10.5 Tidak ada `setInterval`, `setTimeout`, atau timer asinkron

Tidak ada event loop maupun penjadwal di dalam bahasa. Semua pewaktuan bersifat
sinkron:

- gunakan `delay()` untuk menunggu;
- gunakan `millis()` di dalam loop untuk pekerjaan berkala tanpa memblokir;
- interupsi perangkat keras **tidak** memanggil callback BAIK secara langsung —
  callback baru dijalankan saat `serviceInterrupts()` dipanggil (otomatis ketika
  REPL menganggur, atau manual dari dalam loop skrip).

```baik
isi berikut = millis();
untuk (isi i = 0; i < 20; i++) {
  ulang (millis() < berikut) { yield(); }
  berikut = berikut + 500;
  digitalToggle(LED_BUILTIN);
  serviceInterrupts();
}
```

### 10.6 Kata kunci yang dikenali lexer tetapi ditolak parser

Ini sumber kebingungan yang paling sering. Dua belas kata di bawah ini **ada di
dalam daftar kata tercadang** bahasa BAIK, sehingga pemindai token (lexer)
mengenalinya sebagai kata kunci — tetapi parser belum punya aturan untuk
menerjemahkannya menjadi bytecode.

| Kata kunci | Maksud aslinya |
|---|---|
| `var` | deklarasi variabel — **pakai `isi`** |
| `sama` | `case` |
| `pilih` | `switch` |
| `standar` | `default` |
| `kerjakan` | `do` (do-while) |
| `try` | `try` |
| `catch` | `catch` |
| `throw` | `throw` |
| `new` | `new` |
| `delete` | `delete` |
| `instanceof` | `instanceof` |
| `void` | `void` |
| `with` | `with` |

Akibatnya pesan galat yang muncul adalah **`[var] tidak terimplementasi`**
(`BAIK_SYNTAX_ERROR`) — bukan `[var] tidak terdefinisikan`. Bedanya penting saat
membaca galat:

- **`tidak terimplementasi`** = katanya dikenal sebagai kata kunci, tetapi
  fiturnya belum dibuat. Tidak ada gunanya memeriksa ejaan; tulis ulang kodenya
  dengan konstruksi lain.
- **`tidak terdefinisikan`** = itu nama biasa (variabel atau fungsi) yang tidak
  ada isinya. Periksa ejaan, atau pastikan modulnya memang tersedia.

Karena kata-kata itu tercadang, keduanya juga **tidak boleh dipakai sebagai nama
variabel**. `isi pilih = 3;` tetap galat.

Yang benar-benar berjalan sebagai pernyataan hanyalah: `isi`, blok `{ }`,
`balik`, `untuk`, `ulang`, `berhenti`, `teruskan`, `jika`, dan `lainnya`.

#### Pengganti `pilih` / `sama` / `standar` (switch/case/default)

Pakai rantai `jika` / `lainnya jika` / `lainnya`. Cabang `lainnya` terakhir
berperan sebagai `standar` (default).

```baik
fungsi jelaskanStatus(kode) {
  jika (kode === WL_CONNECTED) {
    tulis("tersambung");
  } lainnya jika (kode === WL_NO_SSID_AVAIL) {
    tulis("SSID tidak ditemukan");
  } lainnya jika (kode === WL_CONNECT_FAILED) {
    tulis("sandi salah atau ditolak");
  } lainnya jika (kode === WL_DISCONNECTED) {
    tulis("terputus");
  } lainnya {
    tulis("status lain:", kode);   // ini cabang "standar"
  }
}
jelaskanStatus(WiFi.status());
```

Bila cabangnya banyak dan hanya memetakan nilai ke nilai, sebuah objek sering
lebih ringkas daripada rantai `jika`:

```baik
isi namaMode = { "0": "STA", "1": "AP", "2": "AP+STA" };
tulis(namaMode["1"]);
```

#### Pengganti `kerjakan ... ulang` (do-while)

BAIK tidak punya loop yang badannya dijamin jalan sekali lebih dulu. Tirukan
dengan `ulang (benar)` yang diakhiri `berhenti`:

```baik
isi n = 0;
ulang (benar) {
  n++;                        // badan loop selalu jalan minimal sekali
  tulis("percobaan ke-", n);
  jika (n >= 3) { berhenti; } // syarat keluar dievaluasi di akhir
}
```

#### Pengganti `try` / `catch` / `throw`

Lihat [10.1](#101-try--catch--throw-belum-diimplementasi): periksa nilai balik
(`salah`, `-1`, atau `kosong`), jangan menunggu exception.

### 10.7 Batasan lain yang perlu diketahui

| Batasan | Keterangan |
|---|---|
| Kata kunci ditolak parser | `var`, `pilih`, `sama`, `standar`, `kerjakan`, `try`, `catch`, `throw`, `new`, `delete`, `instanceof`, `void`, `with` — lihat [10.6](#106-kata-kunci-yang-dikenali-lexer-tetapi-ditolak-parser). |
| Metode array terbatas | Hanya `.panjang`, `.push()`, `.splice()`. Tidak ada `map`, `filter`, `forEach`, `slice`, `join`, `sort`. |
| Metode string terbatas | Hanya `.panjang`, `.at()`/`.charCodeAt()`, `.indexOf()`, `.slice()`, dan indeks `s[0]`. Tidak ada `split`, `replace`, `toUpperCase`, `trim`. |
| Tidak ada `Date` | Waktu dinding hanya lewat objek `NTP`. |
| Tidak ada `RegExp`, `Promise`, `async`/`await`, modul `import` | — |
| Tidak ada angka bulat 64 bit | Semua angka adalah `double`; ketelitian bulat aman sampai 2^53. |
| REPL satu baris | Blok bertingkat lebih baik dijalankan dari berkas `.ina`. |

---

## Lihat juga

- `docs/BAHASA.md` — rujukan lengkap sintaks bahasa BAIK
- `docs/PINOUT.md` — tabel pin per papan
- `docs/MULAI-CEPAT.md` — panduan pemasangan dan langkah pertama
- `examples/README.md` — daftar contoh skrip siap jalan
