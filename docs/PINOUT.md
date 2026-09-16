# PINOUT — BAIK di ESP32 & ESP32-S3

Dokumen ini menjelaskan pin mana yang boleh dipakai, mana yang harus
dihindari, dan kenapa. Semua angka di sini dihasilkan dari tabel
`src/esp32/e32_pins.h` / `e32_pins.cpp` — tabel yang sama yang dipakai
BAIK untuk **memvalidasi setiap nomor pin saat runtime**. Kalau BAIK
menolak pin, jawabannya ada di halaman ini.

Sumber angka (diverifikasi, bukan hafalan):

- `arduino-esp32/variants/doitESP32devkitV1/pins_arduino.h`
- `arduino-esp32/variants/esp32s3/pins_arduino.h`
- `esp-idf/components/soc/esp32{,s3}/include/soc/rtc_io_channel.h`
- `esp-idf/components/soc/esp32{,s3}/include/soc/soc_caps.h`

Jumlah pin yang terdaftar: **34** pada ESP32 klasik, **45** pada ESP32-S3.

> Catatan: urutan fisik pin pada header papan bisa berbeda antar revisi.
> Yang mengikat adalah **nomor GPIO**, bukan posisi lubang di papan.

---

## 1. ESP32 klasik — DOIT ESP32 DEVKIT V1 (30 pin)

### 1.1 Diagram

```
                     ESP32 DOIT DEVKIT V1 (30 pin)
              +--------------------------------------------+
              |            [ antena PCB ]                   |
        EN  --| EN                                   GPIO23 |--  D23   MOSI
  GPIO36  --| VP   (input saja)                      GPIO22 |--  D22   SCL
  GPIO39  --| VN   (input saja)                      GPIO1  |--  TX0   konsol
  GPIO34  --| D34  (input saja)                      GPIO3  |--  RX0   konsol
  GPIO35  --| D35  (input saja)                      GPIO21 |--  D21   SDA
  GPIO32  --| D32  ADC1 T9                           GPIO19 |--  D19   MISO
  GPIO33  --| D33  ADC1 T8                           GPIO18 |--  D18   SCK
  GPIO25  --| D25  DAC1                              GPIO5  |--  D5    SS  (strap)
  GPIO26  --| D26  DAC2                              GPIO17 |--  TX2
  GPIO27  --| D27  T7                                GPIO16 |--  RX2
  GPIO14  --| D14  T6  (pulsa PWM saat boot)         GPIO4  |--  D4    T0
  GPIO12  --| D12  T5  (STRAP: harus LOW!)           GPIO2  |--  D2    LED (strap)
  GPIO13  --| D13  T4                                GPIO15 |--  D15   T3  (strap)
       GND  --| GND                                     GND |--  GND
       VIN  --| VIN (5V)                                3V3 |--  3V3
              |               [ USB micro ]                 |
              +--------------------------------------------+

   GPIO0  = tombol BOOT (pada papan 30-pin sering TIDAK ada di header)
   GPIO6..GPIO11 = SPI flash internal, tidak pernah keluar ke header
```

### 1.2 Tabel lengkap per pin

| GPIO | Label | ADC | Touch | DAC | RTC | PWM | Catatan |
|---|---|---|---|---|---|---|---|
| 0 | D0 | ADC2_CH1 | T1 | - | 11 | ya | strapping + tombol BOOT; LOW saat boot = mode unduh |
| 1 | TX0 | - | - | - | - | ya | UART0 TX (konsol serial); hindari, mengeluarkan log saat boot |
| 2 | D2 | ADC2_CH2 | T2 | - | 12 | ya | LED onboard + strapping; harus LOW/mengambang saat boot |
| 3 | RX0 | - | - | - | - | ya | UART0 RX (konsol serial); HIGH saat boot; hindari |
| 4 | D4 | ADC2_CH0 | T0 | - | 10 | ya | ADC2 tidak bisa dipakai saat WiFi aktif |
| 5 | D5 | - | - | - | - | ya | strapping + SS bawaan VSPI; mengeluarkan pulsa PWM saat boot |
| 6 | SD_CLK | - | - | - | - | - | terpakai SPI flash internal - JANGAN dipakai, papan akan gagal boot |
| 7 | SD_DATA0 | - | - | - | - | - | terpakai SPI flash internal - JANGAN dipakai, papan akan gagal boot |
| 8 | SD_DATA1 | - | - | - | - | - | terpakai SPI flash internal - JANGAN dipakai, papan akan gagal boot |
| 9 | SD_DATA2 | - | - | - | - | - | terpakai SPI flash internal - JANGAN dipakai, papan akan gagal boot |
| 10 | SD_DATA3 | - | - | - | - | - | terpakai SPI flash internal - JANGAN dipakai, papan akan gagal boot |
| 11 | SD_CMD | - | - | - | - | - | terpakai SPI flash internal - JANGAN dipakai, papan akan gagal boot |
| 12 | D12 | ADC2_CH5 | T5 | - | 15 | ya | strapping MTDI; HARUS LOW saat boot (HIGH memaksa flash 1.8V) |
| 13 | D13 | ADC2_CH4 | T4 | - | 14 | ya | JTAG MTCK; ADC2 bentrok dengan WiFi |
| 14 | D14 | ADC2_CH6 | T6 | - | 16 | ya | JTAG MTMS; mengeluarkan pulsa PWM saat boot |
| 15 | D15 | ADC2_CH3 | T3 | - | 13 | ya | strapping MTDO; LOW saat boot mematikan log; keluar pulsa PWM saat boot |
| 16 | RX2 | - | - | - | - | ya | UART2 RX bawaan; terpakai PSRAM pada modul WROVER |
| 17 | TX2 | - | - | - | - | ya | UART2 TX bawaan; terpakai PSRAM pada modul WROVER |
| 18 | D18 | - | - | - | - | ya | SCK bawaan VSPI |
| 19 | D19 | - | - | - | - | ya | MISO bawaan VSPI |
| 21 | D21 | - | - | - | - | ya | SDA bawaan (Wire) |
| 22 | D22 | - | - | - | - | ya | SCL bawaan (Wire) |
| 23 | D23 | - | - | - | - | ya | MOSI bawaan VSPI |
| 25 | D25 | ADC2_CH8 | - | DAC1 | 6 | ya | DAC1; ADC2 bentrok dengan WiFi |
| 26 | D26 | ADC2_CH9 | - | DAC2 | 7 | ya | DAC2; ADC2 bentrok dengan WiFi |
| 27 | D27 | ADC2_CH7 | T7 | - | 17 | ya | ADC2 bentrok dengan WiFi |
| 32 | D32 | ADC1_CH4 | T9 | - | 9 | ya |  |
| 33 | D33 | ADC1_CH5 | T8 | - | 8 | ya |  |
| 34 | D34 | ADC1_CH6 | - | - | 4 | - | input saja, tanpa pull-up/pull-down internal |
| 35 | D35 | ADC1_CH7 | - | - | 5 | - | input saja, tanpa pull-up/pull-down internal |
| 36 | VP | ADC1_CH0 | - | - | 0 | - | SENSOR_VP; input saja, tanpa pull-up/pull-down internal |
| 37 | GPIO37 | ADC1_CH1 | - | - | 1 | - | input saja; tidak dibonding pada modul WROOM-32 (tak ada di papan DOIT) |
| 38 | GPIO38 | ADC1_CH2 | - | - | 2 | - | input saja; tidak dibonding pada modul WROOM-32 (tak ada di papan DOIT) |
| 39 | VN | ADC1_CH3 | - | - | 3 | - | SENSOR_VN; input saja, tanpa pull-up/pull-down internal |

### 1.3 Pin aman dipakai

Pin serba-bisa (digital in/out + PWM, bukan strapping, bukan flash,
bukan konsol serial):

```
GPIO 4, 13, 14, 16, 17, 18, 19, 21, 22, 23, 25, 26, 27, 32, 33
```

Yang **paling aman** (tidak melakukan apa pun aneh saat boot):

```
GPIO 16, 17, 18, 19, 21, 22, 23, 25, 26, 27, 32, 33
```

Hanya-input (bagus untuk tombol/sensor analog, **tidak bisa** output,
**tidak punya** pull-up/pull-down internal — pasang resistor luar):

```
GPIO 34, 35, 36 (VP), 39 (VN)
```

### 1.4 Pin yang harus dihindari dan kenapa

| GPIO | Alasan |
|---|---|
| 6, 7, 8, 9, 10, 11 | Tersambung ke **SPI flash internal**. Memakainya membuat papan gagal boot / hang. BAIK menolaknya (`E32_CAP_FLASH`). |
| 0 | Strapping + tombol BOOT. LOW saat boot = masuk mode unduh firmware. |
| 2 | Strapping + LED onboard. Harus LOW atau mengambang saat boot. |
| 5 | Strapping. Mengeluarkan pulsa PWM saat boot (jangan untuk relay/servo). |
| 12 | Strapping MTDI. **HARUS LOW saat boot.** HIGH memaksa tegangan flash ke 1.8V dan papan bisa gagal boot permanen. |
| 15 | Strapping MTDO. LOW saat boot mematikan log boot. Mengeluarkan pulsa PWM saat boot. |
| 1 (TX0), 3 (RX0) | UART0 = konsol serial BAIK. Dipakai monitor & upload. |
| 37, 38 | Tidak dibonding pada modul ESP32-WROOM-32, jadi tidak ada di papan DOIT. Tetap terdaftar karena chip-nya punya. |
| 16, 17 | Aman di WROOM-32, tetapi **terpakai PSRAM** pada modul WROVER. |

### 1.5 ADC

- **ADC1** — `GPIO 32, 33, 34, 35, 36, 37, 38, 39`
- **ADC2** — `GPIO 0, 2, 4, 12, 13, 14, 15, 25, 26, 27`

### 1.6 Touch (sentuh kapasitif)

| T | GPIO | | T | GPIO |
|---|---|---|---|---|
| T0 | 4  | | T5 | 12 |
| T1 | 0  | | T6 | 14 |
| T2 | 2  | | T7 | 27 |
| T3 | 15 | | T8 | 33 |
| T4 | 13 | | T9 | 32 |

### 1.7 DAC

`DAC1 = GPIO25`, `DAC2 = GPIO26` — 8 bit, 0..255, keluaran 0..3.3V.

---

## 2. ESP32-S3 — ESP32-S3-DevKitC-1

### 2.1 Diagram

```
                        ESP32-S3-DevKitC-1
              +--------------------------------------------+
              |            [ antena PCB ]                   |
       3V3  --| 3V3                                     GND |--  GND
       3V3  --| 3V3                                   GPIO43|--  TX0   konsol
       RST  --| RST                                   GPIO44|--  RX0   konsol
    GPIO4  --| D4   ADC1 T4                           GPIO1 |--  D1    ADC1 T1
    GPIO5  --| D5   ADC1 T5                           GPIO2 |--  D2    ADC1 T2
    GPIO6  --| D6   ADC1 T6                           GPIO42|--  D42   MTMS
    GPIO7  --| D7   ADC1 T7                           GPIO41|--  D41   MTDI
   GPIO15  --| D15  ADC2                              GPIO40|--  D40   MTDO
   GPIO16  --| D16  ADC2                              GPIO39|--  D39   MTCK
   GPIO17  --| D17  ADC2                              GPIO38|--  D38
   GPIO18  --| D18  ADC2                              GPIO37|--  D37  (octal PSRAM)
    GPIO8  --| D8   SDA  ADC1 T8                      GPIO36|--  D36  (octal PSRAM)
    GPIO3  --| D3   ADC1 T3 (STRAP)                   GPIO35|--  D35  (octal PSRAM)
   GPIO46  --| D46  (STRAP: harus LOW)                GPIO0 |--  BOOT (STRAP)
    GPIO9  --| D9   SCL  ADC1 T9                      GPIO45|--  D45  (STRAP: harus LOW)
   GPIO10  --| D10  SS   ADC1 T10                     GPIO48|--  D48   LED RGB
   GPIO11  --| D11  MOSI ADC2 T11                     GPIO47|--  D47
   GPIO12  --| D12  SCK  ADC2 T12                     GPIO21|--  D21
   GPIO13  --| D13  MISO ADC2 T13                     GPIO20|--  D20   USB D+
   GPIO14  --| D14  ADC2 T14                          GPIO19|--  D19   USB D-
        5V  --| 5V                                      GND |--  GND
       GND  --| GND                                      5V |--  5V
              |   [ USB UART ]        [ USB OTG/JTAG ]      |
              +--------------------------------------------+

   GPIO22, GPIO23, GPIO24, GPIO25  TIDAK ADA pada ESP32-S3.
   GPIO26..GPIO32 = SPI flash/PSRAM internal, tidak keluar ke header.
   TIDAK ADA DAC pada ESP32-S3.
```

### 2.2 Tabel lengkap per pin

| GPIO | Label | ADC | Touch | DAC | RTC | PWM | Catatan |
|---|---|---|---|---|---|---|---|
| 0 | D0 | - | - | - | 0 | ya | strapping + tombol BOOT; LOW saat boot = mode unduh |
| 1 | D1 | ADC1_CH0 | T1 | - | 1 | ya |  |
| 2 | D2 | ADC1_CH1 | T2 | - | 2 | ya |  |
| 3 | D3 | ADC1_CH2 | T3 | - | 3 | ya | strapping (pilih sumber JTAG); biarkan mengambang saat boot |
| 4 | D4 | ADC1_CH3 | T4 | - | 4 | ya |  |
| 5 | D5 | ADC1_CH4 | T5 | - | 5 | ya |  |
| 6 | D6 | ADC1_CH5 | T6 | - | 6 | ya |  |
| 7 | D7 | ADC1_CH6 | T7 | - | 7 | ya |  |
| 8 | D8 | ADC1_CH7 | T8 | - | 8 | ya | SDA bawaan (Wire) |
| 9 | D9 | ADC1_CH8 | T9 | - | 9 | ya | SCL bawaan (Wire) |
| 10 | D10 | ADC1_CH9 | T10 | - | 10 | ya | SS bawaan (SPI) |
| 11 | D11 | ADC2_CH0 | T11 | - | 11 | ya | MOSI bawaan (SPI); ADC2 bentrok dengan WiFi |
| 12 | D12 | ADC2_CH1 | T12 | - | 12 | ya | SCK bawaan (SPI); ADC2 bentrok dengan WiFi |
| 13 | D13 | ADC2_CH2 | T13 | - | 13 | ya | MISO bawaan (SPI); ADC2 bentrok dengan WiFi |
| 14 | D14 | ADC2_CH3 | T14 | - | 14 | ya | ADC2 bentrok dengan WiFi |
| 15 | D15 | ADC2_CH4 | - | - | 15 | ya | ADC2 bentrok dengan WiFi |
| 16 | D16 | ADC2_CH5 | - | - | 16 | ya | ADC2 bentrok dengan WiFi |
| 17 | D17 | ADC2_CH6 | - | - | 17 | ya | ADC2 bentrok dengan WiFi |
| 18 | D18 | ADC2_CH7 | - | - | 18 | ya | ADC2 bentrok dengan WiFi |
| 19 | D19 | ADC2_CH8 | - | - | 19 | ya | USB D- (USB-JTAG/CDC bawaan); hindari bila memakai USB |
| 20 | D20 | ADC2_CH9 | - | - | 20 | ya | USB D+ (USB-JTAG/CDC bawaan); hindari bila memakai USB |
| 21 | D21 | - | - | - | 21 | ya |  |
| 26 | SPICS1 | - | - | - | - | - | terpakai SPI flash/PSRAM internal - JANGAN dipakai |
| 27 | SPIHD | - | - | - | - | - | terpakai SPI flash/PSRAM internal - JANGAN dipakai |
| 28 | SPIWP | - | - | - | - | - | terpakai SPI flash/PSRAM internal - JANGAN dipakai |
| 29 | SPICS0 | - | - | - | - | - | terpakai SPI flash/PSRAM internal - JANGAN dipakai |
| 30 | SPICLK | - | - | - | - | - | terpakai SPI flash/PSRAM internal - JANGAN dipakai |
| 31 | SPIQ | - | - | - | - | - | terpakai SPI flash/PSRAM internal - JANGAN dipakai |
| 32 | SPID | - | - | - | - | - | terpakai SPI flash/PSRAM internal - JANGAN dipakai |
| 33 | D33 | - | - | - | - | ya | bebas dipakai HANYA bila modul TIDAK memakai octal PSRAM (mis. N8R8) |
| 34 | D34 | - | - | - | - | ya | bebas dipakai HANYA bila modul TIDAK memakai octal PSRAM (mis. N8R8) |
| 35 | D35 | - | - | - | - | ya | bebas dipakai HANYA bila modul TIDAK memakai octal PSRAM (mis. N8R8) |
| 36 | D36 | - | - | - | - | ya | bebas dipakai HANYA bila modul TIDAK memakai octal PSRAM (mis. N8R8) |
| 37 | D37 | - | - | - | - | ya | bebas dipakai HANYA bila modul TIDAK memakai octal PSRAM (mis. N8R8) |
| 38 | D38 | - | - | - | - | ya | pada ESP32-S3-DevKitC-1 v1.1 LED RGB onboard ada di pin ini |
| 39 | D39 | - | - | - | - | ya | JTAG MTCK |
| 40 | D40 | - | - | - | - | ya | JTAG MTDO |
| 41 | D41 | - | - | - | - | ya | JTAG MTDI |
| 42 | D42 | - | - | - | - | ya | JTAG MTMS |
| 43 | TX0 | - | - | - | - | ya | UART0 TX (konsol serial); hindari |
| 44 | RX0 | - | - | - | - | ya | UART0 RX (konsol serial); hindari |
| 45 | D45 | - | - | - | - | ya | strapping VDD_SPI (tegangan flash); harus LOW saat boot |
| 46 | D46 | - | - | - | - | ya | strapping (pengatur log ROM); harus LOW saat boot |
| 47 | D47 | - | - | - | - | ya |  |
| 48 | D48 | - | - | - | - | ya | LED RGB WS2812 onboard (DevKitC-1 v1.0) |

### 2.3 Pin aman dipakai

```
GPIO 1, 2, 4..18, 21, 47   (plus 33..42 bila modul TIDAK memakai octal PSRAM)
```

(GPIO3 sengaja tidak masuk daftar: ia strapping pin.)

Yang **paling aman** (bukan strapping, bukan USB, bukan konsol, bukan PSRAM):

```
GPIO 1, 2, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 21, 47
```

Semua GPIO pada ESP32-S3 bisa jadi input **maupun** output — tidak ada
pin "input saja" seperti GPIO34-39 di ESP32 klasik.

### 2.4 Pin yang harus dihindari dan kenapa

| GPIO | Alasan |
|---|---|
| 26, 27, 28, 29, 30, 31, 32 | **SPI flash/PSRAM internal.** Papan gagal boot bila dipakai. BAIK menolaknya (`E32_CAP_FLASH`). |
| 33, 34, 35, 36, 37 | Terpakai bila modul memakai **octal PSRAM** (mis. N8R8). Pada modul quad/tanpa PSRAM pin ini bebas. Tabel pin otomatis menyesuaikan lewat `CONFIG_SPIRAM_MODE_OCT`. |
| 0 | Strapping + tombol BOOT. LOW saat boot = mode unduh. |
| 3 | Strapping (pilih sumber JTAG). Biarkan mengambang saat boot. |
| 45 | Strapping VDD_SPI (tegangan flash). Harus LOW saat boot. |
| 46 | Strapping (pengatur log ROM). Harus LOW saat boot. |
| 19, 20 | **USB D- / D+** untuk USB-JTAG/CDC bawaan. Memakainya memutus USB bawaan. |
| 43 (TX0), 44 (RX0) | UART0 = konsol serial BAIK. |
| 22, 23, 24, 25 | **Tidak ada** pada chip ESP32-S3. `pinInfo(22)` mengembalikan `kosong`. |

### 2.5 ADC

- **ADC1** — `GPIO 1..10` (channel 0..9)
- **ADC2** — `GPIO 11..20` (channel 0..9)

### 2.6 Touch

`T1..T14 = GPIO1..GPIO14` (nomor sentuh sama dengan nomor GPIO).
Kanal sentuh 0 dipakai internal untuk denoise, jadi tidak ada `T0`.

### 2.7 DAC

**Tidak ada.** ESP32-S3 tidak punya DAC. Konstanta `DAC1`/`DAC2` sengaja
**tidak didaftarkan** pada papan ini, dan `dacWrite()` akan memberi galat
yang jelas.

---

## 3. ADC2 vs WiFi — wajib dibaca

ADC2 dipakai bersama oleh driver WiFi. **Selama WiFi aktif, pembacaan
ADC2 gagal** (`analogRead()` mengembalikan 0 atau nilai ngawur, dan
driver mengembalikan `ESP_ERR_TIMEOUT`).

Aturan praktis:

- Pakai WiFi? → **hanya gunakan pin ADC1.**
  - ESP32: `GPIO 32, 33, 34, 35, 36, 39`
  - ESP32-S3: `GPIO 1..10`
- Butuh ADC2? → matikan WiFi dulu (`WiFi.disconnect()` + `WiFi.mode(WIFI_OFF)`).

Cara memilih pin ADC1 dari dalam skrip BAIK tanpa menghafal:

```
isi aman = daftarPin(CAP_ADC1);
tulis("Pin ADC yang aman dipakai bersama WiFi:", JSON.stringify(aman));
```

Catatan tambahan ADC:

- Resolusi bawaan 12 bit (0..4095).
- ADC ESP32 **tidak linear**, terutama di bawah 0.15V dan di atas 2.5V.
  Pakai `analogReadMilliVolts(pin)` yang sudah dikalibrasi pabrik.
- Atenuasi bawaan `ADC_11db` (jangkauan penuh ~0..3.1V). Ubah dengan
  `analogSetPinAttenuation(pin, ADC_6db)` dst bila perlu presisi.

---

## 4. Strapping pin — kenapa berbahaya

Strapping pin dibaca chip **saat reset** untuk menentukan mode boot,
tegangan flash, dan keluaran log. Bila rangkaian luar (LED, relay,
resistor pull) menahan pin itu ke level yang salah, papan **tidak mau
boot**.

| Papan | Strapping |
|---|---|
| ESP32 | `GPIO 0, 2, 5, 12, 15` |
| ESP32-S3 | `GPIO 0, 3, 45, 46` |

Yang paling sering menggigit:

- **GPIO12 (ESP32)** — kalau ditarik HIGH saat boot, chip mengira flash
  bekerja di 1.8V. Papan dengan flash 3.3V langsung gagal boot.
- **GPIO0** — ditarik LOW saat boot berarti masuk mode unduh firmware,
  sketsa tidak pernah jalan.
- **GPIO2, GPIO5, GPIO15 (ESP32)** — mengeluarkan pulsa saat boot.
  Jangan dipakai untuk relay, servo, atau MOSFET daya.

Memeriksa dari skrip:

```
jika (pinPunya(12, CAP_STRAP)) {
  tulis("Hati-hati: GPIO12 adalah strapping pin.");
}
```

---

## 5. Peripheral bawaan

| Peripheral | ESP32 (DOIT DevKit V1) | ESP32-S3 (DevKitC-1) |
|---|---|---|
| UART0 (konsol BAIK) | TX=`GPIO1`, RX=`GPIO3` | TX=`GPIO43`, RX=`GPIO44` |
| UART2 | TX=`GPIO17`, RX=`GPIO16` | — (S3 hanya UART0/UART1) |
| I2C `Wire` | SDA=`GPIO21`, SCL=`GPIO22` | SDA=`GPIO8`, SCL=`GPIO9` |
| SPI | VSPI: SCK=`18`, MISO=`19`, MOSI=`23`, SS=`5` | FSPI: SCK=`12`, MISO=`13`, MOSI=`11`, SS=`10` |
| LED onboard | `GPIO2` (LED biasa) | `GPIO48` (RGB WS2812, v1.0) / `GPIO38` (v1.1) |
| Tombol BOOT | `GPIO0` | `GPIO0` |
| USB bawaan | — (lewat chip CP2102/CH340) | `GPIO19` (D-), `GPIO20` (D+) |

Semua angka di atas juga tersedia sebagai konstanta di BAIK:
`TX`, `RX`, `SDA`, `SCL`, `SCK`, `MISO`, `MOSI`, `SS`, `LED_BUILTIN`,
`BOOT_BUTTON`.

Pin I2C dan SPI **bisa dipindah** ke pin lain lewat GPIO matrix:
`Wire.begin(sda, scl)` dan `SPI.begin(sck, miso, mosi, ss)`.

---

## 6. Konstanta pin yang tersedia di BAIK

| Bentuk | Contoh | Arti |
|---|---|---|
| `GPIOn` | `GPIO23` | Selalu ada untuk setiap pin papan ini |
| Label sablon | `D23`, `VP`, `VN`, `TX0`, `RX2` | Sesuai cetakan di papan |
| `An` | `A0`, `A17` | Alias ADC, persis seperti variant Arduino resmi |
| `Tn` | `T0`..`T9` (ESP32), `T1`..`T14` (S3) | Kanal sentuh |
| `DACn` | `DAC1`, `DAC2` | Hanya ESP32 klasik |
| `CAP_*` | `CAP_ADC1`, `CAP_STRAP` | Bit kapabilitas untuk `daftarPin`/`pinPunya` |
| `BOARD`, `CHIP` | string | Identitas papan, mis. `"esp32-s3-devkitc-1"` |

Konstanta yang tidak berlaku pada papan aktif **tidak didaftarkan sama
sekali** — `DAC1` tidak ada di ESP32-S3, `T0` tidak ada di ESP32-S3,
`GPIO22` tidak ada di ESP32-S3.

Daftar bit kapabilitas:

| Konstanta | Nilai | Arti |
|---|---|---|
| `CAP_DIGITAL` | 1 | bisa `digitalRead`/`digitalWrite` |
| `CAP_INPUT` | 2 | bisa jadi input |
| `CAP_OUTPUT` | 4 | bisa jadi output |
| `CAP_PULL` | 8 | punya pull-up/pull-down internal |
| `CAP_ADC1` | 16 | ADC unit 1 (aman bersama WiFi) |
| `CAP_ADC2` | 32 | ADC unit 2 (bentrok WiFi) |
| `CAP_DAC` | 64 | DAC 8-bit |
| `CAP_TOUCH` | 128 | sensor sentuh |
| `CAP_RTC` | 256 | RTC GPIO, bisa membangunkan dari deep sleep |
| `CAP_PWM` | 512 | bisa LEDC/PWM |
| `CAP_STRAP` | 1024 | strapping pin |
| `CAP_FLASH` | 2048 | terpakai SPI flash/PSRAM |
| `CAP_USB` | 4096 | USB D-/D+ |
| `CAP_UART0` | 8192 | konsol serial |
| `CAP_I2C` | 16384 | pin I2C bawaan |
| `CAP_SPI` | 32768 | pin SPI bawaan |
| `CAP_BOOT` | 65536 | tombol BOOT |
| `CAP_LED` | 131072 | LED onboard |
| `CAP_ADC` | 48 | gabungan ADC1\|ADC2 |
| `CAP_IO` | 7 | gabungan DIGITAL\|INPUT\|OUTPUT |

---

## 7. Contoh kode BAIK

### 7.1 Melihat detail satu pin

```
isi p = pinInfo(25);
jika (p === kosong) {
  tulis("pin tidak ada pada papan ini");
} lainnya {
  tulis("GPIO   :", p.gpio);
  tulis("Label  :", p.label);
  tulis("ADC    : unit", p.adc.unit, "channel", p.adc.channel);
  tulis("Touch  :", p.touch);
  tulis("DAC    :", p.dac);
  tulis("RTC    :", p.rtc);
  tulis("Kemampuan:", JSON.stringify(p.caps));
  tulis("Catatan:", p.note);
}
```

Keluaran pada ESP32 klasik:

```
GPIO   : 25
Label  : D25
ADC    : unit 2 channel 8
Touch  : -1
DAC    : 1
RTC    : 6
Kemampuan: ["DIGITAL","INPUT","OUTPUT","PULL","ADC2","DAC","RTC","PWM"]
Catatan: DAC1; ADC2 bentrok dengan WiFi
```

### 7.2 Memilih pin ADC yang aman dipakai bersama WiFi

```
isi aman = daftarPin(CAP_ADC1);
tulis("Papan:", BOARD, "chip:", CHIP);
tulis("Pin ADC1:", JSON.stringify(aman));

isi pin = aman[0];
pinMode(pin, INPUT);
tulis("Nilai", pin, "=", analogRead(pin), "->", analogReadMilliVolts(pin), "mV");
```

### 7.3 Menyaring pin sebelum dipakai

```
fungsi pakaiSebagaiOutput(pin) {
  jika (!pinPunya(pin, CAP_OUTPUT)) {
    tulis("GPIO", pin, "tidak bisa jadi output.");
    balik salah;
  }
  jika (pinPunya(pin, CAP_STRAP)) {
    tulis("Peringatan: GPIO", pin, "adalah strapping pin.");
  }
  pinMode(pin, OUTPUT);
  balik benar;
}

pakaiSebagaiOutput(34);   // ditolak: GPIO34 input saja
pakaiSebagaiOutput(12);   // jalan, tapi diperingatkan
pakaiSebagaiOutput(23);   // aman
```

### 7.4 Mencari semua pin sentuh lalu membacanya

```
isi sentuh = daftarPin(CAP_TOUCH);
untuk (isi i = 0; i < sentuh.panjang; i++) {
  isi pin = sentuh[i];
  tulis("GPIO", pin, "->", touchRead(pin));
}
```

### 7.5 Kedip LED onboard tanpa menghafal nomor pin

```
pinMode(LED_BUILTIN, OUTPUT);
ulang (benar) {
  digitalWrite(LED_BUILTIN, HIGH);
  delay(500);
  digitalWrite(LED_BUILTIN, LOW);
  delay(500);
}
```

### 7.6 Mencetak seluruh tabel pinout

```
pinout();
```

`pinout()` mencetak tabel yang sama dengan bagian 1.2 / 2.2 di atas,
langsung ke konsol serial. Berguna saat lupa papan apa yang sedang
tercolok. Perintah konsol `pinout` melakukan hal yang sama.

---

## 8. Ringkasan cepat

| Pertanyaan | ESP32 | ESP32-S3 |
|---|---|---|
| Pin paling aman untuk output | 16, 17, 18, 19, 21, 22, 23, 25, 26, 27, 32, 33 | 1, 2, 4..18, 21, 47 |
| Pin ADC + WiFi | 32, 33, 34, 35, 36, 39 | 1..10 |
| Pin hanya-input | 34, 35, 36, 39 | tidak ada |
| Pin haram | 6..11 | 26..32 |
| Strapping | 0, 2, 5, 12, 15 | 0, 3, 45, 46 |
| Bisa bangunkan dari deep sleep | semua `CAP_RTC` | GPIO 0..21 |
| DAC | GPIO25, GPIO26 | tidak ada |
