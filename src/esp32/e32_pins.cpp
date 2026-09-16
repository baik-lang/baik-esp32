/*
 * e32_pins.cpp - Tabel pinout & kapabilitas GPIO untuk ESP32 dan ESP32-S3.
 *
 * Sumber angka (diverifikasi, bukan hafalan):
 *   - arduino-esp32 variants/doitESP32devkitV1/pins_arduino.h
 *   - arduino-esp32 variants/esp32s3/pins_arduino.h
 *   - esp-idf components/soc/esp32{,s3}/include/soc/rtc_io_channel.h
 *   - esp-idf components/soc/esp32{,s3}/include/soc/soc_caps.h
 *
 * Tabel ini adalah sumber kebenaran tunggal untuk validasi pin, konstanta
 * bahasa BAIK, perintah konsol `pinout`, dan docs/PINOUT.md.
 */

#include "e32_pins.h"

#include <string.h>

/* Ambil definisi CONFIG_IDF_TARGET_* dari sdkconfig/Arduino bila tersedia. */
#if defined(__has_include)
#if __has_include(<sdkconfig.h>)
#include <sdkconfig.h>
#endif
#endif

#if defined(ARDUINO)
#include <Arduino.h>
#endif

/* Jaring pengaman: bila sdkconfig.h tidak terjangkau, pakai makro papan
 * yang dipasang PlatformIO/Arduino untuk mendeteksi ESP32-S3. */
#if !defined(CONFIG_IDF_TARGET_ESP32S3)
#if defined(ARDUINO_ESP32S3_DEV) || defined(ARDUINO_ESP32S3_DEVKITC_1) || \
    defined(ARDUINO_ESP32S3_DEVKITM_1)
#define CONFIG_IDF_TARGET_ESP32S3 1
#endif
#endif

/* ------------------------------------------------------------------------
 * Singkatan kapabilitas supaya tabel di bawah tetap terbaca.
 * ---------------------------------------------------------------------- */

/* Pin serba-bisa: digital in/out, punya pull internal, bisa PWM (LEDC). */
#define E32_IO_FULL \
  (E32_CAP_DIGITAL | E32_CAP_INPUT | E32_CAP_OUTPUT | E32_CAP_PULL | E32_CAP_PWM)

/* Pin input saja: tanpa output, tanpa pull internal, tanpa PWM. */
#define E32_IN_ONLY (E32_CAP_DIGITAL | E32_CAP_INPUT)

/* ------------------------------------------------------------------------
 * Nama kapabilitas
 * ---------------------------------------------------------------------- */

typedef struct e32_cap_name_entry {
  uint32_t bit;
  const char *name;
} e32_cap_name_entry_t;

static const e32_cap_name_entry_t e32_cap_names[] = {
  { E32_CAP_DIGITAL, "DIGITAL" }, { E32_CAP_INPUT, "INPUT" },
  { E32_CAP_OUTPUT,  "OUTPUT"  }, { E32_CAP_PULL,  "PULL"  },
  { E32_CAP_ADC1,    "ADC1"    }, { E32_CAP_ADC2,  "ADC2"  },
  { E32_CAP_DAC,     "DAC"     }, { E32_CAP_TOUCH, "TOUCH" },
  { E32_CAP_RTC,     "RTC"     }, { E32_CAP_PWM,   "PWM"   },
  { E32_CAP_STRAP,   "STRAP"   }, { E32_CAP_FLASH, "FLASH" },
  { E32_CAP_USB,     "USB"     }, { E32_CAP_UART0, "UART0" },
  { E32_CAP_I2C,     "I2C"     }, { E32_CAP_SPI,   "SPI"   },
  { E32_CAP_BOOT,    "BOOT"    }, { E32_CAP_LED,   "LED"   }
};

#define E32_CAP_NAME_COUNT \
  (sizeof(e32_cap_names) / sizeof(e32_cap_names[0]))

/* ========================================================================
 * TABEL PIN
 *
 * Urutan field (lihat e32_pins.h):
 *   gpio, caps, adc_unit, adc_channel, touch_channel, dac_channel,
 *   rtc_gpio, label, note
 * ====================================================================== */

#if CONFIG_IDF_TARGET_ESP32S3

/* ------------------------------------------------------------------------
 * ESP32-S3  (papan ESP32-S3-DevKitC-1)
 *
 * GPIO yang ADA: 0-21 dan 26-48.  GPIO 22, 23, 24, 25 TIDAK ADA pada S3
 * (lihat SOC_GPIO_VALID_GPIO_MASK di soc_caps.h).
 * Tidak ada DAC pada ESP32-S3.
 * ADC1 = GPIO1..GPIO10 (ch0..ch9), ADC2 = GPIO11..GPIO20 (ch0..ch9).
 * Touch T1..T14 = GPIO1..GPIO14 (kanal 0 dipakai denoise internal).
 * RTC GPIO 0..21 = GPIO 0..21 (nomor sama).
 * ---------------------------------------------------------------------- */

/* GPIO33-37 hanya terpakai bila modul memakai octal PSRAM (mis. N8R8).
 * Pada modul quad/ tanpa PSRAM pin ini bebas dipakai. */
#if defined(CONFIG_SPIRAM_MODE_OCT) || defined(BOARD_HAS_OCT_PSRAM)
#define E32_OCT_CAPS E32_CAP_FLASH
#define E32_OCT_NOTE "terpakai octal PSRAM pada modul ini - JANGAN dipakai"
#else
#define E32_OCT_CAPS E32_IO_FULL
#define E32_OCT_NOTE \
  "bebas dipakai HANYA bila modul TIDAK memakai octal PSRAM (mis. N8R8)"
#endif

static const e32_pin_info_t e32_pins[] = {
  /* --- GPIO 0-21: semua RTC GPIO, bisa bangun dari deep sleep --- */
  { 0,  E32_IO_FULL | E32_CAP_RTC | E32_CAP_STRAP | E32_CAP_BOOT,
    -1, -1, -1, -1, 0,  "D0",
    "strapping + tombol BOOT; LOW saat boot = mode unduh" },
  { 1,  E32_IO_FULL | E32_CAP_ADC1 | E32_CAP_TOUCH | E32_CAP_RTC,
    1, 0, 1, -1, 1,  "D1", NULL },
  { 2,  E32_IO_FULL | E32_CAP_ADC1 | E32_CAP_TOUCH | E32_CAP_RTC,
    1, 1, 2, -1, 2,  "D2", NULL },
  { 3,  E32_IO_FULL | E32_CAP_ADC1 | E32_CAP_TOUCH | E32_CAP_RTC |
        E32_CAP_STRAP,
    1, 2, 3, -1, 3,  "D3",
    "strapping (pilih sumber JTAG); biarkan mengambang saat boot" },
  { 4,  E32_IO_FULL | E32_CAP_ADC1 | E32_CAP_TOUCH | E32_CAP_RTC,
    1, 3, 4, -1, 4,  "D4", NULL },
  { 5,  E32_IO_FULL | E32_CAP_ADC1 | E32_CAP_TOUCH | E32_CAP_RTC,
    1, 4, 5, -1, 5,  "D5", NULL },
  { 6,  E32_IO_FULL | E32_CAP_ADC1 | E32_CAP_TOUCH | E32_CAP_RTC,
    1, 5, 6, -1, 6,  "D6", NULL },
  { 7,  E32_IO_FULL | E32_CAP_ADC1 | E32_CAP_TOUCH | E32_CAP_RTC,
    1, 6, 7, -1, 7,  "D7", NULL },
  { 8,  E32_IO_FULL | E32_CAP_ADC1 | E32_CAP_TOUCH | E32_CAP_RTC |
        E32_CAP_I2C,
    1, 7, 8, -1, 8,  "D8", "SDA bawaan (Wire)" },
  { 9,  E32_IO_FULL | E32_CAP_ADC1 | E32_CAP_TOUCH | E32_CAP_RTC |
        E32_CAP_I2C,
    1, 8, 9, -1, 9,  "D9", "SCL bawaan (Wire)" },
  { 10, E32_IO_FULL | E32_CAP_ADC1 | E32_CAP_TOUCH | E32_CAP_RTC |
        E32_CAP_SPI,
    1, 9, 10, -1, 10, "D10", "SS bawaan (SPI)" },
  { 11, E32_IO_FULL | E32_CAP_ADC2 | E32_CAP_TOUCH | E32_CAP_RTC |
        E32_CAP_SPI,
    2, 0, 11, -1, 11, "D11", "MOSI bawaan (SPI); ADC2 bentrok dengan WiFi" },
  { 12, E32_IO_FULL | E32_CAP_ADC2 | E32_CAP_TOUCH | E32_CAP_RTC |
        E32_CAP_SPI,
    2, 1, 12, -1, 12, "D12", "SCK bawaan (SPI); ADC2 bentrok dengan WiFi" },
  { 13, E32_IO_FULL | E32_CAP_ADC2 | E32_CAP_TOUCH | E32_CAP_RTC |
        E32_CAP_SPI,
    2, 2, 13, -1, 13, "D13", "MISO bawaan (SPI); ADC2 bentrok dengan WiFi" },
  { 14, E32_IO_FULL | E32_CAP_ADC2 | E32_CAP_TOUCH | E32_CAP_RTC,
    2, 3, 14, -1, 14, "D14", "ADC2 bentrok dengan WiFi" },
  { 15, E32_IO_FULL | E32_CAP_ADC2 | E32_CAP_RTC,
    2, 4, -1, -1, 15, "D15", "ADC2 bentrok dengan WiFi" },
  { 16, E32_IO_FULL | E32_CAP_ADC2 | E32_CAP_RTC,
    2, 5, -1, -1, 16, "D16", "ADC2 bentrok dengan WiFi" },
  { 17, E32_IO_FULL | E32_CAP_ADC2 | E32_CAP_RTC,
    2, 6, -1, -1, 17, "D17", "ADC2 bentrok dengan WiFi" },
  { 18, E32_IO_FULL | E32_CAP_ADC2 | E32_CAP_RTC,
    2, 7, -1, -1, 18, "D18", "ADC2 bentrok dengan WiFi" },
  { 19, E32_IO_FULL | E32_CAP_ADC2 | E32_CAP_RTC | E32_CAP_USB,
    2, 8, -1, -1, 19, "D19",
    "USB D- (USB-JTAG/CDC bawaan); hindari bila memakai USB" },
  { 20, E32_IO_FULL | E32_CAP_ADC2 | E32_CAP_RTC | E32_CAP_USB,
    2, 9, -1, -1, 20, "D20",
    "USB D+ (USB-JTAG/CDC bawaan); hindari bila memakai USB" },
  { 21, E32_IO_FULL | E32_CAP_RTC,
    -1, -1, -1, -1, 21, "D21", NULL },

  /* GPIO 22-25 TIDAK ADA pada ESP32-S3 - sengaja tidak didaftarkan. */

  /* --- GPIO 26-32: SPI flash / PSRAM internal, haram disentuh --- */
  { 26, E32_CAP_FLASH, -1, -1, -1, -1, -1, "SPICS1",
    "terpakai SPI flash/PSRAM internal - JANGAN dipakai" },
  { 27, E32_CAP_FLASH, -1, -1, -1, -1, -1, "SPIHD",
    "terpakai SPI flash/PSRAM internal - JANGAN dipakai" },
  { 28, E32_CAP_FLASH, -1, -1, -1, -1, -1, "SPIWP",
    "terpakai SPI flash/PSRAM internal - JANGAN dipakai" },
  { 29, E32_CAP_FLASH, -1, -1, -1, -1, -1, "SPICS0",
    "terpakai SPI flash/PSRAM internal - JANGAN dipakai" },
  { 30, E32_CAP_FLASH, -1, -1, -1, -1, -1, "SPICLK",
    "terpakai SPI flash/PSRAM internal - JANGAN dipakai" },
  { 31, E32_CAP_FLASH, -1, -1, -1, -1, -1, "SPIQ",
    "terpakai SPI flash/PSRAM internal - JANGAN dipakai" },
  { 32, E32_CAP_FLASH, -1, -1, -1, -1, -1, "SPID",
    "terpakai SPI flash/PSRAM internal - JANGAN dipakai" },

  /* --- GPIO 33-37: hanya terpakai bila memakai octal PSRAM --- */
  { 33, E32_OCT_CAPS, -1, -1, -1, -1, -1, "D33", E32_OCT_NOTE },
  { 34, E32_OCT_CAPS, -1, -1, -1, -1, -1, "D34", E32_OCT_NOTE },
  { 35, E32_OCT_CAPS, -1, -1, -1, -1, -1, "D35", E32_OCT_NOTE },
  { 36, E32_OCT_CAPS, -1, -1, -1, -1, -1, "D36", E32_OCT_NOTE },
  { 37, E32_OCT_CAPS, -1, -1, -1, -1, -1, "D37", E32_OCT_NOTE },

  /* --- GPIO 38-48: digital biasa --- */
  { 38, E32_IO_FULL, -1, -1, -1, -1, -1, "D38",
    "pada ESP32-S3-DevKitC-1 v1.1 LED RGB onboard ada di pin ini" },
  { 39, E32_IO_FULL, -1, -1, -1, -1, -1, "D39", "JTAG MTCK" },
  { 40, E32_IO_FULL, -1, -1, -1, -1, -1, "D40", "JTAG MTDO" },
  { 41, E32_IO_FULL, -1, -1, -1, -1, -1, "D41", "JTAG MTDI" },
  { 42, E32_IO_FULL, -1, -1, -1, -1, -1, "D42", "JTAG MTMS" },
  { 43, E32_IO_FULL | E32_CAP_UART0, -1, -1, -1, -1, -1, "TX0",
    "UART0 TX (konsol serial); hindari" },
  { 44, E32_IO_FULL | E32_CAP_UART0, -1, -1, -1, -1, -1, "RX0",
    "UART0 RX (konsol serial); hindari" },
  { 45, E32_IO_FULL | E32_CAP_STRAP, -1, -1, -1, -1, -1, "D45",
    "strapping VDD_SPI (tegangan flash); harus LOW saat boot" },
  { 46, E32_IO_FULL | E32_CAP_STRAP, -1, -1, -1, -1, -1, "D46",
    "strapping (pengatur log ROM); harus LOW saat boot" },
  { 47, E32_IO_FULL, -1, -1, -1, -1, -1, "D47", NULL },
  { 48, E32_IO_FULL | E32_CAP_LED, -1, -1, -1, -1, -1, "D48",
    "LED RGB WS2812 onboard (DevKitC-1 v1.0)" }
};

static const char *const e32_board_name_str = "esp32-s3-devkitc-1";
static const char *const e32_board_chip_str = "ESP32-S3";

#else /* ---------------------- ESP32 klasik ---------------------------- */

/* ------------------------------------------------------------------------
 * ESP32 klasik  (papan DOIT ESP32 DEVKIT V1, 30 pin)
 *
 * GPIO yang didaftarkan: 0-5, 6-11 (flash), 12-19, 21-23, 25-27, 32-39.
 * GPIO 20, 24, 28-31 tidak tersedia pada modul ESP32-WROOM-32.
 * GPIO 34/35/36/39 input saja: tanpa output, tanpa pull internal.
 * ADC1 ch0..ch7 = GPIO 36,37,38,39,32,33,34,35
 * ADC2 ch0..ch9 = GPIO 4,0,2,15,13,12,14,27,25,26
 * Touch T0..T9  = GPIO 4,0,2,15,13,12,14,27,33,32
 * DAC1=GPIO25, DAC2=GPIO26
 * ---------------------------------------------------------------------- */

static const e32_pin_info_t e32_pins[] = {
  { 0,  E32_IO_FULL | E32_CAP_ADC2 | E32_CAP_TOUCH | E32_CAP_RTC |
        E32_CAP_STRAP | E32_CAP_BOOT,
    2, 1, 1, -1, 11, "D0",
    "strapping + tombol BOOT; LOW saat boot = mode unduh" },
  { 1,  E32_IO_FULL | E32_CAP_UART0,
    -1, -1, -1, -1, -1, "TX0",
    "UART0 TX (konsol serial); hindari, mengeluarkan log saat boot" },
  { 2,  E32_IO_FULL | E32_CAP_ADC2 | E32_CAP_TOUCH | E32_CAP_RTC |
        E32_CAP_STRAP | E32_CAP_LED,
    2, 2, 2, -1, 12, "D2",
    "LED onboard + strapping; harus LOW/mengambang saat boot" },
  { 3,  E32_IO_FULL | E32_CAP_UART0,
    -1, -1, -1, -1, -1, "RX0",
    "UART0 RX (konsol serial); HIGH saat boot; hindari" },
  { 4,  E32_IO_FULL | E32_CAP_ADC2 | E32_CAP_TOUCH | E32_CAP_RTC,
    2, 0, 0, -1, 10, "D4", "ADC2 tidak bisa dipakai saat WiFi aktif" },
  { 5,  E32_IO_FULL | E32_CAP_STRAP | E32_CAP_SPI,
    -1, -1, -1, -1, -1, "D5",
    "strapping + SS bawaan VSPI; mengeluarkan pulsa PWM saat boot" },

  /* --- GPIO 6-11: SPI flash internal, haram disentuh --- */
  { 6,  E32_CAP_FLASH, -1, -1, -1, -1, -1, "SD_CLK",
    "terpakai SPI flash internal - JANGAN dipakai, papan akan gagal boot" },
  { 7,  E32_CAP_FLASH, -1, -1, -1, -1, -1, "SD_DATA0",
    "terpakai SPI flash internal - JANGAN dipakai, papan akan gagal boot" },
  { 8,  E32_CAP_FLASH, -1, -1, -1, -1, -1, "SD_DATA1",
    "terpakai SPI flash internal - JANGAN dipakai, papan akan gagal boot" },
  { 9,  E32_CAP_FLASH, -1, -1, -1, -1, -1, "SD_DATA2",
    "terpakai SPI flash internal - JANGAN dipakai, papan akan gagal boot" },
  { 10, E32_CAP_FLASH, -1, -1, -1, -1, -1, "SD_DATA3",
    "terpakai SPI flash internal - JANGAN dipakai, papan akan gagal boot" },
  { 11, E32_CAP_FLASH, -1, -1, -1, -1, -1, "SD_CMD",
    "terpakai SPI flash internal - JANGAN dipakai, papan akan gagal boot" },

  /* --- GPIO 12-19 --- */
  { 12, E32_IO_FULL | E32_CAP_ADC2 | E32_CAP_TOUCH | E32_CAP_RTC |
        E32_CAP_STRAP,
    2, 5, 5, -1, 15, "D12",
    "strapping MTDI; HARUS LOW saat boot (HIGH memaksa flash 1.8V)" },
  { 13, E32_IO_FULL | E32_CAP_ADC2 | E32_CAP_TOUCH | E32_CAP_RTC,
    2, 4, 4, -1, 14, "D13", "JTAG MTCK; ADC2 bentrok dengan WiFi" },
  { 14, E32_IO_FULL | E32_CAP_ADC2 | E32_CAP_TOUCH | E32_CAP_RTC,
    2, 6, 6, -1, 16, "D14",
    "JTAG MTMS; mengeluarkan pulsa PWM saat boot" },
  { 15, E32_IO_FULL | E32_CAP_ADC2 | E32_CAP_TOUCH | E32_CAP_RTC |
        E32_CAP_STRAP,
    2, 3, 3, -1, 13, "D15",
    "strapping MTDO; LOW saat boot mematikan log; keluar pulsa PWM saat boot" },
  { 16, E32_IO_FULL, -1, -1, -1, -1, -1, "RX2",
    "UART2 RX bawaan; terpakai PSRAM pada modul WROVER" },
  { 17, E32_IO_FULL, -1, -1, -1, -1, -1, "TX2",
    "UART2 TX bawaan; terpakai PSRAM pada modul WROVER" },
  { 18, E32_IO_FULL | E32_CAP_SPI, -1, -1, -1, -1, -1, "D18",
    "SCK bawaan VSPI" },
  { 19, E32_IO_FULL | E32_CAP_SPI, -1, -1, -1, -1, -1, "D19",
    "MISO bawaan VSPI" },

  /* GPIO 20 tidak dibonding pada modul ESP32-WROOM-32. */

  /* --- GPIO 21-23 --- */
  { 21, E32_IO_FULL | E32_CAP_I2C, -1, -1, -1, -1, -1, "D21",
    "SDA bawaan (Wire)" },
  { 22, E32_IO_FULL | E32_CAP_I2C, -1, -1, -1, -1, -1, "D22",
    "SCL bawaan (Wire)" },
  { 23, E32_IO_FULL | E32_CAP_SPI, -1, -1, -1, -1, -1, "D23",
    "MOSI bawaan VSPI" },

  /* GPIO 24 tidak ada pada ESP32. */

  /* --- GPIO 25-27 --- */
  { 25, E32_IO_FULL | E32_CAP_ADC2 | E32_CAP_DAC | E32_CAP_RTC,
    2, 8, -1, 1, 6, "D25", "DAC1; ADC2 bentrok dengan WiFi" },
  { 26, E32_IO_FULL | E32_CAP_ADC2 | E32_CAP_DAC | E32_CAP_RTC,
    2, 9, -1, 2, 7, "D26", "DAC2; ADC2 bentrok dengan WiFi" },
  { 27, E32_IO_FULL | E32_CAP_ADC2 | E32_CAP_TOUCH | E32_CAP_RTC,
    2, 7, 7, -1, 17, "D27", "ADC2 bentrok dengan WiFi" },

  /* GPIO 28-31 tidak ada pada ESP32. */

  /* --- GPIO 32-33: ADC1 + touch, serba bisa --- */
  { 32, E32_IO_FULL | E32_CAP_ADC1 | E32_CAP_TOUCH | E32_CAP_RTC,
    1, 4, 9, -1, 9, "D32", NULL },
  { 33, E32_IO_FULL | E32_CAP_ADC1 | E32_CAP_TOUCH | E32_CAP_RTC,
    1, 5, 8, -1, 8, "D33", NULL },

  /* --- GPIO 34-39: INPUT SAJA, tanpa pull internal, tanpa PWM --- */
  { 34, E32_IN_ONLY | E32_CAP_ADC1 | E32_CAP_RTC,
    1, 6, -1, -1, 4, "D34",
    "input saja, tanpa pull-up/pull-down internal" },
  { 35, E32_IN_ONLY | E32_CAP_ADC1 | E32_CAP_RTC,
    1, 7, -1, -1, 5, "D35",
    "input saja, tanpa pull-up/pull-down internal" },
  { 36, E32_IN_ONLY | E32_CAP_ADC1 | E32_CAP_RTC,
    1, 0, -1, -1, 0, "VP",
    "SENSOR_VP; input saja, tanpa pull-up/pull-down internal" },
  { 37, E32_IN_ONLY | E32_CAP_ADC1 | E32_CAP_RTC,
    1, 1, -1, -1, 1, "GPIO37",
    "input saja; tidak dibonding pada modul WROOM-32 (tak ada di papan DOIT)" },
  { 38, E32_IN_ONLY | E32_CAP_ADC1 | E32_CAP_RTC,
    1, 2, -1, -1, 2, "GPIO38",
    "input saja; tidak dibonding pada modul WROOM-32 (tak ada di papan DOIT)" },
  { 39, E32_IN_ONLY | E32_CAP_ADC1 | E32_CAP_RTC,
    1, 3, -1, -1, 3, "VN",
    "SENSOR_VN; input saja, tanpa pull-up/pull-down internal" }
};

static const char *const e32_board_name_str = "esp32doit-devkit-v1";
static const char *const e32_board_chip_str = "ESP32-D0WDQ6";

#endif /* CONFIG_IDF_TARGET_ESP32S3 */

#define E32_PIN_COUNT (sizeof(e32_pins) / sizeof(e32_pins[0]))

/* ========================================================================
 * Implementasi fungsi publik
 * ====================================================================== */

const e32_pin_info_t *e32_pin_table(size_t *count) {
  if (count != NULL) {
    *count = (size_t) E32_PIN_COUNT;
  }
  return e32_pins;
}

const e32_pin_info_t *e32_pin_find(int gpio) {
  size_t i;
  for (i = 0; i < (size_t) E32_PIN_COUNT; i++) {
    if ((int) e32_pins[i].gpio == gpio) {
      return &e32_pins[i];
    }
  }
  return NULL;
}

int e32_pin_has(int gpio, uint32_t caps) {
  const e32_pin_info_t *p = e32_pin_find(gpio);
  if (p == NULL) {
    return 0;
  }
  return (p->caps & caps) == caps ? 1 : 0;
}

const char *e32_cap_name(uint32_t single_cap) {
  size_t i;
  for (i = 0; i < E32_CAP_NAME_COUNT; i++) {
    if (e32_cap_names[i].bit == single_cap) {
      return e32_cap_names[i].name;
    }
  }
  return "?";
}

char *e32_caps_to_string(uint32_t caps, char *buf, size_t buflen) {
  size_t pos = 0;
  size_t i;

  if (buf == NULL || buflen == 0) {
    return buf;
  }
  buf[0] = '\0';

  for (i = 0; i < E32_CAP_NAME_COUNT; i++) {
    const char *name;
    size_t nlen;
    size_t need;

    if ((caps & e32_cap_names[i].bit) == 0) {
      continue;
    }
    name = e32_cap_names[i].name;
    nlen = strlen(name);
    /* butuh: koma pemisah (bila bukan yang pertama) + nama + NUL */
    need = nlen + (pos > 0 ? 1u : 0u) + 1u;
    if (pos + need > buflen) {
      break; /* tidak muat lagi, hentikan dengan rapi */
    }
    if (pos > 0) {
      buf[pos++] = ',';
    }
    memcpy(buf + pos, name, nlen);
    pos += nlen;
    buf[pos] = '\0';
  }
  return buf;
}

const char *e32_board_name(void) {
  return e32_board_name_str;
}

const char *e32_board_chip(void) {
  return e32_board_chip_str;
}
