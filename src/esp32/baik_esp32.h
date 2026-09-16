/*
 * baik_esp32.h - Jembatan (binding) antara bahasa BAIK dan API Arduino-ESP32.
 *
 * Satu pintu masuk: panggil baik_esp32_register(baik) sekali setelah
 * baik_create(), maka seluruh fungsi & konstanta ESP32 tersedia di skrip BAIK.
 */
#ifndef BAIK_ESP32_H_
#define BAIK_ESP32_H_

#include "../baik.h"
#include "e32_pins.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ---- Deklarasi object API BAIK -----------------------------------------
 * Fungsi-fungsi ini TERDEFINISI di src/baik.c tetapi blok deklarasinya
 * dikomentari di src/baik.h. Kita deklarasikan ulang di sini supaya modul
 * ESP32 bisa memakainya tanpa perlu mengubah berkas inti bahasa BAIK.
 */
int        baik_is_object(baik_val_t v);
baik_val_t baik_mk_object(struct baik *baik);
baik_val_t baik_get(struct baik *baik, baik_val_t obj, const char *name,
                    size_t name_len);
baik_val_t baik_get_v(struct baik *baik, baik_val_t obj, baik_val_t name);
baik_err_t baik_set(struct baik *baik, baik_val_t obj, const char *name,
                    size_t len, baik_val_t val);
baik_err_t baik_set_v(struct baik *baik, baik_val_t obj, baik_val_t name,
                      baik_val_t val);
int        baik_del(struct baik *baik, baik_val_t obj, const char *name,
                    size_t len);
baik_val_t baik_next(struct baik *baik, baik_val_t obj, baik_val_t *iterator);

/* Daftarkan SEMUA modul ESP32 ke object global interpreter BAIK. */
void baik_esp32_register(struct baik *baik);

/* ---- Registrasi per modul (dipanggil oleh baik_esp32_register) ---------- */
void baik_esp32_register_konstanta(struct baik *baik, baik_val_t g); /* e32_konstanta.cpp */
void baik_esp32_register_gpio(struct baik *baik, baik_val_t g);      /* e32_gpio.cpp      */
void baik_esp32_register_sistem(struct baik *baik, baik_val_t g);    /* e32_sistem.cpp    */
void baik_esp32_register_bus(struct baik *baik, baik_val_t g);       /* e32_bus.cpp       */
void baik_esp32_register_jaringan(struct baik *baik, baik_val_t g);  /* e32_jaringan.cpp  */
void baik_esp32_register_berkas(struct baik *baik, baik_val_t g);    /* e32_berkas.cpp    */

/* ---- Helper bersama (e32_helper.cpp) ------------------------------------ */

/* Ambil argumen ke-n sebagai int/double/bool/string, dengan nilai bawaan
 * bila argumen tidak diberikan atau bertipe salah. */
int         e32_arg_int(struct baik *baik, int n, int def);
double      e32_arg_num(struct baik *baik, int n, double def);
int         e32_arg_bool(struct baik *baik, int n, int def);
const char *e32_arg_str(struct baik *baik, int n, const char *def);

/* Kembalikan nilai ke skrip BAIK. */
void e32_ret_int(struct baik *baik, long v);
void e32_ret_num(struct baik *baik, double v);
void e32_ret_bool(struct baik *baik, int v);
void e32_ret_str(struct baik *baik, const char *s);
void e32_ret_undef(struct baik *baik);

/* Buat/ambil sub-object (namespace) pada global, mis. "WiFi", "Wire", "ESP". */
baik_val_t e32_ns(struct baik *baik, baik_val_t g, const char *name);

/* Daftarkan satu fungsi native pada object `obj`. */
void e32_fn(struct baik *baik, baik_val_t obj, const char *name,
            void (*fn)(struct baik *));

/* Daftarkan satu konstanta angka pada object `obj`. */
void e32_const(struct baik *baik, baik_val_t obj, const char *name, double v);

/* Validasi pin. Bila pin tidak ada / tidak punya kapabilitas `caps`, cetak
 * pesan galat berbahasa Indonesia yang menyebut nama fungsi dan pin alternatif,
 * lalu kembalikan 0. Kembalikan 1 bila pin valid. */
int e32_pin_ok(struct baik *baik, int gpio, uint32_t caps, const char *fname);

/* Cetak tabel pinout papan ini ke stdout (dipakai perintah konsol `pinout`). */
void e32_print_pinout(void);

/* Cetak ringkasan seluruh API BAIK-ESP32 (dipakai perintah konsol `api`). */
void e32_print_api(void);

/* Jalankan callback interupsi/sentuh yang tertunda. Dipanggil dari task
 * konsol (BUKAN dari ISR) setiap kali REPL idle. Diimplementasikan di
 * e32_gpio.cpp. */
void baik_esp32_poll_interrupts(struct baik *baik);

#ifdef __cplusplus
}
#endif
#endif /* BAIK_ESP32_H_ */
