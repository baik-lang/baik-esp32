/*
 * runner.c - Penjalan (runner) skrip BAIK di PC, untuk keperluan pengujian.
 *
 * Kenapa runner sendiri dan bukan blok BAIK_MAIN di src/baik/baik_repl.c?
 * Karena BAIK_MAIN mengaktifkan REPL berbasis GNU readline (readline(),
 * add_history(), rl_bind_key(), ...). Itu menambah ketergantungan pustaka
 * yang tidak perlu untuk uji otomatis, dan REPL-nya interaktif sehingga
 * tidak cocok dipakai di skrip uji. Jadi kita tulis main() sendiri di sini
 * dan HANYA menautkan modul-modul inti di src/baik/ (tanpa -DBAIK_MAIN).
 *
 * Pemakaian:
 *   baik-host [opsi] <berkas.ina> [berkas.ina ...]
 *   baik-host [opsi] -e "<ekspresi>"
 *
 * Opsi:
 *   -e <kode>   Eksekusi potongan kode langsung.
 *   -n          JANGAN daftarkan shim ESP32 (uji bahasa murni).
 *   -q          Jangan cetak nilai balik skrip.
 *   -h          Bantuan.
 *
 * Kode keluar:
 *   0  semua berkas berhasil dieksekusi
 *   1  terjadi galat saat eksekusi (pesan galat dicetak ke stdout)
 *   2  pemakaian salah / berkas tidak ada
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../../src/baik.h"
#include "shim/e32_shim.h"

static void bantuan(const char *prog) {
  printf("Penjalan skrip BAIK untuk PC (uji host)\n");
  printf("Pakai: %s [opsi] <berkas.ina> [berkas.ina ...]\n", prog);
  printf("       %s [opsi] -e \"<ekspresi>\"\n", prog);
  printf("Opsi:\n");
  printf("  -e <kode>  Eksekusi potongan kode BAIK langsung\n");
  printf("  -n         Tanpa shim ESP32 (uji bahasa murni)\n");
  printf("  -q         Jangan cetak nilai balik skrip\n");
  printf("  -h         Tampilkan bantuan ini\n");
}

int main(int argc, char *argv[]) {
  struct baik *baik;
  baik_err_t err = BAIK_OK;
  /* CATATAN: makro BAIK_UNDEFINED di src/baik.h mengembang menjadi
   * BAIK_TAG_UNDEFINED yang hanya terdefinisi di src/baik/baik_internal.h
   * (header internal), sehingga makro itu tidak bisa dipakai dari luar. Kita
   * pakai 0 sebagai penanda "belum diisi", sama seperti yang dilakukan blok
   * BAIK_MAIN di src/baik/baik_repl.c. */
  baik_val_t res = 0;
  int i, pakai_shim = 1, diam = 0, ada_kerja = 0, rc = 0;

  if (argc < 2) { bantuan(argv[0]); return 2; }

  /* Pindai opsi yang mempengaruhi cara interpreter dibuat. */
  for (i = 1; i < argc; i++) {
    if (strcmp(argv[i], "-n") == 0) pakai_shim = 0;
    else if (strcmp(argv[i], "-q") == 0) diam = 1;
    else if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
      bantuan(argv[0]);
      return 0;
    }
  }

  baik = baik_create();
  if (baik == NULL) {
    fprintf(stderr, "GALAT: gagal membuat interpreter BAIK\n");
    return 1;
  }

  if (pakai_shim) baik_shim_register(baik);

  for (i = 1; i < argc && err == BAIK_OK; i++) {
    if (strcmp(argv[i], "-n") == 0 || strcmp(argv[i], "-q") == 0) {
      continue;                              /* sudah diproses di atas */
    } else if (strcmp(argv[i], "-e") == 0) {
      if (i + 1 >= argc) {
        fprintf(stderr, "GALAT: -e butuh argumen\n");
        baik_destroy(baik);
        return 2;
      }
      ada_kerja = 1;
      err = baik_exec(baik, argv[++i], &res);
    } else if (argv[i][0] == '-' && argv[i][1] != '\0') {
      fprintf(stderr, "GALAT: opsi tidak dikenal: %s\n", argv[i]);
      baik_destroy(baik);
      return 2;
    } else {
      FILE *fp = fopen(argv[i], "r");
      if (fp == NULL) {
        fprintf(stderr, "GALAT: berkas tidak bisa dibuka: %s\n", argv[i]);
        baik_destroy(baik);
        return 2;
      }
      fclose(fp);
      ada_kerja = 1;
      err = baik_exec_file(baik, argv[i], &res);
    }
  }

  if (!ada_kerja) { bantuan(argv[0]); baik_destroy(baik); return 2; }

  if (err != BAIK_OK) {
    /* Cetak galat ke stdout supaya ikut terbandingkan dengan .expected. */
    baik_print_error(baik, stdout, "GALAT EKSEKUSI", 1);
    rc = 1;
  } else if (!diam && res != 0 && !baik_is_undefined(res)) {
    char buf[512];
    baik_sprintf(res, baik, buf, sizeof(buf));
    printf("=> %s\n", buf);
  }

  fflush(stdout);
  baik_destroy(baik);
  return rc;
}
