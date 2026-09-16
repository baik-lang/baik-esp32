/*
 * e32_helper.cpp - Helper bersama untuk seluruh modul jembatan BAIK x ESP32.
 *
 * Berisi implementasi dari bagian "Helper bersama" pada baik_esp32.h:
 *   - pengambilan argumen  : e32_arg_int / e32_arg_num / e32_arg_bool / e32_arg_str
 *   - pengembalian nilai   : e32_ret_int / e32_ret_num / e32_ret_bool /
 *                            e32_ret_str / e32_ret_undef
 *   - pendaftaran          : e32_ns / e32_fn / e32_const
 *   - validasi pin         : e32_pin_ok
 *   - cetak informasi      : e32_print_pinout / e32_print_api
 *
 * Catatan GC (penting untuk semua modul):
 *   Pengumpul sampah (GC) BAIK hanya berjalan di dalam gelung interpreter,
 *   yaitu di antara dua opcode (lihat `maybe_gc()` pada src/baik.c). GC TIDAK
 *   pernah berjalan di tengah-tengah pemanggilan fungsi native. Karena itu
 *   nilai sementara (baik_val_t lokal) di dalam helper ini aman tanpa
 *   baik_own(). Objek namespace yang dibuat e32_ns() langsung dipasang ke
 *   object global; object global adalah akar GC (baik->scopes ditandai oleh
 *   baik_gc()), sehingga namespace tersebut otomatis selamat dari GC dan
 *   TIDAK perlu di-baik_own(). Gunakan baik_own() hanya bila sebuah
 *   baik_val_t disimpan di variabel C statis/global yang TIDAK terjangkau
 *   dari object global (mis. daftar callback interupsi di e32_gpio.cpp).
 */

#include "baik_esp32.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ======================================================================== *
 *  Argumen
 * ======================================================================== */

/* Benar bila argumen ke-n memang diberikan oleh pemanggil. */
static int e32_arg_ada(struct baik *baik, int n) {
  return (n >= 0 && n < baik_nargs(baik));
}

int e32_arg_int(struct baik *baik, int n, int def) {
  baik_val_t v;
  double d;

  if (!e32_arg_ada(baik, n)) return def;
  v = baik_arg(baik, n);

  /* Boolean diterima: benar = 1, salah = 0. */
  if (baik_is_boolean(v)) return baik_get_bool(baik, v) ? 1 : 0;

  if (!baik_is_number(v)) return def; /* tipe salah -> nilai bawaan */

  d = baik_get_double(baik, v);
  if (isnan(d)) return def;

  /* Jepit ke rentang int supaya tidak terjadi konversi tak terdefinisi. */
  if (d >= 2147483647.0) return 2147483647;
  if (d <= -2147483648.0) return (-2147483647 - 1);
  return (int) d;
}

double e32_arg_num(struct baik *baik, int n, double def) {
  baik_val_t v;
  double d;

  if (!e32_arg_ada(baik, n)) return def;
  v = baik_arg(baik, n);

  if (baik_is_boolean(v)) return baik_get_bool(baik, v) ? 1.0 : 0.0;
  if (!baik_is_number(v)) return def;

  d = baik_get_double(baik, v);
  if (isnan(d)) return def;
  return d;
}

int e32_arg_bool(struct baik *baik, int n, int def) {
  baik_val_t v;
  double d;

  if (!e32_arg_ada(baik, n)) return def;
  v = baik_arg(baik, n);

  if (baik_is_boolean(v)) return baik_get_bool(baik, v) ? 1 : 0;

  if (baik_is_number(v)) {
    d = baik_get_double(baik, v);
    return (!isnan(d) && d != 0.0) ? 1 : 0;
  }

  if (baik_is_string(v)) {
    size_t len = 0;
    (void) baik_get_string(baik, &v, &len);
    return len > 0 ? 1 : 0;
  }

  if (baik_is_null(v)) return 0;

  return def;
}

/*
 * Cincin slot nilai untuk e32_arg_str().
 *
 * BAIK menyimpan string yang panjangnya <= 5 bita DI DALAM nilai baik_val_t
 * itu sendiri (tag BAIK_TAG_STRING_I dan BAIK_TAG_STRING_5; lihat
 * baik_get_string() pada src/baik.c baris ~11520 yang mengembalikan
 * GET_VAL_NAN_PAYLOAD(*v), yakni pointer ke dalam nilai itu sendiri).
 *
 * Akibatnya implementasi naif berikut RUSAK untuk string pendek
 * ("on", "/a", "ssid", "wifi") karena mengembalikan pointer ke stack yang
 * sudah dibongkar:
 *
 *     baik_val_t v = baik_arg(baik, n);
 *     return baik_get_cstring(baik, &v);   // pointer ke variabel lokal!
 *
 * Solusi: nilai argumen disalin dulu ke salah satu slot statis di bawah ini,
 * lalu baik_get_cstring() dipanggil pada ALAMAT SLOT tersebut. Slot statis
 * hidup selama program, jadi pointer yang dikembalikan tidak pernah menunjuk
 * ke stack. Ukuran cincin 8 supaya fungsi dengan banyak argumen string
 * (mis. WiFi.softAP(ssid, sandi, kanal), Wire.writeReg) tetap aman.
 */
#define E32_STR_SLOTS 8
static baik_val_t e32_str_slot[E32_STR_SLOTS];
static unsigned   e32_str_next = 0;

/*
 * PERINGATAN LIFETIME (WAJIB DIBACA MODUL LAIN):
 *
 *   Pointer yang dikembalikan e32_arg_str() valid sampai 8 (E32_STR_SLOTS)
 *   pemanggilan e32_arg_str() berikutnya; sesudah itu slotnya dipakai ulang.
 *   Untuk string panjang pointer menunjuk ke dalam buffer string milik
 *   interpreter (baik->owned_strings), jadi ia juga hanya sah SELAMA
 *   pemanggilan fungsi native yang sedang berjalan.
 *
 *   JANGAN menyimpan pointer ini di variabel statis/global, JANGAN memakainya
 *   setelah fungsi native selesai, dan JANGAN memakainya lagi setelah kode
 *   Anda memanggil API BAIK yang bisa mengalokasikan string/objek baru
 *   (mis. baik_mk_string, baik_mk_object, baik_set, e32_ret_str) karena
 *   buffer string interpreter bisa direalokasi/dipadatkan oleh GC.
 *
 *   Bila isinya perlu bertahan, SALIN dulu (strdup(), strncpy() ke buffer
 *   sendiri, atau String pada Arduino).
 */
const char *e32_arg_str(struct baik *baik, int n, const char *def) {
  baik_val_t *slot;
  const char *s;

  if (!e32_arg_ada(baik, n)) return def;

  /* Simpan nilai di slot statis; alamat slot inilah yang dipakai supaya
   * string pendek (yang tersimpan inline di dalam nilai) tidak menggantung. */
  slot = &e32_str_slot[e32_str_next];
  *slot = baik_arg(baik, n);

  if (!baik_is_string(*slot)) {
    *slot = baik_mk_undefined();
    return def; /* tipe salah -> nilai bawaan */
  }

  s = baik_get_cstring(baik, slot);
  if (s == NULL) {
    *slot = baik_mk_undefined();
    return def; /* string tidak berakhiran NUL */
  }

  e32_str_next = (e32_str_next + 1) % E32_STR_SLOTS;
  return s;
}

/* ======================================================================== *
 *  Nilai balik
 * ======================================================================== */

void e32_ret_int(struct baik *baik, long v) {
  baik_return(baik, baik_mk_number(baik, (double) v));
}

void e32_ret_num(struct baik *baik, double v) {
  baik_return(baik, baik_mk_number(baik, v));
}

void e32_ret_bool(struct baik *baik, int v) {
  baik_return(baik, baik_mk_boolean(baik, v ? 1 : 0));
}

void e32_ret_str(struct baik *baik, const char *s) {
  if (s == NULL) {
    baik_return(baik, baik_mk_null());
    return;
  }
  /* copy = 1 -> interpreter menyalin isinya, pemanggil boleh membebaskan s. */
  baik_return(baik, baik_mk_string(baik, s, strlen(s), 1));
}

void e32_ret_undef(struct baik *baik) {
  baik_return(baik, baik_mk_undefined());
}

/* ======================================================================== *
 *  Pendaftaran namespace / fungsi / konstanta
 * ======================================================================== */

baik_val_t e32_ns(struct baik *baik, baik_val_t g, const char *name) {
  baik_val_t ns = baik_get(baik, g, name, (size_t) ~0);

  /* Sudah ada dan bertipe object -> pakai yang itu, jangan ditimpa supaya
   * modul yang mendaftar belakangan bisa menambah properti ke namespace
   * yang sama (mis. "ESP" diisi oleh modul sistem lalu modul lain). */
  if (baik_is_object(ns)) return ns;

  ns = baik_mk_object(baik);
  /* Dipasang ke object global; global adalah akar GC sehingga namespace ini
   * otomatis terjaga -- tidak perlu baik_own(). */
  baik_set(baik, g, name, (size_t) ~0, ns);
  return ns;
}

void e32_fn(struct baik *baik, baik_val_t obj, const char *name,
            void (*fn)(struct baik *)) {
  baik_set(baik, obj, name, (size_t) ~0,
           baik_mk_foreign_func(baik, (baik_func_ptr_t) fn));
}

void e32_const(struct baik *baik, baik_val_t obj, const char *name, double v) {
  baik_set(baik, obj, name, (size_t) ~0, baik_mk_number(baik, v));
}

/* ======================================================================== *
 *  Validasi pin
 * ======================================================================== */

/* ADC1 dan ADC2 adalah pilihan "salah satu": pin ADC manapun sudah cukup. */
#define E32_CAP_ADC_ANY (E32_CAP_ADC1 | E32_CAP_ADC2)

/* Seluruh bit kapabilitas yang dikenal (E32_CAP_DIGITAL .. E32_CAP_LED). */
#define E32_CAP_BIT_TERTINGGI 17

/*
 * 1 bila pin `p` memenuhi permintaan kapabilitas `caps`.
 *
 * Aturan khusus:
 *   - Bila `caps` meminta E32_CAP_ADC (ADC1|ADC2), cukup punya salah satu.
 *   - Pin SPI flash/PSRAM (E32_CAP_FLASH) SELALU ditolak, kecuali pemanggil
 *     memang secara eksplisit menyertakan E32_CAP_FLASH pada `caps`.
 */
static int e32_pin_cocok(const e32_pin_info_t *p, uint32_t caps) {
  uint32_t butuh = caps;

  if (p == NULL) return 0;

  if ((p->caps & E32_CAP_FLASH) && !(caps & E32_CAP_FLASH)) return 0;

  if ((butuh & E32_CAP_ADC_ANY) == E32_CAP_ADC_ANY) {
    if (!(p->caps & E32_CAP_ADC_ANY)) return 0;
    butuh &= ~((uint32_t) E32_CAP_ADC_ANY);
  }

  return (butuh & ~p->caps) == 0;
}

/* Hitung kapabilitas yang KURANG dengan aturan yang sama seperti di atas. */
static uint32_t e32_pin_kurang(const e32_pin_info_t *p, uint32_t caps) {
  uint32_t butuh = caps;
  uint32_t kurang = 0;

  if ((butuh & E32_CAP_ADC_ANY) == E32_CAP_ADC_ANY) {
    if (!(p->caps & E32_CAP_ADC_ANY)) kurang |= (uint32_t) E32_CAP_ADC_ANY;
    butuh &= ~((uint32_t) E32_CAP_ADC_ANY);
  }

  kurang |= (butuh & ~p->caps);
  return kurang;
}

/* Lebar terminal yang diasumsikan saat membungkus baris pesan. */
#define E32_LEBAR 78

/*
 * Cetak `teks` dengan pembungkusan kata: baris pertama diawali `awal`,
 * baris lanjutan diawali `lanjut`. Menjaga pesan galat tetap muat di
 * terminal 80 kolom.
 */
static void e32_cetak_bungkus(const char *awal, const char *lanjut,
                              const char *teks) {
  size_t n = strlen(teks);
  size_t i = 0;
  size_t kol;
  const char *indent = awal;

  printf("%s", awal);
  kol = strlen(awal);

  while (i < n) {
    size_t j = i;
    size_t panjang;

    while (j < n && teks[j] != ' ') j++;
    panjang = j - i;

    if (kol > strlen(indent) && kol + 1 + panjang > E32_LEBAR) {
      indent = lanjut;
      printf("\n%s", indent);
      kol = strlen(indent);
    } else if (kol > strlen(indent)) {
      printf(" ");
      kol++;
    }

    printf("%.*s", (int) panjang, teks + i);
    kol += panjang;

    i = j;
    while (i < n && teks[i] == ' ') i++;
  }

  printf("\n");
}

/* Tulis daftar nama kapabilitas (dipisah koma) ke `buf`. */
static void e32_caps_ke_buf(uint32_t caps, char *buf, size_t buflen) {
  int i;
  int pertama = 1;

  if (buflen == 0) return;
  buf[0] = '\0';

  /* ADC1|ADC2 ditulis sebagai satu kata "ADC". */
  if ((caps & E32_CAP_ADC_ANY) == E32_CAP_ADC_ANY) {
    strncat(buf, "ADC", buflen - strlen(buf) - 1);
    pertama = 0;
    caps &= ~((uint32_t) E32_CAP_ADC_ANY);
  }

  for (i = 0; i <= E32_CAP_BIT_TERTINGGI; i++) {
    uint32_t bit = (uint32_t) 1 << i;
    const char *nama;
    if (!(caps & bit)) continue;
    nama = e32_cap_name(bit);
    if (nama == NULL) nama = "?";
    if (!pertama) strncat(buf, ", ", buflen - strlen(buf) - 1);
    strncat(buf, nama, buflen - strlen(buf) - 1);
    pertama = 0;
  }

  if (pertama) strncat(buf, "(tidak ada)", buflen - strlen(buf) - 1);
}

/* Cetak sampai 8 GPIO pertama yang memenuhi `caps`. */
static void e32_cetak_saran(uint32_t caps) {
  size_t jumlah = 0;
  size_t i;
  int ketemu = 0;
  char buf[256];
  const e32_pin_info_t *tbl = e32_pin_table(&jumlah);

  if (tbl == NULL || jumlah == 0) return;

  buf[0] = '\0';
  for (i = 0; i < jumlah && ketemu < 8; i++) {
    char item[24];
    if (!e32_pin_cocok(&tbl[i], caps)) continue;
    snprintf(item, sizeof(item), "%sGPIO%d", ketemu ? ", " : "",
             (int) tbl[i].gpio);
    strncat(buf, item, sizeof(buf) - strlen(buf) - 1);
    ketemu++;
  }

  if (ketemu == 0) {
    e32_cetak_bungkus("       ", "       ",
                      "Tidak ada pin pada papan ini yang memenuhi.");
    return;
  }

  if (ketemu >= 8) {
    strncat(buf, ", ... (lihat pinout())", sizeof(buf) - strlen(buf) - 1);
  }

  e32_cetak_bungkus("       Saran pin: ", "         ", buf);
}

int e32_pin_ok(struct baik *baik, int gpio, uint32_t caps, const char *fname) {
  const e32_pin_info_t *p;
  uint32_t kurang;
  char pesan[256];
  char capbuf[192];

  (void) baik; /* helper ini hanya mencetak, tidak menyentuh interpreter */

  if (fname == NULL) fname = "(fungsi)";

  p = e32_pin_find(gpio);
  if (p == NULL) {
    snprintf(pesan, sizeof(pesan),
             "GALAT %s(): GPIO %d tidak ada pada papan %s.", fname, gpio,
             e32_board_name());
    e32_cetak_bungkus("", "       ", pesan);
    e32_cetak_saran(caps);
    e32_cetak_bungkus("       ", "       ",
                      "Ketik pinout() untuk melihat seluruh pin papan ini.");
    return 0;
  }

  /* Pin SPI flash/PSRAM: memakainya membuat papan hang atau gagal boot. */
  if ((p->caps & E32_CAP_FLASH) && !(caps & E32_CAP_FLASH)) {
    snprintf(pesan, sizeof(pesan),
             "GALAT %s(): GPIO %d (%s) terpakai SPI flash/PSRAM internal.",
             fname, gpio, p->label ? p->label : "-");
    e32_cetak_bungkus("", "       ", pesan);
    e32_cetak_bungkus("       ", "       ",
                      "JANGAN dipakai: papan bisa hang atau gagal boot.");
    if (p->note != NULL) {
      snprintf(pesan, sizeof(pesan), "Catatan: %s", p->note);
      e32_cetak_bungkus("       ", "       ", pesan);
    }
    e32_cetak_saran(caps);
    return 0;
  }

  kurang = e32_pin_kurang(p, caps);
  if (kurang != 0) {
    e32_caps_ke_buf(kurang, capbuf, sizeof(capbuf));
    snprintf(pesan, sizeof(pesan),
             "GALAT %s(): GPIO %d (%s) tidak mendukung %s.", fname, gpio,
             p->label ? p->label : "-", capbuf);
    e32_cetak_bungkus("", "       ", pesan);

    capbuf[0] = '\0';
    e32_caps_to_string(p->caps, capbuf, sizeof(capbuf));
    snprintf(pesan, sizeof(pesan), "Kapabilitas GPIO %d: %s", gpio,
             capbuf[0] ? capbuf : "(tidak ada)");
    e32_cetak_bungkus("       ", "         ", pesan);

    if (p->note != NULL) {
      snprintf(pesan, sizeof(pesan), "Catatan: %s", p->note);
      e32_cetak_bungkus("       ", "         ", pesan);
    }
    e32_cetak_saran(caps);
    return 0;
  }

  return 1;
}

/* ======================================================================== *
 *  Tabel pinout
 * ======================================================================== */

#define E32_NOTE_MAX 34

void e32_print_pinout(void) {
  size_t jumlah = 0;
  size_t i;
  const e32_pin_info_t *tbl = e32_pin_table(&jumlah);

  printf("\n");
  printf("=================================================================="
         "============\n");
  printf(" PINOUT PAPAN : %s   |   CHIP : %s\n", e32_board_name(),
         e32_board_chip());
  printf("=================================================================="
         "============\n");

  if (tbl == NULL || jumlah == 0) {
    printf(" (tabel pin kosong)\n\n");
    return;
  }

  printf(" %-4s  %-6s  %-6s  %-5s  %-4s  %-3s  %-3s  %s\n", "GPIO", "LABEL",
         "ADC", "TOUCH", "DAC", "RTC", "PWM", "CATATAN");
  printf(" ----  ------  ------  -----  ----  ---  ---  "
         "----------------------------------\n");

  for (i = 0; i < jumlah; i++) {
    const e32_pin_info_t *p = &tbl[i];
    char adc[16];
    char touch[16];
    char dac[16];
    char rtc[16];
    char note[E32_NOTE_MAX + 1];
    const char *pwm;
    char tanda;

    if (p->adc_unit > 0 && p->adc_channel >= 0) {
      snprintf(adc, sizeof(adc), "%d/CH%d", (int) p->adc_unit,
               (int) p->adc_channel);
    } else if (p->adc_unit > 0) {
      snprintf(adc, sizeof(adc), "%d/-", (int) p->adc_unit);
    } else {
      snprintf(adc, sizeof(adc), "-");
    }

    if (p->touch_channel >= 0) {
      snprintf(touch, sizeof(touch), "T%d", (int) p->touch_channel);
    } else {
      snprintf(touch, sizeof(touch), "-");
    }

    if (p->dac_channel > 0) {
      snprintf(dac, sizeof(dac), "DAC%d", (int) p->dac_channel);
    } else {
      snprintf(dac, sizeof(dac), "-");
    }

    if (p->rtc_gpio >= 0) {
      snprintf(rtc, sizeof(rtc), "%d", (int) p->rtc_gpio);
    } else {
      snprintf(rtc, sizeof(rtc), "-");
    }

    pwm = (p->caps & E32_CAP_PWM) ? "ya" : "-";

    /* Catatan dipotong agar baris tetap muat di terminal 80 kolom. */
    note[0] = '\0';
    if (p->caps & E32_CAP_FLASH) {
      snprintf(note, sizeof(note), "!! FLASH/PSRAM - JANGAN DIPAKAI");
    } else if (p->note != NULL && p->note[0] != '\0') {
      if (strlen(p->note) > E32_NOTE_MAX) {
        /* Dipotong + "..." supaya jelas catatannya belum utuh. */
        strncpy(note, p->note, E32_NOTE_MAX - 3);
        note[E32_NOTE_MAX - 3] = '\0';
        strcat(note, "...");
      } else {
        strncpy(note, p->note, E32_NOTE_MAX);
        note[E32_NOTE_MAX] = '\0';
      }
    } else {
      snprintf(note, sizeof(note), "-");
    }

    tanda = (p->caps & E32_CAP_FLASH) ? '!' : ' ';

    printf("%c%4d  %-6s  %-6s  %-5s  %-4s  %-3s  %-3s  %s\n", tanda,
           (int) p->gpio, p->label ? p->label : "-", adc, touch, dac, rtc, pwm,
           note);
  }

  printf("=================================================================="
         "============\n");
  printf(" Jumlah pin terdaftar : %u\n", (unsigned) jumlah);
  printf(" Keterangan kolom     : ADC = unit/kanal, TOUCH = kanal sentuh,\n");
  printf("                        DAC = kanal DAC, RTC = nomor RTC GPIO,\n");
  printf("                        PWM = bisa dipakai LEDC/analogWrite.\n");
  printf(" Tanda '!' di depan nomor GPIO = pin terpakai SPI flash/PSRAM,\n");
  printf(" JANGAN dipakai karena papan bisa hang atau gagal boot.\n");
  printf(" Pakai pinInfo(gpio), pinCaps(gpio), daftarPin(CAP_ADC1) untuk\n");
  printf(" memeriksa pin dari dalam skrip BAIK.\n");
  printf("=================================================================="
         "============\n\n");
}

/* ======================================================================== *
 *  Ringkasan API
 * ======================================================================== */

/* Cetak daftar nama dalam 3 kolom (maks 78 kolom, muat di terminal 80). */
static void e32_print_group(const char *judul, const char *const *item,
                            size_t n) {
  size_t i;

  printf("\n-- %s\n", judul);
  for (i = 0; i < n; i++) {
    int akhir_baris = ((i % 3) == 2) || (i + 1 == n);
    /* Item terakhir pada satu baris tidak diberi padding supaya tidak ada
     * spasi menggantung di ujung baris. */
    printf("  ");
    if (akhir_baris) {
      printf("%s\n", item[i]);
    } else {
      printf("%-24s", item[i]);
    }
  }
}

#define E32_NITEM(a) (sizeof(a) / sizeof((a)[0]))

void e32_print_api(void) {
  static const char *const gpio_dasar[] = {
      "pinMode",       "digitalWrite",  "digitalRead",  "digitalToggle",
      "modePin",       "tulisDigital",  "bacaDigital",  "balikDigital",
      "pulseIn",       "bacaPulsa",     "shiftOut",     "shiftIn",
      "rgbLedWrite",   "neopixelWrite", "tulisLedRgb",  "ledBawaan",
      "builtinLed"};

  static const char *const gpio_analog[] = {
      "analogRead",           "bacaAnalog",         "analogReadMilliVolts",
      "bacaMiliVolt",         "analogReadResolution", "analogSetAttenuation",
      "analogSetPinAttenuation", "dacWrite",        "tulisDac",
      "dacDisable",           "touchRead",          "bacaSentuh",
      "touchSetCycles",       "touchAttachInterrupt", "touchDetachInterrupt"};

  static const char *const gpio_pwm[] = {
      "analogWrite",       "tulisAnalog",       "analogWriteResolution",
      "analogWriteFrequency", "ledcSetup",      "ledcAttach",
      "ledcAttachPin",     "ledcDetachPin",     "ledcWrite",
      "tulisPwm",          "ledcRead",          "ledcReadFreq",
      "ledcWriteTone",     "ledcWriteNote",     "ledcAttachChannel",
      "ledcDetach",        "ledcChangeFrequency", "tone",
      "bunyi",             "noTone",            "diam"};

  static const char *const gpio_irq[] = {
      "attachInterrupt",  "pasangInterupsi",  "detachInterrupt",
      "lepasInterupsi",   "interrupts",       "noInterrupts",
      "interruptCount",   "jumlahInterupsi",  "serviceInterrupts",
      "layaniInterupsi"};

  static const char *const sistem_waktu[] = {
      "delay",    "tunggu",       "delayMicroseconds", "tungguMikro",
      "millis",   "milidetik",    "micros",            "mikrodetik",
      "yield",    "random",       "acak",              "randomSeed"};

  static const char *const sistem_math[] = {
      "map",   "peta",  "constrain", "batas", "min",   "max",
      "abs",   "pow",   "sqrt",      "sin",   "cos",   "tan",
      "atan",  "atan2", "floor",     "ceil",  "round", "log",
      "log10", "exp"};

  static const char *const sistem_esp[] = {
      "ESP.restart",        "mulaiUlang",         "ESP.getFreeHeap",
      "ESP.getHeapSize",    "ESP.getMinFreeHeap", "ESP.getMaxAllocHeap",
      "ESP.getPsramSize",   "ESP.getFreePsram",   "ESP.getChipModel",
      "ESP.getChipRevision", "ESP.getChipCores",  "ESP.getCpuFreqMHz",
      "ESP.getSdkVersion",  "ESP.getFlashChipSize", "ESP.getEfuseMac",
      "ESP.getSketchSize",  "setCpuFrequencyMhz", "getCpuFrequencyMhz",
      "getXtalFrequencyMhz", "getApbFrequency",   "temperatureRead",
      "bacaSuhu",           "hallRead",           "bacaHall",
      "getTaskCount",       "getTaskHighWaterMark"};

  static const char *const sistem_tidur[] = {
      "sleepTimer",     "sleepExt0",      "sleepExt1",
      "tidurDalam",
      "tidurRingan",    "sebabBangun",    "sebabReset",
      "rtcSet",         "rtcGet",         "watchdogEnable",
      "watchdogReset",  "watchdogDisable"};

  static const char *const serial_api[] = {
      "Serial.begin",   "Serial.print",   "Serial.println",
      "Serial.printf",  "Serial.available", "Serial.read",
      "Serial.readString", "Serial.write", "Serial.flush"};

  static const char *const bus_i2c[] = {
      "Wire.begin",       "Wire.setClock",   "Wire.beginTransmission",
      "Wire.write",       "Wire.endTransmission", "Wire.requestFrom",
      "Wire.available",   "Wire.read",       "Wire.readBytes",
      "Wire.writeTo",     "Wire.readFrom",   "Wire.writeReg",
      "Wire.readReg",     "Wire.scan",       "i2cScan",
      "pindaiI2C",        "Wire.end",        "Wire1.*"};

  static const char *const bus_spi[] = {
      "SPI.begin",        "SPI.end",          "SPI.setFrequency",
      "SPI.setDataMode",  "SPI.setBitOrder",  "SPI.beginTransaction",
      "SPI.endTransaction", "SPI.transfer",   "SPI.transfer16",
      "SPI.transferBytes", "SPI.write",       "SPI.writeBytes"};

  static const char *const bus_uart[] = {
      "Serial1.begin",   "Serial1.available", "Serial1.read",
      "Serial1.write",   "Serial1.print",     "Serial1.println",
      "Serial1.readString", "Serial1.end",    "Serial2.*"};

  static const char *const net_wifi[] = {
      "WiFi.begin",      "WiFi.disconnect", "WiFi.status",
      "WiFi.statusText", "WiFi.isConnected", "WiFi.localIP",
      "WiFi.gatewayIP",  "WiFi.subnetMask", "WiFi.dnsIP",
      "WiFi.macAddress", "WiFi.RSSI",       "WiFi.SSID",
      "WiFi.channel",    "WiFi.mode",       "WiFi.setHostname",
      "WiFi.softAP",     "WiFi.softAPIP",   "WiFi.scanNetworks",
      "WiFi.sleep",      "WiFi.setTxPower", "WiFi.tungguKoneksi"};

  static const char *const net_lain[] = {
      "HTTP.get",     "HTTP.post",     "HTTP.getJSON",
      "HTTP.statusTerakhir", "NTP.begin", "NTP.sync",
      "NTP.epoch",    "NTP.format",    "NTP.jam",
      "NTP.menit",    "NTP.detik",     "NTP.tanggal",
      "NTP.bulan",    "NTP.tahun",     "MDNS.begin",
      "MDNS.addService"};

  static const char *const berkas_fs[] = {
      "FS.begin",     "FS.exists",    "FS.read",
      "FS.write",     "FS.append",    "FS.remove",
      "FS.rename",    "FS.size",      "FS.list",
      "FS.mkdir",     "FS.rmdir",     "FS.totalBytes",
      "FS.usedBytes", "FS.freeBytes", "FS.format",
      "jalankan",     "run",          "muat"};

  static const char *const berkas_nvs[] = {
      "NVS.begin",     "NVS.end",       "NVS.putInt",
      "NVS.getInt",    "NVS.putFloat",  "NVS.getFloat",
      "NVS.putString", "NVS.getString", "NVS.putBool",
      "NVS.getBool",   "NVS.remove",    "NVS.clear",
      "NVS.isKey",     "NVS.freeEntries"};

  static const char *const pin_info[] = {
      "pinInfo",   "pinCaps",   "pinPunya",
      "pinHas",    "daftarPin", "pinList",
      "pinout",    "bantuan",   "versi"};

  static const char *const konstanta[] = {
      "HIGH / LOW",       "INPUT / OUTPUT",   "INPUT_PULLUP",
      "INPUT_PULLDOWN",   "RISING / FALLING", "CHANGE",
      "LED_BUILTIN",      "BOOT_BUTTON",      "SDA / SCL",
      "MOSI / MISO / SCK", "D0..Dn / GPIOn",  "A0..An",
      "T0..Tn",           "PI / TWO_PI",      "LSBFIRST / MSBFIRST",
      "WL_CONNECTED",     "WIFI_STA / WIFI_AP", "SPI_MODE0..3",
      "CAP_ADC1 / CAP_PWM", "BOARD / CHIP",   "ADC_0db..ADC_11db"};

  printf("\n");
  printf("=================================================================="
         "============\n");
  printf(" RINGKASAN API BAIK-ESP32   (papan: %s)\n", e32_board_name());
  printf("=================================================================="
         "============\n");
  printf(" Nama fungsi sama persis dengan Arduino-ESP32; alias bahasa\n");
  printf(" Indonesia menunjuk ke fungsi native yang sama.\n");

  e32_print_group("GPIO digital", gpio_dasar, E32_NITEM(gpio_dasar));
  e32_print_group("ADC / DAC / sentuh", gpio_analog, E32_NITEM(gpio_analog));
  e32_print_group("PWM / LEDC / nada", gpio_pwm, E32_NITEM(gpio_pwm));
  e32_print_group("Interupsi", gpio_irq, E32_NITEM(gpio_irq));
  e32_print_group("Waktu & acak", sistem_waktu, E32_NITEM(sistem_waktu));
  e32_print_group("Matematika", sistem_math, E32_NITEM(sistem_math));
  e32_print_group("Sistem / ESP", sistem_esp, E32_NITEM(sistem_esp));
  e32_print_group("Tidur & watchdog", sistem_tidur, E32_NITEM(sistem_tidur));
  e32_print_group("Serial (konsol)", serial_api, E32_NITEM(serial_api));
  e32_print_group("I2C (Wire)", bus_i2c, E32_NITEM(bus_i2c));
  e32_print_group("SPI", bus_spi, E32_NITEM(bus_spi));
  e32_print_group("UART tambahan", bus_uart, E32_NITEM(bus_uart));
  e32_print_group("WiFi", net_wifi, E32_NITEM(net_wifi));
  e32_print_group("HTTP / NTP / mDNS", net_lain, E32_NITEM(net_lain));
  e32_print_group("Berkas (SPIFFS)", berkas_fs, E32_NITEM(berkas_fs));
  e32_print_group("Penyimpanan (NVS)", berkas_nvs, E32_NITEM(berkas_nvs));
  e32_print_group("Pinout & bantuan", pin_info, E32_NITEM(pin_info));
  e32_print_group("Konstanta", konstanta, E32_NITEM(konstanta));

  printf("\n");
  printf("=================================================================="
         "============\n");
  printf(" pinout()  -> tabel pin papan ini      versi()  -> versi firmware\n");
  /* Hanya kata kunci yang BENAR-BENAR jalan. `var` dan `kerjakan` dikenali
   * lexer tetapi ditolak parser (lihat parse_statement di src/baik.c), jadi
   * mencantumkannya di sini justru mengajarkan sintaks yang gagal. */
  printf(" Kata kunci: isi fungsi balik jika lainnya untuk ulang berhenti\n");
  printf(" teruskan benar salah kosong takterdefinisi tipe   (deklarasi: isi)\n");
  printf("=================================================================="
         "============\n\n");
}
