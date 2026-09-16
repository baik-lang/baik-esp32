/*
 * e32_konstanta.cpp - Modul 1: konstanta & introspeksi pinout untuk BAIK.
 *
 * Mendaftarkan ke object global:
 *   - konstanta level, mode pin, mode interupsi, matematika, urutan bit,
 *     atenuasi ADC, pin papan (LED_BUILTIN, TX, RX, SDA, SCL, ...),
 *     pin fisik (Dx / GPIOx / Ax / Tx / DACx), penyebab bangun, kapabilitas
 *   - string BOARD dan CHIP
 *   - fungsi introspeksi pinInfo / pinCaps / pinPunya / daftarPin / pinout
 *
 * Seluruh nomor pin diambil dari tabel e32_pins.h supaya tidak pernah
 * bentrok dengan validasi runtime.
 */

#include "baik_esp32.h"
#include "e32_pins.h"

#include <stdio.h>
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

#if !defined(CONFIG_IDF_TARGET_ESP32S3)
#if defined(ARDUINO_ESP32S3_DEV) || defined(ARDUINO_ESP32S3_DEVKITC_1) || \
    defined(ARDUINO_ESP32S3_DEVKITM_1)
#define CONFIG_IDF_TARGET_ESP32S3 1
#endif
#endif

/* ------------------------------------------------------------------------
 * Pin peripheral bawaan papan.
 * Diverifikasi dari berkas pins_arduino.h di direktori variants milik
 * arduino-esp32 (doitESP32devkitV1 dan esp32s3).
 * ---------------------------------------------------------------------- */

#if CONFIG_IDF_TARGET_ESP32S3

#define E32_PIN_LED_BUILTIN 48 /* LED RGB WS2812 pada DevKitC-1 v1.0 */
#define E32_PIN_BOOT        0
#define E32_PIN_TX          43
#define E32_PIN_RX          44
#define E32_PIN_SDA         8
#define E32_PIN_SCL         9
#define E32_PIN_MOSI        11
#define E32_PIN_MISO        13
#define E32_PIN_SCK         12
#define E32_PIN_SS          10

#else /* ESP32 klasik */

#define E32_PIN_LED_BUILTIN 2
#define E32_PIN_BOOT        0
#define E32_PIN_TX          1
#define E32_PIN_RX          3
#define E32_PIN_SDA         21
#define E32_PIN_SCL         22
#define E32_PIN_MOSI        23
#define E32_PIN_MISO        19
#define E32_PIN_SCK         18
#define E32_PIN_SS          5

#endif

/* ------------------------------------------------------------------------
 * Alias ADC (A0..An) - persis mengikuti variant resmi arduino-esp32.
 * Penomoran A tidak berurutan pada ESP32 klasik: A0..A7 = ADC1,
 * A10..A19 = ADC2. A1/A2 tidak ada karena GPIO37/38 tidak dibonding.
 * ---------------------------------------------------------------------- */

typedef struct e32_named_pin {
  const char *name;
  int16_t gpio;
} e32_named_pin_t;

#if CONFIG_IDF_TARGET_ESP32S3
static const e32_named_pin_t e32_adc_alias[] = {
  { "A0", 1 },   { "A1", 2 },   { "A2", 3 },   { "A3", 4 },   { "A4", 5 },
  { "A5", 6 },   { "A6", 7 },   { "A7", 8 },   { "A8", 9 },   { "A9", 10 },
  { "A10", 11 }, { "A11", 12 }, { "A12", 13 }, { "A13", 14 }, { "A14", 15 },
  { "A15", 16 }, { "A16", 17 }, { "A17", 18 }, { "A18", 19 }, { "A19", 20 }
};
#else
static const e32_named_pin_t e32_adc_alias[] = {
  { "A0", 36 },  { "A3", 39 },  { "A4", 32 },  { "A5", 33 },  { "A6", 34 },
  { "A7", 35 },  { "A10", 4 },  { "A11", 0 },  { "A12", 2 },  { "A13", 15 },
  { "A14", 13 }, { "A15", 12 }, { "A16", 14 }, { "A17", 27 }, { "A18", 25 },
  { "A19", 26 }
};
#endif

#define E32_ADC_ALIAS_COUNT \
  (sizeof(e32_adc_alias) / sizeof(e32_adc_alias[0]))

/* Jumlah bit kapabilitas yang dipakai (E32_CAP_DIGITAL .. E32_CAP_LED). */
#define E32_CAP_BIT_COUNT 18

/* ------------------------------------------------------------------------
 * Utilitas lokal
 * ---------------------------------------------------------------------- */

/* 1 bila `s` layak dipakai sebagai nama konstanta di bahasa BAIK. */
static int e32_is_ident(const char *s) {
  size_t i;
  char c;

  if (s == NULL || s[0] == '\0') {
    return 0;
  }
  c = s[0];
  if (!((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || c == '_')) {
    return 0;
  }
  for (i = 1; s[i] != '\0'; i++) {
    c = s[i];
    if (!((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') ||
          (c >= '0' && c <= '9') || c == '_')) {
      return 0;
    }
  }
  return 1;
}

/* Daftarkan satu konstanta bertipe string. */
static void e32_const_str(struct baik *baik, baik_val_t g, const char *name,
                          const char *value) {
  baik_set(baik, g, name, (size_t) ~0,
           baik_mk_string(baik, value, (size_t) ~0, 1));
}

/* ------------------------------------------------------------------------
 * Fungsi introspeksi yang dipanggil dari skrip BAIK
 * ---------------------------------------------------------------------- */

/* pinInfo(gpio) -> object {gpio,label,adc:{unit,channel},touch,dac,rtc,
 *                          caps:[...],capsBit,note}  atau `kosong`. */
static void e32_js_pin_info(struct baik *baik) {
  int gpio = e32_arg_int(baik, 0, -1);
  const e32_pin_info_t *p = e32_pin_find(gpio);
  baik_val_t obj;
  baik_val_t adc;
  baik_val_t arr;
  int i;

  if (p == NULL) {
    printf("pinInfo: GPIO%d tidak ada pada papan %s\n", gpio, e32_board_name());
    baik_return(baik, baik_mk_null());
    return;
  }

  obj = baik_mk_object(baik);
  baik_own(baik, &obj);

  baik_set(baik, obj, "gpio", (size_t) ~0,
           baik_mk_number(baik, (double) p->gpio));
  baik_set(baik, obj, "label", (size_t) ~0,
           baik_mk_string(baik, p->label != NULL ? p->label : "",
                          (size_t) ~0, 1));

  /* sub-object adc */
  adc = baik_mk_object(baik);
  baik_own(baik, &adc);
  baik_set(baik, adc, "unit", (size_t) ~0,
           baik_mk_number(baik, (double) p->adc_unit));
  baik_set(baik, adc, "channel", (size_t) ~0,
           baik_mk_number(baik, (double) p->adc_channel));
  baik_set(baik, obj, "adc", (size_t) ~0, adc);
  baik_disown(baik, &adc);

  baik_set(baik, obj, "touch", (size_t) ~0,
           baik_mk_number(baik, (double) p->touch_channel));
  baik_set(baik, obj, "dac", (size_t) ~0,
           baik_mk_number(baik, (double) p->dac_channel));
  baik_set(baik, obj, "rtc", (size_t) ~0,
           baik_mk_number(baik, (double) p->rtc_gpio));

  /* daftar nama kapabilitas */
  arr = baik_mk_array(baik);
  baik_own(baik, &arr);
  for (i = 0; i < E32_CAP_BIT_COUNT; i++) {
    uint32_t bit = (uint32_t) 1u << i;
    if ((p->caps & bit) != 0) {
      baik_array_push(baik, arr,
                      baik_mk_string(baik, e32_cap_name(bit), (size_t) ~0, 1));
    }
  }
  baik_set(baik, obj, "caps", (size_t) ~0, arr);
  baik_disown(baik, &arr);

  baik_set(baik, obj, "capsBit", (size_t) ~0,
           baik_mk_number(baik, (double) p->caps));
  baik_set(baik, obj, "note", (size_t) ~0,
           p->note != NULL ? baik_mk_string(baik, p->note, (size_t) ~0, 1)
                           : baik_mk_null());

  baik_disown(baik, &obj);
  baik_return(baik, obj);
}

/* pinCaps(gpio) -> angka bitmask kapabilitas (0 bila pin tidak ada). */
static void e32_js_pin_caps(struct baik *baik) {
  int gpio = e32_arg_int(baik, 0, -1);
  const e32_pin_info_t *p = e32_pin_find(gpio);
  e32_ret_num(baik, p != NULL ? (double) p->caps : 0.0);
}

/* pinPunya(gpio, capBitmask) / pinHas(...) -> benar / salah. */
static void e32_js_pin_has(struct baik *baik) {
  int gpio = e32_arg_int(baik, 0, -1);
  int caps = e32_arg_int(baik, 1, 0);
  e32_ret_bool(baik, e32_pin_has(gpio, (uint32_t) caps));
}

/* daftarPin([capBitmask]) / pinList(...) -> array nomor GPIO.
 * Tanpa argumen (atau 0) berarti seluruh pin papan ini. */
static void e32_js_pin_list(struct baik *baik) {
  uint32_t mask = (uint32_t) e32_arg_int(baik, 0, 0);
  const e32_pin_info_t *tbl;
  size_t count = 0;
  size_t i;
  baik_val_t arr;

  tbl = e32_pin_table(&count);
  arr = baik_mk_array(baik);
  baik_own(baik, &arr);
  for (i = 0; i < count; i++) {
    if ((tbl[i].caps & mask) == mask) {
      baik_array_push(baik, arr, baik_mk_number(baik, (double) tbl[i].gpio));
    }
  }
  baik_disown(baik, &arr);
  baik_return(baik, arr);
}

/* pinout() -> cetak tabel pinout ke konsol, balik `takterdefinisi`. */
static void e32_js_pinout(struct baik *baik) {
  e32_print_pinout();
  e32_ret_undef(baik);
}

/* ------------------------------------------------------------------------
 * Registrasi
 * ---------------------------------------------------------------------- */

void baik_esp32_register_konstanta(struct baik *baik, baik_val_t g) {
  const e32_pin_info_t *tbl;
  size_t count = 0;
  size_t i;
  char name[24];

  /* ---- Level logika ---- */
  e32_const(baik, g, "HIGH", 1);
  e32_const(baik, g, "LOW", 0);

  /* ---- Mode pin (nilai persis esp32-hal-gpio.h) ---- */
  e32_const(baik, g, "INPUT", 0x01);
  e32_const(baik, g, "OUTPUT", 0x03);
  e32_const(baik, g, "INPUT_PULLUP", 0x05);
  e32_const(baik, g, "INPUT_PULLDOWN", 0x09);
  e32_const(baik, g, "OUTPUT_OPEN_DRAIN", 0x13);
  e32_const(baik, g, "ANALOG", 0xC0);
  e32_const(baik, g, "PULLUP", 0x04);
  e32_const(baik, g, "PULLDOWN", 0x08);
  e32_const(baik, g, "OPEN_DRAIN", 0x10);

  /* ---- Mode interupsi ---- */
  e32_const(baik, g, "DISABLED", 0x00);
  e32_const(baik, g, "RISING", 0x01);
  e32_const(baik, g, "FALLING", 0x02);
  e32_const(baik, g, "CHANGE", 0x03);
  e32_const(baik, g, "ONLOW", 0x04);
  e32_const(baik, g, "ONHIGH", 0x05);
  e32_const(baik, g, "ONLOW_WE", 0x0C);
  e32_const(baik, g, "ONHIGH_WE", 0x0D);

  /* ---- Konstanta matematika ---- */
  e32_const(baik, g, "PI", 3.1415926535897932384626433832795);
  e32_const(baik, g, "HALF_PI", 1.5707963267948966192313216916398);
  e32_const(baik, g, "TWO_PI", 6.283185307179586476925286766559);
  e32_const(baik, g, "DEG_TO_RAD", 0.017453292519943295769236907684886);
  e32_const(baik, g, "RAD_TO_DEG", 57.295779513082320876798154814105);
  e32_const(baik, g, "EULER", 2.718281828459045235360287471352);

  /* ---- Urutan bit ---- */
  e32_const(baik, g, "LSBFIRST", 0);
  e32_const(baik, g, "MSBFIRST", 1);

  /* ---- Atenuasi ADC ---- */
  e32_const(baik, g, "ADC_0db", 0);
  e32_const(baik, g, "ADC_2_5db", 1);
  e32_const(baik, g, "ADC_6db", 2);
  e32_const(baik, g, "ADC_11db", 3);

  /* ---- Penyebab bangun untuk ext1 (esp_sleep_ext1_wakeup_mode_t) ---- */
  e32_const(baik, g, "WAKEUP_ALL_LOW", 0);
  e32_const(baik, g, "WAKEUP_ANY_HIGH", 1);

  /* ---- Kapabilitas pin (nilai = E32_CAP_*) ---- */
  e32_const(baik, g, "CAP_DIGITAL", (double) E32_CAP_DIGITAL);
  e32_const(baik, g, "CAP_INPUT", (double) E32_CAP_INPUT);
  e32_const(baik, g, "CAP_OUTPUT", (double) E32_CAP_OUTPUT);
  e32_const(baik, g, "CAP_PULL", (double) E32_CAP_PULL);
  e32_const(baik, g, "CAP_ADC1", (double) E32_CAP_ADC1);
  e32_const(baik, g, "CAP_ADC2", (double) E32_CAP_ADC2);
  e32_const(baik, g, "CAP_DAC", (double) E32_CAP_DAC);
  e32_const(baik, g, "CAP_TOUCH", (double) E32_CAP_TOUCH);
  e32_const(baik, g, "CAP_RTC", (double) E32_CAP_RTC);
  e32_const(baik, g, "CAP_PWM", (double) E32_CAP_PWM);
  e32_const(baik, g, "CAP_STRAP", (double) E32_CAP_STRAP);
  e32_const(baik, g, "CAP_FLASH", (double) E32_CAP_FLASH);
  e32_const(baik, g, "CAP_USB", (double) E32_CAP_USB);
  e32_const(baik, g, "CAP_UART0", (double) E32_CAP_UART0);
  e32_const(baik, g, "CAP_I2C", (double) E32_CAP_I2C);
  e32_const(baik, g, "CAP_SPI", (double) E32_CAP_SPI);
  e32_const(baik, g, "CAP_BOOT", (double) E32_CAP_BOOT);
  e32_const(baik, g, "CAP_LED", (double) E32_CAP_LED);
  e32_const(baik, g, "CAP_ADC", (double) E32_CAP_ADC);
  e32_const(baik, g, "CAP_IO", (double) E32_CAP_IO);

  /* ---- Pin peripheral bawaan papan ---- */
  e32_const(baik, g, "LED_BUILTIN", E32_PIN_LED_BUILTIN);
  e32_const(baik, g, "BUILTIN_LED", E32_PIN_LED_BUILTIN);
  e32_const(baik, g, "BOOT_BUTTON", E32_PIN_BOOT);
  e32_const(baik, g, "TX", E32_PIN_TX);
  e32_const(baik, g, "RX", E32_PIN_RX);
  e32_const(baik, g, "SDA", E32_PIN_SDA);
  e32_const(baik, g, "SCL", E32_PIN_SCL);
  e32_const(baik, g, "MOSI", E32_PIN_MOSI);
  e32_const(baik, g, "MISO", E32_PIN_MISO);
  e32_const(baik, g, "SCK", E32_PIN_SCK);
  e32_const(baik, g, "SS", E32_PIN_SS);
#if CONFIG_IDF_TARGET_ESP32S3
  e32_const(baik, g, "RGB_BUILTIN", E32_PIN_LED_BUILTIN);
#endif

  /* ---- Pin fisik: GPIOx, label sablon (Dx/VP/VN/TX0/...), Tx, DACx ----
   * Semua diambil dari tabel pin supaya tidak mungkin bentrok dengan
   * validasi runtime. Pin yang tidak ada pada papan aktif otomatis
   * tidak terdaftar (mis. DAC1/DAC2 pada ESP32-S3). */
  tbl = e32_pin_table(&count);
  for (i = 0; i < count; i++) {
    const e32_pin_info_t *p = &tbl[i];

    snprintf(name, sizeof(name), "GPIO%d", (int) p->gpio);
    e32_const(baik, g, name, (double) p->gpio);

    /* Label sablon didaftarkan juga (D23, VP, VN, TX0, SD_CLK, ...),
     * kecuali bila labelnya memang sama dengan nama "GPIOx" di atas. */
    if (e32_is_ident(p->label) && strcmp(p->label, name) != 0) {
      e32_const(baik, g, p->label, (double) p->gpio);
    }
    if (p->touch_channel >= 0) {
      snprintf(name, sizeof(name), "T%d", (int) p->touch_channel);
      e32_const(baik, g, name, (double) p->gpio);
    }
    if (p->dac_channel >= 0) {
      snprintf(name, sizeof(name), "DAC%d", (int) p->dac_channel);
      e32_const(baik, g, name, (double) p->gpio);
    }
  }

  /* ---- Alias ADC A0..An sesuai variant resmi ---- */
  for (i = 0; i < (size_t) E32_ADC_ALIAS_COUNT; i++) {
    e32_const(baik, g, e32_adc_alias[i].name,
              (double) e32_adc_alias[i].gpio);
  }

  /* ---- Identitas papan (string) ---- */
  e32_const_str(baik, g, "BOARD", e32_board_name());
  e32_const_str(baik, g, "CHIP", e32_board_chip());

  /* ---- Fungsi introspeksi pinout ---- */
  e32_fn(baik, g, "pinInfo", e32_js_pin_info);
  e32_fn(baik, g, "pinCaps", e32_js_pin_caps);
  e32_fn(baik, g, "pinPunya", e32_js_pin_has);
  e32_fn(baik, g, "pinHas", e32_js_pin_has);
  e32_fn(baik, g, "daftarPin", e32_js_pin_list);
  e32_fn(baik, g, "pinList", e32_js_pin_list);
  e32_fn(baik, g, "pinout", e32_js_pinout);
}
