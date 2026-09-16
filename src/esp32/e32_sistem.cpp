/*
 * e32_sistem.cpp - Modul 3: SISTEM & WAKTU untuk bahasa BAIK di ESP32.
 *
 * Isi modul ini:
 *   - waktu          : delay, delayMicroseconds, millis, micros, yield
 *   - acak           : random, randomSeed
 *   - matematika     : map, constrain, min, max, abs, pow, sqrt, sin, cos,
 *                      tan, atan, atan2, floor, ceil, round, log, log10, exp
 *   - object Serial  : begin/print/println/printf/available/read/readString/
 *                      write/flush/end
 *   - object ESP     : restart + seluruh getter memori/chip/flash/sketch
 *   - frekuensi CPU  : setCpuFrequencyMhz, getCpuFrequencyMhz,
 *                      getXtalFrequencyMhz, getApbFrequency
 *   - sensor internal: temperatureRead (hallRead ada di e32_gpio.cpp)
 *   - tidur          : seluruh esp_sleep_* + tidurDalam/tidurRingan +
 *                      sebabBangun/sebabReset
 *   - RTC slow memory: rtcSet / rtcGet (bertahan melewati deep sleep)
 *   - watchdog       : watchdogEnable / watchdogReset / watchdogDisable
 *   - FreeRTOS       : getTaskCount / getTaskHighWaterMark
 *
 * Catatan: seluruh simbol internal berkas ini bersifat `static`. Satu-satunya
 * simbol publik adalah baik_esp32_register_sistem().
 */

/* Header pustaka standar didahulukan: Arduino.h mendefinisikan abs/min/max/
 * round sebagai MAKRO, yang akan merusak deklarasi di <stdlib.h>/<math.h>
 * bila header itu baru diproses sesudahnya. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>

#include <Arduino.h>

#include <esp_attr.h>
#include <esp_idf_version.h>
#include <esp_system.h>
#include <esp_sleep.h>
#include <esp_timer.h>
#if defined(__has_include)
#if __has_include(<esp_random.h>)
#include <esp_random.h>
#endif
#endif
#include <esp_task_wdt.h>
#include <soc/soc_caps.h>
#include <soc/rtc.h>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include "baik_esp32.h"

/* Beberapa versi arduino-esp32 menyembunyikan deklarasi ini di balik penjaga
 * target; deklarasi ulang berikut identik dengan yang di core, jadi aman. */
extern "C" float temperatureRead(void);

/* ---------------------------------------------------------------------------
 * Deteksi kemampuan papan / versi framework
 * ------------------------------------------------------------------------ */

#ifndef ESP_ARDUINO_VERSION_MAJOR
#define ESP_ARDUINO_VERSION_MAJOR 2   /* arduino-esp32 lama tanpa makro versi */
#endif

#ifndef ESP_IDF_VERSION_MAJOR
#define ESP_IDF_VERSION_MAJOR 4
#endif

/* hallRead(): hanya ESP32 klasik, dan dihapus pada arduino-esp32 3.x. */
#if defined(CONFIG_IDF_TARGET_ESP32) && (ESP_ARDUINO_VERSION_MAJOR < 3)
#define E32_HAS_HALL 1
#else
#define E32_HAS_HALL 0
#endif

/* Wake dari pin RTC (ext0/ext1): ada di ESP32 dan ESP32-S3. */
#if defined(SOC_PM_SUPPORT_EXT0_WAKEUP) || defined(SOC_PM_SUPPORT_EXT_WAKEUP) || \
    defined(CONFIG_IDF_TARGET_ESP32) || defined(CONFIG_IDF_TARGET_ESP32S3)
#define E32_HAS_EXT_WAKEUP 1
#else
#define E32_HAS_EXT_WAKEUP 0
#endif

/* Wake dari sensor sentuh. */
#if defined(SOC_TOUCH_SENSOR_SUPPORTED) || defined(CONFIG_IDF_TARGET_ESP32) || \
    defined(CONFIG_IDF_TARGET_ESP32S3)
#define E32_HAS_TOUCH_WAKEUP 1
#else
#define E32_HAS_TOUCH_WAKEUP 0
#endif

/* Jumlah GPIO tertinggi yang mungkin dipakai pada mask ext1. */
#if defined(CONFIG_IDF_TARGET_ESP32S3)
#define E32_GPIO_MAX 48
#else
#define E32_GPIO_MAX 39
#endif

/* ---------------------------------------------------------------------------
 * Utilitas kecil: ubah nilai BAIK apa pun menjadi teks / angka
 * ------------------------------------------------------------------------ */

/* Teks tanpa tanda kutip: string dipakai apa adanya (baik_get_cstring),
 * tipe lain dirender oleh baik_sprintf(). */
static void e32_val_text(struct baik *baik, baik_val_t v, char *buf, size_t len) {
  if (buf == NULL || len == 0) return;
  buf[0] = '\0';

  if (baik_is_string(v)) {
    baik_val_t tmp = v;
    const char *s = baik_get_cstring(baik, &tmp);
    if (s != NULL) {
      snprintf(buf, len, "%s", s);
      return;
    }
    /* Cadangan: string tanpa NUL di heap BAIK -> salin sepanjang ukurannya. */
    {
      baik_val_t t2 = v;
      size_t n = 0;
      const char *raw = baik_get_string(baik, &t2, &n);
      if (raw == NULL) return;
      if (n > len - 1) n = len - 1;
      memcpy(buf, raw, n);
      buf[n] = '\0';
    }
    return;
  }

  baik_sprintf(v, baik, buf, len);
  buf[len - 1] = '\0';
}

/* Angka dari nilai BAIK apa pun (untuk Serial.printf). */
static double e32_val_num(struct baik *baik, baik_val_t v) {
  if (baik_is_number(v)) return baik_get_double(baik, v);
  if (baik_is_boolean(v)) return baik_get_bool(baik, v) ? 1.0 : 0.0;
  if (baik_is_string(v)) {
    char b[48];
    e32_val_text(baik, v, b, sizeof(b));
    return atof(b);
  }
  return 0.0;
}

/* Cetak satu argumen apa adanya ke Serial. */
static void e32_print_arg(struct baik *baik, baik_val_t v) {
  char buf[160];
  e32_val_text(baik, v, buf, sizeof(buf));
  Serial.print(buf);
}

/* ---------------------------------------------------------------------------
 * WAKTU
 * ------------------------------------------------------------------------ */

/* delay(ms) - memakai delay() Arduino yang sudah RTOS-aware (vTaskDelay),
 * jadi task konsol tidak memblokir sistem. Sisa pecahan < 1 ms diselesaikan
 * dengan delayMicroseconds(). */
static void bk_delay(struct baik *baik) {
  double ms = e32_arg_num(baik, 0, 0.0);
  if (!(ms > 0.0)) {
    yield();
    e32_ret_undef(baik);
    return;
  }
  {
    uint32_t whole = (uint32_t) ms;
    double frac = ms - (double) whole;
    if (whole > 0) delay(whole);                 /* vTaskDelay di dalamnya   */
    if (frac > 0.0005) delayMicroseconds((uint32_t) (frac * 1000.0));
  }
  e32_ret_undef(baik);
}

static void bk_delay_microseconds(struct baik *baik) {
  double us = e32_arg_num(baik, 0, 0.0);
  if (us > 0.0) delayMicroseconds((uint32_t) us);
  e32_ret_undef(baik);
}

/* millis()/micros() memakai esp_timer 64-bit supaya tidak melilit (wrap)
 * seperti versi 32-bit Arduino. */
static void bk_millis(struct baik *baik) {
  e32_ret_num(baik, (double) (esp_timer_get_time() / 1000LL));
}

static void bk_micros(struct baik *baik) {
  e32_ret_num(baik, (double) esp_timer_get_time());
}

static void bk_yield(struct baik *baik) {
  yield();
  e32_ret_undef(baik);
}

/* ---------------------------------------------------------------------------
 * ACAK
 * ------------------------------------------------------------------------ */

static uint32_t e32_rand_state = 0;
static int      e32_rand_seeded = 0;

static uint32_t e32_rand32(void) {
  if (!e32_rand_seeded) {
    return esp_random();            /* generator perangkat keras */
  }
  {
    uint32_t x = e32_rand_state;    /* xorshift32, dapat diulang */
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    e32_rand_state = x;
    return x;
  }
}

/* random(maks) -> 0..maks-1 ; random(min, maks) -> min..maks-1 */
static void bk_random(struct baik *baik) {
  int n = baik_nargs(baik);
  long lo = 0, hi = 0;

  if (n <= 0) {
    e32_ret_num(baik, (double) (e32_rand32() & 0x7fffffffu));
    return;
  }
  if (n == 1) {
    hi = (long) e32_arg_num(baik, 0, 0.0);
  } else {
    lo = (long) e32_arg_num(baik, 0, 0.0);
    hi = (long) e32_arg_num(baik, 1, 0.0);
  }
  if (hi <= lo) {
    e32_ret_num(baik, (double) lo);
    return;
  }
  e32_ret_num(baik, (double) (lo + (long) (e32_rand32() % (uint32_t) (hi - lo))));
}

static void bk_random_seed(struct baik *baik) {
  uint32_t s = (uint32_t) e32_arg_num(baik, 0, 0.0);
  if (s == 0) s = 0x9E3779B9u;      /* xorshift tidak boleh berbenih 0 */
  e32_rand_state = s;
  e32_rand_seeded = 1;
  e32_ret_undef(baik);
}

/* ---------------------------------------------------------------------------
 * MATEMATIKA DASAR (BAIK belum punya object Math -> fungsi global)
 * ------------------------------------------------------------------------ */

/* map(x, in_min, in_max, out_min, out_max) - dihitung dalam pecahan (double)
 * karena seluruh angka BAIK adalah double. */
static void bk_map(struct baik *baik) {
  double x = e32_arg_num(baik, 0, 0.0);
  double in_lo = e32_arg_num(baik, 1, 0.0);
  double in_hi = e32_arg_num(baik, 2, 0.0);
  double out_lo = e32_arg_num(baik, 3, 0.0);
  double out_hi = e32_arg_num(baik, 4, 0.0);

  if (in_hi == in_lo) {
    printf("GALAT map(): in_min dan in_max tidak boleh sama (%g)\n", in_lo);
    e32_ret_num(baik, out_lo);
    return;
  }
  e32_ret_num(baik, (x - in_lo) * (out_hi - out_lo) / (in_hi - in_lo) + out_lo);
}

static void bk_constrain(struct baik *baik) {
  double x = e32_arg_num(baik, 0, 0.0);
  double a = e32_arg_num(baik, 1, 0.0);
  double b = e32_arg_num(baik, 2, 0.0);
  double lo = (a < b) ? a : b;
  double hi = (a < b) ? b : a;
  if (x < lo) x = lo;
  if (x > hi) x = hi;
  e32_ret_num(baik, x);
}

static void bk_min(struct baik *baik) {
  int n = baik_nargs(baik), i;
  double r;
  if (n <= 0) { e32_ret_num(baik, 0.0); return; }
  r = e32_arg_num(baik, 0, 0.0);
  for (i = 1; i < n; i++) {
    double v = e32_arg_num(baik, i, 0.0);
    if (v < r) r = v;
  }
  e32_ret_num(baik, r);
}

static void bk_max(struct baik *baik) {
  int n = baik_nargs(baik), i;
  double r;
  if (n <= 0) { e32_ret_num(baik, 0.0); return; }
  r = e32_arg_num(baik, 0, 0.0);
  for (i = 1; i < n; i++) {
    double v = e32_arg_num(baik, i, 0.0);
    if (v > r) r = v;
  }
  e32_ret_num(baik, r);
}

/* Catatan: Arduino.h mendefinisikan abs/round/min/max sebagai MAKRO, jadi di
 * dalam berkas ini kita hanya memakai fungsi <math.h> (fabs, floor, ceil). */
static void bk_abs(struct baik *baik)  { e32_ret_num(baik, fabs(e32_arg_num(baik, 0, 0.0))); }
static void bk_pow(struct baik *baik)  { e32_ret_num(baik, pow(e32_arg_num(baik, 0, 0.0), e32_arg_num(baik, 1, 0.0))); }
static void bk_sqrt(struct baik *baik) { e32_ret_num(baik, sqrt(e32_arg_num(baik, 0, 0.0))); }
static void bk_sin(struct baik *baik)  { e32_ret_num(baik, sin(e32_arg_num(baik, 0, 0.0))); }
static void bk_cos(struct baik *baik)  { e32_ret_num(baik, cos(e32_arg_num(baik, 0, 0.0))); }
static void bk_tan(struct baik *baik)  { e32_ret_num(baik, tan(e32_arg_num(baik, 0, 0.0))); }
static void bk_atan(struct baik *baik) { e32_ret_num(baik, atan(e32_arg_num(baik, 0, 0.0))); }
static void bk_atan2(struct baik *baik){ e32_ret_num(baik, atan2(e32_arg_num(baik, 0, 0.0), e32_arg_num(baik, 1, 0.0))); }
static void bk_floor(struct baik *baik){ e32_ret_num(baik, floor(e32_arg_num(baik, 0, 0.0))); }
static void bk_ceil(struct baik *baik) { e32_ret_num(baik, ceil(e32_arg_num(baik, 0, 0.0))); }
static void bk_exp(struct baik *baik)  { e32_ret_num(baik, exp(e32_arg_num(baik, 0, 0.0))); }

static void bk_round(struct baik *baik) {
  double v = e32_arg_num(baik, 0, 0.0);
  e32_ret_num(baik, (v >= 0.0) ? floor(v + 0.5) : ceil(v - 0.5));
}

static void bk_log(struct baik *baik) {
  double v = e32_arg_num(baik, 0, 0.0);
  if (v <= 0.0) {
    printf("GALAT log(): argumen harus lebih besar dari 0\n");
    e32_ret_undef(baik);
    return;
  }
  e32_ret_num(baik, log(v));
}

static void bk_log10(struct baik *baik) {
  double v = e32_arg_num(baik, 0, 0.0);
  if (v <= 0.0) {
    printf("GALAT log10(): argumen harus lebih besar dari 0\n");
    e32_ret_undef(baik);
    return;
  }
  e32_ret_num(baik, log10(v));
}

/* ---------------------------------------------------------------------------
 * OBJECT Serial
 * ------------------------------------------------------------------------ */

/* Serial.begin() dan Serial.end() sengaja TIDAK meneruskan panggilannya.
 *
 * REPL BAIK hidup di UART0 - port yang sama dengan object Serial ini. Skrip
 * yang memanggil Serial.begin(9600) akan mengubah baud rate di bawah kaki
 * konsolnya sendiri, dan Serial.end() mematikan port itu sepenuhnya. Dua-duanya
 * memutus satu-satunya jalur yang dipakai pengguna untuk mengetik perintah,
 * sehingga papan tampak menggantung dan hanya bisa dipulihkan dengan reset.
 *
 * Di sketch Arduino biasa Serial.begin() memang wajib, jadi nama ini tetap
 * disediakan supaya kode yang disalin dari tutorial tidak langsung galat -
 * tetapi ia hanya menjelaskan keadaan, bukan menyentuh perangkat kerasnya.
 * Konsol sudah dibuka oleh firmware pada 115200 sebelum skrip mana pun jalan.
 *
 * Untuk memakai UART lain, pakai Serial1 / Serial2 yang tidak dipakai konsol.
 */
static void bk_serial_begin(struct baik *baik) {
  long baud = (long) e32_arg_num(baik, 0, 0.0);

  if (baud > 0 && baud != 115200) {
    printf("Catatan Serial.begin(%ld): diabaikan. Konsol BAIK memakai UART0\n"
           "        pada 115200 baud; mengubahnya dari skrip akan memutus\n"
           "        koneksi ini. Pakai Serial1 atau Serial2 untuk UART lain.\n",
           baud);
  }
  e32_ret_undef(baik);
}

static void bk_serial_end(struct baik *baik) {
  printf("Catatan Serial.end(): diabaikan. Menutup UART0 akan mematikan konsol\n"
         "        BAIK dan papan hanya bisa dipulihkan dengan reset.\n");
  e32_ret_undef(baik);
}

static void bk_serial_print(struct baik *baik) {
  int n = baik_nargs(baik), i;
  for (i = 0; i < n; i++) e32_print_arg(baik, baik_arg(baik, i));
  e32_ret_undef(baik);
}

static void bk_serial_println(struct baik *baik) {
  int n = baik_nargs(baik), i;
  for (i = 0; i < n; i++) e32_print_arg(baik, baik_arg(baik, i));
  Serial.print("\r\n");
  e32_ret_undef(baik);
}

/* Serial.printf(fmt, ...) - penerjemah format mini.
 * Pengubah panjang (%ld, %zu, ...) diabaikan; tipe dipaksa sesuai huruf
 * konversi supaya aman walau BAIK hanya punya double/string. */
static void bk_serial_printf(struct baik *baik) {
  char fmt[160];
  char spec[32];
  char out[192];
  const char *p;
  int nargs = baik_nargs(baik);
  int argi = 1;
  baik_val_t f0;

  if (nargs < 1) {
    printf("GALAT Serial.printf(): butuh minimal satu argumen (teks format)\n");
    e32_ret_undef(baik);
    return;
  }
  f0 = baik_arg(baik, 0);
  if (!baik_is_string(f0)) {
    printf("GALAT Serial.printf(): argumen pertama harus teks format, bukan %s\n",
           baik_typeof(f0));
    e32_ret_undef(baik);
    return;
  }
  e32_val_text(baik, f0, fmt, sizeof(fmt));

  for (p = fmt; *p != '\0'; p++) {
    size_t sl;
    char conv;

    if (*p != '%') {
      Serial.write((uint8_t) *p);
      continue;
    }
    p++;
    if (*p == '%') {
      Serial.write((uint8_t) '%');
      continue;
    }
    if (*p == '\0') break;

    /* Salin bendera, lebar, dan presisi apa adanya. */
    sl = 0;
    spec[sl++] = '%';
    while (*p != '\0' && strchr("-+ #0", *p) != NULL && sl < sizeof(spec) - 4) {
      spec[sl++] = *p++;
    }
    while (*p != '\0' && isdigit((unsigned char) *p) && sl < sizeof(spec) - 4) {
      spec[sl++] = *p++;
    }
    if (*p == '.' && sl < sizeof(spec) - 4) {
      spec[sl++] = *p++;
      while (*p != '\0' && isdigit((unsigned char) *p) && sl < sizeof(spec) - 4) {
        spec[sl++] = *p++;
      }
    }
    /* Buang pengubah panjang milik C; kita tentukan sendiri tipenya. */
    while (*p != '\0' && strchr("hlLjzt", *p) != NULL) p++;
    if (*p == '\0') break;

    conv = *p;
    switch (conv) {
      case 'd':
      case 'i': {
        double d = (argi < nargs) ? e32_val_num(baik, baik_arg(baik, argi)) : 0.0;
        argi++;
        spec[sl++] = 'l';
        spec[sl++] = conv;
        spec[sl] = '\0';
        snprintf(out, sizeof(out), spec, (long) d);
        Serial.print(out);
        break;
      }
      case 'u':
      case 'x':
      case 'X':
      case 'o': {
        double d = (argi < nargs) ? e32_val_num(baik, baik_arg(baik, argi)) : 0.0;
        argi++;
        spec[sl++] = 'l';
        spec[sl++] = conv;
        spec[sl] = '\0';
        snprintf(out, sizeof(out), spec, (unsigned long) (long) d);
        Serial.print(out);
        break;
      }
      case 'f':
      case 'F':
      case 'e':
      case 'E':
      case 'g':
      case 'G': {
        double d = (argi < nargs) ? e32_val_num(baik, baik_arg(baik, argi)) : 0.0;
        argi++;
        spec[sl++] = conv;
        spec[sl] = '\0';
        snprintf(out, sizeof(out), spec, d);
        Serial.print(out);
        break;
      }
      case 'c': {
        double d = (argi < nargs) ? e32_val_num(baik, baik_arg(baik, argi)) : 0.0;
        argi++;
        spec[sl++] = 'c';
        spec[sl] = '\0';
        snprintf(out, sizeof(out), spec, (int) d);
        Serial.print(out);
        break;
      }
      case 's': {
        char tb[128];
        tb[0] = '\0';
        if (argi < nargs) e32_val_text(baik, baik_arg(baik, argi), tb, sizeof(tb));
        argi++;
        spec[sl++] = 's';
        spec[sl] = '\0';
        snprintf(out, sizeof(out), spec, tb);
        Serial.print(out);
        break;
      }
      default:
        /* Konversi tidak dikenal: cetak apa adanya. */
        Serial.write((uint8_t) '%');
        Serial.write((uint8_t) conv);
        break;
    }
  }
  e32_ret_undef(baik);
}

static void bk_serial_available(struct baik *baik) {
  e32_ret_num(baik, (double) Serial.available());
}

static void bk_serial_read(struct baik *baik) {
  e32_ret_num(baik, (double) Serial.read());   /* -1 bila tidak ada data */
}

static void bk_serial_read_string(struct baik *baik) {
  String s = Serial.readString();
  e32_ret_str(baik, s.c_str());
}

static void bk_serial_write(struct baik *baik) {
  baik_val_t v = baik_arg(baik, 0);
  if (baik_nargs(baik) < 1) {
    printf("GALAT Serial.write(): butuh satu argumen (angka byte atau teks)\n");
    e32_ret_num(baik, 0.0);
    return;
  }
  if (baik_is_string(v)) {
    char buf[160];
    e32_val_text(baik, v, buf, sizeof(buf));
    e32_ret_num(baik, (double) Serial.write((const uint8_t *) buf, strlen(buf)));
    return;
  }
  e32_ret_num(baik, (double) Serial.write((uint8_t) ((long) e32_val_num(baik, v) & 0xFF)));
}

static void bk_serial_flush(struct baik *baik) {
  Serial.flush();
  e32_ret_undef(baik);
}

/* ---------------------------------------------------------------------------
 * OBJECT ESP
 * ------------------------------------------------------------------------ */

static void bk_esp_restart(struct baik *baik) {
  printf("INFO memulai ulang papan...\n");
  Serial.flush();
  delay(50);
  ESP.restart();
  e32_ret_undef(baik);   /* tidak pernah tercapai */
}

static void bk_esp_free_heap(struct baik *baik)      { e32_ret_num(baik, (double) ESP.getFreeHeap()); }
static void bk_esp_min_free_heap(struct baik *baik)  { e32_ret_num(baik, (double) ESP.getMinFreeHeap()); }
static void bk_esp_heap_size(struct baik *baik)      { e32_ret_num(baik, (double) ESP.getHeapSize()); }
static void bk_esp_max_alloc_heap(struct baik *baik) { e32_ret_num(baik, (double) ESP.getMaxAllocHeap()); }

/* PSRAM: kembalikan 0 bila papan tidak punya PSRAM. */
static void bk_esp_psram_size(struct baik *baik) {
  e32_ret_num(baik, psramFound() ? (double) ESP.getPsramSize() : 0.0);
}

static void bk_esp_free_psram(struct baik *baik) {
  e32_ret_num(baik, psramFound() ? (double) ESP.getFreePsram() : 0.0);
}

static void bk_esp_chip_model(struct baik *baik)   { e32_ret_str(baik, ESP.getChipModel()); }
static void bk_esp_chip_revision(struct baik *baik){ e32_ret_num(baik, (double) ESP.getChipRevision()); }
static void bk_esp_chip_cores(struct baik *baik)   { e32_ret_num(baik, (double) ESP.getChipCores()); }
static void bk_esp_cpu_freq_mhz(struct baik *baik) { e32_ret_num(baik, (double) ESP.getCpuFreqMHz()); }
static void bk_esp_sdk_version(struct baik *baik)  { e32_ret_str(baik, ESP.getSdkVersion()); }
static void bk_esp_flash_size(struct baik *baik)   { e32_ret_num(baik, (double) ESP.getFlashChipSize()); }
static void bk_esp_flash_speed(struct baik *baik)  { e32_ret_num(baik, (double) ESP.getFlashChipSpeed()); }
static void bk_esp_sketch_size(struct baik *baik)  { e32_ret_num(baik, (double) ESP.getSketchSize()); }
static void bk_esp_free_sketch(struct baik *baik)  { e32_ret_num(baik, (double) ESP.getFreeSketchSpace()); }

/* MAC eFuse 48 bit -> muat persis di mantissa double (53 bit). */
static void bk_esp_efuse_mac(struct baik *baik) {
  e32_ret_num(baik, (double) ESP.getEfuseMac());
}

/* ---------------------------------------------------------------------------
 * FREKUENSI CPU
 * ------------------------------------------------------------------------ */

static void bk_set_cpu_freq_mhz(struct baik *baik) {
  int mhz = e32_arg_int(baik, 0, 0);
  if (mhz <= 0) {
    printf("GALAT setCpuFrequencyMhz(): frekuensi harus > 0 MHz\n");
    e32_ret_bool(baik, 0);
    return;
  }
  Serial.flush();
  if (!setCpuFrequencyMhz((uint32_t) mhz)) {
    printf("GALAT setCpuFrequencyMhz(): %d MHz tidak didukung papan ini.\n"
           "        Nilai lazim: 240, 160, 80, 40, 20, 10 MHz.\n", mhz);
    e32_ret_bool(baik, 0);
    return;
  }
  e32_ret_bool(baik, 1);
}

static void bk_get_cpu_freq_mhz(struct baik *baik)  { e32_ret_num(baik, (double) getCpuFrequencyMhz()); }
static void bk_get_xtal_freq_mhz(struct baik *baik) { e32_ret_num(baik, (double) getXtalFrequencyMhz()); }
static void bk_get_apb_freq(struct baik *baik)      { e32_ret_num(baik, (double) getApbFrequency()); }

/* ---------------------------------------------------------------------------
 * SENSOR INTERNAL
 * ------------------------------------------------------------------------ */

static void bk_temperature_read(struct baik *baik) {
  e32_ret_num(baik, (double) temperatureRead());
}

/* hallRead() TIDAK didaftarkan di sini.
 *
 * Sensor hall didaftarkan oleh e32_gpio.cpp (bersama alias `bacaHall`), yang
 * juga menyediakan cabang galat untuk papan/inti yang tidak mendukungnya.
 * Sebelumnya kedua modul mendaftarkan nama `hallRead`, dan karena registrasi
 * sistem berjalan sesudah gpio, versi di sinilah yang menang - sementara alias
 * `bacaHall` tetap menunjuk versi gpio. Tidak salah hasilnya, tapi
 * membingungkan. Satu nama, satu pemilik.
 */

/* ---------------------------------------------------------------------------
 * TIDUR (deep sleep / light sleep)
 * ------------------------------------------------------------------------ */

static void bk_sleep_enable_timer(struct baik *baik) {
  double us = e32_arg_num(baik, 0, 0.0);
  if (!(us > 0.0)) {
    printf("GALAT esp_sleep_enable_timer_wakeup(): mikrodetik harus > 0\n");
    e32_ret_num(baik, -1.0);
    return;
  }
  e32_ret_num(baik, (double) esp_sleep_enable_timer_wakeup((uint64_t) us));
}

static void bk_sleep_enable_ext0(struct baik *baik) {
#if E32_HAS_EXT_WAKEUP
  int pin = e32_arg_int(baik, 0, -1);
  int level = e32_arg_int(baik, 1, 1);
  if (!e32_pin_ok(baik, pin, E32_CAP_RTC, "esp_sleep_enable_ext0_wakeup")) {
    e32_ret_undef(baik);   /* aturan kontrak: galat pin -> takterdefinisi */
    return;
  }
  if (level != 0 && level != 1) {
    printf("GALAT esp_sleep_enable_ext0_wakeup(): level harus 0 (LOW) atau 1 (HIGH)\n");
    e32_ret_num(baik, -1.0);
    return;
  }
  e32_ret_num(baik, (double) esp_sleep_enable_ext0_wakeup((gpio_num_t) pin, level));
#else
  printf("GALAT esp_sleep_enable_ext0_wakeup(): tidak tersedia pada %s\n",
         e32_board_chip());
  e32_ret_num(baik, -1.0);
#endif
}

static void bk_sleep_enable_ext1(struct baik *baik) {
#if E32_HAS_EXT_WAKEUP
  double dmask = e32_arg_num(baik, 0, 0.0);
  int mode = e32_arg_int(baik, 1, 1);   /* 0 = semua LOW, 1 = salah satu HIGH */
  uint64_t mask;
  int gpio;

  if (!(dmask > 0.0)) {
    printf("GALAT esp_sleep_enable_ext1_wakeup(): mask pin tidak boleh 0.\n"
           "        Contoh: esp_sleep_enable_ext1_wakeup(pow(2,33) + pow(2,32), 1)\n");
    e32_ret_num(baik, -1.0);
    return;
  }
  if (mode != 0 && mode != 1) {
    printf("GALAT esp_sleep_enable_ext1_wakeup(): mode harus 0 (WAKEUP_ALL_LOW)"
           " atau 1 (WAKEUP_ANY_HIGH)\n");
    e32_ret_num(baik, -1.0);
    return;
  }
  mask = (uint64_t) dmask;
  if (E32_GPIO_MAX < 63 && (mask >> (E32_GPIO_MAX + 1)) != 0ULL) {
    printf("GALAT esp_sleep_enable_ext1_wakeup(): mask memuat pin di atas"
           " GPIO%d, yang tidak ada pada %s\n", E32_GPIO_MAX, e32_board_chip());
    e32_ret_undef(baik);
    return;
  }
  for (gpio = 0; gpio <= E32_GPIO_MAX; gpio++) {
    if ((mask >> gpio) & 1ULL) {
      if (!e32_pin_ok(baik, gpio, E32_CAP_RTC, "esp_sleep_enable_ext1_wakeup")) {
        e32_ret_undef(baik);   /* aturan kontrak: galat pin -> takterdefinisi */
        return;
      }
    }
  }
  /* Nama konstanta enum berbeda antar versi IDF (ALL_LOW vs ANY_LOW),
   * jadi kita kirim angkanya langsung. */
  e32_ret_num(baik, (double) esp_sleep_enable_ext1_wakeup(
                        mask, (esp_sleep_ext1_wakeup_mode_t) mode));
#else
  printf("GALAT esp_sleep_enable_ext1_wakeup(): tidak tersedia pada %s\n",
         e32_board_chip());
  e32_ret_num(baik, -1.0);
#endif
}

static void bk_sleep_enable_touchpad(struct baik *baik) {
#if E32_HAS_TOUCH_WAKEUP
  e32_ret_num(baik, (double) esp_sleep_enable_touchpad_wakeup());
#else
  printf("GALAT esp_sleep_enable_touchpad_wakeup(): sensor sentuh tidak"
         " tersedia pada %s\n", e32_board_chip());
  e32_ret_num(baik, -1.0);
#endif
}

static void bk_sleep_disable_source(struct baik *baik) {
  int src = e32_arg_int(baik, 0, (int) ESP_SLEEP_WAKEUP_ALL);
  e32_ret_num(baik, (double) esp_sleep_disable_wakeup_source(
                        (esp_sleep_source_t) src));
}

static void bk_deep_sleep_start(struct baik *baik) {
  printf("INFO masuk deep sleep...\n");
  Serial.flush();
  delay(20);
  esp_deep_sleep_start();          /* tidak pernah kembali */
  e32_ret_undef(baik);
}

static void bk_light_sleep_start(struct baik *baik) {
  esp_err_t err;
  Serial.flush();
  err = esp_light_sleep_start();
  e32_ret_num(baik, (double) err);
}

/* tidurDalam(us) = pasang timer lalu deep sleep. */
static void bk_tidur_dalam(struct baik *baik) {
  double us = e32_arg_num(baik, 0, 0.0);
  if (us > 0.0) esp_sleep_enable_timer_wakeup((uint64_t) us);
  printf("INFO deep sleep %.0f us...\n", us);
  Serial.flush();
  delay(20);
  esp_deep_sleep_start();
  e32_ret_undef(baik);
}

/* tidurRingan(us) = pasang timer lalu light sleep; kembali setelah bangun. */
static void bk_tidur_ringan(struct baik *baik) {
  double us = e32_arg_num(baik, 0, 0.0);
  esp_err_t err;
  if (us > 0.0) esp_sleep_enable_timer_wakeup((uint64_t) us);
  Serial.flush();
  err = esp_light_sleep_start();
  e32_ret_num(baik, (double) err);
}

/* Kode mentah (sesuai ESP-IDF). */
static void bk_sleep_get_wakeup_cause(struct baik *baik) {
  e32_ret_num(baik, (double) esp_sleep_get_wakeup_cause());
}

static void bk_reset_reason_code(struct baik *baik) {
  e32_ret_num(baik, (double) esp_reset_reason());
}

/* sebabBangun() -> penjelasan bahasa Indonesia. */
static const char *e32_wakeup_text(esp_sleep_wakeup_cause_t c) {
  switch (c) {
    case ESP_SLEEP_WAKEUP_UNDEFINED:
      return "UNDEFINED (bukan bangun dari sleep, ini boot biasa)";
    case ESP_SLEEP_WAKEUP_ALL:
      return "ALL (semua sumber bangun diaktifkan)";
    case ESP_SLEEP_WAKEUP_EXT0:
      return "EXT0 (dibangunkan oleh satu pin RTC)";
    case ESP_SLEEP_WAKEUP_EXT1:
      return "EXT1 (dibangunkan oleh sekelompok pin RTC)";
    case ESP_SLEEP_WAKEUP_TIMER:
      return "TIMER (dibangunkan oleh timer RTC)";
    case ESP_SLEEP_WAKEUP_TOUCHPAD:
      return "TOUCH (dibangunkan oleh sensor sentuh)";
    case ESP_SLEEP_WAKEUP_ULP:
      return "ULP (dibangunkan oleh koprosesor ULP)";
    case ESP_SLEEP_WAKEUP_GPIO:
      return "GPIO (dibangunkan oleh GPIO, khusus light sleep)";
    case ESP_SLEEP_WAKEUP_UART:
      return "UART (dibangunkan oleh data masuk UART)";
    default:
      return "LAINNYA (sumber bangun tidak dikenali)";
  }
}

static void bk_sebab_bangun(struct baik *baik) {
  e32_ret_str(baik, e32_wakeup_text(esp_sleep_get_wakeup_cause()));
}

/* sebabReset() -> penjelasan bahasa Indonesia. */
static const char *e32_reset_text(esp_reset_reason_t r) {
  switch (r) {
    case ESP_RST_POWERON:
      return "POWERON (papan baru dinyalakan)";
    case ESP_RST_EXT:
      return "EXT (reset dari pin EN/RST luar)";
    case ESP_RST_SW:
      return "SW (reset perangkat lunak, mis. ESP.restart())";
    case ESP_RST_PANIC:
      return "PANIC (program kacau / exception, papan di-reset)";
    case ESP_RST_INT_WDT:
      return "INT_WDT (watchdog interupsi menyala)";
    case ESP_RST_TASK_WDT:
      return "TASK_WDT (watchdog task menyala, ada task yang macet)";
    case ESP_RST_WDT:
      return "WDT (watchdog lain menyala)";
    case ESP_RST_DEEPSLEEP:
      return "DEEPSLEEP (bangun dari deep sleep)";
    case ESP_RST_BROWNOUT:
      return "BROWNOUT (tegangan catu daya turun, listrik kurang kuat)";
    case ESP_RST_SDIO:
      return "SDIO (reset lewat antarmuka SDIO)";
    case ESP_RST_UNKNOWN:
    default:
      return "UNKNOWN (sebab reset tidak diketahui)";
  }
}

static void bk_sebab_reset(struct baik *baik) {
  e32_ret_str(baik, e32_reset_text(esp_reset_reason()));
}

/* ---------------------------------------------------------------------------
 * PENYIMPANAN RTC SLOW MEMORY (bertahan melewati deep sleep)
 * ------------------------------------------------------------------------ */

#define E32_RTC_SLOTS    16
#define E32_RTC_NAME_MAX 16
#define E32_RTC_MAGIC    0x42414931u   /* 'BAI1' */

static RTC_DATA_ATTR uint32_t e32_rtc_magic = 0;
static RTC_DATA_ATTR int32_t  e32_rtc_used = 0;
static RTC_DATA_ATTR char     e32_rtc_name[E32_RTC_SLOTS][E32_RTC_NAME_MAX] = {{0}};
static RTC_DATA_ATTR double   e32_rtc_value[E32_RTC_SLOTS] = {0};

static void e32_rtc_init(void) {
  if (e32_rtc_magic != E32_RTC_MAGIC) {
    memset(e32_rtc_name, 0, sizeof(e32_rtc_name));
    memset(e32_rtc_value, 0, sizeof(e32_rtc_value));
    e32_rtc_used = 0;
    e32_rtc_magic = E32_RTC_MAGIC;
  }
  if (e32_rtc_used < 0 || e32_rtc_used > E32_RTC_SLOTS) e32_rtc_used = 0;
}

static int e32_rtc_find(const char *name) {
  int i;
  for (i = 0; i < e32_rtc_used; i++) {
    if (strncmp(e32_rtc_name[i], name, E32_RTC_NAME_MAX - 1) == 0) return i;
  }
  return -1;
}

static void bk_rtc_set(struct baik *baik) {
  const char *name = e32_arg_str(baik, 0, NULL);
  double val = e32_arg_num(baik, 1, 0.0);
  int idx;

  e32_rtc_init();
  if (name == NULL || name[0] == '\0') {
    printf("GALAT rtcSet(): argumen pertama harus nama slot berupa teks\n");
    e32_ret_bool(baik, 0);
    return;
  }
  if (strlen(name) > (size_t) (E32_RTC_NAME_MAX - 1)) {
    printf("GALAT rtcSet(): nama '%s' terlalu panjang (maks %d huruf)\n",
           name, E32_RTC_NAME_MAX - 1);
    e32_ret_bool(baik, 0);
    return;
  }
  idx = e32_rtc_find(name);
  if (idx < 0) {
    if (e32_rtc_used >= E32_RTC_SLOTS) {
      printf("GALAT rtcSet(): slot RTC penuh (maks %d slot). Pakai ulang nama"
             " yang sudah ada.\n", E32_RTC_SLOTS);
      e32_ret_bool(baik, 0);
      return;
    }
    idx = e32_rtc_used++;
    memset(e32_rtc_name[idx], 0, E32_RTC_NAME_MAX);
    memcpy(e32_rtc_name[idx], name, strlen(name));
  }
  e32_rtc_value[idx] = val;
  e32_ret_bool(baik, 1);
}

static void bk_rtc_get(struct baik *baik) {
  const char *name = e32_arg_str(baik, 0, NULL);
  int idx;

  e32_rtc_init();
  if (name == NULL || name[0] == '\0') {
    printf("GALAT rtcGet(): argumen pertama harus nama slot berupa teks\n");
    e32_ret_undef(baik);
    return;
  }
  idx = e32_rtc_find(name);
  if (idx < 0) {
    if (baik_nargs(baik) >= 2) {
      e32_ret_num(baik, e32_arg_num(baik, 1, 0.0));   /* nilai bawaan */
    } else {
      e32_ret_undef(baik);
    }
    return;
  }
  e32_ret_num(baik, e32_rtc_value[idx]);
}

/* ---------------------------------------------------------------------------
 * WATCHDOG TASK
 * ------------------------------------------------------------------------ */

static int e32_wdt_subscribed = 0;

static void bk_watchdog_enable(struct baik *baik) {
  int ms = e32_arg_int(baik, 0, 5000);
  esp_err_t err;

  if (ms < 100) {
    printf("GALAT watchdogEnable(): waktu terlalu pendek (minimal 100 ms)\n");
    e32_ret_bool(baik, 0);
    return;
  }

#if ESP_IDF_VERSION_MAJOR >= 5
  {
    esp_task_wdt_config_t cfg;
    memset(&cfg, 0, sizeof(cfg));
    cfg.timeout_ms = (uint32_t) ms;
    cfg.idle_core_mask = 0;
    cfg.trigger_panic = true;
    err = esp_task_wdt_init(&cfg);
    if (err == ESP_ERR_INVALID_STATE) {
      /* Arduino 3.x sudah menyalakan task WDT saat boot -> cukup atur ulang. */
      err = esp_task_wdt_reconfigure(&cfg);
    }
  }
#else
  err = esp_task_wdt_init((uint32_t) ((ms + 999) / 1000), true);
#endif

  if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) {
    printf("GALAT watchdogEnable(): gagal menyiapkan watchdog (kode %d)\n",
           (int) err);
    e32_ret_bool(baik, 0);
    return;
  }

  err = esp_task_wdt_add(NULL);     /* awasi task konsol BAIK */
  if (err != ESP_OK && err != ESP_ERR_INVALID_ARG) {
    printf("GALAT watchdogEnable(): gagal mendaftarkan task (kode %d)\n",
           (int) err);
    e32_ret_bool(baik, 0);
    return;
  }
  e32_wdt_subscribed = 1;
  e32_ret_bool(baik, 1);
}

static void bk_watchdog_reset(struct baik *baik) {
  if (!e32_wdt_subscribed) {
    printf("GALAT watchdogReset(): watchdog belum dinyalakan."
           " Panggil watchdogEnable(ms) dulu.\n");
    e32_ret_bool(baik, 0);
    return;
  }
  e32_ret_bool(baik, esp_task_wdt_reset() == ESP_OK);
}

static void bk_watchdog_disable(struct baik *baik) {
  esp_err_t err = esp_task_wdt_delete(NULL);
  e32_wdt_subscribed = 0;
  e32_ret_bool(baik, (err == ESP_OK || err == ESP_ERR_NOT_FOUND));
}

/* ---------------------------------------------------------------------------
 * INFO FreeRTOS
 * ------------------------------------------------------------------------ */

static void bk_get_task_count(struct baik *baik) {
  e32_ret_num(baik, (double) uxTaskGetNumberOfTasks());
}

/* Sisa ruang tumpukan (stack) terkecil task ini, dalam byte. */
static void bk_get_task_high_water_mark(struct baik *baik) {
  e32_ret_num(baik,
              (double) ((uint32_t) uxTaskGetStackHighWaterMark(NULL) *
                        (uint32_t) sizeof(StackType_t)));
}

/* ---------------------------------------------------------------------------
 * REGISTRASI
 * ------------------------------------------------------------------------ */

void baik_esp32_register_sistem(struct baik *baik, baik_val_t g) {
  baik_val_t serial_obj;
  baik_val_t esp_obj;

  e32_rtc_init();

  /* ---- waktu ---- */
  e32_fn(baik, g, "delay", bk_delay);
  e32_fn(baik, g, "tunggu", bk_delay);
  e32_fn(baik, g, "delayMicroseconds", bk_delay_microseconds);
  e32_fn(baik, g, "tungguMikro", bk_delay_microseconds);
  e32_fn(baik, g, "millis", bk_millis);
  e32_fn(baik, g, "milidetik", bk_millis);
  e32_fn(baik, g, "micros", bk_micros);
  e32_fn(baik, g, "mikrodetik", bk_micros);
  e32_fn(baik, g, "yield", bk_yield);

  /* ---- acak ---- */
  e32_fn(baik, g, "random", bk_random);
  e32_fn(baik, g, "acak", bk_random);
  e32_fn(baik, g, "randomSeed", bk_random_seed);

  /* ---- matematika ---- */
  e32_fn(baik, g, "map", bk_map);
  e32_fn(baik, g, "peta", bk_map);
  e32_fn(baik, g, "constrain", bk_constrain);
  e32_fn(baik, g, "batas", bk_constrain);
  e32_fn(baik, g, "min", bk_min);
  e32_fn(baik, g, "max", bk_max);
  e32_fn(baik, g, "abs", bk_abs);
  e32_fn(baik, g, "pow", bk_pow);
  e32_fn(baik, g, "sqrt", bk_sqrt);
  e32_fn(baik, g, "sin", bk_sin);
  e32_fn(baik, g, "cos", bk_cos);
  e32_fn(baik, g, "tan", bk_tan);
  e32_fn(baik, g, "atan", bk_atan);
  e32_fn(baik, g, "atan2", bk_atan2);
  e32_fn(baik, g, "floor", bk_floor);
  e32_fn(baik, g, "ceil", bk_ceil);
  e32_fn(baik, g, "round", bk_round);
  e32_fn(baik, g, "log", bk_log);
  e32_fn(baik, g, "log10", bk_log10);
  e32_fn(baik, g, "exp", bk_exp);

  /* ---- object Serial ---- */
  serial_obj = e32_ns(baik, g, "Serial");
  e32_fn(baik, serial_obj, "begin", bk_serial_begin);
  e32_fn(baik, serial_obj, "end", bk_serial_end);
  e32_fn(baik, serial_obj, "print", bk_serial_print);
  e32_fn(baik, serial_obj, "println", bk_serial_println);
  e32_fn(baik, serial_obj, "printf", bk_serial_printf);
  e32_fn(baik, serial_obj, "available", bk_serial_available);
  e32_fn(baik, serial_obj, "read", bk_serial_read);
  e32_fn(baik, serial_obj, "readString", bk_serial_read_string);
  e32_fn(baik, serial_obj, "write", bk_serial_write);
  e32_fn(baik, serial_obj, "flush", bk_serial_flush);

  /* ---- object ESP ---- */
  esp_obj = e32_ns(baik, g, "ESP");
  e32_fn(baik, esp_obj, "restart", bk_esp_restart);
  e32_fn(baik, esp_obj, "getFreeHeap", bk_esp_free_heap);
  e32_fn(baik, esp_obj, "getMinFreeHeap", bk_esp_min_free_heap);
  e32_fn(baik, esp_obj, "getHeapSize", bk_esp_heap_size);
  e32_fn(baik, esp_obj, "getMaxAllocHeap", bk_esp_max_alloc_heap);
  e32_fn(baik, esp_obj, "getPsramSize", bk_esp_psram_size);
  e32_fn(baik, esp_obj, "getFreePsram", bk_esp_free_psram);
  e32_fn(baik, esp_obj, "getChipModel", bk_esp_chip_model);
  e32_fn(baik, esp_obj, "getChipRevision", bk_esp_chip_revision);
  e32_fn(baik, esp_obj, "getChipCores", bk_esp_chip_cores);
  e32_fn(baik, esp_obj, "getCpuFreqMHz", bk_esp_cpu_freq_mhz);
  e32_fn(baik, esp_obj, "getSdkVersion", bk_esp_sdk_version);
  e32_fn(baik, esp_obj, "getFlashChipSize", bk_esp_flash_size);
  e32_fn(baik, esp_obj, "getFlashChipSpeed", bk_esp_flash_speed);
  e32_fn(baik, esp_obj, "getSketchSize", bk_esp_sketch_size);
  e32_fn(baik, esp_obj, "getFreeSketchSpace", bk_esp_free_sketch);
  e32_fn(baik, esp_obj, "getEfuseMac", bk_esp_efuse_mac);

  /* Alias global yang sering dipakai. */
  e32_fn(baik, g, "mulaiUlang", bk_esp_restart);

  /* ---- frekuensi CPU ---- */
  e32_fn(baik, g, "setCpuFrequencyMhz", bk_set_cpu_freq_mhz);
  e32_fn(baik, g, "getCpuFrequencyMhz", bk_get_cpu_freq_mhz);
  e32_fn(baik, g, "getXtalFrequencyMhz", bk_get_xtal_freq_mhz);
  e32_fn(baik, g, "getApbFrequency", bk_get_apb_freq);

  /* ---- sensor internal ---- */
  e32_fn(baik, g, "temperatureRead", bk_temperature_read);
  e32_fn(baik, g, "bacaSuhu", bk_temperature_read);

  /* ---- tidur ---- */
  e32_fn(baik, g, "esp_sleep_enable_timer_wakeup", bk_sleep_enable_timer);
  e32_fn(baik, g, "sleepTimer", bk_sleep_enable_timer);
  e32_fn(baik, g, "esp_sleep_enable_ext0_wakeup", bk_sleep_enable_ext0);
  e32_fn(baik, g, "sleepExt0", bk_sleep_enable_ext0);
  e32_fn(baik, g, "esp_sleep_enable_ext1_wakeup", bk_sleep_enable_ext1);
  e32_fn(baik, g, "sleepExt1", bk_sleep_enable_ext1);
  e32_fn(baik, g, "esp_sleep_enable_touchpad_wakeup", bk_sleep_enable_touchpad);
  e32_fn(baik, g, "esp_sleep_disable_wakeup_source", bk_sleep_disable_source);
  e32_fn(baik, g, "esp_deep_sleep_start", bk_deep_sleep_start);
  e32_fn(baik, g, "esp_light_sleep_start", bk_light_sleep_start);
  e32_fn(baik, g, "tidurDalam", bk_tidur_dalam);
  e32_fn(baik, g, "tidurRingan", bk_tidur_ringan);
  e32_fn(baik, g, "esp_sleep_get_wakeup_cause", bk_sleep_get_wakeup_cause);
  e32_fn(baik, g, "esp_reset_reason", bk_reset_reason_code);
  e32_fn(baik, g, "sebabBangun", bk_sebab_bangun);
  e32_fn(baik, g, "sebabReset", bk_sebab_reset);

  /* ---- RTC slow memory ---- */
  e32_fn(baik, g, "rtcSet", bk_rtc_set);
  e32_fn(baik, g, "rtcGet", bk_rtc_get);

  /* ---- watchdog ---- */
  e32_fn(baik, g, "watchdogEnable", bk_watchdog_enable);
  e32_fn(baik, g, "watchdogReset", bk_watchdog_reset);
  e32_fn(baik, g, "watchdogDisable", bk_watchdog_disable);

  /* ---- info FreeRTOS ---- */
  e32_fn(baik, g, "getTaskCount", bk_get_task_count);
  e32_fn(baik, g, "getTaskHighWaterMark", bk_get_task_high_water_mark);
}
