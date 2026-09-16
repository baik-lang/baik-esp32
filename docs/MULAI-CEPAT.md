# Mulai Cepat — dari Nol sampai LED Berkedip

Panduan ini membawamu dari papan yang masih di dalam plastik sampai LED-nya berkedip
karena perintahmu sendiri. Waktu yang dibutuhkan sekitar **10 menit**, di luar unduhan
*toolchain* pertama kali (yang bisa memakan beberapa ratus megabita dan beberapa menit).

Yang kamu butuhkan:

- Papan **ESP32 DevKit V1** atau **ESP32-S3-DevKitC-1**.
- **Kabel USB data.** Banyak kabel murah hanya punya kabel daya; kabel seperti itu
  membuat papan menyala tetapi tidak pernah terdeteksi komputer.
- Komputer dengan Python 3.

---

## Langkah 1 — Pasang PlatformIO (2 menit)

Pilih salah satu.

**Lewat VS Code (paling mudah):** buka VS Code → Extensions → cari *PlatformIO IDE* →
Install. Tunggu sampai PlatformIO selesai menyiapkan diri.

**Lewat terminal:**

```bash
python3 -m venv ~/.baik-pio-venv
~/.baik-pio-venv/bin/pip install -U platformio
export PATH="$HOME/.baik-pio-venv/bin:$PATH"
pio --version
```

Lalu ambil kode proyeknya:

```bash
git clone https://github.com/baik-lang/baik-esp32.git
cd baik-esp32
```

---

## Langkah 2 — Sambungkan papan (1 menit)

Colokkan papan ke port USB. Periksa apakah terdeteksi:

```bash
pio device list
```

Kamu harus melihat satu port seperti `/dev/ttyUSB0` (Linux), `/dev/cu.usbserial-…` atau
`/dev/cu.wchusbserial…` (macOS), atau `COM3` (Windows). Kalau tidak ada, lompat ke
[Pemecahan masalah](#pemecahan-masalah) di bawah.

Di Linux, pengguna biasanya perlu masuk grup `dialout` agar boleh memakai port serial:

```bash
sudo usermod -a -G dialout $USER      # lalu logout & login lagi
```

---

## Langkah 3 — Flash firmware (3–5 menit pertama kali)

Pilih environment sesuai papanmu:

```bash
# ESP32 DevKit V1
pio run -e esp32doit-devkit-v1 -t upload

# ESP32-S3-DevKitC-1
pio run -e esp32-s3-devkitc-1 -t upload
```

Unduhan pertama memakan waktu paling lama karena PlatformIO mengambil *toolchain*
Xtensa dan framework Arduino-ESP32. Perintah berikutnya jauh lebih cepat.

Berhasil bila baris terakhir berbunyi `SUCCESS`.

---

## Langkah 4 — Unggah SPIFFS (1 menit)

Ini langkah yang paling sering terlupa, padahal wajib:

```bash
pio run -e esp32doit-devkit-v1 -t uploadfs
```

Perintah ini memindahkan seluruh isi folder `data/` ke sistem berkas SPIFFS di papan:

| Berkas | Guna |
|---|---|
| `data/index.html` | Halaman editor skrip di `http://<ip-papan>/` |
| `data/config.html` | Halaman pengaturan Wi-Fi di `http://<ip-papan>/ap` |
| `data/baik.ina` | Skrip yang dijalankan otomatis setiap papan menyala |

Tanpa langkah ini, editor web tidak muncul dan tidak ada skrip autorun.

---

## Langkah 5 — Buka monitor serial (30 detik)

```bash
pio device monitor -e esp32doit-devkit-v1
```

Tekan tombol **EN/RESET** di papan. Kamu akan melihat kira-kira:

```
Started AP mode

Kode BAIK ditemukan dan dijalankan.....
---------------------------------------
=================================
 BAIK siap dipakai di ESP32
=================================
Papan  : esp32doit-devkit-v1
...
---------------------------------------

Selamat datang di BAIK X.
Ketik 'help' untuk melihat daftar perintah.
Ketik 'api' untuk daftar fungsi ESP32, 'pinout' untuk peta pin.
Gunakan tombol UP/DOWN untuk navigasi histori perintah.
Baik>
```

Prompt `Baik>` itulah REPL-nya. Untuk keluar dari monitor, tekan `Ctrl-C`
(di beberapa sistem `Ctrl-]`).

---

## Langkah 6 — Perintah pertama di REPL (1 menit)

Ketik satu per satu, tekan Enter setiap kali:

```javascript
tulis("halo dari papanku");
```

```javascript
tulis(1 + 2 * 3);
```

```javascript
isi n = 21;
tulis(n * 2);
```

Lalu coba perintah konsolnya (ini **bukan** kode BAIK, melainkan perintah bawaan):

```
help          — daftar perintah
pinout        — peta pin papan ini
api           — daftar fungsi ESP32 yang tersedia
ls            — daftar berkas di SPIFFS
sysinfo       — info chip dan SDK
```

Firmware membedakan keduanya dari **kata pertama** baris: kalau kata itu nama perintah
terdaftar, baris dijalankan sebagai perintah; kalau bukan, baris dilempar ke interpreter
BAIK.

---

## Langkah 7 — LED berkedip (1 menit)

Ketik baris demi baris di prompt `Baik>`:

```javascript
pinMode(LED_BUILTIN, OUTPUT);
```
```javascript
digitalWrite(LED_BUILTIN, HIGH);
```
```javascript
digitalWrite(LED_BUILTIN, LOW);
```

LED onboard menyala dan padam mengikuti perintahmu, **tanpa kompilasi ulang apa pun.**
Inilah inti dari proyek ini.

Sekarang kedipkan otomatis. Tempel seluruh blok ini sekaligus:

```javascript
fungsi kedip(pin, jumlah, jeda) { pinMode(pin, OUTPUT); untuk (isi i = 0; i < jumlah; i++) { digitalWrite(pin, HIGH); delay(jeda); digitalWrite(pin, LOW); delay(jeda); } balik jumlah; }
```

Lalu:

```javascript
kedip(LED_BUILTIN, 10, 100);
```

> REPL membaca **satu baris** per Enter, jadi fungsi berbaris-banyak lebih enak ditulis
> di berkas `.ina` (langkah berikutnya) daripada diketik di prompt.

Tambahkan tombol:

```javascript
pinMode(BOOT_BUTTON, INPUT_PULLUP);
```
```javascript
tulis("tombol:", digitalRead(BOOT_BUTTON));
```

Tekan dan tahan tombol **BOOT** sambil menjalankan baris terakhir — nilainya berubah
dari `1` menjadi `0` (tombol aktif-rendah karena memakai pull-up).

---

## Langkah 8 — Skrip pertama di berkas

Buat berkas `data/kedip.ina` di komputermu:

```javascript
// kedip.ina - kedipkan LED mengikuti tombol BOOT
pinMode(LED_BUILTIN, OUTPUT);
pinMode(BOOT_BUTTON, INPUT_PULLUP);

tulis("Tekan tombol BOOT untuk menyalakan LED. 100 putaran.");

untuk (isi i = 0; i < 100; i++) {
  isi ditekan = digitalRead(BOOT_BUTTON) === LOW;
  digitalWrite(LED_BUILTIN, ditekan ? HIGH : LOW);
  delay(50);
}

tulis("selesai");
```

Unggah ulang SPIFFS lalu jalankan dari REPL:

```bash
pio run -e esp32doit-devkit-v1 -t uploadfs
```

```
Baik> run /kedip.ina
```

Perintah `run` menerima `kedip.ina`, `/kedip.ina`, maupun `/spiffs/kedip.ina`.

---

## Langkah 9 — Unggah lewat editor web

Menghubungkan kabel setiap kali mengubah skrip itu melelahkan. Papan melayani editornya
sendiri.

**Bila papan belum dikonfigurasi Wi-Fi**, ia otomatis menyalakan Access Point:

- Nama jaringan: **`ESP32_AP`**
- Kata sandi: **`12345678`**

Sambungkan laptop atau ponselmu ke jaringan itu, lalu buka alamat IP papan (biasanya
`http://192.168.4.1/`). Halaman editornya sengaja tidak memuat apa pun dari internet,
jadi tetap terbuka walau AP itu tidak punya akses internet.

Di halaman editor:

1. Tulis kode BAIK-mu.
2. Isi **nama berkas**, misalnya `baik.ina` (yang dijalankan otomatis saat boot) atau
   `kedip.ina`.
3. Tekan tombol unggah.

Papan menyimpan berkas ke SPIFFS lalu **mulai ulang**. Setelah menyala kembali,
`/baik.ina` dijalankan otomatis dan hasilnya terlihat di monitor serial.

---

## Langkah 10 — Sambungkan ke Wi-Fi rumah

Buka **`http://192.168.4.1/ap`** (atau klik tautan *Pengaturan Wi-Fi* di editor),
masukkan SSID dan kata sandi jaringanmu, lalu simpan. Papan mulai ulang dan mencoba
menyambung.

Alamat IP barunya dicetak ke monitor serial:

```
Connected to WiFi
IP Address: 192.168.100.194
Access the editor at: http://192.168.100.194/
```

Mulai sekarang, editor bisa dibuka dari alamat itu selama laptopmu berada di jaringan
yang sama — tanpa kabel USB sama sekali.

> **Hati-hati:** kalau SSID atau kata sandi salah, firmware saat ini menunggu koneksi
> di dalam `setup()` dan mencetak `Connecting to WiFi...` terus-menerus, sehingga REPL
> tidak pernah dimulai. Untuk keluar dari keadaan itu, flash ulang firmware atau hapus
> partisi NVS (`pio run -t erase`, lalu upload dan uploadfs lagi).

---

## Langkah 11 — Jalankan otomatis saat boot

Apa pun yang disimpan sebagai **`/baik.ina`** dijalankan tepat sebelum REPL siap.
Gunakan itu untuk menyiapkan pin, membaca konfigurasi, atau mencetak salam pembuka.

```javascript
// baik.ina
tulis("Papan  :", BOARD);
tulis("Chip   :", CHIP);
tulis("Heap   :", ESP.getFreeHeap(), "bita bebas");

pinMode(LED_BUILTIN, OUTPUT);
untuk (isi i = 0; i < 3; i++) {
  digitalWrite(LED_BUILTIN, HIGH); delay(150);
  digitalWrite(LED_BUILTIN, LOW);  delay(150);
}
```

> **JANGAN menulis loop tak berujung di `/baik.ina`.** Selama skrip autorun berjalan,
> REPL ikut terblokir dan papan tidak bisa dikendalikan lagi — satu-satunya jalan keluar
> adalah flash ulang. Taruh loop panjang di berkas terpisah dan panggil dengan
> `run <berkas>` saat kamu memang menginginkannya.

---

## Pemecahan masalah

### Port serial tidak terdeteksi

- **Ganti kabel.** Ini penyebab nomor satu. Kabel pengisi daya tidak punya jalur data.
- **Pasang driver USB-serial.** ESP32 DevKit V1 umumnya memakai CP2102 (driver Silicon
  Labs) atau CH340/CH9102 (driver WCH). ESP32-S3-DevKitC-1 punya dua port USB: port
  **UART** (lewat chip serial) dan port **USB** (USB-Serial/JTAG bawaan chip). Coba
  keduanya. Di macOS, papan berbasis CH34x butuh
  [driver WCH](https://www.wch.cn/downloads/CH34XSER_MAC_ZIP.html) — `.dmg` untuk Apple
  Silicon, `.pkg` untuk Intel.
- **Linux:** pastikan kamu anggota grup `dialout`
  (`sudo usermod -a -G dialout $USER`, lalu logout-login). Cek juga apakah `brltty`
  merebut perangkat CH340 (`sudo systemctl stop brltty`).
- **Tutup program lain** yang sedang memegang port: monitor serial, Arduino IDE, `screen`.

### Papan tidak mau masuk mode unduh

Gejalanya: `Failed to connect to ESP32: Timed out waiting for packet header` atau
`A fatal error occurred: Wrong boot mode detected`.

1. Tahan tombol **BOOT** (kadang berlabel `IO0`).
2. Sambil menahan BOOT, tekan lalu lepas **EN/RESET**.
3. Lepaskan BOOT.
4. Jalankan ulang `pio run -t upload`.

Beberapa papan butuh kecepatan unggah lebih rendah; tambahkan `upload_speed = 115200`
di bagian environment `platformio.ini` bila perlu. Cabut juga apa pun yang tersambung ke
GPIO0, GPIO2, GPIO12, dan GPIO15 — itu pin *strapping* yang menentukan mode boot
(lihat [PINOUT.md](PINOUT.md)).

### Editor web kosong / 404, atau tidak ada skrip autorun

SPIFFS belum diisi. Jalankan:

```bash
pio run -e <env-papanmu> -t uploadfs
```

`-t upload` (firmware) dan `-t uploadfs` (sistem berkas) adalah dua hal berbeda; banyak
orang lupa yang kedua. Periksa hasilnya dengan `ls` di REPL — harus muncul
`index.html`, `config.html`, dan `baik.ina`.

### Monitor serial menampilkan karakter sampah

Contoh: `␦␦x␦␦l␦` atau deretan simbol acak.

- **Baud rate salah.** Firmware ini memakai **115200**. `pio device monitor` membacanya
  dari `monitor_speed` di `platformio.ini`, tetapi kalau kamu memakai `screen`, `minicom`,
  atau Arduino IDE, setel sendiri ke 115200 8N1.
- Sedikit karakter aneh **saat papan baru direset itu wajar** — itu pesan bootloader
  ROM yang memang dikirim pada 74880 baud sebelum firmware mengambil alih. Setelah
  beberapa baris, tampilan akan normal kembali.
- Kalau muncul peringatan "Your terminal application does not support escape sequences",
  terminalmu tidak mendukung penyuntingan baris. REPL tetap jalan, hanya riwayat dan
  tombol panah yang mati. Di Windows, coba PuTTY.

### `[var] tidak terimplementasi`

Bahasa BAIK memakai **`isi`**, bukan `var`:

```javascript
isi a = 1;      // benar
```

Lihat [BAHASA.md](BAHASA.md) untuk daftar kata kunci yang benar.

### `Use ===, not ==`

BAIK tidak mengizinkan `==`. Ganti dengan `===` (dan `!=` dengan `!==`).

### `konversi tipe implisit dilarang`

Kamu mencoba menyambung string dengan angka. Ubah angkanya dulu:

```javascript
tulis("suhu=" + JSON.stringify(suhu));
```

### REPL tidak merespons apa pun

Kemungkinan besar ada loop tak berujung yang sedang berjalan (`ulang (benar) { ... }`),
entah dari perintah yang kamu ketik atau dari `/baik.ina`. Tekan **EN/RESET**. Kalau
loop itu ada di `/baik.ina`, papan akan terjebak lagi setelah reset — unggah ulang
SPIFFS dengan `baik.ina` yang aman:

```bash
pio run -e <env-papanmu> -t uploadfs
```

### Galat pada pin: "pin tidak mendukung ..."

Firmware memvalidasi setiap nomor pin sebelum memakainya. Ketik `pinout` di REPL untuk
melihat pin mana yang mendukung ADC, DAC, sentuh, atau PWM di papanmu, atau baca
[PINOUT.md](PINOUT.md).

---

## Langkah berikutnya

- [BAHASA.md](BAHASA.md) — pelajari bahasanya secara utuh.
- [API.md](API.md) — seluruh fungsi ESP32: sensor, I2C, Wi-Fi, HTTP, penyimpanan.
- [PINOUT.md](PINOUT.md) — pin mana yang aman untuk apa.
- [examples/](../examples/) — skrip siap pakai untuk disalin.
- [ARSITEKTUR.md](ARSITEKTUR.md) — kalau kamu ingin menambahkan fungsi API sendiri.
