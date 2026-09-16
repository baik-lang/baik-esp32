# Arsitektur Firmware BAIK × ESP32

Dokumen ini menjelaskan bagaimana potongan-potongan firmware ini menyatu: apa yang
terjadi sejak papan menyala, di mana interpreter dibuat, bagaimana API ESP32 sampai ke
skrip BAIK, dan bagaimana kamu menambahkan fungsi baru sendiri.

Sasaran pembaca: kontributor, dan siapa pun yang penasaran mengapa `attachInterrupt` di
sini tidak benar-benar memanggil kodemu dari dalam ISR.

---

## Gambaran besar

```
                    ┌──────────────────────────────────────────┐
                    │              ESP32 / ESP32-S3            │
                    └──────────────────────────────────────────┘

   USB serial                    task "console_repl"
   115200 8N1                    (prioritas 2, stack 4096 B)
       │                                   │
       │   ┌───────────────────────────────┴───────────────────────────┐
       └──▶│  linenoise  ──▶  kata pertama?  ──┬──▶ esp_console_run()   │
           │  (edit baris,                    │    help/ls/pinout/run… │
           │   riwayat)                       │                        │
           │                                  └──▶ baik_exec()         │
           │                                       interpreter BAIK    │
           └───────────────────────────────────────────┬───────────────┘
                                                       │
                             baik_esp32_register()  ────┘
                                       │
   ┌───────────────────────────────────┴────────────────────────────────┐
   │ e32_konstanta │ e32_gpio │ e32_sistem │ e32_bus │ e32_jaringan │ e32_berkas │
   └───────────────────────────────────┬────────────────────────────────┘
                                       │
                              Arduino-ESP32 / ESP-IDF
                                       │
                    GPIO · ADC · LEDC · I2C · SPI · Wi-Fi · SPIFFS · NVS

   task "loopTask" (Arduino)                 task ESPAsyncWebServer
       setup() lalu loop() kosong                │
                                                 ├─ GET  /        index.html (editor)
                                                 ├─ GET  /ap      config.html (Wi-Fi)
                                                 ├─ POST /upload  tulis berkas -> SPIFFS -> restart
                                                 └─ POST /config  simpan SSID -> NVS -> restart
```

Tiga task berjalan berdampingan: task Arduino (`setup`/`loop`), task REPL yang memiliki
interpreter, dan task server web asinkron. Interpreter BAIK **hanya** disentuh dari task
REPL — ini bukan kebetulan, lihat [bagian interupsi](#interupsi-ditunda-dan-alasannya).

---

## Alur boot

Urutannya ada di `src/main.cpp` (`setup()`) dan `src/console.cpp`
(`Console::repl_task`).

**1. `Serial.begin(115200)`** — serial dasar untuk pesan boot.

**2. `SPIFFS.begin(true)`** — pasang sistem berkas. Argumen `true` berarti "format
kalau gagal dipasang", sehingga papan baru tidak langsung mati. Kalau pemasangan gagal,
`setup()` **langsung `return`** dan tidak ada yang berjalan setelahnya — termasuk REPL.

**3. `preferences.begin("wifi", false)`** — buka namespace NVS bernama `wifi` dan baca
`ssid` serta `password` yang tersimpan.

**4. Wi-Fi atau Access Point:**
- SSID kosong → `WiFi.softAP("ESP32_AP", "12345678")`. Papan menjadi titik akses sendiri
  (biasanya di `192.168.4.1`).
- SSID ada → `WiFi.begin(...)` lalu **menunggu di dalam loop** sampai `WL_CONNECTED`,
  mencetak `Connecting to WiFi...` setiap detik. Perlu diketahui: loop ini tidak punya
  batas waktu, jadi kredensial yang salah membuat boot tertahan di sini dan REPL tidak
  pernah dimulai. *(TODO: batas waktu + kembali ke mode AP.)*

**5. `baikRun()`** — menyiapkan konsol:
```cpp
console.setPrompt("Baik> ");
console.begin(115200);              // UART + esp_console + task REPL
console.registerSystemCommands();   // sysinfo, meminfo, restart
console.registerVFSCommands();      // ls, cat, cd, pwd, mv, cp, rm, rmdir, edit
console.registerGPIOCommands();     // pinMode, digitalRead, digitalWrite, analogRead
console.registerNetworkCommands();  // ping, ipconfig
console.registerBaikCommands();     // pinout, api, run
```
Urutannya penting: `begin()` memanggil `esp_console_init()`, dan
`esp_console_cmd_register()` menuntut inisialisasi itu sudah terjadi. `begin()` juga
mendaftarkan `help`, `clear`, dan `history`, lalu membuat task REPL dengan
`xTaskCreate(..., "console_repl", 4096, this, 2, &task_)`.

**6. Rute server web didaftarkan**, lalu `server.begin()`.

**7. Di dalam task REPL** (`Console::repl_task`), berurutan:
```cpp
struct baik *baik = baik_create();   // interpreter dibuat DI SINI
s_baik = baik;                       // disimpan untuk perintah `run` & poll interupsi
baik_esp32_register(baik);           // seluruh API ESP32 masuk ke object global
readFileToCStr("/baik.ina", isi);    // autorun bila ada
baik_exec(baik, isi.c_str(), &res);
```
Galat pada autorun dicetak tetapi **tidak** mematikan REPL — statusnya direset supaya
prompt tetap muncul.

**8. Salam pembuka dicetak, `linenoiseProbe()` menguji terminal**, lalu loop REPL
berjalan selamanya.

Perhatikan bahwa interpreter dibuat di **task REPL**, bukan di `setup()`. Alasannya
`stdin`/`stdout` baru diarahkan ke UART yang benar di dalam task itu, dan interpreter
mencetak lewat `stdout`.

---

## Peran tiap berkas

### `src/`

| Berkas | Peran |
|---|---|
| `baik/` | Seluruh bahasa BAIK, dipecah menjadi 17 berkas `.c` + `baik_internal.h` (±12.800 baris): tokenizer, parser, penghasil bytecode, mesin virtual, pemungut sampah, tipe data, dan fungsi bawaan. **Jangan diedit** untuk menambah API ESP32 — lihat [cara menambah fungsi](#cara-menambah-fungsi-api-baru) dan [susunan modul](#susunan-modul-interpreter-srcbaik). |
| `baik.h` | API publik interpreter: `baik_create`, `baik_exec`, `baik_arg`, `baik_return`, `baik_own`, dan seterusnya. Sebagian deklarasi object (`baik_mk_object`, `baik_set`, `baik_get`) dikomentari di sini; deklarasi ulangnya ada di `esp32/baik_esp32.h`. |
| `main.cpp` | `setup()`: SPIFFS → NVS → Wi-Fi/AP → konsol → server web. `loop()` sengaja kosong. Juga berisi `handleFileUpload()` untuk POST `/upload`. |
| `console.h` | Kelas `ESP32Console::Console` — versi lokal dari pustaka ESP32Console, ditambah `registerBaikCommands()`, `catatPerintah()`, dan `adalahPerintah()`. |
| `console.cpp` | Isi task REPL, pencatat nama perintah, dan implementasi perintah `pinout`, `api`, `run`. |

### Susunan modul interpreter (`src/baik/`)

Interpreter dulu ditulis sebagai satu berkas tunggal `src/baik.c`. Berkas itu sudah
dipecah menjadi modul di folder `src/baik/`, dan `src/baik.c` tidak ada lagi.

Yang penting untuk diketahui kode di luar interpreter: **tidak ada yang berubah**.
`src/baik.h` sama persis seperti sebelumnya, jadi `main.cpp`, `console.cpp`, dan seluruh
`src/esp32/` tetap cukup menulis `#include "baik.h"`. Jejak RAM dan hasil build kedua
papan juga tidak berubah.

Modulnya tersusun berlapis — lapisan bawah tidak memanggil lapisan atas:

```
  baik_builtin.c                       tulis, muat, gc, die, JSON, Object
        ▲
  baik_tokenizer.c → baik_parser.c → baik_bcode.c → baik_exec.c
        ▲
  baik_primitive.c · baik_string.c · baik_array.c · baik_object.c
  baik_conversion.c · baik_json.c
        ▲
  baik_core.c  ◀──▶  baik_gc.c        daur hidup, tumpukan, lingkup | arena, mark/sweep
        ▲
  baik_em_common.c                     log, berkas, varint, mbuf, utilitas string
```

Di samping alur utama: `baik_util.c` (typeof, pencetakan nilai, disassembler, nomor
baris), `baik_ffi.c` (sebagian besar nonaktif di build ini), dan `baik_repl.c` (REPL
build host, hanya aktif bila `BAIK_MAIN` didefinisikan).

Dua hal teknis yang perlu diingat bila kamu menyunting folder itu:

- `baik_internal.h` mendefinisikan `BAIK_EXPOSE_PRIVATE`, yang membuat makro
  `BAIK_PRIVATE` memberi **linkage eksternal**. Sewaktu interpreter masih satu berkas,
  `BAIK_PRIVATE` berarti `static` dan itu memadai; setelah dipecah, simbol-simbol itu
  harus terlihat antar berkas objek. Fungsi internal yang dipakai lintas modul ditandai
  `BAIK_PRIVATE` dan wajib punya prototipe di `baik_internal.h`; pembantu yang hanya
  dipakai di satu berkas tetap `static`.
- PlatformIO mengompilasi `src/**` secara **rekursif** dan `platformio.ini` sengaja tidak
  memasang `build_src_filter`, jadi berkas `.c` baru di `src/baik/` otomatis ikut
  terkompilasi tanpa perubahan konfigurasi.

Tabel lengkap tiap modul beserta jumlah barisnya, diagram ketergantungan yang lebih
rinci, dan panduan menambah fungsi bawaan bahasa ada di
[`src/baik/README.md`](../src/baik/README.md).

### `src/esp32/` — jembatan BAIK ⇄ Arduino

| Berkas | Peran |
|---|---|
| `baik_esp32.h` | Satu pintu masuk (`baik_esp32_register`), deklarasi keenam fungsi registrasi modul, deklarasi helper (`e32_arg_*`, `e32_ret_*`, `e32_ns`, `e32_fn`, `e32_const`, `e32_pin_ok`), dan deklarasi ulang API object interpreter. |
| `e32_pins.h` | Tabel kapabilitas pin (`E32_CAP_ADC1`, `E32_CAP_DAC`, `E32_CAP_TOUCH`, `E32_CAP_STRAP`, `E32_CAP_FLASH`, …) dan struct `e32_pin_info_t`. **Sumber kebenaran tunggal** untuk validasi pin, perintah `pinout`, konstanta pin, dan `docs/PINOUT.md`. |
| `e32_konstanta.cpp` | `HIGH`, `LOW`, `INPUT`, `OUTPUT`, `LED_BUILTIN`, `BOOT_BUTTON`, `A0..An`, `T0..Tn`, `BOARD`, `CHIP`, plus fungsi introspeksi `pinInfo`, `pinCaps`, `daftarPin`, `pinout`. Didaftarkan **pertama**, karena modul lain memakai konstantanya. |
| `e32_gpio.cpp` | GPIO, ADC, DAC, PWM/LEDC, sentuh, `tone`, interupsi, `pulseIn`, `shiftIn`/`shiftOut`. Juga rumah bagi `baik_esp32_poll_interrupts()`. |
| `e32_sistem.cpp` | Waktu (`delay`, `millis`), matematika, `Serial`, object `ESP`, frekuensi CPU, suhu internal, *deep/light sleep*, RTC memory, watchdog. |
| `e32_bus.cpp` | `Wire`/`Wire1` (I2C), `SPI`, `Serial1`/`Serial2` (UART). |
| `e32_jaringan.cpp` | `WiFi`, `HTTP`, `NTP`, `MDNS`. |
| `e32_berkas.cpp` | `FS` (SPIFFS), `NVS` (Preferences), `jalankan()`. |
| `e32_helper.cpp` | Implementasi helper bersama, `baik_esp32_register()`, `e32_print_pinout()`, `e32_print_api()`. |

Daftar fungsi lengkap per modul ada di [API.md](API.md).

---

## Bagaimana API ESP32 didaftarkan

Satu panggilan mengurus semuanya:

```cpp
baik_esp32_register(baik);
```

Isinya memanggil keenam modul secara berurutan, dengan `g` = object global interpreter:

```cpp
void baik_esp32_register(struct baik *baik) {
  baik_val_t g = baik_get_global(baik);
  baik_esp32_register_konstanta(baik, g);   /* HARUS pertama */
  baik_esp32_register_gpio(baik, g);
  baik_esp32_register_sistem(baik, g);
  baik_esp32_register_bus(baik, g);
  baik_esp32_register_jaringan(baik, g);
  baik_esp32_register_berkas(baik, g);
}
```

Setiap modul menambahkan properti ke object global. Nama datar (`pinMode`) langsung
menempel di `g`; nama bernamespace (`WiFi.begin`) menempel di sub-object yang dibuat
dengan `e32_ns(baik, g, "WiFi")`.

Dua jenis nilai yang didaftarkan:

```cpp
e32_fn(baik, g, "digitalWrite", e32_digitalWrite);   /* fungsi native  */
e32_const(baik, g, "HIGH", 1);                       /* konstanta angka */
```

Di balik layar keduanya memakai `baik_set(baik, obj, nama, ~0, nilai)` — `~0` berarti
"hitung sendiri panjang namanya". Fungsi native dibungkus dengan
`baik_mk_foreign_func()`, itulah sebabnya `tipe pinMode` menghasilkan `"foreign_ptr"`
dan bukan `"fungsi"`.

**Alias** bukan pembungkus. `tulisDigital` adalah nilai yang **sama persis** dengan
`digitalWrite` — pendaftaran kedua dengan pointer fungsi yang sama, tanpa biaya
tambahan saat dipanggil.

Objek atau array yang disimpan di sisi C harus di-`baik_own()` agar tidak dibuang
pemungut sampah.

---

## Perintah konsol vs ekspresi BAIK

Setiap baris yang kamu ketik harus diputuskan: perintah konsol atau kode BAIK?

Aturannya sederhana dan hanya melihat **kata pertama**:

```cpp
String baris = interpolated_line;  baris.trim();
int batas = /* posisi spasi atau tab pertama */;
String kataPertama = (batas < 0) ? baris : baris.substring(0, batas);

if (Console::adalahPerintah(kataPertama.c_str()))
    esp_console_run(interpolated_line.c_str(), &ret);   // perintah konsol
else
    baik_exec(baik, interpolated_line.c_str(), &res);   // kode BAIK
```

Daftar nama perintah tidak ditulis tangan. `esp_console` tidak menyediakan cara
menelusuri perintah yang terdaftar, jadi `Console::registerCommand()` memanggil
`catatPerintah()` setiap kali ia mendaftarkan sesuatu. Hasilnya: daftar itu otomatis
ikut bertambah ketika `registerVFSCommands()` atau `registerBaikCommands()` dipanggil,
dan tidak pernah basi. (`help` adalah pengecualian: ia didaftarkan langsung oleh
`esp_console_register_help_command()`, jadi namanya dicatat manual.)

Konsekuensi yang perlu diingat:

- **Nama perintah menutupi nama variabel.** Baris `ls` selalu berarti perintah `ls`.
  Kalau kamu punya fungsi BAIK bernama `run`, panggil dengan tanda kurung — `run(1)`
  bukan kata pertama yang cocok persis, tetapi `run` sendirian akan dianggap perintah.
- **Baris kosong dilewati** sebelum pengecekan.
- **Galat BAIK tidak mematikan REPL.** `baik_print_error()` mencetak jejaknya, status
  galat direset, prompt kembali.

---

## Interupsi: ditunda, dan alasannya

Interpreter BAIK **tidak reentrant** dan mengalokasikan memori saat menjalankan kode.
Memanggilnya dari dalam ISR akan merusak *heap* atau membuat papan panik: ISR ESP32
berjalan dengan stack terbatas, tidak boleh memblokir, dan bisa menyela task REPL tepat
di tengah-tengah interpreter sedang bekerja.

Karena itu `attachInterrupt(pin, fungsiBAIK, RISING)` dipasang dengan mekanisme dua
tahap:

```
  ┌──────────────┐  tepi sinyal   ┌───────────────────────────────┐
  │  pin GPIO    │ ─────────────▶ │ ISR C (IRAM)                  │
  └──────────────┘                │   penghitung[pin]++           │  ← cepat, tanpa alokasi
                                  └───────────────┬───────────────┘
                                                  │
                                  (kembali ke eksekusi normal)
                                                  │
  ┌────────────────────────────────────────────────▼──────────────┐
  │ task REPL, tepat sebelum menunggu baris berikutnya:           │
  │   baik_esp32_poll_interrupts(s_baik);                         │
  │     untuk tiap pin dengan penghitung > 0:                     │
  │       kosongkan penghitung, lalu panggil callback BAIK-nya    │  ← aman, di luar ISR
  └───────────────────────────────────────────────────────────────┘
```

ISR hanya menaikkan `volatile uint32_t` per pin. Callback BAIK-nya dieksekusi kemudian
dari task konsol lewat `baik_esp32_poll_interrupts(baik)`, yang dipanggil di awal setiap
putaran loop REPL.

Akibat praktisnya:

- **Latensi mengikuti REPL.** Kalau kamu sedang tidak mengetik, callback dilayani saat
  putaran berikutnya. Kalau sebuah skrip panjang sedang berjalan, callback menunggu
  sampai skrip itu selesai.
- **Interupsi beruntun digabung** bila penghitungnya belum sempat dibaca. Gunakan
  `interruptCount(pin)` untuk mengetahui berapa banyak yang tertahan.
- **Panggil `serviceInterrupts()` sendiri** dari dalam loop panjangmu supaya callback
  tetap dilayani:

```javascript
untuk (isi i = 0; i < 1000; i++) {
  serviceInterrupts();
  delay(10);
}
```

Prinsip yang sama berlaku untuk `touchAttachInterrupt`.

---

## Jejak memori

Angka yang bisa dipastikan dari kode:

| Hal | Nilai | Sumber |
|---|---|---|
| Stack task REPL | 4096 bita | `xTaskCreate(..., "console_repl", 4096, ...)` di `console.cpp` |
| Prioritas task REPL | 2 | idem |
| Panjang maksimum satu baris masukan | 256 karakter | nilai bawaan konstruktor `Console` |
| Jumlah maksimum argumen perintah | 8 | idem |
| Panjang riwayat perintah | 40 baris | `max_history_len_` |
| Kapasitas daftar nama perintah | 64 | `BAIK_MAKS_PERINTAH` di `console.cpp` |
| Arena objek GC (awal / tambahan) | 20 / 10 sel | `BAIK_OBJECT_ARENA_SIZE`, `BAIK_OBJECT_ARENA_INC_SIZE` di `src/baik/baik_internal.h` |
| Arena properti GC (awal / tambahan) | 20 / 10 sel | `BAIK_PROPERTY_ARENA_SIZE`, `BAIK_PROPERTY_ARENA_INC_SIZE` di `src/baik/baik_internal.h` |
| Batas kedalaman parser | 512 tingkat | `STACK_LIMIT / BINOP_STACK_FRAME_SIZE` (8192/16) |

Selain itu:

- Bytecode hasil kompilasi **menumpuk** di dalam interpreter. Setiap baris REPL dan
  setiap `run` menambah bagian bytecode baru; memori itu tidak dilepas selama papan
  menyala. Sesi REPL yang sangat panjang akan menggerus heap.
- Arena tumbuh dengan `realloc` sesuai kebutuhan; `gc(benar)` memaksa siklus penuh.
- `ESP.getFreeHeap()` dan perintah `meminfo` adalah cara termudah memantau sisa memori
  saat berjalan.
- Ukuran biner firmware bergantung papan dan versi framework. *TODO: catat angka
  nyatanya dari `pio run` untuk kedua environment.*

---

## Cara menambah fungsi API baru

Misalkan kita ingin menambahkan `blinkCepat(pin, jumlah)` yang mengedipkan sebuah pin.

### 1. Pilih modulnya

Fungsi GPIO → `src/esp32/e32_gpio.cpp`. Jangan sekali-kali menambahkannya ke
`src/baik/`: folder itu adalah bahasa, bukan perangkat keras.

### 2. Tulis fungsi native

Semua fungsi native punya tanda tangan yang sama: `void fn(struct baik *baik)`. Argumen
diambil dengan helper `e32_arg_*`, nilai balik dikirim dengan `e32_ret_*`.

```cpp
/* blinkCepat(pin, jumlah) -> jumlah kedipan yang benar-benar dilakukan */
static void e32_blink_cepat(struct baik *baik) {
  int pin    = e32_arg_int(baik, 0, -1);
  int jumlah = e32_arg_int(baik, 1, 1);

  /* WAJIB: validasi pin sebelum menyentuh perangkat keras.
   * e32_pin_ok() sudah mencetak pesan galat berbahasa Indonesia bila gagal. */
  if (!e32_pin_ok(baik, pin, E32_CAP_OUTPUT, "blinkCepat")) {
    e32_ret_undef(baik);
    return;
  }

  pinMode(pin, OUTPUT);
  for (int i = 0; i < jumlah; i++) {
    digitalWrite(pin, HIGH);
    delay(50);
    digitalWrite(pin, LOW);
    delay(50);
  }

  e32_ret_int(baik, jumlah);
}
```

Aturan yang tidak boleh dilanggar:

- **Selalu validasi pin** dengan `e32_pin_ok(baik, pin, CAPS, "namaFungsi")`.
- **Selalu kembalikan nilai** — `e32_ret_*` atau `e32_ret_undef()` di setiap jalur
  keluar, termasuk jalur galat.
- **Jangan melempar exception**; bahasa ini tidak punya `try`/`catch`. Laporkan galat
  lewat nilai balik (`salah`, `-1`, `kosong`) dan pesan ke konsol.
- **Jangan memblokir lama-lama.** `delay()` di sini memakai `vTaskDelay`, jadi RTOS tetap
  berjalan, tetapi REPL tetap tertahan selama fungsimu berjalan.

### 3. Daftarkan

Di dalam `baik_esp32_register_gpio()`, tambahkan satu baris — plus alias bahasa
Indonesia bila fungsinya sering dipakai:

```cpp
e32_fn(baik, g, "blinkCepat", e32_blink_cepat);
e32_fn(baik, g, "kedipCepat", e32_blink_cepat);   /* alias, fungsi yang sama persis */
```

Untuk fungsi yang bernamespace, ambil sub-object-nya dulu:

```cpp
baik_val_t wifi = e32_ns(baik, g, "WiFi");
e32_fn(baik, wifi, "kekuatanSinyal", e32_wifi_rssi);
```

Untuk konstanta:

```cpp
e32_const(baik, g, "MODE_CEPAT", 3);
```

### 4. Kompatibilitas dua papan dan dua versi Arduino

Fitur yang tidak ada di salah satu chip tetap **harus didaftarkan**, tetapi mengembalikan
galat yang jelas:

```cpp
static void e32_dac_write(struct baik *baik) {
#if defined(SOC_DAC_SUPPORTED)
  /* ... implementasi normal ... */
#else
  printf("Galat: DAC tidak tersedia pada %s\n", e32_board_chip());
  e32_ret_undef(baik);
#endif
}
```

API LEDC berubah antara arduino-esp32 2.x dan 3.x; pisahkan dengan
`#if ESP_ARDUINO_VERSION_MAJOR >= 3`.

### 5. Dokumentasikan dan uji

1. Tambahkan barisnya ke tabel yang sesuai di [API.md](API.md).
2. Tambahkan ke keluaran `e32_print_api()` supaya muncul di perintah konsol `api`.
3. Bangun **kedua** environment:
   ```bash
   pio run -e esp32doit-devkit-v1 && pio run -e esp32-s3-devkitc-1
   ```
4. Coba di papan:
   ```
   Baik> kedipCepat(LED_BUILTIN, 5)
   Baik> kedipCepat(6, 5)        # pin flash -> harus ditolak dengan pesan jelas
   ```
5. Lihat [PENGUJIAN.md](PENGUJIAN.md) untuk uji host dan QEMU.

---

## Catatan tambahan untuk kontributor

- **`src/baik/` dan `src/baik.h` tidak boleh diedit** untuk keperluan ESP32. Deklarasi
  yang hilang ditambahkan sebagai `extern "C"` di `src/esp32/baik_esp32.h`. Panduan
  menyunting interpreter itu sendiri ada di [`src/baik/README.md`](../src/baik/README.md).
- **Editor web tidak boleh memuat apa pun dari CDN.** Dalam mode Access Point, perangkat
  yang terhubung tidak punya akses internet; halaman yang bergantung pada CDN akan
  tampil kosong. Semua CSS dan JS ditulis inline di `data/index.html` dan
  `data/config.html`.
- **Setiap unggahan berkas memicu `ESP.restart()`** (lihat `handleFileUpload` di
  `main.cpp`). Itu disengaja: cara paling sederhana memastikan `/baik.ina` yang baru
  benar-benar dijalankan.
- **`muat()` memakai path POSIX** (`/spiffs/berkas.ina`) karena ia memanggil `fopen()`,
  sedangkan API `FS.*` dan perintah `run` memakai path gaya Arduino (`/berkas.ina`).
  Perbedaan ini sering membingungkan; perintah `run` menerima kedua bentuk.

---

## Lihat juga

- [BAHASA.md](BAHASA.md) — apa saja yang bisa dan tidak bisa dilakukan bahasanya.
- [API.md](API.md) — daftar lengkap fungsi ESP32.
- [PINOUT.md](PINOUT.md) — tabel kapabilitas pin.
- [PENGUJIAN.md](PENGUJIAN.md) — membangun, menguji, dan mengemulasi.
