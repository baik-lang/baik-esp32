/*
 * e32_shim.h - Tiruan (shim) API ESP32 untuk dijalankan di PC.
 *
 * Tujuan: skrip .ina yang SAMA bisa dijalankan di PC (uji otomatis) maupun di
 * papan ESP32 sungguhan. Nama fungsi dan semantiknya mengikuti
 * scratchpad/API-SPEC.md, jadi `pinMode`, `digitalWrite`, `millis`, dst.
 * tersedia dengan arti yang sama.
 *
 * Perbedaannya: di PC tidak ada perangkat keras, jadi shim ini
 *   - mencatat setiap aksi ke stdout dalam format yang mudah dibandingkan,
 *   - menyimpan state pin virtual di memori,
 *   - memakai JAM VIRTUAL (millis/micros hanya maju bila delay dipanggil)
 *     dan PRNG deterministik, supaya keluaran uji selalu sama persis.
 *
 * BERKAS INI MANDIRI: ia TIDAK menyertakan apa pun dari src/esp32/ (yang
 * butuh Arduino.h). Tabel pin di sini adalah salinan ringkas untuk PC saja.
 */
#ifndef BAIK_E32_SHIM_H_
#define BAIK_E32_SHIM_H_

#include "../../../src/baik.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ---- Deklarasi object API BAIK ------------------------------------------
 * Fungsi-fungsi ini TERDEFINISI di src/baik/ (baik_object.c dsb.) tetapi
 * blok deklarasinya
 * dikomentari di src/baik.h. Kita deklarasikan ulang di sini (sama seperti
 * yang dilakukan src/esp32/baik_esp32.h) supaya shim bisa memakainya tanpa
 * mengubah berkas inti bahasa BAIK.
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

/* Daftarkan seluruh shim ESP32 ke object global interpreter BAIK. */
void baik_shim_register(struct baik *baik);

/* Kembalikan state shim ke kondisi awal (jam virtual = 0, semua pin LOW). */
void baik_shim_reset(void);

#ifdef __cplusplus
}
#endif
#endif /* BAIK_E32_SHIM_H_ */
