# `src/baik/` — Interpreter bahasa BAIK

Folder ini berisi seluruh implementasi bahasa BAIK: pemindai leksikal, pengurai,
penghasil bytecode, mesin virtual, pemungut sampah, tipe data, dan fungsi bawaan.
Dulu semuanya ditulis dalam satu berkas raksasa `src/baik.c` (12.598 baris,
±340 KB). Berkas itu **sudah tidak ada**; isinya kini tersebar ke 17 berkas `.c`
dan satu header internal di folder ini.

Yang **tidak** berubah: antarmuka publiknya. Kode di luar interpreter — `main.cpp`,
`console.cpp`, seluruh `src/esp32/` — tetap cukup menulis:

```c
#include "baik.h"
```

`src/baik.h` sama persis seperti sebelumnya. Pemecahan ini murni penataan ulang di
dalam; jejak RAM dan hasil build kedua papan tidak berubah.

> **Kamu mau menambah fungsi perangkat keras ESP32 (pin, Wi-Fi, I2C, dan sejenisnya)?**
> Jangan di sini. Tempatnya `src/esp32/`. Lihat
> [docs/ARSITEKTUR.md](../../docs/ARSITEKTUR.md#cara-menambah-fungsi-api-baru).
> Folder `src/baik/` adalah **bahasa**, bukan perangkat keras.

---

## Daftar berkas

Angka baris di bawah dihitung dengan `wc -l src/baik/*` dan bisa kamu verifikasi ulang
sendiri; angkanya akan bergeser seiring kode berubah.

| Berkas | Baris | Isi |
|---|---:|---|
| `baik_internal.h` | 3144 | Deklarasi bersama seluruh modul: tipe, struct, makro, opcode, dan prototipe. Di sinilah `BAIK_EXPOSE_PRIVATE` didefinisikan, begitu pula makro ukuran arena GC dan `typedef unsigned short Rune`. Bukan antarmuka publik. |
| `baik_em_common.c` | 2380 | Pustaka pendukung yang tidak spesifik bahasa: log, akses berkas, varint, buffer `mbuf`, utilitas string, serta pengurai/pencetak JSON tingkat rendah. Lapisan paling bawah — tidak bergantung pada modul lain. |
| `baik_ffi.c` | 1539 | Antarmuka fungsi asing (FFI): tanda tangan, marshalling argumen, callback. **Sebagian besar dinonaktifkan pada build ini** — `ffi()` dan `ffi_cb_free()` sengaja dikomentari di `baik_init_builtin()`, jadi skrip BAIK tidak bisa memanggil simbol C sembarangan. |
| `baik_exec.c` | 1057 | Mesin virtual. Operator aritmetika/logika (`do_op`, `do_arith_op`, `check_equal`), evaluasi ekspresi (`exec_expr`), properti bawaan (`getprop_builtin_*`), dan gelung eksekusi bytecode (`baik_execute`). Rumah bagi `baik_exec`, `baik_call`, `baik_apply`. |
| `baik_parser.c` | 907 | Pengurai turun-rekursif. Satu fungsi per tingkat presedensi (`parse_mul_div_rem` → `parse_plus_minus` → ... → `parse_assignment`), plus pernyataan, blok, literal objek/untaian, dan `fungsi`. Menghasilkan bytecode langsung sambil mengurai. |
| `baik_string.c` | 528 | Tipe huruf (string): pembuatan, pembacaan, perbandingan, penggabungan, `slice`, `indexOf`, `charCodeAt`, penanganan escape, dan kode rune. |
| `baik_core.c` | 471 | Inti daur hidup interpreter: `baik_create`/`baik_destroy`, pelaporan dan jejak galat, tumpukan nilai (`baik_push`/`baik_pop`), bingkai panggilan, pencarian lingkup, `baik_own`/`baik_disown`, `baik_arg`/`baik_return`. |
| `baik_json.c` | 477 | JSON pada tingkat bahasa: `JSON.stringify` dan `JSON.parse` sebagaimana terlihat dari skrip BAIK, termasuk pengutipan dan penelusuran objek bersarang. |
| `baik_util.c` | 422 | Utilitas lintas modul: `tipe` (`baik_typeof`), pencetakan nilai (`baik_fprintf`, `baik_sprintf`), disassembler bytecode (`baik_disasm`), `baik_dump`, dan pemetaan offset bytecode ke nomor baris sumber. |
| `baik_gc.c` | 405 | Pemungut sampah: arena objek/properti/tanda-tangan-FFI, alokasi sel, penandaan (*mark*), penyapuan (*sweep*), dan pemadatan buffer string. Termasuk fungsi bawaan `gc()`. |
| `baik_object.c` | 374 | Tipe objek: struct properti, pembuatan, pencarian (`baik_get`), penyetelan (`baik_set`), penghapusan (`baik_del`), iterasi (`baik_next`), dan rantai prototipe. |
| `baik_tokenizer.c` | 242 | Pemindai leksikal: token, operator panjang (`===`, `>>>=`), angka, string, komentar, dan tabel 31 kata cadangan (`is_reserved_word_token`) — di sinilah `isi`, `jika`, `ulang`, `balik`, dan kawan-kawannya terdaftar. |
| `baik_array.c` | 218 | Tipe untaian (array): pembuatan, akses indeks, `panjang`, `push`, `splice`, dan penghapusan elemen. |
| `baik_primitive.c` | 158 | Tipe primitif: angka, boolean, `kosong`, `takterdefinisi`, pointer asing (*foreign*), dan nilai fungsi. Semua angka disimpan sebagai `double`. |
| `baik_builtin.c` | 146 | Fungsi bawaan bahasa dan tempat pendaftarannya: `tulis`, `muat`, `mkstr`, `chr`, `gc`, `die`, `s2o`, `getBAIK`, `isNaN`, object `JSON` dan `Object`. Semua dirangkai di `baik_init_builtin()`. |
| `baik_repl.c` | 137 | REPL baris perintah untuk build host. Seluruh isinya dibungkus `#ifdef BAIK_MAIN`, jadi pada build firmware ESP32 berkas ini mengompilasi menjadi kosong. |
| `baik_bcode.c` | 128 | Penghasil dan pengelola bytecode: `emit_byte`/`emit_int`/`emit_str`, potongan bytecode per berkas (`baik_bcode_part_*`), peta nomor baris, dan pemuatan berkas skrip. |
| `baik_conversion.c` | 84 | Konversi antar tipe: nilai ke string, nilai ke boolean, dan uji kebenaran (`baik_is_truthy`). Sengaja kecil — BAIK **tidak** melakukan konversi tipe implisit. |

Total kode interpreter saat ini 12.817 baris (`wc -l src/baik/*`), sedikit lebih banyak
daripada berkas tunggal lama karena setiap modul membawa header komentar dan
`#include "baik_internal.h"` sendiri.

---

## Lapisan dan ketergantungan

Panah berarti "memakai". Semua modul menyertakan `baik_internal.h`; header itu tidak
digambar supaya diagramnya terbaca.

```
                    ┌──────────────────────────────────────┐
   paling atas      │            baik_builtin.c            │   tulis, muat, gc, die,
                    │       (baik_init_builtin())          │   JSON, Object, isNaN
                    └──────────────────┬───────────────────┘
                                       │
   alur bahasa      ┌──────────────────┴───────────────────┐
                    │  baik_tokenizer.c                    │
                    │        │                             │
                    │        ▼                             │
                    │  baik_parser.c                       │
                    │        │                             │
                    │        ▼                             │
                    │  baik_bcode.c                        │
                    │        │                             │
                    │        ▼                             │
                    │  baik_exec.c   (mesin virtual)       │
                    └──────────────────┬───────────────────┘
                                       │
   tipe data        ┌──────────────────┴───────────────────────────────────────┐
                    │ baik_primitive.c · baik_string.c · baik_array.c          │
                    │ baik_object.c · baik_conversion.c · baik_json.c          │
                    └──────────────────┬───────────────────────────────────────┘
                                       │
   inti runtime     ┌──────────────────┴───────────────────┐
                    │  baik_core.c   ◀───────▶   baik_gc.c │
                    │  (daur hidup, tumpukan,   (arena,    │
                    │   bingkai, lingkup)        mark/sweep)│
                    └──────────────────┬───────────────────┘
                                       │
   paling bawah     ┌──────────────────┴───────────────────┐
                    │           baik_em_common.c           │   log, berkas, varint,
                    │                                      │   mbuf, string, JSON mentah
                    └──────────────────────────────────────┘

   di samping alur utama:
     baik_util.c   — dipakai hampir semua lapisan (typeof, cetak, disasm, nomor baris)
     baik_ffi.c    — menempel di baik_exec.c; sebagian besar nonaktif di build ini
     baik_repl.c   — hanya hidup bila BAIK_MAIN didefinisikan (build host)
```

Aturan praktisnya: lapisan bawah tidak boleh memanggil lapisan atas. `baik_em_common.c`
tidak tahu apa-apa soal nilai BAIK; `baik_gc.c` tahu soal sel dan arena tetapi tidak soal
sintaks; `baik_exec.c` boleh memanggil apa saja di bawahnya.

---

## `BAIK_PRIVATE` dan `BAIK_EXPOSE_PRIVATE`

Ini bagian yang paling mudah membingungkan, jadi pahami sekali di awal.

Di `baik_internal.h`:

```c
#ifndef BAIK_EXPOSE_PRIVATE
#define BAIK_EXPOSE_PRIVATE 1
#endif

...

#ifdef BAIK_EXPOSE_PRIVATE
#define BAIK_PRIVATE          /* kosong -> linkage eksternal */
#else
#define BAIK_PRIVATE static   /* linkage internal            */
#endif
```

Artinya: `BAIK_PRIVATE` menandai fungsi yang **internal bagi interpreter, tetapi bukan
`static`**. Sewaktu seluruh interpreter masih satu berkas, `static` sudah memadai —
semua saling melihat karena berada dalam satu unit terjemahan. Setelah dipecah, fungsi
seperti `baik_push` (didefinisikan di `baik_core.c`, dipakai `baik_exec.c`) harus punya
linkage eksternal supaya *linker* bisa menyambungkannya antar berkas objek.

Karena `BAIK_EXPOSE_PRIVATE` didefinisikan di `baik_internal.h` itu sendiri, kamu tidak
perlu mendefinisikannya di `platformio.ini` atau di baris perintah kompilasi.

**Mana yang dipakai kapan:**

| Penanda | Pakai untuk | Contoh |
|---|---|---|
| tanpa penanda (polos) | Fungsi API publik yang dideklarasikan di `src/baik.h`. | `baik_create`, `baik_exec`, `baik_set`, `baik_arg` |
| `BAIK_PRIVATE` | Fungsi internal yang dipakai **lebih dari satu** berkas di folder ini. Wajib punya prototipe di `baik_internal.h`. | `baik_push`, `emit_byte`, `gc_mark`, `baik_to_string` |
| `static` | Pembantu yang hanya dipakai di dalam satu berkas. Tidak usah muncul di `baik_internal.h`. | `do_arith_op`, `getprop_builtin_string`, `add_lineno_map_item` |

Tiga fungsi diubah dari `static` menjadi `BAIK_PRIVATE` saat pemecahan, karena ternyata
benar-benar dipakai lintas modul — semuanya didefinisikan di `baik_core.c` dan dipanggil
dari `baik_exec.c`:

- `call_stack_push_frame`
- `call_stack_restore_frame`
- `baik_find_scope`

Selain itu, makro ukuran arena GC (`BAIK_OBJECT_ARENA_SIZE`, `BAIK_PROPERTY_ARENA_SIZE`,
`BAIK_FUNC_FFI_ARENA_SIZE`, beserta varian `_INC_SIZE`) dan `typedef unsigned short Rune`
dipindah ke `baik_internal.h` karena dipakai modul lain selain tempat asalnya.

**Kalau kamu menambah fungsi `BAIK_PRIVATE` baru:** tulis definisinya di berkas `.c`
yang sesuai, lalu tambahkan prototipenya di `baik_internal.h` — juga dengan `BAIK_PRIVATE`
di depan. Tanpa prototipe itu, kompilator akan mengeluh tentang deklarasi implisit dan
*linker* tidak akan menemukan simbolnya.

---

## Menambah fungsi bawaan bahasa: sentuh berkas mana?

"Fungsi bawaan bahasa" artinya sesuatu yang masuk akal di komputer mana pun —
`tulis`, `mkstr`, sebuah metode string baru. Kalau yang kamu maksud adalah pin,
Wi-Fi, atau I2C, berhenti di sini dan buka `src/esp32/` (lihat
[docs/ARSITEKTUR.md](../../docs/ARSITEKTUR.md#cara-menambah-fungsi-api-baru)).

### Kasus 1 — fungsi global baru, mis. `panjangTeks(s)`

1. **Tulis implementasinya** di modul yang paling cocok. Fungsi yang berkaitan dengan
   huruf → `baik_string.c`; dengan untaian → `baik_array.c`; yang benar-benar umum →
   `baik_builtin.c`. Tanda tangannya selalu `void fn(struct baik *baik)`; ambil argumen
   dengan `baik_arg()`, kirim hasil dengan `baik_return()`.
2. **Deklarasikan prototipenya** di `baik_internal.h` dengan `BAIK_PRIVATE`, kecuali kalau
   implementasinya berada di `baik_builtin.c` dan hanya dipakai di situ — dalam hal itu
   `static` sudah cukup.
3. **Daftarkan** di `baik_init_builtin()` dalam `baik_builtin.c`:

   ```c
   baik_set(baik, obj, "panjangTeks", ~0,
           baik_mk_foreign_func(baik, (baik_func_ptr_t) baik_panjang_teks));
   ```

   `~0` berarti "hitung sendiri panjang namanya".

### Kasus 2 — properti atau metode pada tipe bawaan, mis. `"abc".balikkan()`

Properti seperti `.panjang`, `.slice`, `.push` tidak disimpan sebagai properti objek
biasa; semuanya dilayani oleh fungsi *dispatch* di `baik_exec.c`:

| Tipe penerima | Fungsi yang harus disunting |
|---|---|
| huruf (string) | `getprop_builtin_string()` di `baik_exec.c` |
| untaian (array) | `getprop_builtin_array()` di `baik_exec.c` |
| nilai asing (*foreign*) | `getprop_builtin_foreign()` di `baik_exec.c` |

Tambahkan satu cabang `strcmp` di sana yang mengembalikan
`baik_mk_foreign_func(...)`, lalu tulis implementasinya di modul tipe yang sesuai
(`baik_string.c`, `baik_array.c`) sebagai `BAIK_PRIVATE` plus prototipe di
`baik_internal.h`.

### Kasus 3 — kata kunci atau sintaks baru

Ini pekerjaan tiga berkas sekaligus dan jauh lebih berat:

1. `baik_tokenizer.c` — tambahkan katanya ke tabel `is_reserved_word_token()` dan
   token barunya. Perhatikan urutan tabel: nilai baliknya adalah indeks + 1, jadi
   menyisipkan di tengah akan menggeser token yang lain.
2. `baik_parser.c` — tangani token itu di `parse_statement()` atau di tingkat
   presedensi yang tepat, lalu pancarkan opcode-nya.
3. `baik_exec.c` — tangani opcode baru itu di gelung `baik_execute()`; daftarkan juga
   namanya di `opcodetostr()` (`baik_util.c`) supaya disassembler tetap terbaca.

Kata kunci BAIK memakai bahasa Indonesia. Perhatikan bahwa `pilih`, `sama`, `standar`,
`kerjakan`, `var`, `try`/`catch`/`finally`/`throw`, `delete`, `new`, `void`, `with`,
`instanceof` sudah ada di tabel kata cadangan tetapi **ditolak pengurai** — itu disengaja,
bukan bug.

### Setelah menyunting apa pun di folder ini

```bash
pio run -e esp32doit-devkit-v1 && pio run -e esp32-s3-devkitc-1
```

Kedua environment wajib hijau. PlatformIO mengompilasi `src/**` secara **rekursif** dan
`platformio.ini` sengaja tidak memasang `build_src_filter`, jadi berkas `.c` baru di
folder ini otomatis ikut terkompilasi tanpa perubahan konfigurasi apa pun.

Untuk uji cepat tanpa papan, lihat [docs/PENGUJIAN.md](../../docs/PENGUJIAN.md) —
build host mengompilasi folder ini dengan `BAIK_MAIN` sehingga `baik_repl.c` aktif dan
kamu mendapat REPL di terminal.

---

## Contoh skrip untuk menguji perubahanmu

Kode BAIK memakai kata kunci berbahasa Indonesia. Cuplikan ini dipakai sebagai
pemeriksaan cepat setelah menyentuh pengurai atau mesin virtual:

```javascript
fungsi faktorial(n) {
  jika (n <= 1) {
    balik 1;
  } lainnya {
    balik n * faktorial(n - 1);
  }
}

isi daftar = [1, 2, 3];
daftar.push(4);
tulis(daftar.panjang);            // 4
tulis(faktorial(5));              // 120

isi i = 0;
ulang (i < 3) {
  tulis(i);
  i++;
}

untuk (isi j = 0; j < 3; j++) {
  tulis(j);
}

isi cocok = (1 === 1);            // gunakan === dan !==, bukan == dan !=
tulis(cocok);                     // benar
tulis(cocok !== salah);           // benar
tulis(JSON.stringify({a: 1}));
tulis(tipe kosong);
```

Ingat batasannya: tidak ada konversi tipe implisit (`"n=" + 1` adalah galat — pakai
`"n=" + JSON.stringify(1)`), `==`/`!=` ditolak dengan pesan `Use ===, not ==`, dan
panjang huruf/untaian adalah `.panjang`, bukan `.length`.

---

## Lihat juga

- [`../baik.h`](../baik.h) — antarmuka publik interpreter (tidak berubah oleh pemecahan ini).
- [docs/ARSITEKTUR.md](../../docs/ARSITEKTUR.md) — cara kerja firmware secara keseluruhan
  dan cara menambah fungsi API ESP32 di `src/esp32/`.
- [docs/BAHASA.md](../../docs/BAHASA.md) — rujukan bahasa BAIK dari sisi pengguna.
- [docs/PENGUJIAN.md](../../docs/PENGUJIAN.md) — build host, QEMU, dan uji di papan asli.
