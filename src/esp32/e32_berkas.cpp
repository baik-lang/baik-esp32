/*
 * e32_berkas.cpp - Modul 6: BERKAS & PENYIMPANAN untuk bahasa BAIK di ESP32.
 *
 * Mendaftarkan:
 *   - object  FS   (alias Berkas) : sistem berkas SPIFFS
 *   - object  NVS  (alias Simpan) : penyimpanan kunci-nilai (Preferences/NVS)
 *   - fungsi  jalankan(path) (alias run) : jalankan berkas skrip .ina dari SPIFFS
 *
 * Catatan umum:
 *   - Bahasa BAIK tidak punya exception. Semua galat dilaporkan lewat nilai
 *     balik (salah / -1 / kosong) ditambah pesan berbahasa Indonesia ke konsol.
 *   - Semua simbol internal berkas ini `static` supaya tidak bentrok dengan
 *     modul lain.
 */

#include <Arduino.h>
#include <FS.h>
#include <SPIFFS.h>
#include <Preferences.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "baik_esp32.h"

/* ------------------------------------------------------------------ *
 * Konstanta & keadaan modul
 * ------------------------------------------------------------------ */

/* SPIFFS membatasi nama berkas 31 karakter + NUL (SPIFFS_OBJ_NAME_LEN = 32),
 * sudah termasuk garis miring di depan. */
#define E32_PATH_MAX 32

/* NVS membatasi nama kunci DAN nama namespace 15 karakter + NUL
 * (NVS_KEY_NAME_MAX_SIZE = 16). */
#define E32_NVS_NAME_MAX 16

/* Batas rekursi `jalankan()` supaya skrip yang memanggil dirinya sendiri
 * tidak menghabiskan stack task konsol. */
#define E32_JALANKAN_MAKS_DALAM 8

/* Satu-satunya instance Preferences milik skrip BAIK. Firmware punya
 * instance-nya sendiri di main.cpp (namespace "wifi"); keduanya TIDAK boleh
 * saling mengganggu. */
static Preferences s_prefs;
static bool        s_prefs_terbuka = false;

/* Penanda SPIFFS sudah di-mount. main.cpp biasanya sudah memanggil
 * SPIFFS.begin(true) saat boot, tetapi kita tetap siap me-mount sendiri. */
static bool s_fs_siap = false;

/* Penghitung kedalaman rekursi jalankan(). */
static int s_dalam_jalankan = 0;

/* ------------------------------------------------------------------ *
 * Helper galat
 * ------------------------------------------------------------------ */

/* Cetak galat berbahasa Indonesia ke konsol (stdout REPL). */
static void bk_galat(const char *fname, const char *pesan) {
  printf("GALAT %s: %s\r\n", fname, pesan);
}

/* ------------------------------------------------------------------ *
 * Normalisasi path
 * ------------------------------------------------------------------ *
 *
 * Aturan (dipakai SEMUA fungsi FS.* dan jalankan()):
 *   1. Prefiks mount point "/spiffs" dibuang: "/spiffs/data.txt" -> "/data.txt".
 *      Ini supaya path gaya POSIX (yang dipakai builtin `muat()` dan perintah
 *      konsol `cat`/`ls`) bisa dipakai apa adanya di API FS.*.
 *      Bentuk tanpa garis miring depan, "spiffs/data.txt", juga dibuang.
 *   2. Bila hasilnya tidak diawali "/", garis miring ditambahkan di depan:
 *      "data.txt" -> "/data.txt".
 *   3. Garis miring di ujung dibuang, kecuali untuk akar: "/sub/" -> "/sub",
 *      tetapi "/" tetap "/".
 *   4. Path kosong dianggap akar "/".
 *   5. Hasil akhir wajib muat dalam E32_PATH_MAX (31 karakter + NUL); bila
 *      lebih panjang dianggap galat karena SPIFFS pasti menolaknya.
 *
 * Mengembalikan 1 bila berhasil, 0 bila gagal.
 */
static int bk_norm_path(const char *in, char *out, size_t outsz) {
  const char *p = in;
  int n;
  size_t len;

  if (p == NULL || out == NULL || outsz < 2) return 0;

  /* Aturan 1: buang prefiks mount point. */
  if (strncmp(p, "/spiffs", 7) == 0 && (p[7] == '\0' || p[7] == '/')) {
    p += 7;
  } else if (strncmp(p, "spiffs/", 7) == 0) {
    p += 6; /* sisakan garis miringnya: "spiffs/x" -> "/x" */
  }

  /* Aturan 4 */
  if (*p == '\0') {
    out[0] = '/';
    out[1] = '\0';
    return 1;
  }

  /* Aturan 2 */
  if (*p == '/') {
    n = snprintf(out, outsz, "%s", p);
  } else {
    n = snprintf(out, outsz, "/%s", p);
  }
  /* Aturan 5 */
  if (n < 0 || (size_t) n >= outsz) return 0;

  /* Aturan 3 */
  len = strlen(out);
  while (len > 1 && out[len - 1] == '/') {
    len--;
    out[len] = '\0';
  }
  return 1;
}

/*
 * Ambil argumen ke-n sebagai path, lalu normalkan ke `out`.
 * Mengembalikan 1 bila valid, 0 bila tidak (pesan galat sudah dicetak).
 *
 * PENTING: hasil e32_arg_str() menunjuk ke memori internal interpreter yang
 * bisa berpindah begitu nilai BAIK baru dibuat, jadi isinya LANGSUNG disalin
 * ke buffer milik pemanggil di sini.
 */
static int bk_arg_path(struct baik *baik, int n, char *out, size_t outsz,
                       const char *fname) {
  const char *mentah = e32_arg_str(baik, n, NULL);

  if (mentah == NULL || mentah[0] == '\0') {
    bk_galat(fname, "argumen path harus berupa teks yang tidak kosong");
    return 0;
  }
  if (!bk_norm_path(mentah, out, outsz)) {
    printf("GALAT %s: path \"%s\" tidak sah atau melebihi %d karakter "
           "(batas SPIFFS)\r\n",
           fname, mentah, (int) (E32_PATH_MAX - 1));
    return 0;
  }
  return 1;
}

/* ------------------------------------------------------------------ *
 * Helper SPIFFS
 * ------------------------------------------------------------------ */

/* Pastikan SPIFFS ter-mount. Aman dipanggil berkali-kali: SPIFFS.begin()
 * langsung mengembalikan true bila partisi sudah ter-mount. */
static bool bk_fs_siap(const char *fname) {
  if (!s_fs_siap) {
    s_fs_siap = SPIFFS.begin(false);
  }
  if (!s_fs_siap) {
    bk_galat(fname, "SPIFFS belum ter-mount; panggil FS.begin(benar) untuk "
                    "memformat dan me-mount");
  }
  return s_fs_siap;
}

/* Batas aman ukuran berkas yang boleh dibaca ke RAM: setengah sisa heap.
 * Tujuannya supaya membaca berkas besar tidak membuat sistem kehabisan
 * memori (heap masih perlu untuk string BAIK hasil pembacaan). */
static size_t bk_batas_baca(void) {
  return (size_t) (ESP.getFreeHeap() / 2);
}

/*
 * Baca seluruh isi berkas ke buffer malloc() baru (selalu NUL-terminated).
 * Mengembalikan buffer (pemanggil wajib free()) atau NULL bila gagal;
 * `*out_len` diisi jumlah bita yang benar-benar terbaca.
 * File yang dibuka SELALU ditutup, termasuk pada jalur galat.
 */
static char *bk_baca_berkas(const char *path, size_t *out_len,
                            const char *fname) {
  File   f;
  size_t ukuran, batas, dibaca;
  char  *buf;

  if (out_len != NULL) *out_len = 0;

  f = SPIFFS.open(path, "r");
  if (!f) {
    printf("GALAT %s: berkas \"%s\" tidak dapat dibuka\r\n", fname, path);
    return NULL;
  }
  if (f.isDirectory()) {
    f.close();
    printf("GALAT %s: \"%s\" adalah direktori, bukan berkas\r\n", fname, path);
    return NULL;
  }

  ukuran = (size_t) f.size();
  batas  = bk_batas_baca();
  if (ukuran + 1 >= batas) {
    f.close();
    printf("GALAT %s: berkas \"%s\" berukuran %u bita, melebihi batas aman "
           "%u bita (setengah sisa heap). Baca sebagian saja atau bebaskan "
           "memori dulu.\r\n",
           fname, path, (unsigned) ukuran, (unsigned) batas);
    return NULL;
  }

  buf = (char *) malloc(ukuran + 1);
  if (buf == NULL) {
    f.close();
    printf("GALAT %s: gagal mengalokasikan %u bita untuk isi \"%s\"\r\n",
           fname, (unsigned) (ukuran + 1), path);
    return NULL;
  }

  dibaca = (ukuran > 0) ? f.read((uint8_t *) buf, ukuran) : 0;
  f.close(); /* ditutup sebelum apa pun yang lain */
  buf[dibaca] = '\0';
  if (out_len != NULL) *out_len = dibaca;
  return buf;
}

/* Tulis/tambah isi ke berkas. `mode` = "w" (timpa) atau "a" (tambah). */
static void bk_fs_tulis_umum(struct baik *baik, const char *mode,
                             const char *fname) {
  char        path[E32_PATH_MAX];
  const char *isi;
  size_t      panjang, ditulis;
  File        f;

  if (!bk_fs_siap(fname)) {
    e32_ret_bool(baik, 0);
    return;
  }
  if (!bk_arg_path(baik, 0, path, sizeof(path), fname)) {
    e32_ret_bool(baik, 0);
    return;
  }

  /* Ambil isi SETELAH path disalin, dan jangan membuat nilai BAIK baru
   * sebelum isi selesai ditulis (pointer bisa berpindah). */
  isi = e32_arg_str(baik, 1, NULL);
  if (isi == NULL) {
    bk_galat(fname, "argumen kedua (isi) harus berupa teks");
    e32_ret_bool(baik, 0);
    return;
  }
  panjang = strlen(isi);

  f = SPIFFS.open(path, mode);
  if (!f) {
    printf("GALAT %s: berkas \"%s\" tidak dapat dibuka untuk ditulis\r\n",
           fname, path);
    e32_ret_bool(baik, 0);
    return;
  }
  if (f.isDirectory()) {
    f.close();
    printf("GALAT %s: \"%s\" adalah direktori\r\n", fname, path);
    e32_ret_bool(baik, 0);
    return;
  }

  ditulis = (panjang > 0) ? f.write((const uint8_t *) isi, panjang) : 0;
  f.close(); /* selalu ditutup */

  if (ditulis != panjang) {
    printf("GALAT %s: hanya %u dari %u bita yang tertulis ke \"%s\" "
           "(penyimpanan penuh?)\r\n",
           fname, (unsigned) ditulis, (unsigned) panjang, path);
    e32_ret_bool(baik, 0);
    return;
  }
  e32_ret_bool(baik, 1);
}

/* ------------------------------------------------------------------ *
 * FS.* — sistem berkas SPIFFS
 * ------------------------------------------------------------------ */

/* FS.begin([format]) -> boolean. `format` benar = format otomatis bila
 * partisi gagal di-mount. */
static void bk_fs_begin(struct baik *baik) {
  int format = e32_arg_bool(baik, 0, 0);

  s_fs_siap = SPIFFS.begin(format != 0);
  if (!s_fs_siap) {
    bk_galat("FS.begin", "SPIFFS gagal di-mount; coba FS.begin(benar) untuk "
                         "memformat partisi");
  }
  e32_ret_bool(baik, s_fs_siap ? 1 : 0);
}

/* FS.exists(path) -> boolean */
static void bk_fs_exists(struct baik *baik) {
  char path[E32_PATH_MAX];

  if (!bk_fs_siap("FS.exists") ||
      !bk_arg_path(baik, 0, path, sizeof(path), "FS.exists")) {
    e32_ret_bool(baik, 0);
    return;
  }
  e32_ret_bool(baik, SPIFFS.exists(path) ? 1 : 0);
}

/* FS.read(path) -> string (atau kosong bila gagal) */
static void bk_fs_read(struct baik *baik) {
  char   path[E32_PATH_MAX];
  char  *buf;
  size_t panjang = 0;

  if (!bk_fs_siap("FS.read") ||
      !bk_arg_path(baik, 0, path, sizeof(path), "FS.read")) {
    baik_return(baik, baik_mk_null());
    return;
  }

  buf = bk_baca_berkas(path, &panjang, "FS.read");
  if (buf == NULL) {
    baik_return(baik, baik_mk_null());
    return;
  }

  /* copy = 1 -> interpreter menyalin isinya, jadi buf boleh dibebaskan. */
  baik_return(baik, baik_mk_string(baik, buf, panjang, 1));
  free(buf);
}

/* FS.write(path, isi) -> boolean */
static void bk_fs_write(struct baik *baik) {
  bk_fs_tulis_umum(baik, "w", "FS.write");
}

/* FS.append(path, isi) -> boolean */
static void bk_fs_append(struct baik *baik) {
  bk_fs_tulis_umum(baik, "a", "FS.append");
}

/* FS.remove(path) -> boolean */
static void bk_fs_remove(struct baik *baik) {
  char path[E32_PATH_MAX];

  if (!bk_fs_siap("FS.remove") ||
      !bk_arg_path(baik, 0, path, sizeof(path), "FS.remove")) {
    e32_ret_bool(baik, 0);
    return;
  }
  if (!SPIFFS.exists(path)) {
    printf("GALAT FS.remove: berkas \"%s\" tidak ada\r\n", path);
    e32_ret_bool(baik, 0);
    return;
  }
  e32_ret_bool(baik, SPIFFS.remove(path) ? 1 : 0);
}

/* FS.rename(lama, baru) -> boolean */
static void bk_fs_rename(struct baik *baik) {
  char lama[E32_PATH_MAX];
  char baru[E32_PATH_MAX];

  if (!bk_fs_siap("FS.rename") ||
      !bk_arg_path(baik, 0, lama, sizeof(lama), "FS.rename") ||
      !bk_arg_path(baik, 1, baru, sizeof(baru), "FS.rename")) {
    e32_ret_bool(baik, 0);
    return;
  }
  if (!SPIFFS.exists(lama)) {
    printf("GALAT FS.rename: berkas \"%s\" tidak ada\r\n", lama);
    e32_ret_bool(baik, 0);
    return;
  }
  e32_ret_bool(baik, SPIFFS.rename(lama, baru) ? 1 : 0);
}

/* FS.size(path) -> angka bita, atau -1 bila berkas tidak bisa dibuka */
static void bk_fs_size(struct baik *baik) {
  char path[E32_PATH_MAX];
  File f;
  long ukuran;

  if (!bk_fs_siap("FS.size") ||
      !bk_arg_path(baik, 0, path, sizeof(path), "FS.size")) {
    e32_ret_int(baik, -1);
    return;
  }

  f = SPIFFS.open(path, "r");
  if (!f) {
    printf("GALAT FS.size: berkas \"%s\" tidak dapat dibuka\r\n", path);
    e32_ret_int(baik, -1);
    return;
  }
  ukuran = (long) f.size();
  f.close(); /* selalu ditutup */
  e32_ret_int(baik, ukuran);
}

/*
 * FS.list([dir]) -> array object {nama, ukuran, direktori}
 *
 * CATATAN SPIFFS: SPIFFS TIDAK punya direktori sungguhan. Nama berkas hanya
 * berupa teks datar yang kebetulan boleh mengandung "/", jadi "/sub/a.txt"
 * adalah satu berkas bernama demikian, bukan berkas "a.txt" di dalam folder
 * "sub". Akibatnya:
 *   - hanya "/" (akar) yang dijamin bisa dibuka sebagai direktori; VFS SPIFFS
 *     memang mengemulasi pembukaan direktori dengan mencocokkan awalan nama,
 *     tetapi direktori kosong tidak pernah ada;
 *   - field `direktori` praktis selalu `salah` pada SPIFFS. Field tetap
 *     disediakan supaya skrip yang sama jalan bila kelak dipindah ke LittleFS
 *     atau SD.
 * Bila `dir` gagal dibuka atau ternyata bukan direktori, fungsi ini mencetak
 * galat dan mengembalikan `kosong`.
 */
static void bk_fs_list(struct baik *baik) {
  char       dir[E32_PATH_MAX];
  File       root;
  File       anak;
  baik_val_t arr;

  if (!bk_fs_siap("FS.list")) {
    baik_return(baik, baik_mk_null());
    return;
  }

  if (baik_nargs(baik) < 1) {
    dir[0] = '/';
    dir[1] = '\0';
  } else if (!bk_arg_path(baik, 0, dir, sizeof(dir), "FS.list")) {
    baik_return(baik, baik_mk_null());
    return;
  }

  root = SPIFFS.open(dir, "r");
  if (!root) {
    printf("GALAT FS.list: direktori \"%s\" tidak dapat dibuka\r\n", dir);
    baik_return(baik, baik_mk_null());
    return;
  }
  if (!root.isDirectory()) {
    root.close();
    printf("GALAT FS.list: \"%s\" bukan direktori (SPIFFS tidak punya "
           "direktori sungguhan, coba FS.list(\"/\"))\r\n",
           dir);
    baik_return(baik, baik_mk_null());
    return;
  }

  arr = baik_mk_array(baik);
  baik_own(baik, &arr); /* lindungi dari GC selama loop */

  anak = root.openNextFile();
  while (anak) {
    baik_val_t o = baik_mk_object(baik);
    /* `nama` pada core 2.x/3.x adalah nama dasar tanpa path; karena
     * normalisasi path menambahkan "/" di depan, nilainya tetap bisa
     * langsung dipakai kembali di FS.read()/FS.remove() pada SPIFFS. */
    const char *nama = anak.name();
    size_t      ukuran = (size_t) anak.size();
    int         adalah_dir = anak.isDirectory() ? 1 : 0;

    baik_own(baik, &o);
    if (nama == NULL) nama = "";
    baik_set(baik, o, "nama", ~(size_t) 0,
             baik_mk_string(baik, nama, ~(size_t) 0, 1));
    baik_set(baik, o, "ukuran", ~(size_t) 0,
             baik_mk_number(baik, (double) ukuran));
    baik_set(baik, o, "direktori", ~(size_t) 0,
             baik_mk_boolean(baik, adalah_dir));
    baik_array_push(baik, arr, o);
    baik_disown(baik, &o);

    anak.close(); /* tutup entri sebelum lanjut */
    anak = root.openNextFile();
  }
  root.close(); /* selalu ditutup */

  baik_return(baik, arr);
  baik_disown(baik, &arr);
}

/*
 * FS.mkdir(path) / FS.rmdir(path) -> selalu salah pada SPIFFS.
 * Tetap didaftarkan (sesuai kontrak API) supaya skrip tidak mati karena
 * fungsi tidak dikenal, tetapi memberi penjelasan yang jelas.
 */
static void bk_fs_mkdir(struct baik *baik) {
  char path[E32_PATH_MAX];

  if (bk_arg_path(baik, 0, path, sizeof(path), "FS.mkdir")) {
    printf("GALAT FS.mkdir: SPIFFS tidak mendukung direktori; \"%s\" tidak "
           "dibuat. Simpan saja berkas dengan nama \"%s/berkas.txt\".\r\n",
           path, path);
  }
  e32_ret_bool(baik, 0);
}

static void bk_fs_rmdir(struct baik *baik) {
  char path[E32_PATH_MAX];

  if (bk_arg_path(baik, 0, path, sizeof(path), "FS.rmdir")) {
    printf("GALAT FS.rmdir: SPIFFS tidak mendukung direktori; hapus tiap "
           "berkas dengan FS.remove() (mis. isi dari FS.list(\"%s\")).\r\n",
           path);
  }
  e32_ret_bool(baik, 0);
}

/* FS.totalBytes() -> angka */
static void bk_fs_total_bytes(struct baik *baik) {
  if (!bk_fs_siap("FS.totalBytes")) {
    e32_ret_int(baik, 0);
    return;
  }
  e32_ret_num(baik, (double) SPIFFS.totalBytes());
}

/* FS.usedBytes() -> angka */
static void bk_fs_used_bytes(struct baik *baik) {
  if (!bk_fs_siap("FS.usedBytes")) {
    e32_ret_int(baik, 0);
    return;
  }
  e32_ret_num(baik, (double) SPIFFS.usedBytes());
}

/* FS.freeBytes() -> angka (total - terpakai) */
static void bk_fs_free_bytes(struct baik *baik) {
  size_t total, terpakai;

  if (!bk_fs_siap("FS.freeBytes")) {
    e32_ret_int(baik, 0);
    return;
  }
  total    = (size_t) SPIFFS.totalBytes();
  terpakai = (size_t) SPIFFS.usedBytes();
  e32_ret_num(baik, (double) ((terpakai > total) ? 0 : (total - terpakai)));
}

/* FS.format() -> boolean. MENGHAPUS SELURUH ISI SPIFFS. */
static void bk_fs_format(struct baik *baik) {
  bool ok;

  printf("FS.format: memformat SPIFFS, seluruh berkas akan hilang...\r\n");
  ok = SPIFFS.format();
  if (ok) {
    s_fs_siap = SPIFFS.begin(false);
  } else {
    bk_galat("FS.format", "gagal memformat SPIFFS");
  }
  e32_ret_bool(baik, ok ? 1 : 0);
}

/* ------------------------------------------------------------------ *
 * NVS.* — Preferences
 * ------------------------------------------------------------------ */

/* Pastikan sebuah namespace sedang terbuka. */
static bool bk_nvs_terbuka(const char *fname) {
  if (!s_prefs_terbuka) {
    bk_galat(fname, "belum ada namespace yang dibuka; panggil "
                    "NVS.begin(\"nama\") lebih dulu");
    return false;
  }
  return true;
}

/*
 * Ambil argumen ke-n sebagai nama kunci NVS dan salin ke `out`.
 * Nama kunci NVS maksimal 15 karakter; lebih dari itu akan dipotong diam-diam
 * oleh driver NVS dan menimbulkan tabrakan kunci, jadi kita tolak tegas.
 */
static int bk_nvs_arg_key(struct baik *baik, int n, char *out, size_t outsz,
                          const char *fname) {
  const char *k = e32_arg_str(baik, n, NULL);
  size_t      panjang;

  if (k == NULL || k[0] == '\0') {
    bk_galat(fname, "nama kunci harus berupa teks yang tidak kosong");
    return 0;
  }
  panjang = strlen(k);
  if (panjang >= outsz) {
    printf("GALAT %s: nama kunci \"%s\" terlalu panjang (%u karakter), "
           "maksimal %d karakter\r\n",
           fname, k, (unsigned) panjang, (int) (E32_NVS_NAME_MAX - 1));
    return 0;
  }
  memcpy(out, k, panjang + 1); /* salin segera: pointer bisa berpindah */
  return 1;
}

/*
 * NVS.begin(namespace[, readOnly]) -> boolean
 *
 * - Namespace sebelumnya ditutup dulu bila masih terbuka, supaya handle NVS
 *   tidak bocor ketika skrip berpindah namespace.
 * - Namespace "wifi" DITOLAK: firmware memakainya untuk menyimpan SSID dan
 *   sandi WiFi (lihat src/main.cpp). Bila skrip pengguna menulis ke sana,
 *   konfigurasi WiFi bisa rusak dan papan tidak bisa terhubung lagi.
 */
static void bk_nvs_begin(struct baik *baik) {
  char        ruang[E32_NVS_NAME_MAX];
  const char *nama = e32_arg_str(baik, 0, NULL);
  size_t      panjang;
  int         ro;
  bool        ok;

  if (nama == NULL || nama[0] == '\0') {
    bk_galat("NVS.begin", "nama namespace harus berupa teks yang tidak kosong");
    e32_ret_bool(baik, 0);
    return;
  }
  panjang = strlen(nama);
  if (panjang >= sizeof(ruang)) {
    printf("GALAT NVS.begin: nama namespace \"%s\" terlalu panjang "
           "(%u karakter), maksimal %d karakter\r\n",
           nama, (unsigned) panjang, (int) (E32_NVS_NAME_MAX - 1));
    e32_ret_bool(baik, 0);
    return;
  }
  memcpy(ruang, nama, panjang + 1);

  if (strcmp(ruang, "wifi") == 0) {
    bk_galat("NVS.begin",
             "namespace \"wifi\" dipakai firmware untuk menyimpan kredensial "
             "WiFi dan tidak boleh diubah dari skrip; pakai nama lain, "
             "mis. \"aplikasi\"");
    e32_ret_bool(baik, 0);
    return;
  }

  ro = e32_arg_bool(baik, 1, 0);

  if (s_prefs_terbuka) {
    s_prefs.end(); /* tutup namespace sebelumnya */
    s_prefs_terbuka = false;
  }

  ok = s_prefs.begin(ruang, ro != 0);
  if (!ok) {
    printf("GALAT NVS.begin: namespace \"%s\" gagal dibuka\r\n", ruang);
  }
  s_prefs_terbuka = ok;
  e32_ret_bool(baik, ok ? 1 : 0);
}

/* NVS.end() -> takterdefinisi */
static void bk_nvs_end(struct baik *baik) {
  if (s_prefs_terbuka) {
    s_prefs.end();
    s_prefs_terbuka = false;
  }
  e32_ret_undef(baik);
}

/* NVS.putInt(kunci, nilai) -> boolean */
static void bk_nvs_put_int(struct baik *baik) {
  char kunci[E32_NVS_NAME_MAX];
  int  nilai;

  if (!bk_nvs_terbuka("NVS.putInt") ||
      !bk_nvs_arg_key(baik, 0, kunci, sizeof(kunci), "NVS.putInt")) {
    e32_ret_bool(baik, 0);
    return;
  }
  nilai = e32_arg_int(baik, 1, 0);
  e32_ret_bool(baik, s_prefs.putInt(kunci, (int32_t) nilai) > 0 ? 1 : 0);
}

/* NVS.getInt(kunci[, bawaan]) -> angka */
static void bk_nvs_get_int(struct baik *baik) {
  char kunci[E32_NVS_NAME_MAX];
  int  bawaan;

  if (!bk_nvs_terbuka("NVS.getInt") ||
      !bk_nvs_arg_key(baik, 0, kunci, sizeof(kunci), "NVS.getInt")) {
    e32_ret_int(baik, 0);
    return;
  }
  bawaan = e32_arg_int(baik, 1, 0);
  e32_ret_int(baik, (long) s_prefs.getInt(kunci, (int32_t) bawaan));
}

/* NVS.putFloat(kunci, nilai) -> boolean */
static void bk_nvs_put_float(struct baik *baik) {
  char   kunci[E32_NVS_NAME_MAX];
  double nilai;

  if (!bk_nvs_terbuka("NVS.putFloat") ||
      !bk_nvs_arg_key(baik, 0, kunci, sizeof(kunci), "NVS.putFloat")) {
    e32_ret_bool(baik, 0);
    return;
  }
  nilai = e32_arg_num(baik, 1, 0.0);
  e32_ret_bool(baik, s_prefs.putFloat(kunci, (float) nilai) > 0 ? 1 : 0);
}

/* NVS.getFloat(kunci[, bawaan]) -> angka */
static void bk_nvs_get_float(struct baik *baik) {
  char   kunci[E32_NVS_NAME_MAX];
  double bawaan;

  if (!bk_nvs_terbuka("NVS.getFloat") ||
      !bk_nvs_arg_key(baik, 0, kunci, sizeof(kunci), "NVS.getFloat")) {
    e32_ret_num(baik, 0.0);
    return;
  }
  bawaan = e32_arg_num(baik, 1, 0.0);
  e32_ret_num(baik, (double) s_prefs.getFloat(kunci, (float) bawaan));
}

/* NVS.putString(kunci, teks) -> boolean */
static void bk_nvs_put_string(struct baik *baik) {
  char        kunci[E32_NVS_NAME_MAX];
  const char *nilai;

  if (!bk_nvs_terbuka("NVS.putString") ||
      !bk_nvs_arg_key(baik, 0, kunci, sizeof(kunci), "NVS.putString")) {
    e32_ret_bool(baik, 0);
    return;
  }
  /* Diambil setelah kunci disalin; jangan membuat nilai BAIK baru sebelum
   * nilai ini selesai dipakai. */
  nilai = e32_arg_str(baik, 1, NULL);
  if (nilai == NULL) {
    bk_galat("NVS.putString", "argumen kedua harus berupa teks");
    e32_ret_bool(baik, 0);
    return;
  }
  e32_ret_bool(baik, s_prefs.putString(kunci, nilai) > 0 ? 1 : 0);
}

/* NVS.getString(kunci[, bawaan]) -> string */
static void bk_nvs_get_string(struct baik *baik) {
  char        kunci[E32_NVS_NAME_MAX];
  const char *bawaan;
  String      hasil;

  if (!bk_nvs_terbuka("NVS.getString") ||
      !bk_nvs_arg_key(baik, 0, kunci, sizeof(kunci), "NVS.getString")) {
    e32_ret_str(baik, "");
    return;
  }
  bawaan = e32_arg_str(baik, 1, "");
  if (bawaan == NULL) bawaan = "";
  hasil = s_prefs.getString(kunci, String(bawaan));
  e32_ret_str(baik, hasil.c_str());
}

/* NVS.putBool(kunci, nilai) -> boolean */
static void bk_nvs_put_bool(struct baik *baik) {
  char kunci[E32_NVS_NAME_MAX];
  int  nilai;

  if (!bk_nvs_terbuka("NVS.putBool") ||
      !bk_nvs_arg_key(baik, 0, kunci, sizeof(kunci), "NVS.putBool")) {
    e32_ret_bool(baik, 0);
    return;
  }
  nilai = e32_arg_bool(baik, 1, 0);
  e32_ret_bool(baik, s_prefs.putBool(kunci, nilai != 0) > 0 ? 1 : 0);
}

/* NVS.getBool(kunci[, bawaan]) -> boolean */
static void bk_nvs_get_bool(struct baik *baik) {
  char kunci[E32_NVS_NAME_MAX];
  int  bawaan;

  if (!bk_nvs_terbuka("NVS.getBool") ||
      !bk_nvs_arg_key(baik, 0, kunci, sizeof(kunci), "NVS.getBool")) {
    e32_ret_bool(baik, 0);
    return;
  }
  bawaan = e32_arg_bool(baik, 1, 0);
  e32_ret_bool(baik, s_prefs.getBool(kunci, bawaan != 0) ? 1 : 0);
}

/* NVS.remove(kunci) -> boolean */
static void bk_nvs_remove(struct baik *baik) {
  char kunci[E32_NVS_NAME_MAX];

  if (!bk_nvs_terbuka("NVS.remove") ||
      !bk_nvs_arg_key(baik, 0, kunci, sizeof(kunci), "NVS.remove")) {
    e32_ret_bool(baik, 0);
    return;
  }
  e32_ret_bool(baik, s_prefs.remove(kunci) ? 1 : 0);
}

/* NVS.clear() -> boolean. Menghapus SELURUH kunci di namespace aktif. */
static void bk_nvs_clear(struct baik *baik) {
  if (!bk_nvs_terbuka("NVS.clear")) {
    e32_ret_bool(baik, 0);
    return;
  }
  e32_ret_bool(baik, s_prefs.clear() ? 1 : 0);
}

/* NVS.isKey(kunci) -> boolean */
static void bk_nvs_is_key(struct baik *baik) {
  char kunci[E32_NVS_NAME_MAX];

  if (!bk_nvs_terbuka("NVS.isKey") ||
      !bk_nvs_arg_key(baik, 0, kunci, sizeof(kunci), "NVS.isKey")) {
    e32_ret_bool(baik, 0);
    return;
  }
  e32_ret_bool(baik, s_prefs.isKey(kunci) ? 1 : 0);
}

/* NVS.freeEntries() -> angka entri NVS yang masih tersisa */
static void bk_nvs_free_entries(struct baik *baik) {
  if (!bk_nvs_terbuka("NVS.freeEntries")) {
    e32_ret_int(baik, 0);
    return;
  }
  e32_ret_int(baik, (long) s_prefs.freeEntries());
}

/* ------------------------------------------------------------------ *
 * jalankan(path) — eksekusi berkas skrip BAIK dari SPIFFS
 * ------------------------------------------------------------------ */

/*
 * jalankan(path) -> benar bila sukses, salah bila gagal.
 *
 * Berbeda dengan builtin `muat()` yang memakai FILE* POSIX dan karenanya
 * butuh path lengkap "/spiffs/skrip.ina", `jalankan()` memakai API FS.* dan
 * menerima path pendek "/skrip.ina" (atau "skrip.ina").
 *
 * Rekursi dibatasi E32_JALANKAN_MAKS_DALAM tingkat supaya skrip yang saling
 * memanggil tidak menghabiskan stack task konsol.
 */
static void bk_jalankan(struct baik *baik) {
  char       path[E32_PATH_MAX];
  char      *sumber;
  size_t     panjang = 0;
  baik_err_t err;

  if (s_dalam_jalankan >= E32_JALANKAN_MAKS_DALAM) {
    printf("GALAT jalankan: kedalaman rekursi melebihi %d tingkat; "
           "kemungkinan skrip saling memanggil tanpa henti\r\n",
           E32_JALANKAN_MAKS_DALAM);
    e32_ret_bool(baik, 0);
    return;
  }

  if (!bk_fs_siap("jalankan") ||
      !bk_arg_path(baik, 0, path, sizeof(path), "jalankan")) {
    e32_ret_bool(baik, 0);
    return;
  }

  sumber = bk_baca_berkas(path, &panjang, "jalankan");
  if (sumber == NULL) {
    e32_ret_bool(baik, 0);
    return;
  }

  s_dalam_jalankan++;
  err = baik_exec(baik, sumber, NULL);
  s_dalam_jalankan--;

  /* Bytecode sudah dibentuk saat baik_exec(), sumber tidak perlu hidup lagi
   * (lihat baik_exec_file() di src/baik.c yang juga membebaskannya). */
  free(sumber);

  if (err != BAIK_OK) {
    printf("GALAT jalankan: eksekusi \"%s\" gagal\r\n", path);
    baik_print_error(baik, stdout, NULL, 1);
    e32_ret_bool(baik, 0);
    return;
  }
  e32_ret_bool(baik, 1);
}

/* ------------------------------------------------------------------ *
 * Registrasi
 * ------------------------------------------------------------------ */

void baik_esp32_register_berkas(struct baik *baik, baik_val_t g) {
  /* Nama variabel sengaja TIDAK "fs"/"nvs": keduanya adalah nama namespace
   * C++ milik Arduino-ESP32 (fs::File) dan ESP-IDF. */
  baik_val_t obj_fs  = e32_ns(baik, g, "FS");
  baik_val_t obj_nvs = e32_ns(baik, g, "NVS");
  baik_val_t fn_jalankan;

  /* ---- FS.* ---- */
  e32_fn(baik, obj_fs, "begin", bk_fs_begin);
  e32_fn(baik, obj_fs, "exists", bk_fs_exists);
  e32_fn(baik, obj_fs, "read", bk_fs_read);
  e32_fn(baik, obj_fs, "write", bk_fs_write);
  e32_fn(baik, obj_fs, "append", bk_fs_append);
  e32_fn(baik, obj_fs, "remove", bk_fs_remove);
  e32_fn(baik, obj_fs, "rename", bk_fs_rename);
  e32_fn(baik, obj_fs, "size", bk_fs_size);
  e32_fn(baik, obj_fs, "list", bk_fs_list);
  e32_fn(baik, obj_fs, "mkdir", bk_fs_mkdir);
  e32_fn(baik, obj_fs, "rmdir", bk_fs_rmdir);
  e32_fn(baik, obj_fs, "totalBytes", bk_fs_total_bytes);
  e32_fn(baik, obj_fs, "usedBytes", bk_fs_used_bytes);
  e32_fn(baik, obj_fs, "freeBytes", bk_fs_free_bytes);
  e32_fn(baik, obj_fs, "format", bk_fs_format);

  /* ---- NVS.* ---- */
  e32_fn(baik, obj_nvs, "begin", bk_nvs_begin);
  e32_fn(baik, obj_nvs, "end", bk_nvs_end);
  e32_fn(baik, obj_nvs, "putInt", bk_nvs_put_int);
  e32_fn(baik, obj_nvs, "getInt", bk_nvs_get_int);
  e32_fn(baik, obj_nvs, "putFloat", bk_nvs_put_float);
  e32_fn(baik, obj_nvs, "getFloat", bk_nvs_get_float);
  e32_fn(baik, obj_nvs, "putString", bk_nvs_put_string);
  e32_fn(baik, obj_nvs, "getString", bk_nvs_get_string);
  e32_fn(baik, obj_nvs, "putBool", bk_nvs_put_bool);
  e32_fn(baik, obj_nvs, "getBool", bk_nvs_get_bool);
  e32_fn(baik, obj_nvs, "remove", bk_nvs_remove);
  e32_fn(baik, obj_nvs, "clear", bk_nvs_clear);
  e32_fn(baik, obj_nvs, "isKey", bk_nvs_is_key);
  e32_fn(baik, obj_nvs, "freeEntries", bk_nvs_free_entries);

  /* ---- Alias object (nilai yang sama persis, bukan pembungkus) ---- */
  baik_set(baik, g, "Berkas", ~(size_t) 0, obj_fs);
  baik_set(baik, g, "Simpan", ~(size_t) 0, obj_nvs);

  /* ---- jalankan(path) + alias run(path) ---- */
  fn_jalankan = baik_mk_foreign_func(baik, (baik_func_ptr_t) bk_jalankan);
  baik_set(baik, g, "jalankan", ~(size_t) 0, fn_jalankan);
  baik_set(baik, g, "run", ~(size_t) 0, fn_jalankan);
}
