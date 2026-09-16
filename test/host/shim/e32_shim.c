/*
 * e32_shim.c - Implementasi tiruan API ESP32 untuk PC.
 *
 * Lihat e32_shim.h untuk penjelasan tujuan. Ringkasnya: setiap fungsi di sini
 * punya NAMA dan SEMANTIK yang sama dengan padanannya di papan (lihat
 * scratchpad/API-SPEC.md), tetapi bekerja pada perangkat keras virtual dan
 * mencatat aksinya ke stdout.
 *
 * Format catatan (sengaja stabil supaya bisa dibandingkan dengan .expected):
 *   [GPIO] mode pin 2 <- OUTPUT
 *   [GPIO] pin 2 <- HIGH
 *   [GPIO] pin 4 -> LOW
 *   [ADC]  pin 34 -> 1234
 *   [PWM]  pin 5 <- 128/255
 *   [WAKTU] delay 500 ms (millis=500)
 *   [GALAT] digitalWrite: pin 6 tidak punya kapabilitas OUTPUT
 */
#include "e32_shim.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

/* ======================================================================== *
 * 1. Tabel pin virtual (ESP32 devkit v1 — salinan ringkas, mandiri)
 * ======================================================================== */

/* Kapabilitas; nilainya sengaja sama dengan E32_CAP_* di src/esp32/e32_pins.h
 * agar skrip yang memakai konstanta CAP_* berperilaku sama di PC dan papan. */
#define SH_DIGITAL (1u << 0)
#define SH_INPUT   (1u << 1)
#define SH_OUTPUT  (1u << 2)
#define SH_PULL    (1u << 3)
#define SH_ADC1    (1u << 4)
#define SH_ADC2    (1u << 5)
#define SH_DAC     (1u << 6)
#define SH_TOUCH   (1u << 7)
#define SH_RTC     (1u << 8)
#define SH_PWM     (1u << 9)
#define SH_STRAP   (1u << 10)
#define SH_FLASH   (1u << 11)
#define SH_USB     (1u << 12)
#define SH_UART0   (1u << 13)
#define SH_I2C     (1u << 14)
#define SH_SPI     (1u << 15)
#define SH_BOOT    (1u << 16)
#define SH_LED     (1u << 17)

#define SH_ADC (SH_ADC1 | SH_ADC2)
#define SH_IO  (SH_DIGITAL | SH_INPUT | SH_OUTPUT)

/* Kombinasi yang sering dipakai pin GPIO biasa pada ESP32. */
#define SH_GP  (SH_IO | SH_PULL | SH_PWM)          /* GPIO serbaguna        */
#define SH_IN  (SH_DIGITAL | SH_INPUT)             /* input-only (34..39)   */

typedef struct {
  int         gpio;
  unsigned    caps;
  int         touch;   /* nomor T*, atau -1 */
  const char *label;
} sh_pin_t;

/* Tabel pin ESP32-D0WD (esp32doit-devkit-v1). */
static const sh_pin_t SH_PINS[] = {
  {  0, SH_GP | SH_ADC2 | SH_TOUCH | SH_RTC | SH_STRAP | SH_BOOT,  1, "D0"  },
  {  1, SH_GP | SH_UART0,                                         -1, "TX0" },
  {  2, SH_GP | SH_ADC2 | SH_TOUCH | SH_RTC | SH_LED,              2, "D2"  },
  {  3, SH_GP | SH_UART0,                                         -1, "RX0" },
  {  4, SH_GP | SH_ADC2 | SH_TOUCH | SH_RTC,                       0, "D4"  },
  {  5, SH_GP | SH_STRAP | SH_SPI,                                -1, "D5"  },
  {  6, SH_DIGITAL | SH_FLASH,                                    -1, "SD_CLK"  },
  {  7, SH_DIGITAL | SH_FLASH,                                    -1, "SD_D0"   },
  {  8, SH_DIGITAL | SH_FLASH,                                    -1, "SD_D1"   },
  {  9, SH_DIGITAL | SH_FLASH,                                    -1, "SD_D2"   },
  { 10, SH_DIGITAL | SH_FLASH,                                    -1, "SD_D3"   },
  { 11, SH_DIGITAL | SH_FLASH,                                    -1, "SD_CMD"  },
  { 12, SH_GP | SH_ADC2 | SH_TOUCH | SH_RTC | SH_STRAP,            5, "D12" },
  { 13, SH_GP | SH_ADC2 | SH_TOUCH | SH_RTC,                       4, "D13" },
  { 14, SH_GP | SH_ADC2 | SH_TOUCH | SH_RTC,                       6, "D14" },
  { 15, SH_GP | SH_ADC2 | SH_TOUCH | SH_RTC | SH_STRAP,            3, "D15" },
  { 16, SH_GP,                                                    -1, "D16" },
  { 17, SH_GP,                                                    -1, "D17" },
  { 18, SH_GP | SH_SPI,                                           -1, "D18/SCK"  },
  { 19, SH_GP | SH_SPI,                                           -1, "D19/MISO" },
  { 21, SH_GP | SH_I2C,                                           -1, "D21/SDA"  },
  { 22, SH_GP | SH_I2C,                                           -1, "D22/SCL"  },
  { 23, SH_GP | SH_SPI,                                           -1, "D23/MOSI" },
  { 25, SH_GP | SH_ADC2 | SH_DAC | SH_RTC,                        -1, "D25/DAC1" },
  { 26, SH_GP | SH_ADC2 | SH_DAC | SH_RTC,                        -1, "D26/DAC2" },
  { 27, SH_GP | SH_ADC2 | SH_TOUCH | SH_RTC,                       7, "D27" },
  { 32, SH_GP | SH_ADC1 | SH_TOUCH | SH_RTC,                       9, "D32" },
  { 33, SH_GP | SH_ADC1 | SH_TOUCH | SH_RTC,                       8, "D33" },
  { 34, SH_IN | SH_ADC1 | SH_RTC,                                 -1, "D34" },
  { 35, SH_IN | SH_ADC1 | SH_RTC,                                 -1, "D35" },
  { 36, SH_IN | SH_ADC1 | SH_RTC,                                 -1, "VP/D36" },
  { 39, SH_IN | SH_ADC1 | SH_RTC,                                 -1, "VN/D39" },
};
#define SH_NPINS ((int) (sizeof(SH_PINS) / sizeof(SH_PINS[0])))

static const sh_pin_t *sh_find(int gpio) {
  int i;
  for (i = 0; i < SH_NPINS; i++)
    if (SH_PINS[i].gpio == gpio) return &SH_PINS[i];
  return NULL;
}

static const char *sh_cap_name(unsigned c) {
  switch (c) {
    case SH_DIGITAL: return "DIGITAL";
    case SH_INPUT:   return "INPUT";
    case SH_OUTPUT:  return "OUTPUT";
    case SH_PULL:    return "PULL";
    case SH_ADC1:    return "ADC1";
    case SH_ADC2:    return "ADC2";
    case SH_DAC:     return "DAC";
    case SH_TOUCH:   return "TOUCH";
    case SH_RTC:     return "RTC";
    case SH_PWM:     return "PWM";
    case SH_STRAP:   return "STRAP";
    case SH_FLASH:   return "FLASH";
    case SH_USB:     return "USB";
    case SH_UART0:   return "UART0";
    case SH_I2C:     return "I2C";
    case SH_SPI:     return "SPI";
    case SH_BOOT:    return "BOOT";
    case SH_LED:     return "LED";
    default:         return "?";
  }
}

/* Nama gabungan untuk pesan galat, mis. "ADC" untuk (ADC1|ADC2). */
static const char *sh_caps_label(unsigned caps) {
  if (caps == SH_ADC) return "ADC";
  if (caps == SH_IO)  return "DIGITAL";
  {
    unsigned b;
    for (b = 1; b; b <<= 1)
      if (caps & b) return sh_cap_name(b);
  }
  return "?";
}

/* ======================================================================== *
 * 2. State perangkat keras virtual
 * ======================================================================== */

#define SH_MAXPIN 48

typedef struct {
  int mode;        /* nilai terakhir pinMode()                              */
  int level;       /* level digital virtual (0/1)                           */
  int adc;         /* nilai ADC yang disuntik uji; -1 = pakai rumus bawaan  */
  int pwm;         /* duty PWM terakhir                                     */
  int pwm_max;     /* rentang PWM terakhir                                  */
  int touch;       /* nilai touchRead yang disuntik; -1 = rumus bawaan      */
} sh_pin_state_t;

static sh_pin_state_t  sh_state[SH_MAXPIN + 1];
static unsigned long   sh_micros;      /* jam virtual dalam mikrodetik      */
static unsigned long   sh_rng;         /* state PRNG deterministik          */
static int             sh_log = 1;     /* 1 = cetak catatan aksi            */

void baik_shim_reset(void) {
  int i;
  memset(sh_state, 0, sizeof(sh_state));
  for (i = 0; i <= SH_MAXPIN; i++) {
    sh_state[i].mode    = -1;
    sh_state[i].adc     = -1;
    sh_state[i].touch   = -1;
    sh_state[i].pwm_max = 255;
  }
  sh_micros = 0;
  sh_rng    = 12345u;
  sh_log    = 1;
}

/* PRNG LCG deterministik — hasilnya sama di setiap mesin, jadi keluaran uji
 * yang memakai random() tetap bisa dibandingkan dengan berkas .expected. */
static unsigned long sh_rand_next(void) {
  sh_rng = sh_rng * 1103515245uL + 12345uL;
  return (sh_rng >> 16) & 0x7fffffffuL;
}

/* Nilai ADC bawaan bila uji tidak menyuntik nilai: deterministik per pin. */
static int sh_adc_default(int pin) { return (pin * 97 + 1234) % 4096; }
static int sh_touch_default(int pin) { return 40 + (pin * 7) % 60; }

/* ======================================================================== *
 * 3. Helper argumen / nilai balik (sepadan dengan e32_* di src/esp32)
 * ======================================================================== */

static int sh_arg_int(struct baik *baik, int n, int def) {
  baik_val_t v;
  if (n >= baik_nargs(baik)) return def;
  v = baik_arg(baik, n);
  if (baik_is_number(v))  return baik_get_int(baik, v);
  if (baik_is_boolean(v)) return baik_get_bool(baik, v) ? 1 : 0;
  return def;
}

static double sh_arg_num(struct baik *baik, int n, double def) {
  baik_val_t v;
  if (n >= baik_nargs(baik)) return def;
  v = baik_arg(baik, n);
  if (baik_is_number(v))  return baik_get_double(baik, v);
  if (baik_is_boolean(v)) return baik_get_bool(baik, v) ? 1.0 : 0.0;
  return def;
}

/* PENTING: BAIK menyimpan string pendek LANGSUNG DI DALAM baik_val_t
 * (inline). Karena itu pointer hasil baik_get_string()/baik_get_cstring()
 * bisa menunjuk ke variabel lokal `v`, dan menjadi menggantung (dangling)
 * begitu fungsi ini selesai. Maka isinya WAJIB disalin ke buffer statis. */
static const char *sh_arg_str(struct baik *baik, int n, const char *def) {
  static char buf[512];
  baik_val_t v;
  if (n >= baik_nargs(baik)) return def;
  v = baik_arg(baik, n);
  if (baik_is_string(v)) {
    size_t len = 0;
    const char *p = baik_get_string(baik, &v, &len);
    if (p == NULL) return def;
    if (len >= sizeof(buf)) len = sizeof(buf) - 1;
    memcpy(buf, p, len);
    buf[len] = '\0';
    return buf;
  }
  baik_sprintf(v, baik, buf, sizeof(buf));
  return buf;
}

static void sh_ret_int(struct baik *baik, long v) {
  baik_return(baik, baik_mk_number(baik, (double) v));
}
static void sh_ret_num(struct baik *baik, double v) {
  baik_return(baik, baik_mk_number(baik, v));
}
static void sh_ret_bool(struct baik *baik, int v) {
  baik_return(baik, baik_mk_boolean(baik, v ? 1 : 0));
}
static void sh_ret_str(struct baik *baik, const char *s) {
  baik_return(baik, baik_mk_string(baik, s, ~(size_t) 0, 1));
}
static void sh_ret_undef(struct baik *baik) {
  baik_return(baik, baik_mk_undefined());
}

/* Validasi pin — sepadan dengan e32_pin_ok() di papan. */
static int sh_pin_ok(struct baik *baik, int gpio, unsigned caps,
                     const char *fname) {
  const sh_pin_t *p = sh_find(gpio);
  (void) baik;
  if (p == NULL) {
    printf("[GALAT] %s: GPIO %d tidak ada pada papan ini\n", fname, gpio);
    return 0;
  }
  if (p->caps & SH_FLASH) {
    printf("[GALAT] %s: pin %d terpakai SPI flash, jangan dipakai\n",
           fname, gpio);
    return 0;
  }
  /* caps yang berisi beberapa bit (mis. ADC1|ADC2) cukup terpenuhi salah satu */
  if (caps == SH_ADC) {
    if (!(p->caps & SH_ADC)) {
      printf("[GALAT] %s: pin %d tidak punya kapabilitas ADC\n", fname, gpio);
      return 0;
    }
    return 1;
  }
  if ((p->caps & caps) != caps) {
    printf("[GALAT] %s: pin %d tidak punya kapabilitas %s\n",
           fname, gpio, sh_caps_label(caps));
    return 0;
  }
  return 1;
}

static const char *sh_lvl(int v) { return v ? "HIGH" : "LOW"; }

static const char *sh_mode_name(int m) {
  switch (m) {
    case 0x01: return "INPUT";
    case 0x03: return "OUTPUT";
    case 0x05: return "INPUT_PULLUP";
    case 0x09: return "INPUT_PULLDOWN";
    case 0x13: return "OUTPUT_OPEN_DRAIN";
    case 0xC0: return "ANALOG";
    default:   return "MODE?";
  }
}

/* ======================================================================== *
 * 4. GPIO
 * ======================================================================== */

static void sh_pinMode(struct baik *baik) {
  int pin  = sh_arg_int(baik, 0, -1);
  int mode = sh_arg_int(baik, 1, 0x01);
  unsigned need = SH_DIGITAL;
  if (mode == 0x03 || mode == 0x13) need |= SH_OUTPUT;
  if (!sh_pin_ok(baik, pin, need, "pinMode")) { sh_ret_undef(baik); return; }
  sh_state[pin].mode = mode;
  if (sh_log) printf("[GPIO] mode pin %d <- %s\n", pin, sh_mode_name(mode));
  sh_ret_undef(baik);
}

static void sh_digitalWrite(struct baik *baik) {
  int pin = sh_arg_int(baik, 0, -1);
  int val = sh_arg_int(baik, 1, 0) ? 1 : 0;
  if (!sh_pin_ok(baik, pin, SH_OUTPUT, "digitalWrite")) {
    sh_ret_undef(baik); return;
  }
  sh_state[pin].level = val;
  if (sh_log) printf("[GPIO] pin %d <- %s\n", pin, sh_lvl(val));
  sh_ret_undef(baik);
}

static void sh_digitalRead(struct baik *baik) {
  int pin = sh_arg_int(baik, 0, -1);
  int val;
  if (!sh_pin_ok(baik, pin, SH_INPUT, "digitalRead")) { sh_ret_int(baik, -1); return; }
  val = sh_state[pin].level;
  if (sh_log) printf("[GPIO] pin %d -> %s\n", pin, sh_lvl(val));
  sh_ret_int(baik, val);
}

static void sh_digitalToggle(struct baik *baik) {
  int pin = sh_arg_int(baik, 0, -1);
  if (!sh_pin_ok(baik, pin, SH_OUTPUT, "digitalToggle")) {
    sh_ret_undef(baik); return;
  }
  sh_state[pin].level = !sh_state[pin].level;
  if (sh_log) printf("[GPIO] pin %d <- %s (toggle)\n", pin, sh_lvl(sh_state[pin].level));
  sh_ret_undef(baik);
}

static void sh_analogRead(struct baik *baik) {
  int pin = sh_arg_int(baik, 0, -1);
  int val;
  if (!sh_pin_ok(baik, pin, SH_ADC, "analogRead")) { sh_ret_int(baik, -1); return; }
  val = sh_state[pin].adc >= 0 ? sh_state[pin].adc : sh_adc_default(pin);
  if (sh_log) printf("[ADC] pin %d -> %d\n", pin, val);
  sh_ret_int(baik, val);
}

static void sh_analogReadMilliVolts(struct baik *baik) {
  int pin = sh_arg_int(baik, 0, -1);
  int raw, mv;
  if (!sh_pin_ok(baik, pin, SH_ADC, "analogReadMilliVolts")) {
    sh_ret_int(baik, -1); return;
  }
  raw = sh_state[pin].adc >= 0 ? sh_state[pin].adc : sh_adc_default(pin);
  mv  = (int) ((long) raw * 3300 / 4095);
  if (sh_log) printf("[ADC] pin %d -> %d mV\n", pin, mv);
  sh_ret_int(baik, mv);
}

static void sh_analogWrite(struct baik *baik) {
  int pin = sh_arg_int(baik, 0, -1);
  int val = sh_arg_int(baik, 1, 0);
  int max = sh_arg_int(baik, 2, 255);
  if (!sh_pin_ok(baik, pin, SH_PWM, "analogWrite")) { sh_ret_undef(baik); return; }
  if (val < 0) val = 0;
  if (val > max) val = max;
  sh_state[pin].pwm = val;
  sh_state[pin].pwm_max = max;
  if (sh_log) printf("[PWM] pin %d <- %d/%d\n", pin, val, max);
  sh_ret_undef(baik);
}

static void sh_ledcWrite(struct baik *baik) {
  int pin = sh_arg_int(baik, 0, -1);
  int val = sh_arg_int(baik, 1, 0);
  if (sh_log) printf("[PWM] ledc %d <- %d\n", pin, val);
  if (pin >= 0 && pin <= SH_MAXPIN) sh_state[pin].pwm = val;
  sh_ret_undef(baik);
}

static void sh_ledcAttach(struct baik *baik) {
  int pin  = sh_arg_int(baik, 0, -1);
  int freq = sh_arg_int(baik, 1, 5000);
  int bits = sh_arg_int(baik, 2, 8);
  if (!sh_pin_ok(baik, pin, SH_PWM, "ledcAttach")) { sh_ret_bool(baik, 0); return; }
  if (sh_log) printf("[PWM] ledcAttach pin %d freq %d Hz %d bit\n", pin, freq, bits);
  sh_ret_bool(baik, 1);
}

static void sh_touchRead(struct baik *baik) {
  int pin = sh_arg_int(baik, 0, -1);
  int val;
  if (!sh_pin_ok(baik, pin, SH_TOUCH, "touchRead")) { sh_ret_int(baik, -1); return; }
  val = sh_state[pin].touch >= 0 ? sh_state[pin].touch : sh_touch_default(pin);
  if (sh_log) printf("[TOUCH] pin %d -> %d\n", pin, val);
  sh_ret_int(baik, val);
}

static void sh_tone(struct baik *baik) {
  int pin  = sh_arg_int(baik, 0, -1);
  int freq = sh_arg_int(baik, 1, 0);
  int dur  = sh_arg_int(baik, 2, 0);
  if (!sh_pin_ok(baik, pin, SH_PWM, "tone")) { sh_ret_undef(baik); return; }
  if (sh_log) printf("[TONE] pin %d <- %d Hz (%d ms)\n", pin, freq, dur);
  sh_ret_undef(baik);
}

static void sh_noTone(struct baik *baik) {
  int pin = sh_arg_int(baik, 0, -1);
  if (sh_log) printf("[TONE] pin %d diam\n", pin);
  sh_ret_undef(baik);
}

/* ---- Introspeksi pinout ------------------------------------------------- */

static void sh_pinCaps(struct baik *baik) {
  const sh_pin_t *p = sh_find(sh_arg_int(baik, 0, -1));
  sh_ret_int(baik, p ? (long) p->caps : 0);
}

static void sh_pinPunya(struct baik *baik) {
  const sh_pin_t *p = sh_find(sh_arg_int(baik, 0, -1));
  unsigned caps = (unsigned) sh_arg_int(baik, 1, 0);
  sh_ret_bool(baik, p != NULL && (p->caps & caps) == caps);
}

static void sh_daftarPin(struct baik *baik) {
  unsigned caps = (unsigned) sh_arg_int(baik, 0, 0);
  baik_val_t arr = baik_mk_array(baik);
  int i;
  for (i = 0; i < SH_NPINS; i++)
    if ((SH_PINS[i].caps & caps) == caps)
      baik_array_push(baik, arr, baik_mk_number(baik, SH_PINS[i].gpio));
  baik_return(baik, arr);
}

static void sh_pinInfo(struct baik *baik) {
  int gpio = sh_arg_int(baik, 0, -1);
  const sh_pin_t *p = sh_find(gpio);
  baik_val_t o;
  if (p == NULL) { baik_return(baik, baik_mk_null()); return; }
  o = baik_mk_object(baik);
  baik_set(baik, o, "gpio",  ~(size_t) 0, baik_mk_number(baik, p->gpio));
  baik_set(baik, o, "label", ~(size_t) 0,
           baik_mk_string(baik, p->label, ~(size_t) 0, 1));
  baik_set(baik, o, "caps",  ~(size_t) 0, baik_mk_number(baik, p->caps));
  baik_set(baik, o, "touch", ~(size_t) 0, baik_mk_number(baik, p->touch));
  baik_return(baik, o);
}

static void sh_pinout(struct baik *baik) {
  int i;
  char buf[160];
  printf("GPIO  LABEL       KAPABILITAS\n");
  for (i = 0; i < SH_NPINS; i++) {
    unsigned b;
    size_t n = 0;
    buf[0] = 0;
    for (b = 1; b; b <<= 1) {
      if (SH_PINS[i].caps & b) {
        const char *nm = sh_cap_name(b);
        if (n) n += (size_t) snprintf(buf + n, sizeof(buf) - n, ",");
        n += (size_t) snprintf(buf + n, sizeof(buf) - n, "%s", nm);
      }
    }
    printf("%-5d %-11s %s\n", SH_PINS[i].gpio, SH_PINS[i].label, buf);
  }
  sh_ret_undef(baik);
}

/* ======================================================================== *
 * 5. Waktu (jam virtual) & matematika
 * ======================================================================== */

static void sh_delay(struct baik *baik) {
  long ms = (long) sh_arg_num(baik, 0, 0);
  if (ms < 0) ms = 0;
  sh_micros += (unsigned long) ms * 1000uL;
  if (sh_log) printf("[WAKTU] delay %ld ms (millis=%lu)\n", ms, sh_micros / 1000uL);
  sh_ret_undef(baik);
}

static void sh_delayMicroseconds(struct baik *baik) {
  long us = (long) sh_arg_num(baik, 0, 0);
  if (us < 0) us = 0;
  sh_micros += (unsigned long) us;
  if (sh_log) printf("[WAKTU] delayMicroseconds %ld us (micros=%lu)\n", us, sh_micros);
  sh_ret_undef(baik);
}

static void sh_millis(struct baik *baik) { sh_ret_int(baik, (long) (sh_micros / 1000uL)); }
static void sh_micros_fn(struct baik *baik) { sh_ret_int(baik, (long) sh_micros); }
static void sh_yield(struct baik *baik) { sh_ret_undef(baik); }

static void sh_random(struct baik *baik) {
  int n = baik_nargs(baik);
  long lo = 0, hi;
  if (n >= 2) { lo = (long) sh_arg_num(baik, 0, 0); hi = (long) sh_arg_num(baik, 1, 0); }
  else        { hi = (long) sh_arg_num(baik, 0, 0); }
  if (hi <= lo) { sh_ret_int(baik, lo); return; }
  sh_ret_int(baik, lo + (long) (sh_rand_next() % (unsigned long) (hi - lo)));
}

static void sh_randomSeed(struct baik *baik) {
  sh_rng = (unsigned long) sh_arg_num(baik, 0, 1);
  sh_ret_undef(baik);
}

static void sh_map(struct baik *baik) {
  double x = sh_arg_num(baik, 0, 0), im = sh_arg_num(baik, 1, 0),
         iM = sh_arg_num(baik, 2, 0), om = sh_arg_num(baik, 3, 0),
         oM = sh_arg_num(baik, 4, 0);
  if (iM == im) { sh_ret_num(baik, om); return; }
  /* Arduino map() memakai aritmetika bulat; kita tiru itu. */
  sh_ret_int(baik, (long) (((long) x - (long) im) * ((long) oM - (long) om) /
                           ((long) iM - (long) im) + (long) om));
}

static void sh_constrain(struct baik *baik) {
  double x = sh_arg_num(baik, 0, 0), a = sh_arg_num(baik, 1, 0), b = sh_arg_num(baik, 2, 0);
  sh_ret_num(baik, x < a ? a : (x > b ? b : x));
}

static void sh_min(struct baik *baik) {
  double a = sh_arg_num(baik, 0, 0), b = sh_arg_num(baik, 1, 0);
  sh_ret_num(baik, a < b ? a : b);
}
static void sh_max(struct baik *baik) {
  double a = sh_arg_num(baik, 0, 0), b = sh_arg_num(baik, 1, 0);
  sh_ret_num(baik, a > b ? a : b);
}
static void sh_abs(struct baik *baik)   { sh_ret_num(baik, fabs(sh_arg_num(baik, 0, 0))); }
static void sh_pow(struct baik *baik)   { sh_ret_num(baik, pow(sh_arg_num(baik, 0, 0), sh_arg_num(baik, 1, 0))); }
static void sh_sqrt(struct baik *baik)  { sh_ret_num(baik, sqrt(sh_arg_num(baik, 0, 0))); }
static void sh_sin(struct baik *baik)   { sh_ret_num(baik, sin(sh_arg_num(baik, 0, 0))); }
static void sh_cos(struct baik *baik)   { sh_ret_num(baik, cos(sh_arg_num(baik, 0, 0))); }
static void sh_tan(struct baik *baik)   { sh_ret_num(baik, tan(sh_arg_num(baik, 0, 0))); }
static void sh_floor(struct baik *baik) { sh_ret_num(baik, floor(sh_arg_num(baik, 0, 0))); }
static void sh_ceil(struct baik *baik)  { sh_ret_num(baik, ceil(sh_arg_num(baik, 0, 0))); }
static void sh_round(struct baik *baik) { sh_ret_num(baik, floor(sh_arg_num(baik, 0, 0) + 0.5)); }
static void sh_log_fn(struct baik *baik){ sh_ret_num(baik, log(sh_arg_num(baik, 0, 1))); }
static void sh_exp(struct baik *baik)   { sh_ret_num(baik, exp(sh_arg_num(baik, 0, 0))); }

/* ======================================================================== *
 * 6. Serial & ESP (secukupnya untuk uji)
 * ======================================================================== */

static void sh_Serial_begin(struct baik *baik) {
  if (sh_log) printf("[SERIAL] begin %d baud\n", sh_arg_int(baik, 0, 115200));
  sh_ret_undef(baik);
}
static void sh_Serial_print(struct baik *baik) {
  printf("%s", sh_arg_str(baik, 0, ""));
  sh_ret_undef(baik);
}
static void sh_Serial_println(struct baik *baik) {
  printf("%s\n", baik_nargs(baik) > 0 ? sh_arg_str(baik, 0, "") : "");
  sh_ret_undef(baik);
}
static void sh_Serial_available(struct baik *baik) { sh_ret_int(baik, 0); }
static void sh_Serial_read(struct baik *baik)      { sh_ret_int(baik, -1); }
static void sh_Serial_flush(struct baik *baik)     { sh_ret_undef(baik); }

static void sh_ESP_getFreeHeap(struct baik *baik)  { sh_ret_int(baik, 200000); }
static void sh_ESP_getHeapSize(struct baik *baik)  { sh_ret_int(baik, 320000); }
static void sh_ESP_getChipModel(struct baik *baik) { sh_ret_str(baik, "ESP32-HOST-SHIM"); }
static void sh_ESP_getChipCores(struct baik *baik) { sh_ret_int(baik, 2); }
static void sh_ESP_getCpuFreqMHz(struct baik *baik){ sh_ret_int(baik, 240); }
static void sh_ESP_restart(struct baik *baik) {
  printf("[ESP] restart (diabaikan di shim PC)\n");
  sh_ret_undef(baik);
}
static void sh_temperatureRead(struct baik *baik)  { sh_ret_num(baik, 42.5); }

/* ======================================================================== *
 * 7. Kendali uji (hanya ada di PC, tidak ada di papan)
 * ======================================================================== */

/* shimSetDigital(pin, nilai) — suntik level yang akan dibaca digitalRead. */
static void sh_shimSetDigital(struct baik *baik) {
  int pin = sh_arg_int(baik, 0, -1);
  int val = sh_arg_int(baik, 1, 0) ? 1 : 0;
  if (pin >= 0 && pin <= SH_MAXPIN) sh_state[pin].level = val;
  sh_ret_undef(baik);
}
/* shimSetAnalog(pin, nilai) — suntik nilai yang akan dibaca analogRead. */
static void sh_shimSetAnalog(struct baik *baik) {
  int pin = sh_arg_int(baik, 0, -1);
  int val = sh_arg_int(baik, 1, 0);
  if (pin >= 0 && pin <= SH_MAXPIN) sh_state[pin].adc = val;
  sh_ret_undef(baik);
}
/* shimSetTouch(pin, nilai) */
static void sh_shimSetTouch(struct baik *baik) {
  int pin = sh_arg_int(baik, 0, -1);
  int val = sh_arg_int(baik, 1, 0);
  if (pin >= 0 && pin <= SH_MAXPIN) sh_state[pin].touch = val;
  sh_ret_undef(baik);
}
/* shimGetLevel(pin) — baca level pin TANPA mencetak catatan & tanpa validasi. */
static void sh_shimGetLevel(struct baik *baik) {
  int pin = sh_arg_int(baik, 0, -1);
  sh_ret_int(baik, (pin >= 0 && pin <= SH_MAXPIN) ? sh_state[pin].level : -1);
}
/* shimGetPwm(pin) */
static void sh_shimGetPwm(struct baik *baik) {
  int pin = sh_arg_int(baik, 0, -1);
  sh_ret_int(baik, (pin >= 0 && pin <= SH_MAXPIN) ? sh_state[pin].pwm : -1);
}
/* shimLog(benar/salah) — nyalakan/matikan catatan aksi. */
static void sh_shimLog(struct baik *baik) {
  sh_log = sh_arg_int(baik, 0, 1) ? 1 : 0;
  sh_ret_undef(baik);
}
/* shimReset() — kembalikan seluruh state ke kondisi awal. */
static void sh_shimReset(struct baik *baik) {
  baik_shim_reset();
  sh_ret_undef(baik);
}

/* ======================================================================== *
 * 8. Registrasi
 * ======================================================================== */

static void sh_fn(struct baik *baik, baik_val_t obj, const char *name,
                  void (*fn)(struct baik *)) {
  baik_set(baik, obj, name, ~(size_t) 0,
           baik_mk_foreign_func(baik, (baik_func_ptr_t) fn));
}

static void sh_const(struct baik *baik, baik_val_t obj, const char *name, double v) {
  baik_set(baik, obj, name, ~(size_t) 0, baik_mk_number(baik, v));
}

static void sh_str_const(struct baik *baik, baik_val_t obj, const char *name,
                         const char *v) {
  baik_set(baik, obj, name, ~(size_t) 0, baik_mk_string(baik, v, ~(size_t) 0, 1));
}

static baik_val_t sh_ns(struct baik *baik, baik_val_t g, const char *name) {
  baik_val_t o = baik_mk_object(baik);
  baik_set(baik, g, name, ~(size_t) 0, o);
  return o;
}

void baik_shim_register(struct baik *baik) {
  baik_val_t g = baik_get_global(baik);
  baik_val_t Serial, ESPo;

  baik_shim_reset();

  /* ---- Konstanta (Modul 1 API-SPEC) ---- */
  sh_const(baik, g, "HIGH", 1);
  sh_const(baik, g, "LOW", 0);
  sh_const(baik, g, "INPUT", 0x01);
  sh_const(baik, g, "OUTPUT", 0x03);
  sh_const(baik, g, "INPUT_PULLUP", 0x05);
  sh_const(baik, g, "INPUT_PULLDOWN", 0x09);
  sh_const(baik, g, "OUTPUT_OPEN_DRAIN", 0x13);
  sh_const(baik, g, "ANALOG", 0xC0);
  sh_const(baik, g, "RISING", 1);
  sh_const(baik, g, "FALLING", 2);
  sh_const(baik, g, "CHANGE", 3);
  sh_const(baik, g, "ONLOW", 4);
  sh_const(baik, g, "ONHIGH", 5);
  sh_const(baik, g, "LSBFIRST", 0);
  sh_const(baik, g, "MSBFIRST", 1);
  sh_const(baik, g, "PI", 3.1415926535897932384626433832795);
  sh_const(baik, g, "HALF_PI", 1.5707963267948966192313216916398);
  sh_const(baik, g, "TWO_PI", 6.283185307179586476925286766559);
  sh_const(baik, g, "DEG_TO_RAD", 0.017453292519943295769236907684886);
  sh_const(baik, g, "RAD_TO_DEG", 57.295779513082320876798154814105);
  sh_const(baik, g, "EULER", 2.718281828459045235360287471352);
  sh_const(baik, g, "ADC_0db", 0);
  sh_const(baik, g, "ADC_2_5db", 1);
  sh_const(baik, g, "ADC_6db", 2);
  sh_const(baik, g, "ADC_11db", 3);

  /* Pin papan (esp32doit-devkit-v1) */
  sh_const(baik, g, "LED_BUILTIN", 2);
  sh_const(baik, g, "BUILTIN_LED", 2);
  sh_const(baik, g, "BOOT_BUTTON", 0);
  sh_const(baik, g, "TX", 1);
  sh_const(baik, g, "RX", 3);
  sh_const(baik, g, "SDA", 21);
  sh_const(baik, g, "SCL", 22);
  sh_const(baik, g, "MOSI", 23);
  sh_const(baik, g, "MISO", 19);
  sh_const(baik, g, "SCK", 18);
  sh_const(baik, g, "SS", 5);

  /* ADC alias ESP32 */
  sh_const(baik, g, "A0", 36);  sh_const(baik, g, "A3", 39);
  sh_const(baik, g, "A4", 32);  sh_const(baik, g, "A5", 33);
  sh_const(baik, g, "A6", 34);  sh_const(baik, g, "A7", 35);
  sh_const(baik, g, "A10", 4);  sh_const(baik, g, "A11", 0);
  sh_const(baik, g, "A12", 2);  sh_const(baik, g, "A13", 15);
  sh_const(baik, g, "A14", 13); sh_const(baik, g, "A15", 12);
  sh_const(baik, g, "A16", 14); sh_const(baik, g, "A17", 27);
  sh_const(baik, g, "A18", 25); sh_const(baik, g, "A19", 26);

  /* GPIOn */
  {
    int i; char nm[12];
    for (i = 0; i < SH_NPINS; i++) {
      snprintf(nm, sizeof(nm), "GPIO%d", SH_PINS[i].gpio);
      sh_const(baik, g, nm, SH_PINS[i].gpio);
    }
  }

  /* Kapabilitas */
  sh_const(baik, g, "CAP_ADC1", SH_ADC1);
  sh_const(baik, g, "CAP_ADC2", SH_ADC2);
  sh_const(baik, g, "CAP_DAC", SH_DAC);
  sh_const(baik, g, "CAP_TOUCH", SH_TOUCH);
  sh_const(baik, g, "CAP_RTC", SH_RTC);
  sh_const(baik, g, "CAP_PWM", SH_PWM);
  sh_const(baik, g, "CAP_STRAP", SH_STRAP);
  sh_const(baik, g, "CAP_FLASH", SH_FLASH);
  sh_const(baik, g, "CAP_INPUT", SH_INPUT);
  sh_const(baik, g, "CAP_OUTPUT", SH_OUTPUT);

  sh_str_const(baik, g, "BOARD", "host-shim (tiruan esp32doit-devkit-v1)");
  sh_str_const(baik, g, "CHIP", "ESP32-HOST-SHIM");

  /* ---- GPIO (Modul 2) + alias Indonesia ---- */
  sh_fn(baik, g, "pinMode", sh_pinMode);
  sh_fn(baik, g, "modePin", sh_pinMode);
  sh_fn(baik, g, "digitalWrite", sh_digitalWrite);
  sh_fn(baik, g, "tulisDigital", sh_digitalWrite);
  sh_fn(baik, g, "digitalRead", sh_digitalRead);
  sh_fn(baik, g, "bacaDigital", sh_digitalRead);
  sh_fn(baik, g, "digitalToggle", sh_digitalToggle);
  sh_fn(baik, g, "analogRead", sh_analogRead);
  sh_fn(baik, g, "bacaAnalog", sh_analogRead);
  sh_fn(baik, g, "analogReadMilliVolts", sh_analogReadMilliVolts);
  sh_fn(baik, g, "bacaMiliVolt", sh_analogReadMilliVolts);
  sh_fn(baik, g, "analogWrite", sh_analogWrite);
  sh_fn(baik, g, "tulisAnalog", sh_analogWrite);
  sh_fn(baik, g, "ledcWrite", sh_ledcWrite);
  sh_fn(baik, g, "tulisPwm", sh_ledcWrite);
  sh_fn(baik, g, "ledcAttach", sh_ledcAttach);
  sh_fn(baik, g, "touchRead", sh_touchRead);
  sh_fn(baik, g, "bacaSentuh", sh_touchRead);
  sh_fn(baik, g, "tone", sh_tone);
  sh_fn(baik, g, "bunyi", sh_tone);
  sh_fn(baik, g, "noTone", sh_noTone);
  sh_fn(baik, g, "diam", sh_noTone);

  /* Introspeksi pinout */
  sh_fn(baik, g, "pinInfo", sh_pinInfo);
  sh_fn(baik, g, "pinCaps", sh_pinCaps);
  sh_fn(baik, g, "pinPunya", sh_pinPunya);
  sh_fn(baik, g, "pinHas", sh_pinPunya);
  sh_fn(baik, g, "daftarPin", sh_daftarPin);
  sh_fn(baik, g, "pinList", sh_daftarPin);
  sh_fn(baik, g, "pinout", sh_pinout);

  /* ---- Waktu & matematika (Modul 3) ---- */
  sh_fn(baik, g, "delay", sh_delay);
  sh_fn(baik, g, "tunggu", sh_delay);
  sh_fn(baik, g, "delayMicroseconds", sh_delayMicroseconds);
  sh_fn(baik, g, "tungguMikro", sh_delayMicroseconds);
  sh_fn(baik, g, "millis", sh_millis);
  sh_fn(baik, g, "milidetik", sh_millis);
  sh_fn(baik, g, "micros", sh_micros_fn);
  sh_fn(baik, g, "mikrodetik", sh_micros_fn);
  sh_fn(baik, g, "yield", sh_yield);
  sh_fn(baik, g, "random", sh_random);
  sh_fn(baik, g, "acak", sh_random);
  sh_fn(baik, g, "randomSeed", sh_randomSeed);
  sh_fn(baik, g, "map", sh_map);
  sh_fn(baik, g, "peta", sh_map);
  sh_fn(baik, g, "constrain", sh_constrain);
  sh_fn(baik, g, "batas", sh_constrain);
  sh_fn(baik, g, "min", sh_min);
  sh_fn(baik, g, "max", sh_max);
  sh_fn(baik, g, "abs", sh_abs);
  sh_fn(baik, g, "pow", sh_pow);
  sh_fn(baik, g, "sqrt", sh_sqrt);
  sh_fn(baik, g, "sin", sh_sin);
  sh_fn(baik, g, "cos", sh_cos);
  sh_fn(baik, g, "tan", sh_tan);
  sh_fn(baik, g, "floor", sh_floor);
  sh_fn(baik, g, "ceil", sh_ceil);
  sh_fn(baik, g, "round", sh_round);
  sh_fn(baik, g, "log", sh_log_fn);
  sh_fn(baik, g, "exp", sh_exp);
  sh_fn(baik, g, "temperatureRead", sh_temperatureRead);
  sh_fn(baik, g, "bacaSuhu", sh_temperatureRead);

  /* ---- object Serial ---- */
  Serial = sh_ns(baik, g, "Serial");
  sh_fn(baik, Serial, "begin", sh_Serial_begin);
  sh_fn(baik, Serial, "print", sh_Serial_print);
  sh_fn(baik, Serial, "println", sh_Serial_println);
  sh_fn(baik, Serial, "available", sh_Serial_available);
  sh_fn(baik, Serial, "read", sh_Serial_read);
  sh_fn(baik, Serial, "flush", sh_Serial_flush);

  /* ---- object ESP ---- */
  ESPo = sh_ns(baik, g, "ESP");
  sh_fn(baik, ESPo, "getFreeHeap", sh_ESP_getFreeHeap);
  sh_fn(baik, ESPo, "getHeapSize", sh_ESP_getHeapSize);
  sh_fn(baik, ESPo, "getChipModel", sh_ESP_getChipModel);
  sh_fn(baik, ESPo, "getChipCores", sh_ESP_getChipCores);
  sh_fn(baik, ESPo, "getCpuFreqMHz", sh_ESP_getCpuFreqMHz);
  sh_fn(baik, ESPo, "restart", sh_ESP_restart);

  /* ---- Kendali uji (khusus PC) ---- */
  sh_fn(baik, g, "shimSetDigital", sh_shimSetDigital);
  sh_fn(baik, g, "shimSetAnalog", sh_shimSetAnalog);
  sh_fn(baik, g, "shimSetTouch", sh_shimSetTouch);
  sh_fn(baik, g, "shimGetLevel", sh_shimGetLevel);
  sh_fn(baik, g, "shimGetPwm", sh_shimGetPwm);
  sh_fn(baik, g, "shimLog", sh_shimLog);
  sh_fn(baik, g, "shimReset", sh_shimReset);
}
