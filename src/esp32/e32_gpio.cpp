/*
 * e32_gpio.cpp - Modul 2 BAIK-ESP32: GPIO / ANALOG / PWM / INTERUPSI.
 *
 * Mendaftarkan seluruh fungsi pada tabel "Modul 2" kontrak API (lihat
 * scratchpad/API-SPEC.md) ke object global interpreter BAIK, lengkap dengan
 * alias bahasa Indonesianya.
 *
 * Tiga hal yang membuat berkas ini agak panjang:
 *
 *  1. Kompatibilitas arduino-esp32 2.x vs 3.x.
 *     Di 3.x, ledcSetup()/ledcAttachPin()/ledcDetachPin() DIHAPUS dan diganti
 *     ledcAttach(pin,freq,res) / ledcAttachChannel(pin,freq,res,kanal) /
 *     ledcWrite(pin,duty) / ledcDetach(pin).  Skrip BAIK tetap boleh memakai
 *     KEDUA gaya: berkas ini menyimpan tabel pemetaan kanal<->pin sendiri dan
 *     mengemulasikan gaya yang tidak tersedia pada versi inti yang dipakai.
 *
 *  2. Kompatibilitas ESP32 vs ESP32-S3.
 *     S3 tidak punya DAC maupun sensor Hall, dan jumlah kanal LEDC-nya 8
 *     (ESP32: 16).  Fungsi yang tidak didukung TETAP terdaftar supaya skrip
 *     tidak "ReferenceError", tetapi mencetak galat berbahasa Indonesia dan
 *     mengembalikan `takterdefinisi`.
 *
 *  3. Interupsi.
 *     Interpreter BAIK TIDAK reentrant dan HARAM dipanggil dari ISR.  Karena
 *     itu ISR di sini hanya menaikkan pencacah `volatile uint32_t`; callback
 *     BAIK dijalankan belakangan dari task konsol lewat
 *     baik_esp32_poll_interrupts() / serviceInterrupts().
 */

#include <Arduino.h>

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "soc/soc_caps.h"
#include "driver/gpio.h"

#include "esp32-hal-gpio.h"
#include "esp32-hal-adc.h"
#include "esp32-hal-ledc.h"
#include "esp32-hal-touch.h"
#include "esp32-hal-dac.h"

#if defined(__has_include)
#if __has_include(<esp_arduino_version.h>)
#include <esp_arduino_version.h>
#endif
#endif

#include "baik_esp32.h"

/* ========================================================================
 * 0. Makro kemampuan chip & versi inti Arduino
 * ===================================================================== */

#ifndef ESP_ARDUINO_VERSION_MAJOR
/* Inti yang sangat lawas tidak punya esp_arduino_version.h -> anggap 2.x. */
#define ESP_ARDUINO_VERSION_MAJOR 2
#endif

#if ESP_ARDUINO_VERSION_MAJOR >= 3
#define E32_ARDUINO3 1
#else
#define E32_ARDUINO3 0
#endif

/* --- DAC ---------------------------------------------------------------- */
#if defined(SOC_DAC_SUPPORTED) && SOC_DAC_SUPPORTED
#define E32_HAS_DAC 1
#elif defined(SOC_DAC_PERIPH_NUM) && (SOC_DAC_PERIPH_NUM > 0)
#define E32_HAS_DAC 1
#elif defined(CONFIG_IDF_TARGET_ESP32) || defined(CONFIG_IDF_TARGET_ESP32S2)
#define E32_HAS_DAC 1
#else
#define E32_HAS_DAC 0
#endif

/* --- Sensor sentuh ------------------------------------------------------ */
#if defined(SOC_TOUCH_SENSOR_SUPPORTED) && SOC_TOUCH_SENSOR_SUPPORTED
#define E32_HAS_TOUCH 1
#elif defined(SOC_TOUCH_SENSOR_NUM) && (SOC_TOUCH_SENSOR_NUM > 0)
#define E32_HAS_TOUCH 1
#elif defined(CONFIG_IDF_TARGET_ESP32) || defined(CONFIG_IDF_TARGET_ESP32S2) || \
    defined(CONFIG_IDF_TARGET_ESP32S3)
#define E32_HAS_TOUCH 1
#else
#define E32_HAS_TOUCH 0
#endif

/* --- Driver LED RGB pintar (WS2812) -------------------------------------
 * neopixelWrite() hadir sejak arduino-esp32 2.0.7 dan tetap ada di 3.x (di
 * 3.x namanya rgbLedWrite(), dengan neopixelWrite tetap tersedia).  Ia butuh
 * periferal RMT.  Uji keberadaan headernya, itu paling jujur. */
#if defined(__has_include)
#if __has_include(<esp32-hal-rgb-led.h>)
#include <esp32-hal-rgb-led.h>
#if !defined(SOC_RMT_SUPPORTED) || SOC_RMT_SUPPORTED
#define E32_HAS_RGBLED 1
#endif
#endif
#endif
#ifndef E32_HAS_RGBLED
#define E32_HAS_RGBLED 0
#endif

/* Papan ini memakai LED RGB pintar sebagai LED bawaan? */
#if E32_HAS_RGBLED && defined(RGB_BUILTIN)
#define E32_LED_IS_RGB 1
#else
#define E32_LED_IS_RGB 0
#endif

/* Terang bawaan untuk ledBawaan() pada papan berLED RGB: putih redup supaya
 * tidak menyilaukan dan tidak boros arus. */
#define E32_LED_RGB_DIM 16

/* --- Sensor Hall (hanya ESP32 klasik, dan dihapus di inti 3.x) ---------- */
#if (!E32_ARDUINO3) && defined(CONFIG_IDF_TARGET_ESP32)
#define E32_HAS_HALL 1
#else
#define E32_HAS_HALL 0
#endif

/* --- Jumlah kanal LEDC --------------------------------------------------
 * soc_caps.h menghitung kanal PER grup kecepatan.  ESP32 punya grup
 * high-speed + low-speed (8 + 8 = 16 kanal), S3/C3 hanya low-speed (8). */
#ifndef SOC_LEDC_CHANNEL_NUM
#define SOC_LEDC_CHANNEL_NUM 8
#endif
#if defined(SOC_LEDC_SUPPORT_HS_MODE) && SOC_LEDC_SUPPORT_HS_MODE
#define E32_LEDC_CHANNELS (SOC_LEDC_CHANNEL_NUM * 2)
#else
#define E32_LEDC_CHANNELS (SOC_LEDC_CHANNEL_NUM)
#endif

/* --- Jumlah pin ---------------------------------------------------------- */
#ifndef SOC_GPIO_PIN_COUNT
#define SOC_GPIO_PIN_COUNT 49
#endif
#define E32_MAX_PINS (SOC_GPIO_PIN_COUNT)

/* Nilai bawaan PWM bila pengguna langsung analogWrite() tanpa setup. */
#define E32_PWM_DEF_FREQ 5000u
#define E32_PWM_DEF_RES  8

/* Batas jumlah callback interupsi yang dijalankan per pin per satu kali
 * poll, supaya sinyal yang memantul (bouncing) tidak membanjiri REPL. */
#define E32_ISR_BURST 8

/* Konstanta mode pin Arduino, ditulis ulang supaya tidak bergantung pada
 * makro inti yang bisa berbeda antar versi. */
#define E32_MODE_INPUT  0x01
#define E32_MODE_OUTPUT 0x02
#define E32_MODE_ANALOG 0xC0

/* ========================================================================
 * 1. Utilitas kecil
 * ===================================================================== */

/* Cetak pesan galat berbahasa Indonesia dengan awalan seragam. */
static void e32_err(const char *fname, const char *fmt, ...)
    __attribute__((format(printf, 2, 3)));

static void e32_err(const char *fname, const char *fmt, ...) {
  va_list ap;
  printf("[BAIK] galat %s(): ", fname);
  va_start(ap, fmt);
  vprintf(fmt, ap);
  va_end(ap);
  printf("\n");
}

static int e32_pin_in_range(int pin) {
  return pin >= 0 && pin < E32_MAX_PINS;
}

/* Validasi pin ADC.
 *
 * e32_pin_has() menuntut SEMUA bit pada mask terpenuhi, sedangkan sebuah pin
 * hanya punya SALAH SATU dari ADC1/ADC2.  Jadi kita pilih dulu bit yang cocok,
 * baru serahkan ke e32_pin_ok() supaya pesan galatnya tetap seragam dengan
 * modul lain. */
static int e32_adc_pin_ok(struct baik *baik, int pin, const char *fname) {
  uint32_t want = E32_CAP_ADC1;
  if (e32_pin_has(pin, E32_CAP_ADC2) && !e32_pin_has(pin, E32_CAP_ADC1)) {
    want = E32_CAP_ADC2;
  }
  return e32_pin_ok(baik, pin, want, fname);
}

/* Pastikan sebuah nilai boleh dipanggil dari baik_apply().
 *
 * PENJAGAAN INI WAJIB ADA, JANGAN DIHAPUS.  Lihat src/baik.c:7941-7951:
 * seluruh cabang untuk fungsi foreign di dalam baik_apply() masih
 * dikomentari, sehingga baik_apply() SELALU menganggap argumennya fungsi
 * bytecode dan langsung melompat ke baik_get_func_addr(func).  Bila nilai
 * yang dikirim ternyata fungsi NATIVE (dan semua fungsi ESP32 di berkas ini
 * native alias foreign!), alamat yang dihasilkan adalah sampah dan
 * baik_execute() akan melompat ke situ -> papan reset tanpa pesan apa pun.
 *
 * Jadi: hanya baik_is_function() (fungsi bytecode, yaitu fungsi yang ditulis
 * dengan kata kunci `fungsi` di skrip) yang boleh lolos.  Perhatikan
 * baik_is_foreign() bernilai BENAR untuk fungsi bawaan — justru itulah kasus
 * berbahaya yang harus ditolak. */
static int e32_cb_ok(baik_val_t cb, const char *fname, const char *contoh) {
  if (baik_is_function(cb)) return 1;
  if (baik_is_foreign(cb)) {
    e32_err(fname,
            "callback harus fungsi BAIK, bukan fungsi bawaan. "
            "Bungkus begini: %s",
            contoh);
  } else {
    e32_err(fname, "callback harus berupa fungsi BAIK, bukan %s. Contoh: %s",
            baik_typeof(cb), contoh);
  }
  return 0;
}

/* Ambil argumen level logika: menerima benar/salah maupun angka HIGH/LOW. */
static int e32_arg_level(struct baik *baik, int n, int def) {
  baik_val_t v = baik_arg(baik, n);
  if (baik_is_boolean(v)) return baik_get_bool(baik, v) ? 1 : 0;
  return e32_arg_int(baik, n, def) ? 1 : 0;
}

/* ========================================================================
 * 2. Tabel PWM/LEDC: jembatan gaya 2.x (kanal) <-> gaya 3.x (pin)
 * ===================================================================== */

typedef struct e32_ledc_chan {
  uint8_t  used;      /* 1 bila kanal pernah di-ledcSetup / dialokasikan    */
  uint8_t  autoalloc; /* 1 bila kanal dialokasikan otomatis oleh ledcAttach */
  int16_t  pin;       /* pin yang sedang terpasang, -1 bila belum           */
  uint32_t freq;      /* frekuensi (Hz)                                     */
  uint8_t  res;       /* resolusi (bit)                                     */
} e32_ledc_chan_t;

static e32_ledc_chan_t s_ch[E32_LEDC_CHANNELS];

static int8_t   s_pin_ch[E32_MAX_PINS];       /* pin -> kanal, -1 = tidak tahu */
static uint8_t  s_pin_attached[E32_MAX_PINS]; /* 1 bila pin terpasang ke LEDC  */
static uint8_t  s_pin_cfg[E32_MAX_PINS];      /* 1 bila freq/res pin diatur    */
static uint32_t s_pin_freq[E32_MAX_PINS];
static uint8_t  s_pin_res[E32_MAX_PINS];

static uint32_t s_def_freq = E32_PWM_DEF_FREQ;
static uint8_t  s_def_res  = E32_PWM_DEF_RES;

static int s_pwm_ready = 0;

static void e32_pwm_init(void) {
  int i;
  if (s_pwm_ready) return;
  for (i = 0; i < E32_LEDC_CHANNELS; i++) {
    s_ch[i].used = 0;
    s_ch[i].autoalloc = 0;
    s_ch[i].pin = -1;
    s_ch[i].freq = E32_PWM_DEF_FREQ;
    s_ch[i].res = E32_PWM_DEF_RES;
  }
  for (i = 0; i < E32_MAX_PINS; i++) {
    s_pin_ch[i] = -1;
    s_pin_attached[i] = 0;
    s_pin_cfg[i] = 0;
    s_pin_freq[i] = E32_PWM_DEF_FREQ;
    s_pin_res[i] = E32_PWM_DEF_RES;
  }
  s_pwm_ready = 1;
}

/* Cari kanal bebas.  Kanal genap didahulukan: pada inti 2.x dua kanal
 * bersebelahan berbagi satu timer, jadi memakai 0,2,4,... membuat tiap pin
 * bisa punya frekuensi sendiri. */
static int e32_ledc_alloc_chan(void) {
  int i;
  for (i = 0; i < E32_LEDC_CHANNELS; i += 2) {
    if (!s_ch[i].used) return i;
  }
  for (i = 1; i < E32_LEDC_CHANNELS; i += 2) {
    if (!s_ch[i].used) return i;
  }
  return -1;
}

/* Pasang `pin` ke LEDC.  `chan` < 0 berarti pilih kanal otomatis.
 * Balik nomor kanal (>= 0) bila sukses, -1 bila gagal.
 * Catatan: pada inti 3.x nomor kanal untuk mode otomatis tidak diketahui
 * inti Arduino, jadi kita tetap mencatat kanal versi kita sendiri agar
 * ledcRead(kanal) gaya 2.x tetap masuk akal. */
static int e32_ledc_attach(int pin, uint32_t freq, uint8_t res, int chan) {
  int autoalloc = 0;

  e32_pwm_init();
  if (!e32_pin_in_range(pin)) return -1;
  if (res < 1) res = 1;
  if (res > 20) res = 20;
  if (freq == 0) freq = E32_PWM_DEF_FREQ;

  if (chan < 0) {
    chan = e32_ledc_alloc_chan();
    autoalloc = 1;
    if (chan < 0) return -1;
  }
  if (chan >= E32_LEDC_CHANNELS) return -1;

#if E32_ARDUINO3
  if (!ledcAttachChannel((uint8_t) pin, (uint32_t) freq, (uint8_t) res,
                         (uint8_t) chan)) {
    return -1;
  }
#else
  if (ledcSetup((uint8_t) chan, (double) freq, (uint8_t) res) == 0) {
    /* ledcSetup() balik 0 bila kombinasi freq/resolusi mustahil. */
    return -1;
  }
  ledcAttachPin((uint8_t) pin, (uint8_t) chan);
#endif

  s_ch[chan].used = 1;
  s_ch[chan].autoalloc = (uint8_t) autoalloc;
  s_ch[chan].pin = (int16_t) pin;
  s_ch[chan].freq = freq;
  s_ch[chan].res = res;

  s_pin_ch[pin] = (int8_t) chan;
  s_pin_attached[pin] = 1;
  s_pin_cfg[pin] = 1;
  s_pin_freq[pin] = freq;
  s_pin_res[pin] = res;
  return chan;
}

static void e32_ledc_detach(int pin) {
  int ch;
  e32_pwm_init();
  if (!e32_pin_in_range(pin)) return;

#if E32_ARDUINO3
  ledcDetach((uint8_t) pin);
#else
  ledcDetachPin((uint8_t) pin);
#endif

  ch = s_pin_ch[pin];
  if (ch >= 0 && ch < E32_LEDC_CHANNELS && s_ch[ch].pin == (int16_t) pin) {
    s_ch[ch].pin = -1;
    /* Kanal yang dikonfigurasi manual lewat ledcSetup() tetap dianggap
     * "terpakai" (seperti perilaku inti 2.x); kanal hasil alokasi otomatis
     * dikembalikan ke kolam bebas. */
    if (s_ch[ch].autoalloc) {
      s_ch[ch].used = 0;
      s_ch[ch].autoalloc = 0;
    }
  }
  s_pin_ch[pin] = -1;
  s_pin_attached[pin] = 0;
}

/* Pastikan pin sudah terpasang ke LEDC (dipakai analogWrite).
 * Pin yang belum pernah diatur sendiri mewarisi frekuensi/resolusi bawaan
 * (yang bisa diubah lewat analogWriteFrequency/Resolution bentuk 1 argumen). */
static int e32_ledc_ensure(int pin) {
  e32_pwm_init();
  if (!e32_pin_in_range(pin)) return -1;
  if (s_pin_attached[pin]) return s_pin_ch[pin];
  if (!s_pin_cfg[pin]) {
    s_pin_freq[pin] = s_def_freq;
    s_pin_res[pin] = s_def_res;
  }
  return e32_ledc_attach(pin, s_pin_freq[pin], s_pin_res[pin], -1);
}

/* Pasang ulang pin dengan freq/resolusi terbaru (untuk analogWriteResolution
 * dan analogWriteFrequency). Sifat "kanal hasil alokasi otomatis" ikut
 * dipertahankan supaya pembukuan kanal tidak bocor. */
static void e32_ledc_reconfig(int pin) {
  int ch, was_auto;
  if (!e32_pin_in_range(pin)) return;
  if (!s_pin_attached[pin]) return;
  ch = s_pin_ch[pin];
  was_auto = (ch >= 0 && ch < E32_LEDC_CHANNELS) ? s_ch[ch].autoalloc : 0;
  e32_ledc_detach(pin);
  if (e32_ledc_attach(pin, s_pin_freq[pin], s_pin_res[pin], ch) >= 0 &&
      was_auto && ch >= 0 && ch < E32_LEDC_CHANNELS) {
    s_ch[ch].autoalloc = 1;
  }
}

/* --- Penerjemah argumen "kanal_atau_pin" -------------------------------- */

/* Pecah argumen pertama ledcWrite/ledcRead/... menjadi pasangan (kanal, pin).
 * Urutan penebakan:
 *   1. nilai adalah PIN yang sedang terpasang       -> gaya 3.x
 *   2. nilai adalah KANAL yang sudah dikonfigurasi  -> gaya 2.x
 *   3. selain itu: perlakukan sebagai pin mentah
 * Urutan ini membuat dua idiom paling umum tetap benar:
 *   2.x: ledcSetup(0,5000,8); ledcAttachPin(2,0); ledcWrite(0,128);
 *   3.x: ledcAttach(2,5000,8);                    ledcWrite(2,128);
 */
static void e32_ledc_resolve(int x, int *out_ch, int *out_pin) {
  e32_pwm_init();
  if (e32_pin_in_range(x) && s_pin_attached[x]) {
    *out_pin = x;
    *out_ch = s_pin_ch[x];
    return;
  }
  if (x >= 0 && x < E32_LEDC_CHANNELS && s_ch[x].used) {
    *out_ch = x;
    *out_pin = s_ch[x].pin;
    return;
  }
  *out_pin = x;
  *out_ch = (e32_pin_in_range(x) ? s_pin_ch[x] : -1);
}

/* Resolusi aktif untuk sebuah target (dipakai analogWrite & ledcWrite). */
static uint8_t e32_ledc_res_of(int ch, int pin) {
  if (ch >= 0 && ch < E32_LEDC_CHANNELS && s_ch[ch].used) return s_ch[ch].res;
  if (e32_pin_in_range(pin)) return s_pin_res[pin];
  return s_def_res;
}

/* ========================================================================
 * 3. Interupsi tertunda (defer): ISR -> pencacah -> callback BAIK
 * ===================================================================== */

typedef struct e32_isr_slot {
  volatile uint32_t count; /* jumlah interupsi yang belum dilayani */
  baik_val_t cb;           /* fungsi BAIK; di-own() supaya aman dari GC   */
  uint8_t owned;           /* 1 bila cb sedang di-own()                   */
  uint8_t active;          /* 1 bila interupsi terpasang                  */
  uint8_t pin;             /* nomor GPIO (untuk argumen callback)         */
} e32_isr_slot_t;

static e32_isr_slot_t s_gpio_isr[E32_MAX_PINS];
static e32_isr_slot_t s_touch_isr[E32_MAX_PINS];

static portMUX_TYPE s_isr_mux = portMUX_INITIALIZER_UNLOCKED;

static uint32_t s_last_dispatched = 0;

/* ISR tunggal untuk SEMUA pin: `arg` menunjuk ke slot pemiliknya, jadi tidak
 * perlu 40 fungsi ISR terpisah.  HARAM memanggil apa pun milik interpreter
 * BAIK dari sini. */
static void IRAM_ATTR e32_gpio_isr_handler(void *arg) {
  e32_isr_slot_t *s = (e32_isr_slot_t *) arg;
  portENTER_CRITICAL_ISR(&s_isr_mux);
  s->count++;
  portEXIT_CRITICAL_ISR(&s_isr_mux);
}

#if E32_HAS_TOUCH
static void IRAM_ATTR e32_touch_isr_handler(void *arg) {
  e32_isr_slot_t *s = (e32_isr_slot_t *) arg;
  portENTER_CRITICAL_ISR(&s_isr_mux);
  s->count++;
  portEXIT_CRITICAL_ISR(&s_isr_mux);
}
#endif

static void e32_slot_set_cb(struct baik *baik, e32_isr_slot_t *s,
                            baik_val_t cb) {
  if (s->owned) {
    baik_disown(baik, &s->cb);
    s->owned = 0;
  }
  s->cb = cb;
  baik_own(baik, &s->cb);
  s->owned = 1;
}

static void e32_slot_clear_cb(struct baik *baik, e32_isr_slot_t *s) {
  if (s->owned) {
    baik_disown(baik, &s->cb);
    s->owned = 0;
  }
  s->cb = baik_mk_undefined();
}

/* Ambil (dan nolkan) sebagian pencacah dengan aman dari task. */
static uint32_t e32_slot_take(e32_isr_slot_t *s, uint32_t max) {
  uint32_t n;
  portENTER_CRITICAL(&s_isr_mux);
  n = s->count;
  if (n > max) {
    s->count = n - max;
    n = max;
  } else {
    s->count = 0;
  }
  portEXIT_CRITICAL(&s_isr_mux);
  return n;
}

static uint32_t e32_run_slots(struct baik *baik, e32_isr_slot_t *tab,
                              const char *jenis) {
  uint32_t done = 0;
  int i;
  for (i = 0; i < E32_MAX_PINS; i++) {
    e32_isr_slot_t *s = &tab[i];
    uint32_t n;
    if (!s->active) continue;
    /* Penjagaan kedua, murah dan sengaja berulang: baik_apply() akan
     * melompat ke alamat sampah bila cb bukan fungsi bytecode
     * (lihat src/baik.c:7941-7951 dan komentar pada e32_cb_ok()). Slot yang
     * entah bagaimana rusak dimatikan, bukan dieksekusi. */
    if (!baik_is_function(s->cb)) {
      printf("[BAIK] callback %s pin %d tidak sah (bukan fungsi BAIK) - "
             "interupsi dilepas.\n",
             jenis, (int) s->pin);
      s->active = 0;
      (void) e32_slot_take(s, 0xFFFFFFFFu);
      e32_slot_clear_cb(baik, s);
      continue;
    }
    n = e32_slot_take(s, (uint32_t) E32_ISR_BURST);
    while (n-- > 0) {
      baik_val_t res = baik_mk_undefined();
      baik_val_t args[1];
      args[0] = baik_mk_number(baik, (double) s->pin);
      if (baik_apply(baik, &res, s->cb, baik_mk_undefined(), 1, args) !=
          BAIK_OK) {
        printf("[BAIK] galat pada callback %s pin %d:\n", jenis, (int) s->pin);
        baik_print_error(baik, stdout, "  ", 0);
        /* Buang sisa antrean supaya konsol tidak dibanjiri galat yang sama. */
        (void) e32_slot_take(s, 0xFFFFFFFFu);
        break;
      }
      done++;
    }
  }
  return done;
}

/* Dipanggil dari task konsol (BUKAN dari ISR), lihat baik_esp32.h. */
void baik_esp32_poll_interrupts(struct baik *baik) {
  static int in_poll = 0;
  uint32_t done = 0;

  if (baik == NULL) return;
  if (in_poll) return; /* jangan reentrant */
  in_poll = 1;

  done += e32_run_slots(baik, s_gpio_isr, "interupsi");
#if E32_HAS_TOUCH
  done += e32_run_slots(baik, s_touch_isr, "sentuh");
#endif

  s_last_dispatched = done;
  in_poll = 0;
}

/* ========================================================================
 * 4. GPIO digital
 * ===================================================================== */

static void bk_pinMode(struct baik *baik) {
  int pin = e32_arg_int(baik, 0, -1);
  int mode = e32_arg_int(baik, 1, E32_MODE_INPUT);
  uint32_t caps;

  if (mode == E32_MODE_ANALOG) {
    if (!e32_adc_pin_ok(baik, pin, "pinMode")) {
      e32_ret_undef(baik);
      return;
    }
  } else {
    caps = E32_CAP_DIGITAL;
    if (mode & E32_MODE_OUTPUT) caps |= E32_CAP_OUTPUT;
    if (!e32_pin_ok(baik, pin, caps, "pinMode")) {
      e32_ret_undef(baik);
      return;
    }
  }
  pinMode((uint8_t) pin, (uint8_t) mode);
  e32_ret_undef(baik);
}

static void bk_digitalWrite(struct baik *baik) {
  int pin = e32_arg_int(baik, 0, -1);
  int level = e32_arg_level(baik, 1, 0);
  if (!e32_pin_ok(baik, pin, E32_CAP_OUTPUT, "digitalWrite")) {
    e32_ret_undef(baik);
    return;
  }
  digitalWrite((uint8_t) pin, (uint8_t) level);
  e32_ret_undef(baik);
}

static void bk_digitalRead(struct baik *baik) {
  int pin = e32_arg_int(baik, 0, -1);
  if (!e32_pin_ok(baik, pin, E32_CAP_INPUT, "digitalRead")) {
    e32_ret_undef(baik);
    return;
  }
  e32_ret_int(baik, (long) digitalRead((uint8_t) pin));
}

static void bk_digitalToggle(struct baik *baik) {
  int pin = e32_arg_int(baik, 0, -1);
  int level;
  if (!e32_pin_ok(baik, pin, E32_CAP_OUTPUT, "digitalToggle")) {
    e32_ret_undef(baik);
    return;
  }
  level = digitalRead((uint8_t) pin) ? 0 : 1;
  digitalWrite((uint8_t) pin, (uint8_t) level);
  e32_ret_int(baik, (long) level);
}

/* ========================================================================
 * 5. ADC
 * ===================================================================== */

static void bk_analogRead(struct baik *baik) {
  int pin = e32_arg_int(baik, 0, -1);
  if (!e32_adc_pin_ok(baik, pin, "analogRead")) {
    e32_ret_undef(baik);
    return;
  }
  e32_ret_int(baik, (long) analogRead((uint8_t) pin));
}

static void bk_analogReadMilliVolts(struct baik *baik) {
  int pin = e32_arg_int(baik, 0, -1);
  if (!e32_adc_pin_ok(baik, pin, "analogReadMilliVolts")) {
    e32_ret_undef(baik);
    return;
  }
  e32_ret_int(baik, (long) analogReadMilliVolts((uint8_t) pin));
}

static void bk_analogReadResolution(struct baik *baik) {
  int bits = e32_arg_int(baik, 0, 12);
  if (bits < 9 || bits > 12) {
    e32_err("analogReadResolution", "resolusi harus 9..12 bit, diberi %d",
            bits);
    e32_ret_undef(baik);
    return;
  }
  analogReadResolution((uint8_t) bits);
  e32_ret_int(baik, (long) bits);
}

static void bk_analogSetAttenuation(struct baik *baik) {
  int at = e32_arg_int(baik, 0, 3);
  if (at < 0 || at > 3) {
    e32_err("analogSetAttenuation",
            "atenuasi harus 0..3 (ADC_0db..ADC_11db), diberi %d", at);
    e32_ret_undef(baik);
    return;
  }
  analogSetAttenuation((adc_attenuation_t) at);
  e32_ret_undef(baik);
}

static void bk_analogSetPinAttenuation(struct baik *baik) {
  int pin = e32_arg_int(baik, 0, -1);
  int at = e32_arg_int(baik, 1, 3);
  if (!e32_adc_pin_ok(baik, pin, "analogSetPinAttenuation")) {
    e32_ret_undef(baik);
    return;
  }
  if (at < 0 || at > 3) {
    e32_err("analogSetPinAttenuation",
            "atenuasi harus 0..3 (ADC_0db..ADC_11db), diberi %d", at);
    e32_ret_undef(baik);
    return;
  }
  analogSetPinAttenuation((uint8_t) pin, (adc_attenuation_t) at);
  e32_ret_undef(baik);
}

/* ========================================================================
 * 6. PWM gaya Arduino (analogWrite*)
 * ===================================================================== */

static void bk_analogWrite(struct baik *baik) {
  int pin = e32_arg_int(baik, 0, -1);
  int value = e32_arg_int(baik, 1, 0);
  int vmax = e32_arg_int(baik, 2, 255);
  uint32_t top, duty;
  uint8_t res;

  if (!e32_pin_ok(baik, pin, E32_CAP_PWM, "analogWrite")) {
    e32_ret_undef(baik);
    return;
  }
  if (vmax <= 0) vmax = 255;
  if (value < 0) value = 0;
  if (value > vmax) value = vmax;

  if (e32_ledc_ensure(pin) < 0) {
    e32_err("analogWrite", "tidak ada kanal LEDC yang tersisa (maks %d kanal)",
            (int) E32_LEDC_CHANNELS);
    e32_ret_undef(baik);
    return;
  }
  res = s_pin_res[pin];
  top = (1u << res) - 1u;
  duty = (uint32_t) (((uint64_t) (uint32_t) value * (uint64_t) top) /
                     (uint64_t) (uint32_t) vmax);

#if E32_ARDUINO3
  ledcWrite((uint8_t) pin, duty);
#else
  ledcWrite((uint8_t) s_pin_ch[pin], duty);
#endif
  e32_ret_int(baik, (long) duty);
}

static void bk_analogWriteResolution(struct baik *baik) {
  int n = baik_nargs(baik);
  int pin, bits;

  e32_pwm_init();
  if (n < 2) {
    /* Bentuk satu argumen: ubah resolusi bawaan untuk pin berikutnya. */
    bits = e32_arg_int(baik, 0, E32_PWM_DEF_RES);
    if (bits < 1 || bits > 20) {
      e32_err("analogWriteResolution", "resolusi harus 1..20 bit, diberi %d",
              bits);
      e32_ret_undef(baik);
      return;
    }
    s_def_res = (uint8_t) bits;
    e32_ret_int(baik, (long) bits);
    return;
  }

  pin = e32_arg_int(baik, 0, -1);
  bits = e32_arg_int(baik, 1, E32_PWM_DEF_RES);
  if (!e32_pin_ok(baik, pin, E32_CAP_PWM, "analogWriteResolution")) {
    e32_ret_undef(baik);
    return;
  }
  if (bits < 1 || bits > 20) {
    e32_err("analogWriteResolution", "resolusi harus 1..20 bit, diberi %d",
            bits);
    e32_ret_undef(baik);
    return;
  }
  s_pin_res[pin] = (uint8_t) bits;
  s_pin_cfg[pin] = 1;
  e32_ledc_reconfig(pin);
  e32_ret_int(baik, (long) bits);
}

static void bk_analogWriteFrequency(struct baik *baik) {
  int n = baik_nargs(baik);
  int pin;
  double hz;

  e32_pwm_init();
  if (n < 2) {
    hz = e32_arg_num(baik, 0, (double) E32_PWM_DEF_FREQ);
    if (hz < 1.0) {
      e32_err("analogWriteFrequency", "frekuensi harus > 0 Hz");
      e32_ret_undef(baik);
      return;
    }
    s_def_freq = (uint32_t) hz;
    e32_ret_num(baik, (double) s_def_freq);
    return;
  }

  pin = e32_arg_int(baik, 0, -1);
  hz = e32_arg_num(baik, 1, (double) E32_PWM_DEF_FREQ);
  if (!e32_pin_ok(baik, pin, E32_CAP_PWM, "analogWriteFrequency")) {
    e32_ret_undef(baik);
    return;
  }
  if (hz < 1.0) {
    e32_err("analogWriteFrequency", "frekuensi harus > 0 Hz");
    e32_ret_undef(baik);
    return;
  }
  s_pin_freq[pin] = (uint32_t) hz;
  s_pin_cfg[pin] = 1;
  e32_ledc_reconfig(pin);
  e32_ret_num(baik, (double) s_pin_freq[pin]);
}

/* ========================================================================
 * 7. DAC
 * ===================================================================== */

static void bk_dacWrite(struct baik *baik) {
#if E32_HAS_DAC
  int pin = e32_arg_int(baik, 0, -1);
  int val = e32_arg_int(baik, 1, 0);
  if (!e32_pin_ok(baik, pin, E32_CAP_DAC, "dacWrite")) {
    e32_ret_undef(baik);
    return;
  }
  if (val < 0) val = 0;
  if (val > 255) val = 255;
  dacWrite((uint8_t) pin, (uint8_t) val);
  e32_ret_int(baik, (long) val);
#else
  e32_err("dacWrite", "DAC tidak tersedia pada chip %s", e32_board_chip());
  e32_ret_undef(baik);
#endif
}

static void bk_dacDisable(struct baik *baik) {
#if E32_HAS_DAC
  int pin = e32_arg_int(baik, 0, -1);
  if (!e32_pin_ok(baik, pin, E32_CAP_DAC, "dacDisable")) {
    e32_ret_undef(baik);
    return;
  }
  dacDisable((uint8_t) pin);
  e32_ret_undef(baik);
#else
  e32_err("dacDisable", "DAC tidak tersedia pada chip %s", e32_board_chip());
  e32_ret_undef(baik);
#endif
}

/* ========================================================================
 * 7b. LED RGB pintar (WS2812) & LED bawaan papan
 * ========================================================================
 *
 * KENAPA BAGIAN INI ADA.
 *
 * Pada ESP32-S3-DevKitC-1, LED bawaan bukan LED biasa melainkan sebuah
 * WS2812 (LED RGB "pintar") yang dikendalikan lewat protokol satu kabel.
 * Tabel pin BAIK (e32_pins.h) sengaja mendaftarkan LED_BUILTIN = 48, yaitu
 * nomor GPIO yang SUNGGUHAN, supaya validasi pin lewat tabel tetap masuk
 * akal.  Variant resmi arduino-esp32 justru memakai nilai penanda
 * (SOC_GPIO_PIN_COUNT + 48 = 97) khusus supaya digitalWrite() bisa
 * mengenalinya dan diam-diam mengalihkannya ke driver RMT.
 *
 * Akibatnya: di S3, `digitalWrite(48, HIGH)` hanya menggoyang jalur data
 * sesaat dan LED TIDAK akan menyala.  Contoh kedip LED harus memakai
 * rgbLedWrite() — atau, kalau ingin satu contoh yang sama jalan di ESP32
 * maupun S3, memakai ledBawaan().
 */

static void bk_rgbLedWrite(struct baik *baik) {
#if E32_HAS_RGBLED
  int pin = e32_arg_int(baik, 0, -1);
  int r = e32_arg_int(baik, 1, 0);
  int g = e32_arg_int(baik, 2, 0);
  int b = e32_arg_int(baik, 3, 0);

  if (!e32_pin_ok(baik, pin, E32_CAP_OUTPUT, "rgbLedWrite")) {
    e32_ret_undef(baik);
    return;
  }
  if (r < 0) r = 0;
  if (r > 255) r = 255;
  if (g < 0) g = 0;
  if (g > 255) g = 255;
  if (b < 0) b = 0;
  if (b > 255) b = 255;

  /* Nomor pin diteruskan apa adanya: kita memang memakai GPIO asli (mis. 48),
   * bukan nilai penanda RGB_BUILTIN milik variant. */
  neopixelWrite((uint8_t) pin, (uint8_t) r, (uint8_t) g, (uint8_t) b);
  e32_ret_undef(baik);
#else
  e32_err("rgbLedWrite",
          "driver LED RGB tidak tersedia (perlu arduino-esp32 2.0.7+ dan "
          "periferal RMT; inti terpasang %d.x, chip %s)",
          (int) ESP_ARDUINO_VERSION_MAJOR, e32_board_chip());
  e32_ret_undef(baik);
#endif
}

/* Cari pin LED bawaan: utamakan tabel pinout (kapabilitas E32_CAP_LED) supaya
 * nomornya sama dengan konstanta LED_BUILTIN yang diekspor Modul 1; bila tabel
 * tidak menandai apa pun, jatuh ke makro variant. */
static int e32_led_gpio(void) {
  size_t n = 0, i;
  const e32_pin_info_t *t = e32_pin_table(&n);
  if (t != NULL) {
    for (i = 0; i < n; i++) {
      if (t[i].caps & E32_CAP_LED) return (int) t[i].gpio;
    }
  }
#if E32_LED_IS_RGB
  {
    /* Variant resmi memakai nilai penanda RGB_BUILTIN = SOC_GPIO_PIN_COUNT +
     * gpio; kembalikan GPIO aslinya. */
    int p = (int) RGB_BUILTIN;
    if (p >= (int) SOC_GPIO_PIN_COUNT) p -= (int) SOC_GPIO_PIN_COUNT;
    return p;
  }
#elif defined(LED_BUILTIN)
  return (int) LED_BUILTIN;
#else
  return -1;
#endif
}

/* ledBawaan(nyala) / builtinLed(nyala)
 *
 * Fungsi KENYAMANAN khas BAIK — TIDAK ada padanannya di API Arduino.
 * Gunanya supaya satu contoh "hello world" yang sama bisa berkedip di kedua
 * papan: di papan berLED RGB ia memakai driver WS2812 (putih redup), di papan
 * berLED biasa ia memakai pinMode()+digitalWrite() seperti biasa. */
static void bk_ledBawaan(struct baik *baik) {
  int on = e32_arg_level(baik, 0, 1);
  int pin = e32_led_gpio();

  if (pin < 0) {
    e32_err("ledBawaan", "papan %s tidak punya LED bawaan yang dikenali",
            e32_board_name());
    e32_ret_undef(baik);
    return;
  }
#if E32_LED_IS_RGB
  {
    uint8_t v = on ? (uint8_t) E32_LED_RGB_DIM : (uint8_t) 0;
    neopixelWrite((uint8_t) pin, v, v, v);
  }
#else
  {
    static int mode_siap = 0;
    if (!mode_siap) {
      pinMode((uint8_t) pin, OUTPUT);
      mode_siap = 1;
    }
    digitalWrite((uint8_t) pin, on ? HIGH : LOW);
  }
#endif
  e32_ret_bool(baik, on);
}

/* ========================================================================
 * 8. Sensor sentuh
 * ===================================================================== */

static void bk_touchRead(struct baik *baik) {
#if E32_HAS_TOUCH
  int pin = e32_arg_int(baik, 0, -1);
  if (!e32_pin_ok(baik, pin, E32_CAP_TOUCH, "touchRead")) {
    e32_ret_undef(baik);
    return;
  }
  e32_ret_int(baik, (long) touchRead((uint8_t) pin));
#else
  e32_err("touchRead", "sensor sentuh tidak tersedia pada chip %s",
          e32_board_chip());
  e32_ret_undef(baik);
#endif
}

static void bk_touchSetCycles(struct baik *baik) {
#if E32_HAS_TOUCH
  int measure = e32_arg_int(baik, 0, 0x1000);
  int sleepc = e32_arg_int(baik, 1, 0x1000);
  if (measure < 0) measure = 0;
  if (sleepc < 0) sleepc = 0;
  if (measure > 0xFFFF) measure = 0xFFFF;
  if (sleepc > 0xFFFF) sleepc = 0xFFFF;
  touchSetCycles((uint16_t) measure, (uint16_t) sleepc);
  e32_ret_undef(baik);
#else
  e32_err("touchSetCycles", "sensor sentuh tidak tersedia pada chip %s",
          e32_board_chip());
  e32_ret_undef(baik);
#endif
}

static void bk_touchAttachInterrupt(struct baik *baik) {
#if E32_HAS_TOUCH
  int pin = e32_arg_int(baik, 0, -1);
  baik_val_t cb = baik_arg(baik, 1);
  double amb = e32_arg_num(baik, 2, 40.0);
  e32_isr_slot_t *s;

  if (!e32_pin_ok(baik, pin, E32_CAP_TOUCH, "touchAttachInterrupt")) {
    e32_ret_bool(baik, 0);
    return;
  }
  if (!e32_cb_ok(cb, "touchAttachInterrupt",
                 "touchAttachInterrupt(T0, fungsi(pin) { tulis(\"tersentuh\", "
                 "pin); }, 40)")) {
    e32_ret_bool(baik, 0);
    return;
  }
  if (amb < 0) amb = 0;

  s = &s_touch_isr[pin];
  if (s->active) touchDetachInterrupt((uint8_t) pin);
  s->pin = (uint8_t) pin;
  (void) e32_slot_take(s, 0xFFFFFFFFu);
  e32_slot_set_cb(baik, s, cb);
  s->active = 1;

  touchAttachInterruptArg((uint8_t) pin, e32_touch_isr_handler, (void *) s,
                          (touch_value_t) amb);
  e32_ret_bool(baik, 1);
#else
  e32_err("touchAttachInterrupt", "sensor sentuh tidak tersedia pada chip %s",
          e32_board_chip());
  e32_ret_bool(baik, 0);
#endif
}

static void bk_touchDetachInterrupt(struct baik *baik) {
#if E32_HAS_TOUCH
  int pin = e32_arg_int(baik, 0, -1);
  e32_isr_slot_t *s;
  if (!e32_pin_in_range(pin)) {
    e32_err("touchDetachInterrupt", "pin %d di luar jangkauan (0..%d)", pin,
            (int) (E32_MAX_PINS - 1));
    e32_ret_undef(baik);
    return;
  }
  s = &s_touch_isr[pin];
  touchDetachInterrupt((uint8_t) pin);
  s->active = 0;
  (void) e32_slot_take(s, 0xFFFFFFFFu);
  e32_slot_clear_cb(baik, s);
  e32_ret_undef(baik);
#else
  e32_err("touchDetachInterrupt", "sensor sentuh tidak tersedia pada chip %s",
          e32_board_chip());
  e32_ret_undef(baik);
#endif
}

/* ========================================================================
 * 9. Keluarga ledc*
 * ===================================================================== */

/* Ubah nama nada ("C", "C#", "Eb", ...) menjadi indeks note_t.
 * Balik -1 bila tidak dikenal. */
static int e32_note_index(const char *s) {
  char base;
  int idx;
  int mod = 0;

  if (s == NULL || s[0] == '\0') return -1;
  base = s[0];
  if (base >= 'a' && base <= 'z') base = (char) (base - 'a' + 'A');

  switch (base) {
    case 'C': idx = 0; break;  /* NOTE_C  */
    case 'D': idx = 2; break;  /* NOTE_D  */
    case 'E': idx = 4; break;  /* NOTE_E  */
    case 'F': idx = 5; break;  /* NOTE_F  */
    case 'G': idx = 7; break;  /* NOTE_G  */
    case 'A': idx = 9; break;  /* NOTE_A  */
    case 'B': idx = 11; break; /* NOTE_B  */
    default: return -1;
  }
  if (s[1] == '#' || s[1] == 's' || s[1] == 'S') {
    mod = 1;
  } else if (s[1] == 'b' || s[1] == 'B') {
    mod = -1;
  } else if (s[1] != '\0') {
    return -1;
  }
  idx += mod;
  if (idx < 0) idx += 12;
  if (idx > 11) idx -= 12;
  return idx;
}

static void bk_ledcSetup(struct baik *baik) {
  int chan = e32_arg_int(baik, 0, -1);
  double freq = e32_arg_num(baik, 1, (double) E32_PWM_DEF_FREQ);
  int bits = e32_arg_int(baik, 2, E32_PWM_DEF_RES);

  e32_pwm_init();
  if (chan < 0 || chan >= E32_LEDC_CHANNELS) {
    e32_err("ledcSetup", "kanal %d di luar jangkauan (0..%d) untuk chip %s",
            chan, (int) (E32_LEDC_CHANNELS - 1), e32_board_chip());
    e32_ret_num(baik, 0);
    return;
  }
  if (freq < 1.0) {
    e32_err("ledcSetup", "frekuensi harus > 0 Hz");
    e32_ret_num(baik, 0);
    return;
  }
  if (bits < 1 || bits > 20) {
    e32_err("ledcSetup", "resolusi harus 1..20 bit, diberi %d", bits);
    e32_ret_num(baik, 0);
    return;
  }

  s_ch[chan].used = 1;
  s_ch[chan].autoalloc = 0;
  s_ch[chan].freq = (uint32_t) freq;
  s_ch[chan].res = (uint8_t) bits;

#if E32_ARDUINO3
  /* Inti 3.x tidak punya ledcSetup(): konfigurasi hanya dicatat di sini dan
   * baru diterapkan saat ledcAttachPin().  Bila kanal ini sudah terpasang ke
   * sebuah pin, pasang ulang sekarang juga. */
  if (s_ch[chan].pin >= 0) {
    int pin = s_ch[chan].pin;
    e32_ledc_detach(pin);
    if (e32_ledc_attach(pin, (uint32_t) freq, (uint8_t) bits, chan) < 0) {
      e32_err("ledcSetup", "gagal memasang ulang pin %d ke kanal %d", pin,
              chan);
      e32_ret_num(baik, 0);
      return;
    }
  }
  e32_ret_num(baik, (double) s_ch[chan].freq);
#else
  {
    double real = ledcSetup((uint8_t) chan, freq, (uint8_t) bits);
    if (real == 0) {
      e32_err("ledcSetup",
              "kombinasi frekuensi %.1f Hz dan resolusi %d bit tidak bisa "
              "dipenuhi",
              freq, bits);
      s_ch[chan].used = 0;
      e32_ret_num(baik, 0);
      return;
    }
    s_ch[chan].freq = (uint32_t) real;
    e32_ret_num(baik, real);
  }
#endif
}

static void bk_ledcAttachPin(struct baik *baik) {
  int pin = e32_arg_int(baik, 0, -1);
  int chan = e32_arg_int(baik, 1, -1);

  e32_pwm_init();
  if (!e32_pin_ok(baik, pin, E32_CAP_PWM, "ledcAttachPin")) {
    e32_ret_bool(baik, 0);
    return;
  }
  if (chan < 0 || chan >= E32_LEDC_CHANNELS) {
    e32_err("ledcAttachPin", "kanal %d di luar jangkauan (0..%d) untuk chip %s",
            chan, (int) (E32_LEDC_CHANNELS - 1), e32_board_chip());
    e32_ret_bool(baik, 0);
    return;
  }
  if (!s_ch[chan].used) {
    /* Belum pernah ledcSetup(): pakai nilai bawaan supaya tetap jalan. */
    s_ch[chan].used = 1;
    s_ch[chan].autoalloc = 0;
    s_ch[chan].freq = s_def_freq;
    s_ch[chan].res = s_def_res;
  }
  if (e32_ledc_attach(pin, s_ch[chan].freq, s_ch[chan].res, chan) < 0) {
    e32_err("ledcAttachPin", "gagal memasang pin %d ke kanal %d", pin, chan);
    e32_ret_bool(baik, 0);
    return;
  }
  e32_ret_bool(baik, 1);
}

static void bk_ledcAttach(struct baik *baik) {
  int pin = e32_arg_int(baik, 0, -1);
  double freq = e32_arg_num(baik, 1, (double) E32_PWM_DEF_FREQ);
  int bits = e32_arg_int(baik, 2, E32_PWM_DEF_RES);

  e32_pwm_init();
  if (!e32_pin_ok(baik, pin, E32_CAP_PWM, "ledcAttach")) {
    e32_ret_bool(baik, 0);
    return;
  }
  if (freq < 1.0) {
    e32_err("ledcAttach", "frekuensi harus > 0 Hz");
    e32_ret_bool(baik, 0);
    return;
  }
  if (bits < 1 || bits > 20) {
    e32_err("ledcAttach", "resolusi harus 1..20 bit, diberi %d", bits);
    e32_ret_bool(baik, 0);
    return;
  }
  if (e32_ledc_attach(pin, (uint32_t) freq, (uint8_t) bits, -1) < 0) {
    e32_err("ledcAttach",
            "gagal memasang pin %d (kanal LEDC habis atau freq/resolusi tidak "
            "didukung)",
            pin);
    e32_ret_bool(baik, 0);
    return;
  }
  e32_ret_bool(baik, 1);
}

static void bk_ledcAttachChannel(struct baik *baik) {
  int pin = e32_arg_int(baik, 0, -1);
  double freq = e32_arg_num(baik, 1, (double) E32_PWM_DEF_FREQ);
  int bits = e32_arg_int(baik, 2, E32_PWM_DEF_RES);
  int chan = e32_arg_int(baik, 3, -1);

  e32_pwm_init();
  if (!e32_pin_ok(baik, pin, E32_CAP_PWM, "ledcAttachChannel")) {
    e32_ret_bool(baik, 0);
    return;
  }
  if (chan < 0 || chan >= E32_LEDC_CHANNELS) {
    e32_err("ledcAttachChannel",
            "kanal %d di luar jangkauan (0..%d) untuk chip %s", chan,
            (int) (E32_LEDC_CHANNELS - 1), e32_board_chip());
    e32_ret_bool(baik, 0);
    return;
  }
  if (freq < 1.0 || bits < 1 || bits > 20) {
    e32_err("ledcAttachChannel", "frekuensi/resolusi tidak masuk akal");
    e32_ret_bool(baik, 0);
    return;
  }
  if (e32_ledc_attach(pin, (uint32_t) freq, (uint8_t) bits, chan) < 0) {
    e32_err("ledcAttachChannel", "gagal memasang pin %d ke kanal %d", pin,
            chan);
    e32_ret_bool(baik, 0);
    return;
  }
  e32_ret_bool(baik, 1);
}

static void bk_ledcWrite(struct baik *baik) {
  int x = e32_arg_int(baik, 0, -1);
  long duty = (long) e32_arg_num(baik, 1, 0);
  int ch = -1, pin = -1;
  uint32_t top;
  uint8_t res;

  e32_ledc_resolve(x, &ch, &pin);
  res = e32_ledc_res_of(ch, pin);
  top = (1u << res);
  if (duty < 0) duty = 0;
  if ((uint32_t) duty > top) duty = (long) top;

#if E32_ARDUINO3
  if (pin < 0 || !e32_pin_in_range(pin)) {
    e32_err("ledcWrite",
            "kanal %d belum dipasang ke pin mana pun (panggil ledcAttachPin "
            "atau ledcAttach dulu)",
            x);
    e32_ret_bool(baik, 0);
    return;
  }
  if (!s_pin_attached[pin]) {
    e32_err("ledcWrite", "pin %d belum dipasang ke LEDC (panggil ledcAttach)",
            pin);
    e32_ret_bool(baik, 0);
    return;
  }
  ledcWrite((uint8_t) pin, (uint32_t) duty);
#else
  if (ch < 0 || ch >= E32_LEDC_CHANNELS) {
    e32_err("ledcWrite",
            "%d bukan kanal yang sudah di-ledcSetup() maupun pin yang sudah "
            "di-ledcAttach()",
            x);
    e32_ret_bool(baik, 0);
    return;
  }
  ledcWrite((uint8_t) ch, (uint32_t) duty);
#endif
  e32_ret_bool(baik, 1);
}

static void bk_ledcRead(struct baik *baik) {
  int x = e32_arg_int(baik, 0, -1);
  int ch = -1, pin = -1;

  e32_ledc_resolve(x, &ch, &pin);
#if E32_ARDUINO3
  if (pin < 0 || !e32_pin_in_range(pin) || !s_pin_attached[pin]) {
    e32_err("ledcRead", "%d bukan pin/kanal LEDC yang sedang terpasang", x);
    e32_ret_undef(baik);
    return;
  }
  e32_ret_int(baik, (long) ledcRead((uint8_t) pin));
#else
  if (ch < 0 || ch >= E32_LEDC_CHANNELS) {
    e32_err("ledcRead", "%d bukan pin/kanal LEDC yang sedang terpasang", x);
    e32_ret_undef(baik);
    return;
  }
  e32_ret_int(baik, (long) ledcRead((uint8_t) ch));
#endif
}

static void bk_ledcReadFreq(struct baik *baik) {
  int x = e32_arg_int(baik, 0, -1);
  int ch = -1, pin = -1;

  e32_ledc_resolve(x, &ch, &pin);
#if E32_ARDUINO3
  if (pin < 0 || !e32_pin_in_range(pin) || !s_pin_attached[pin]) {
    e32_err("ledcReadFreq", "%d bukan pin/kanal LEDC yang sedang terpasang", x);
    e32_ret_undef(baik);
    return;
  }
  e32_ret_num(baik, (double) ledcReadFreq((uint8_t) pin));
#else
  if (ch < 0 || ch >= E32_LEDC_CHANNELS) {
    e32_err("ledcReadFreq", "%d bukan pin/kanal LEDC yang sedang terpasang", x);
    e32_ret_undef(baik);
    return;
  }
  e32_ret_num(baik, (double) ledcReadFreq((uint8_t) ch));
#endif
}

static void bk_ledcWriteTone(struct baik *baik) {
  int x = e32_arg_int(baik, 0, -1);
  double freq = e32_arg_num(baik, 1, 0);
  int ch = -1, pin = -1;

  e32_ledc_resolve(x, &ch, &pin);
  if (freq < 0) freq = 0;
#if E32_ARDUINO3
  if (pin < 0 || !e32_pin_in_range(pin) || !s_pin_attached[pin]) {
    e32_err("ledcWriteTone", "%d bukan pin/kanal LEDC yang sedang terpasang",
            x);
    e32_ret_num(baik, 0);
    return;
  }
  e32_ret_num(baik, (double) ledcWriteTone((uint8_t) pin, (uint32_t) freq));
#else
  if (ch < 0 || ch >= E32_LEDC_CHANNELS) {
    e32_err("ledcWriteTone", "%d bukan pin/kanal LEDC yang sedang terpasang",
            x);
    e32_ret_num(baik, 0);
    return;
  }
  e32_ret_num(baik, (double) ledcWriteTone((uint8_t) ch, freq));
#endif
}

static void bk_ledcWriteNote(struct baik *baik) {
  int x = e32_arg_int(baik, 0, -1);
  baik_val_t nv = baik_arg(baik, 1);
  int octave = e32_arg_int(baik, 2, 4);
  int ch = -1, pin = -1;
  int note;

  if (baik_is_number(nv)) {
    note = e32_arg_int(baik, 1, -1);
  } else {
    note = e32_note_index(e32_arg_str(baik, 1, NULL));
  }
  if (note < 0 || note > 11) {
    e32_err("ledcWriteNote",
            "nada tidak dikenal; pakai \"C\",\"C#\",\"D\",\"Eb\",...,\"B\"");
    e32_ret_num(baik, 0);
    return;
  }
  if (octave < 0) octave = 0;
  if (octave > 8) octave = 8;

  e32_ledc_resolve(x, &ch, &pin);
#if E32_ARDUINO3
  if (pin < 0 || !e32_pin_in_range(pin) || !s_pin_attached[pin]) {
    e32_err("ledcWriteNote", "%d bukan pin/kanal LEDC yang sedang terpasang",
            x);
    e32_ret_num(baik, 0);
    return;
  }
  e32_ret_num(baik, (double) ledcWriteNote((uint8_t) pin, (note_t) note,
                                           (uint8_t) octave));
#else
  if (ch < 0 || ch >= E32_LEDC_CHANNELS) {
    e32_err("ledcWriteNote", "%d bukan pin/kanal LEDC yang sedang terpasang",
            x);
    e32_ret_num(baik, 0);
    return;
  }
  e32_ret_num(baik, (double) ledcWriteNote((uint8_t) ch, (note_t) note,
                                           (uint8_t) octave));
#endif
}

static void bk_ledcDetachPin(struct baik *baik) {
  int pin = e32_arg_int(baik, 0, -1);
  if (!e32_pin_ok(baik, pin, E32_CAP_PWM, "ledcDetachPin")) {
    e32_ret_undef(baik);
    return;
  }
  e32_ledc_detach(pin);
  e32_ret_undef(baik);
}

static void bk_ledcChangeFrequency(struct baik *baik) {
  int x = e32_arg_int(baik, 0, -1);
  double freq = e32_arg_num(baik, 1, (double) E32_PWM_DEF_FREQ);
  int bits = e32_arg_int(baik, 2, 0);
  int ch = -1, pin = -1;

  e32_ledc_resolve(x, &ch, &pin);
  if (!e32_pin_in_range(pin) || !s_pin_attached[pin]) {
    e32_err("ledcChangeFrequency",
            "%d bukan pin/kanal LEDC yang sedang terpasang", x);
    e32_ret_bool(baik, 0);
    return;
  }
  if (freq < 1.0) {
    e32_err("ledcChangeFrequency", "frekuensi harus > 0 Hz");
    e32_ret_bool(baik, 0);
    return;
  }
  if (bits <= 0) bits = s_pin_res[pin];
  s_pin_freq[pin] = (uint32_t) freq;
  s_pin_res[pin] = (uint8_t) bits;
  e32_ledc_reconfig(pin);
  e32_ret_bool(baik, s_pin_attached[pin] ? 1 : 0);
}

/* ========================================================================
 * 10. tone / noTone
 * ===================================================================== */

static void bk_tone(struct baik *baik) {
  int pin = e32_arg_int(baik, 0, -1);
  double freq = e32_arg_num(baik, 1, 0);
  double dur = e32_arg_num(baik, 2, 0);

  if (!e32_pin_ok(baik, pin, E32_CAP_PWM, "tone")) {
    e32_ret_undef(baik);
    return;
  }
  if (freq < 1.0) {
    e32_err("tone", "frekuensi harus > 0 Hz");
    e32_ret_undef(baik);
    return;
  }
  if (dur < 0) dur = 0;
  tone((uint8_t) pin, (unsigned int) freq, (unsigned long) dur);
  e32_ret_undef(baik);
}

static void bk_noTone(struct baik *baik) {
  int pin = e32_arg_int(baik, 0, -1);
  if (!e32_pin_ok(baik, pin, E32_CAP_PWM, "noTone")) {
    e32_ret_undef(baik);
    return;
  }
  noTone((uint8_t) pin);
  e32_ret_undef(baik);
}

/* ========================================================================
 * 11. Interupsi GPIO
 * ===================================================================== */

static void bk_attachInterrupt(struct baik *baik) {
  int pin = e32_arg_int(baik, 0, -1);
  baik_val_t cb = baik_arg(baik, 1);
  int mode = e32_arg_int(baik, 2, 3 /* CHANGE */);
  e32_isr_slot_t *s;

  if (!e32_pin_ok(baik, pin, E32_CAP_INPUT, "attachInterrupt")) {
    e32_ret_bool(baik, 0);
    return;
  }
  if (!e32_cb_ok(cb, "attachInterrupt",
                 "attachInterrupt(0, fungsi(pin) { digitalToggle(2); }, "
                 "RISING)")) {
    e32_ret_bool(baik, 0);
    return;
  }
  if (mode < 1 || mode > 5) {
    e32_err("attachInterrupt",
            "mode %d tidak sah; pakai RISING, FALLING, CHANGE, ONLOW atau "
            "ONHIGH",
            mode);
    e32_ret_bool(baik, 0);
    return;
  }

  s = &s_gpio_isr[pin];
  if (s->active) detachInterrupt((uint8_t) pin);
  s->pin = (uint8_t) pin;
  (void) e32_slot_take(s, 0xFFFFFFFFu);
  e32_slot_set_cb(baik, s, cb);
  s->active = 1;

  /* Satu handler ISR ber-argumen untuk semua pin; argumennya adalah slot. */
  attachInterruptArg((uint8_t) pin, e32_gpio_isr_handler, (void *) s, mode);
  e32_ret_bool(baik, 1);
}

static void bk_detachInterrupt(struct baik *baik) {
  int pin = e32_arg_int(baik, 0, -1);
  e32_isr_slot_t *s;

  if (!e32_pin_in_range(pin)) {
    e32_err("detachInterrupt", "pin %d di luar jangkauan (0..%d)", pin,
            (int) (E32_MAX_PINS - 1));
    e32_ret_undef(baik);
    return;
  }
  s = &s_gpio_isr[pin];
  detachInterrupt((uint8_t) pin);
  s->active = 0;
  (void) e32_slot_take(s, 0xFFFFFFFFu);
  e32_slot_clear_cb(baik, s);
  e32_ret_undef(baik);
}

static void bk_interrupts(struct baik *baik) {
#if defined(interrupts)
  interrupts();
#else
  portENABLE_INTERRUPTS();
#endif
  e32_ret_undef(baik);
}

static void bk_noInterrupts(struct baik *baik) {
  /* Peringatan: mematikan interupsi terlalu lama akan mengganggu WiFi dan
   * konsol.  Nyalakan lagi secepatnya dengan interrupts(). */
#if defined(noInterrupts)
  noInterrupts();
#else
  portDISABLE_INTERRUPTS();
#endif
  e32_ret_undef(baik);
}

static void bk_interruptCount(struct baik *baik) {
  int pin = e32_arg_int(baik, 0, -1);
  uint32_t n;
  if (!e32_pin_in_range(pin)) {
    e32_err("interruptCount", "pin %d di luar jangkauan (0..%d)", pin,
            (int) (E32_MAX_PINS - 1));
    e32_ret_int(baik, -1);
    return;
  }
  portENTER_CRITICAL(&s_isr_mux);
  n = s_gpio_isr[pin].count + s_touch_isr[pin].count;
  portEXIT_CRITICAL(&s_isr_mux);
  e32_ret_int(baik, (long) n);
}

static void bk_serviceInterrupts(struct baik *baik) {
  baik_esp32_poll_interrupts(baik);
  e32_ret_int(baik, (long) s_last_dispatched);
}

/* ========================================================================
 * 12. pulseIn / shiftOut / shiftIn
 * ===================================================================== */

static void bk_pulseIn(struct baik *baik) {
  int pin = e32_arg_int(baik, 0, -1);
  int state = e32_arg_level(baik, 1, 1);
  double tout = e32_arg_num(baik, 2, 1000000.0);
  unsigned long r;

  if (!e32_pin_ok(baik, pin, E32_CAP_INPUT, "pulseIn")) {
    e32_ret_undef(baik);
    return;
  }
  if (tout < 1.0) tout = 1000000.0;
  r = pulseIn((uint8_t) pin, (uint8_t) state, (unsigned long) tout);
  e32_ret_num(baik, (double) r);
}

static void bk_shiftOut(struct baik *baik) {
  int dataPin = e32_arg_int(baik, 0, -1);
  int clockPin = e32_arg_int(baik, 1, -1);
  int order = e32_arg_int(baik, 2, 1 /* MSBFIRST */);
  int val = e32_arg_int(baik, 3, 0);

  if (!e32_pin_ok(baik, dataPin, E32_CAP_OUTPUT, "shiftOut")) {
    e32_ret_undef(baik);
    return;
  }
  if (!e32_pin_ok(baik, clockPin, E32_CAP_OUTPUT, "shiftOut")) {
    e32_ret_undef(baik);
    return;
  }
  shiftOut((uint8_t) dataPin, (uint8_t) clockPin, (uint8_t) (order ? 1 : 0),
           (uint8_t) (val & 0xFF));
  e32_ret_undef(baik);
}

static void bk_shiftIn(struct baik *baik) {
  int dataPin = e32_arg_int(baik, 0, -1);
  int clockPin = e32_arg_int(baik, 1, -1);
  int order = e32_arg_int(baik, 2, 1 /* MSBFIRST */);

  if (!e32_pin_ok(baik, dataPin, E32_CAP_INPUT, "shiftIn")) {
    e32_ret_undef(baik);
    return;
  }
  if (!e32_pin_ok(baik, clockPin, E32_CAP_OUTPUT, "shiftIn")) {
    e32_ret_undef(baik);
    return;
  }
  e32_ret_int(baik, (long) shiftIn((uint8_t) dataPin, (uint8_t) clockPin,
                                   (uint8_t) (order ? 1 : 0)));
}

/* ========================================================================
 * 13. Sensor Hall (bonus, hanya ESP32 klasik dengan inti 2.x)
 * ===================================================================== */

static void bk_hallRead(struct baik *baik) {
#if E32_HAS_HALL
  e32_ret_int(baik, (long) hallRead());
#else
  e32_err("hallRead",
          "sensor Hall tidak tersedia (chip %s / inti arduino-esp32 %d.x)",
          e32_board_chip(), (int) ESP_ARDUINO_VERSION_MAJOR);
  e32_ret_undef(baik);
#endif
}

/* ========================================================================
 * 14. Registrasi
 * ===================================================================== */

void baik_esp32_register_gpio(struct baik *baik, baik_val_t g) {
  int i;

  e32_pwm_init();
  for (i = 0; i < E32_MAX_PINS; i++) {
    s_gpio_isr[i].count = 0;
    s_gpio_isr[i].cb = baik_mk_undefined();
    s_gpio_isr[i].owned = 0;
    s_gpio_isr[i].active = 0;
    s_gpio_isr[i].pin = (uint8_t) i;

    s_touch_isr[i].count = 0;
    s_touch_isr[i].cb = baik_mk_undefined();
    s_touch_isr[i].owned = 0;
    s_touch_isr[i].active = 0;
    s_touch_isr[i].pin = (uint8_t) i;
  }

  /* --- GPIO digital --- */
  e32_fn(baik, g, "pinMode", bk_pinMode);
  e32_fn(baik, g, "modePin", bk_pinMode);
  e32_fn(baik, g, "digitalWrite", bk_digitalWrite);
  e32_fn(baik, g, "tulisDigital", bk_digitalWrite);
  e32_fn(baik, g, "digitalRead", bk_digitalRead);
  e32_fn(baik, g, "bacaDigital", bk_digitalRead);
  e32_fn(baik, g, "digitalToggle", bk_digitalToggle);
  e32_fn(baik, g, "balikDigital", bk_digitalToggle);

  /* --- ADC --- */
  e32_fn(baik, g, "analogRead", bk_analogRead);
  e32_fn(baik, g, "bacaAnalog", bk_analogRead);
  e32_fn(baik, g, "analogReadMilliVolts", bk_analogReadMilliVolts);
  e32_fn(baik, g, "bacaMiliVolt", bk_analogReadMilliVolts);
  e32_fn(baik, g, "analogReadResolution", bk_analogReadResolution);
  e32_fn(baik, g, "analogSetAttenuation", bk_analogSetAttenuation);
  e32_fn(baik, g, "analogSetPinAttenuation", bk_analogSetPinAttenuation);

  /* --- PWM gaya Arduino --- */
  e32_fn(baik, g, "analogWrite", bk_analogWrite);
  e32_fn(baik, g, "tulisAnalog", bk_analogWrite);
  e32_fn(baik, g, "analogWriteResolution", bk_analogWriteResolution);
  e32_fn(baik, g, "analogWriteFrequency", bk_analogWriteFrequency);

  /* --- DAC --- */
  e32_fn(baik, g, "dacWrite", bk_dacWrite);
  e32_fn(baik, g, "tulisDac", bk_dacWrite);
  e32_fn(baik, g, "dacDisable", bk_dacDisable);

  /* --- LED RGB pintar & LED bawaan papan ---
   * Catatan: di ESP32-S3, LED_BUILTIN = 48 adalah WS2812, jadi
   * digitalWrite(48, HIGH) TIDAK menyalakannya. Pakai rgbLedWrite() atau
   * ledBawaan(). */
  e32_fn(baik, g, "rgbLedWrite", bk_rgbLedWrite);
  e32_fn(baik, g, "neopixelWrite", bk_rgbLedWrite);
  e32_fn(baik, g, "tulisLedRgb", bk_rgbLedWrite);
  e32_fn(baik, g, "ledBawaan", bk_ledBawaan);
  e32_fn(baik, g, "builtinLed", bk_ledBawaan);

  /* --- Sentuh --- */
  e32_fn(baik, g, "touchRead", bk_touchRead);
  e32_fn(baik, g, "bacaSentuh", bk_touchRead);
  e32_fn(baik, g, "touchSetCycles", bk_touchSetCycles);
  e32_fn(baik, g, "touchAttachInterrupt", bk_touchAttachInterrupt);
  e32_fn(baik, g, "sentuhInterupsi", bk_touchAttachInterrupt);
  e32_fn(baik, g, "touchDetachInterrupt", bk_touchDetachInterrupt);

  /* --- LEDC --- */
  e32_fn(baik, g, "ledcSetup", bk_ledcSetup);
  e32_fn(baik, g, "ledcAttachPin", bk_ledcAttachPin);
  e32_fn(baik, g, "ledcAttach", bk_ledcAttach);
  e32_fn(baik, g, "ledcAttachChannel", bk_ledcAttachChannel);
  e32_fn(baik, g, "ledcWrite", bk_ledcWrite);
  e32_fn(baik, g, "tulisPwm", bk_ledcWrite);
  e32_fn(baik, g, "ledcRead", bk_ledcRead);
  e32_fn(baik, g, "ledcReadFreq", bk_ledcReadFreq);
  e32_fn(baik, g, "ledcWriteTone", bk_ledcWriteTone);
  e32_fn(baik, g, "ledcWriteNote", bk_ledcWriteNote);
  e32_fn(baik, g, "ledcDetachPin", bk_ledcDetachPin);
  e32_fn(baik, g, "ledcDetach", bk_ledcDetachPin);
  e32_fn(baik, g, "ledcChangeFrequency", bk_ledcChangeFrequency);

  /* --- tone --- */
  e32_fn(baik, g, "tone", bk_tone);
  e32_fn(baik, g, "bunyi", bk_tone);
  e32_fn(baik, g, "noTone", bk_noTone);
  e32_fn(baik, g, "diam", bk_noTone);

  /* --- Interupsi --- */
  e32_fn(baik, g, "attachInterrupt", bk_attachInterrupt);
  e32_fn(baik, g, "pasangInterupsi", bk_attachInterrupt);
  e32_fn(baik, g, "detachInterrupt", bk_detachInterrupt);
  e32_fn(baik, g, "lepasInterupsi", bk_detachInterrupt);
  e32_fn(baik, g, "interrupts", bk_interrupts);
  e32_fn(baik, g, "noInterrupts", bk_noInterrupts);
  e32_fn(baik, g, "interruptCount", bk_interruptCount);
  e32_fn(baik, g, "jumlahInterupsi", bk_interruptCount);
  e32_fn(baik, g, "serviceInterrupts", bk_serviceInterrupts);
  e32_fn(baik, g, "layaniInterupsi", bk_serviceInterrupts);

  /* --- Pulsa & geser bit --- */
  e32_fn(baik, g, "pulseIn", bk_pulseIn);
  e32_fn(baik, g, "bacaPulsa", bk_pulseIn);
  e32_fn(baik, g, "shiftOut", bk_shiftOut);
  e32_fn(baik, g, "shiftIn", bk_shiftIn);

  /* --- Bonus --- */
  e32_fn(baik, g, "hallRead", bk_hallRead);
  e32_fn(baik, g, "bacaHall", bk_hallRead);
}
