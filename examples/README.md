# Contoh skrip BAIK untuk ESP32

Direktori ini berisi 17 skrip `.ina` yang siap dijalankan di papan ESP32 atau
ESP32-S3. Semuanya sudah diuji terhadap parser BAIK yang sesungguhnya, memakai
kata kunci bahasa BAIK yang benar, dan berakhir sendiri sehingga REPL tetap bisa
dipakai setelah skrip selesai.

Rujukan API lengkap ada di [`../docs/API.md`](../docs/API.md).

---

## Daftar contoh

| Berkas | Yang diperagakan | Perangkat keras | Papan |
|---|---|---|---|
| [`01-halo.ina`](01-halo.ina) | `tulis()`, konstanta `BOARD`/`CHIP`, objek `ESP`, cara mencetak angka tanpa konversi implisit | — | ESP32, S3 |
| [`02-kedip-led.ina`](02-kedip-led.ina) | `ledBawaan` (portabel), `pinMode`, `digitalWrite`, `digitalToggle`, `rgbLedWrite` | LED bawaan (atau LED + resistor 220 Ω di GPIO2) | ESP32, S3 |
| [`03-tombol.ina`](03-tombol.ina) | `INPUT_PULLUP`, `digitalRead`, anti-pantul, `berhenti` | Tombol BOOT bawaan (GPIO0) | ESP32, S3 |
| [`04-pwm-fade.ina`](04-pwm-fade.ina) | `analogWrite`, `ledcAttach`, `ledcWrite`, `ledcRead`, `ledcReadFreq` | LED bawaan atau LED + resistor 220 Ω | ESP32, S3 |
| [`05-baca-analog.ina`](05-baca-analog.ina) | `analogRead`, `analogReadMilliVolts`, `map`, `daftarPin(CAP_ADC1)` | Potensiometer 10 kΩ (opsional) | ESP32, S3 |
| [`06-sensor-sentuh.ina`](06-sensor-sentuh.ina) | `touchRead`, `touchSetCycles`, kalibrasi ambang per papan | Kabel jumper 10–20 cm sebagai pelat sentuh | ESP32, S3 |
| [`07-dac-gelombang.ina`](07-dac-gelombang.ina) | `dacWrite`, `dacDisable`, `sin()`, deteksi kapabilitas | Multimeter/osiloskop, atau LED + resistor 1 kΩ di GPIO25 | **ESP32 saja** |
| [`08-pindai-i2c.ina`](08-pindai-i2c.ina) | `Wire.begin`, `pindaiI2C()`, `Wire.readReg` | Minimal satu modul I2C (OLED, RTC, BME280, …) | ESP32, S3 |
| [`09-suhu-chip.ina`](09-suhu-chip.ina) | `bacaSuhu()`, `millis`, pemantauan berkala | — | ESP32, S3 |
| [`10-wifi-sambung.ina`](10-wifi-sambung.ina) | `WiFi.begin`, `WiFi.tungguKoneksi`, `WiFi.localIP`, `WiFi.RSSI` | Jaringan WiFi **2,4 GHz** | ESP32, S3 |
| [`11-wifi-pindai.ina`](11-wifi-pindai.ina) | `WiFi.scanNetworks`, menelusuri array objek | — | ESP32, S3 |
| [`12-http-get.ina`](12-http-get.ina) | `HTTP.get`, `HTTP.statusTerakhir`, `HTTP.getJSON` | WiFi sudah tersambung (jalankan `10-` dulu) | ESP32, S3 |
| [`13-simpan-nvs.ina`](13-simpan-nvs.ina) | `NVS.begin/end`, `putInt/getInt`, `putString`, `isKey`, `remove` | — | ESP32, S3 |
| [`14-berkas-spiffs.ina`](14-berkas-spiffs.ina) | `FS.write/append/read/list/rename/remove`, statistik ruang | — | ESP32, S3 |
| [`15-interupsi-tombol.ina`](15-interupsi-tombol.ina) | `pasangInterupsi` (callback wajib fungsi BAIK), `interruptCount`, `serviceInterrupts` | Tombol BOOT bawaan (GPIO0) | ESP32, S3 |
| [`16-tidur-dalam.ina`](16-tidur-dalam.ina) | `sebabBangun`, `rtcSet/rtcGet`, `sleepTimer`, `tidurDalam` | — (opsional tombol di pin RTC) | ESP32, S3 |
| [`17-pinout-info.ina`](17-pinout-info.ina) | `pinInfo`, `pinPunya`, `daftarPin`, `pinout` | — | ESP32, S3 |

Urutan yang disarankan untuk belajar: **01 → 02 → 03 → 17**, lalu pilih topik
sesuai kebutuhan (analog: 04–07, bus: 08, jaringan: 10–12, penyimpanan: 13–14,
tingkat lanjut: 15–16).

---

## Cara menjalankan

### Cara 1 — unggah ke SPIFFS lalu `jalankan()` (disarankan)

1. Sambungkan komputer ke papan (lewat jaringan WiFi yang sama, atau ke titik
   akses `ESP32_AP` yang dibuat papan bila belum dikonfigurasi).
2. Buka editor web papan di peramban: `http://<ip-papan>/`. Alamat IP-nya
   dicetak ke konsol serial saat papan menyala.
3. Unggah berkas `.ina` yang diinginkan. Papan akan **boot ulang sendiri**
   setelah unggahan selesai.
4. Buka monitor serial pada **115200 baud**, lalu di prompt `Baik> ` ketik:

   ```baik
   jalankan("/02-kedip-led.ina")
   ```

   `run` adalah alias yang sama persis:

   ```baik
   run("/02-kedip-led.ina")
   ```

Bila proyek ini dibangun dari sumber, berkas di direktori `data/` juga bisa
diunggah sekaligus dengan `pio run -t uploadfs`.

### Cara 2 — tempel langsung ke REPL

REPL memproses **satu baris tiap kali Enter**, jadi cara ini cocok untuk
mencoba beberapa baris pendek, bukan untuk menempelkan seluruh berkas:

```baik
Baik> isi pin = LED_BUILTIN
Baik> pinMode(pin, OUTPUT)
Baik> digitalWrite(pin, HIGH)
```

Blok bertingkat (`jika { … }`, `untuk { … }`, `fungsi { … }`) harus muat dalam
**satu baris** bila ditempel ke REPL:

```baik
Baik> untuk (isi i = 0; i < 5; i++) { digitalToggle(LED_BUILTIN); delay(200); }
```

Kalau blokmu lebih panjang dari itu, pakai Cara 1.

### Cara 3 — jalan otomatis saat papan menyala

Berkas bernama **`/baik.ina`** dijalankan sendiri setiap kali papan boot,
sebelum prompt REPL muncul. Unggah salah satu contoh dengan nama `baik.ina`
untuk menjadikannya program tetap papan.

> **Hati-hati.** Bila `/baik.ina` berisi perulangan tak terbatas, papan akan
> terkunci di skrip itu setiap kali menyala dan REPL **tidak pernah** muncul.
> Untuk memulihkannya: buka editor web dan unggah `baik.ina` baru yang kosong,
> atau hapus berkasnya. Bila editor web pun tak bisa diakses, unggah ulang
> sistem berkas dari komputer (`pio run -t uploadfs`).

---

## Catatan penting sebelum mengedit contoh

### Kata kunci BAIK, bukan JavaScript

Contoh-contoh ini memakai kata kunci bahasa BAIK. Yang paling sering menjebak:

| Tulis ini | **Bukan** ini |
|---|---|
| `isi x = 1;` | `var x = 1;` / `let x = 1;` |
| `fungsi f() { balik 1; }` | `function f() { return 1; }` |
| `jika (a) { } lainnya { }` | `if (a) { } else { }` |
| `ulang (a) { }` | `while (a) { }` |
| `untuk (isi i = 0; …)` | `for (var i = 0; …)` |
| `berhenti;` / `teruskan;` | `break;` / `continue;` |
| `benar` / `salah` / `kosong` | `true` / `false` / `null` |
| `tulis(x)` | `console.log(x)` |
| `a.panjang` | `a.length` |

**`var` TIDAK berfungsi** — parser menolaknya dengan
`[var] tidak terimplementasi`. Deklarasi variabel selalu memakai **`isi`**.
Begitu pula `pilih`/`sama`/`standar` (switch) dan `kerjakan` (do-while) belum
diimplementasi; pakai rantai `jika` / `lainnya jika` dan `ulang` + `berhenti`.
Daftar lengkapnya ada di
[`../docs/API.md` bagian 10.6](../docs/API.md#106-kata-kunci-yang-dikenali-lexer-tetapi-ditolak-parser).

### Dua jebakan operator

1. **Pakai `===` dan `!==`.** Operator `==` ditolak dengan galat
   `Use ===, not ==`.
2. **Tidak ada konversi tipe implisit.** `"suhu: " + 25` adalah galat. Cetak
   dengan beberapa argumen — `tulis("suhu:", 25)` — atau ubah eksplisit dengan
   `JSON.stringify(25)`.
3. **`map()` memakai pecahan, bukan bilangan bulat.** `map(5, 0, 9, 0, 100)`
   menghasilkan `55.5556` di BAIK, sedangkan Arduino memberi `55`. Bungkus
   dengan `round()` bila kamu butuh bilangan bulat.
4. **Nilai penanda galat tidak seragam.** `FS.read`/`FS.list` dan seluruh
   `HTTP.*` mengembalikan `kosong`; `FS.size` mengembalikan `-1`; sisanya
   `FS.*` dan `NVS.*` mengembalikan `salah`. Periksa yang tepat untuk fungsi
   yang kamu panggil.

### Perulangan tak terbatas memblokir REPL

Semua contoh di sini sengaja memakai perulangan **terbatas** (`untuk` dengan
batas cacah) supaya prompt `Baik> ` kembali muncul setelah skrip selesai.

Kalau kamu mengubahnya menjadi tak terbatas:

```baik
ulang (benar) { digitalToggle(LED_BUILTIN); delay(500); }
```

maka skrip tidak akan pernah berhenti: prompt REPL terblokir dan papan berhenti
menanggapi perintah apa pun. **Satu-satunya cara keluar adalah menekan tombol
RESET (EN) pada papan**, atau mencabut lalu memasang kembali kabel USB. Tidak
ada Ctrl+C. Loop panjang juga sebaiknya memanggil `delay()` atau `yield()` di
setiap putaran, kalau tidak task watchdog akan me-reset papan sendiri.

### ⚠ LED bawaan ESP32-S3 adalah WS2812 — `digitalWrite` tidak berpengaruh

Ini jebakan nomor satu bagi pengguna ESP32-S3. `LED_BUILTIN` di sana bernilai
**48**, tetapi LED di pin itu **bukan LED biasa** melainkan **WS2812** (LED RGB
"pintar") yang dikendalikan lewat protokol satu kabel, bukan level logika:

```baik
digitalWrite(LED_BUILTIN, HIGH);   // ESP32: menyala. ESP32-S3: TIDAK terjadi apa-apa.
```

Kode itu **tidak memberi galat** — LED-nya hanya diam, sehingga mudah disangka
papannya rusak. Yang benar:

```baik
ledBawaan(benar);                       // portabel, jalan di KEDUA papan
rgbLedWrite(LED_BUILTIN, 16, 16, 16);   // atau kendalikan WS2812 langsung
```

Contoh `02`, `03`, dan `15` semuanya memakai `ledBawaan()` justru karena itu.

### Callback interupsi wajib fungsi BAIK

`pasangInterupsi()` dan `sentuhInterupsi()` **hanya menerima fungsi yang kamu
tulis sendiri** dengan kata kunci `fungsi`. Fungsi bawaan ditolak:

```baik
pasangInterupsi(0, digitalToggle, RISING);   // DITOLAK: itu fungsi bawaan
fungsi kedip(pin) { digitalToggle(2); }
pasangInterupsi(0, kedip, RISING);           // BENAR
```

Penanganannya juga **tertunda, bukan real-time**: ISR hanya mencatat, callback
baru jalan saat `serviceInterrupts()` dipanggil, dan paling banyak **8 interupsi
per pin** dilayani tiap putaran. Lihat `15-interupsi-tombol.ina`.

### Contoh yang khusus satu papan

- **`07-dac-gelombang.ina` hanya berjalan penuh di ESP32.** ESP32-S3 tidak punya
  DAC. Skrip itu mendeteksinya sendiri dengan `daftarPin(CAP_DAC)` dan berhenti
  dengan pesan yang jelas, jadi aman dijalankan di kedua papan.

Pola yang sama layak ditiru saat kamu menulis skrip sendiri: tanyakan
kapabilitas pin, jangan menuliskan nomor pin secara kaku.

```baik
jika (pinPunya(25, CAP_DAC)) { dacWrite(25, 128); }
```

---

## Lihat juga

- [`../docs/API.md`](../docs/API.md) — rujukan lengkap seluruh fungsi dan konstanta
- [`../docs/BAHASA.md`](../docs/BAHASA.md) — sintaks bahasa BAIK
- [`../docs/PINOUT.md`](../docs/PINOUT.md) — tabel pin per papan
