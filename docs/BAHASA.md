# Rujukan Bahasa BAIK

BAIK adalah bahasa skrip kecil yang ditanam di dalam firmware ESP32 ini: sebuah
mesin skrip mungil untuk mikrokontroler, dengan kata kunci, nama tipe, dan pesan
galat sepenuhnya dalam bahasa Indonesia.

Bentuknya bergaya C/JavaScript, jadi kalau kamu pernah menulis JavaScript kamu akan
langsung paham bentuknya.
Tetapi BAIK **bukan** JavaScript lengkap. Dokumen ini menjelaskan apa yang benar-benar
ada, dan sama pentingnya, apa yang tidak ada. Semua klaim di sini diverifikasi langsung
ke sumber interpreter di `src/baik/` dan diuji dengan menjalankan skripnya.

> **Satu hal yang paling sering bikin tersandung:** deklarasi variabel memakai `isi`,
> bukan `var`. Kata `var` masih dicadangkan tetapi parser menolaknya.

---

## Daftar isi

1. [Bentuk program](#bentuk-program)
2. [Tipe data](#tipe-data)
3. [Literal](#literal)
4. [Operator dan presedensi](#operator-dan-presedensi)
5. [Kata kunci](#kata-kunci)
6. [Variabel dan lingkup](#variabel-dan-lingkup)
7. [Percabangan dan perulangan](#percabangan-dan-perulangan)
8. [Fungsi](#fungsi)
9. [Array dan objek](#array-dan-objek)
10. [Fungsi bawaan](#fungsi-bawaan)
11. [Perilaku angka](#perilaku-angka)
12. [Keterbatasan yang nyata](#keterbatasan-yang-nyata)
13. [Kalau kamu tahu JavaScript](#kalau-kamu-tahu-javascript)

---

## Bentuk program

Program BAIK adalah deretan pernyataan yang dipisah titik koma. Komentar memakai gaya C:

```javascript
// komentar satu baris
/* komentar
   beberapa baris */

isi nama = "dunia";
tulis("Halo,", nama);
```

Berkas skrip BAIK memakai akhiran `.ina`. Di ESP32, berkas `/baik.ina` di SPIFFS
dijalankan otomatis saat papan menyala; berkas lain bisa dijalankan dengan perintah
konsol `run <berkas>`.

Di REPL, satu baris = satu program kecil. Variabel yang kamu deklarasikan di satu baris
tetap ada di baris berikutnya, karena interpreter-nya sama sepanjang papan menyala.

---

## Tipe data

BAIK punya tujuh tipe. Operator `tipe` (padanan `typeof`) mengembalikan namanya sebagai
string berbahasa Indonesia:

| Tipe | Hasil `tipe` | Contoh nilai | Keterangan |
|---|---|---|---|
| Angka | `"angka"` | `42`, `-1.5`, `0xFF`, `1e3` | Selalu `double` 64-bit. Tidak ada tipe bilangan bulat terpisah. |
| String | `"huruf"` | `"halo"`, `'halo'` | Deretan bita; bukan UTF-16 seperti JavaScript. |
| Boolean | `"boolean"` | `benar`, `salah` | |
| Null | `"kosong"` | `kosong` | |
| Undefined | `"takterdefinisi"` | `takterdefinisi` | Nilai variabel yang belum diisi dan argumen yang tidak diberikan. |
| Array | `"untaian"` | `[1, 2, 3]` | |
| Objek | `"objek"` | `{a: 1}` | |
| Fungsi | `"fungsi"` | `fungsi (x) { balik x; }` | Fungsi yang ditulis di BAIK. |
| Foreign | `"foreign_ptr"` | `tulis`, `pinMode`, `WiFi.begin` | Fungsi/pointer native dari C/C++. Semua API ESP32 bertipe ini. |

```javascript
tulis(tipe 1);                 // angka
tulis(tipe "s");               // huruf
tulis(tipe benar);             // boolean
tulis(tipe kosong);            // kosong
tulis(tipe takterdefinisi);    // takterdefinisi
tulis(tipe [1]);               // untaian
tulis(tipe {});                // objek
tulis(tipe fungsi(){});        // fungsi
tulis(tipe tulis);             // foreign_ptr
```

---

## Literal

```javascript
isi bulat    = 42;
isi pecahan  = 3.14;
isi ilmiah   = 1e3;          // 1000
isi heksa    = 0xFF;         // 255
isi negatif  = -7;

isi teks1    = "petik ganda";
isi teks2    = 'petik tunggal';
isi lolos    = "tab:\t baris baru:\n";   // \b \f \n \r \t \v \\ \" \' didukung

isi larik    = [1, "dua", benar, kosong, [3]];
isi objek    = { a: 1, b: "dua", "kunci berspasi": 3 };

isi kosongan = kosong;
isi belum    = takterdefinisi;
```

### Cara `tulis` mencetak nilai

`tulis` memisahkan argumen dengan spasi dan menutup dengan baris baru. Perhatikan
bahwa **array, objek, dan fungsi tidak dicetak isinya**:

```javascript
tulis([1, 2]);         // <untaian>
tulis({a: 1});         // <objek>
tulis(fungsi(){});     // <fungsi@75>
tulis(tulis);          // <foreign_ptr@...>
tulis("halo", 42);     // halo 42
```

Untuk melihat isi array/objek, gunakan `JSON.stringify`:

```javascript
tulis(JSON.stringify([1, {a: 2}]));   // [1,{"a":2}]
```

Karakter yang tidak bisa dicetak ditampilkan sebagai `\xNN`:

```javascript
tulis("a\tb");         // a\x09b
```

---

## Operator dan presedensi

Dari yang paling mengikat ke yang paling longgar:

| # | Operator | Arah | Catatan |
|---|---|---|---|
| 1 | `( )` `[ ]` `.` pemanggilan fungsi | kiri → kanan | |
| 2 | `x++` `x--` (postfix) | — | |
| 3 | `!` `~` `++x` `--x` `tipe` `-` `+` (unary) | kanan → kiri | `tipe` = `typeof` |
| 4 | `*` `/` `%` | kiri → kanan | |
| 5 | `+` `-` | kiri → kanan | `+` juga menyambung string **dengan string** |
| 6 | `<<` `>>` `>>>` | kiri → kanan | operasi 32-bit |
| 7 | `<` `<=` `>` `>=` | kiri → kanan | **hanya bermakna untuk angka** |
| 8 | `===` `!==` | kiri → kanan | `==` dan `!=` ditolak saat dijalankan |
| 9 | `&` | kiri → kanan | |
| 10 | `^` | kiri → kanan | |
| 11 | `\|` | kiri → kanan | |
| 12 | `&&` | kiri → kanan | hubung-singkat (short-circuit) |
| 13 | `\|\|` | kiri → kanan | hubung-singkat |
| 14 | `? :` | kanan → kiri | |
| 15 | `=` `+=` `-=` `*=` `/=` `%=` `<<=` `>>=` `>>>=` `&=` `^=` `\|=` | kanan → kiri | |
| 16 | `,` | kiri → kanan | hanya di tingkat pernyataan |

```javascript
tulis(1 + 2 * 3);      // 7   — perkalian lebih dulu
tulis((1 + 2) * 3);    // 9
tulis(10 - 2 - 3);     // 5   — kiri ke kanan
tulis(5 & 3, 5 | 3, 5 ^ 3, ~5, 1 << 3, 16 >> 2);   // 1 7 6 -6 8 4
```

### Tiga jebakan operator

**1. `==` dilarang.** Interpreter menolaknya dengan pesan `Use ===, not ==`:

```javascript
tulis(1 == 1);     // GALAT: Use ===, not ==
tulis(1 === 1);    // benar
```

**2. Tidak ada konversi tipe implisit.** Menyambung string dengan angka adalah galat:

```javascript
tulis("n=" + 1);                    // GALAT: konversi tipe implisit dilarang
tulis("n=" + JSON.stringify(1));    // n=1      <- cara yang benar
```

**3. `<` `>` `<=` `>=` tidak bekerja untuk string.** Perbandingan dilakukan sebagai
angka, sehingga `"a" < "b"` menghasilkan `salah`. Untuk string, yang bisa dipakai
hanyalah `===` dan `!==`.

---

## Kata kunci

Lexer BAIK mengenali 31 kata cadangan (`is_reserved_word_token()` di `src/baik/baik_tokenizer.c`),
tetapi **parser hanya mengimplementasikan sebagian**. Dua daftar berikut memisahkan
keduanya dengan tegas. Diverifikasi ke `parse_statement()` dan `parse_expr()`
(`src/baik/`) serta diuji dengan menjalankan skripnya.

### (a) Kata kunci yang berfungsi

| Kata kunci BAIK | Padanan JavaScript | Contoh |
|---|---|---|
| `isi` | `let` | `isi n = 1;` |
| `fungsi` | `function` | `fungsi f(a) { balik a; }` |
| `balik` | `return` | `balik n * 2;` |
| `jika` | `if` | `jika (n > 0) { ... }` |
| `lainnya` | `else` | `jika (a) { ... } lainnya { ... }` |
| `untuk` | `for` | `untuk (isi i = 0; i < 3; i++) { ... }` |
| `ulang` | `while` | `ulang (i < 3) { i++; }` |
| `berhenti` | `break` | `jika (i === 3) { berhenti; }` |
| `teruskan` | `continue` | `jika (i === 2) { teruskan; }` |
| `in` | `in` | **hanya di dalam `untuk`**: `untuk (isi k in obj) { ... }` |
| `benar` | `true` | `isi ok = benar;` |
| `salah` | `false` | `isi ok = salah;` |
| `kosong` | `null` | `balik kosong;` |
| `takterdefinisi` | `undefined` | `jika (x === takterdefinisi) { ... }` |
| `tipe` | `typeof` | `tulis(tipe x);` |
| `this` | `this` | di dalam metode objek: `balik this.n;` |

Ditambah blok `{ ... }` yang membentuk lingkup baru.

### (b) Kata kunci yang dicadangkan tetapi DITOLAK parser

Kata-kata ini dikenali lexer — sehingga tidak boleh dipakai sebagai nama variabel —
tetapi `parse_statement()` menolaknya dengan galat
**`[kata] tidak terimplementasi`** (`BAIK_SYNTAX_ERROR`). Ada dua belas:

| Kata kunci BAIK | Padanan JavaScript | Tulis begini sebagai gantinya |
|---|---|---|
| `var` | `var` | `isi` |
| `pilih` | `switch` | rantai `jika` / `lainnya jika` / `lainnya` |
| `sama` | `case` | idem |
| `kerjakan` | `do` | `ulang` |
| `try` | `try` | periksa nilai balik fungsi |
| `catch` | `catch` | idem |
| `throw` | `throw` | `die("pesan")` |
| `new` | `new` | literal objek atau `Object.create` |
| `delete` | `delete` | setel propertinya ke `kosong` |
| `instanceof` | `instanceof` | `tipe x === "objek"` |
| `void` | `void` | `takterdefinisi` |
| `with` | `with` | tulis nama objeknya secara eksplisit |

Tiga kata cadangan lain — `standar` (`default`), `finally`, dan `debugger` — bahkan
tidak sampai ke tahap itu: memakainya menghasilkan **parse error** biasa.

```javascript
var a = 1;                                  // GALAT: [var] tidak terimplementasi
pilih (a) { sama 1: tulis("satu"); }        // GALAT: [pilih] tidak terimplementasi
kerjakan { i++; } ulang (i < 2);            // GALAT: [kerjakan] tidak terimplementasi
try { ... } catch (e) { ... }               // GALAT: [try] tidak terimplementasi
standar: tulis(1);                          // GALAT: parse error
```

### Akibatnya: tidak ada penanganan eksepsi sama sekali

Karena `try`, `catch`, `throw`, dan `finally` semuanya masuk kategori (b), **BAIK tidak
punya mekanisme eksepsi apa pun.** Tidak ada cara menangkap galat dari dalam skrip.
Konsekuensinya:

- **Galat runtime menghentikan seluruh skrip** dan mencetak jejaknya ke konsol. REPL
  tetap hidup dan siap menerima baris berikutnya, tetapi sisa skrip tidak dijalankan.
- **Fungsi melaporkan kegagalan lewat nilai balik**, bukan lewat lemparan. Itulah
  sebabnya API ESP32 di proyek ini konsisten mengembalikan `salah`, `-1`, atau `kosong`
  saat gagal, sambil mencetak pesan berbahasa Indonesia ke konsol.
- **`die("pesan")` adalah satu-satunya padanan `throw`** — dan tidak bisa ditangkap.
  Gunakan hanya untuk kegagalan yang memang harus menghentikan semuanya.

Pola penanganan galat yang dianjurkan:

```javascript
isi isi = FS.read("/konfig.json");
jika (isi === kosong) {
  tulis("Galat: /konfig.json tidak bisa dibaca, memakai nilai bawaan.");
  isi = "{}";
}
isi konfig = JSON.parse(isi);
```

### Pengganti `switch`: rantai `jika` / `lainnya jika`

`lainnya` boleh langsung diikuti `jika` tanpa kurung kurawal tambahan, sehingga rantai
kondisi tetap rata dan mudah dibaca:

```javascript
isi mode = 2;

jika (mode === 0) {
  tulis("mati");
} lainnya jika (mode === 1) {
  tulis("manual");
} lainnya jika (mode === 2) {
  tulis("otomatis");
} lainnya {
  tulis("mode tidak dikenal:", JSON.stringify(mode));
}
```

Untuk pemetaan nilai → nilai yang panjang, objek sering lebih rapi daripada rantai
`jika`:

```javascript
isi namaMode = { "0": "mati", "1": "manual", "2": "otomatis" };
isi nama = namaMode[JSON.stringify(mode)];
jika (nama === takterdefinisi) { nama = "tidak dikenal"; }
tulis(nama);
```

### Pengganti `do ... while`

`kerjakan` belum ada. Kalau badan loop harus berjalan minimal sekali, pakai `ulang`
dengan penanda:

```javascript
isi lagi = benar;
ulang (lagi) {
  isi nilai = analogRead(34);
  tulis("nilai:", nilai);
  lagi = nilai > 2000;      // kondisi dievaluasi setelah badan dijalankan
}
```

---

## Variabel dan lingkup

Variabel **wajib** dideklarasikan dengan `isi`. Menugaskan nilai ke nama yang belum
dideklarasikan adalah galat:

```javascript
x = 5;             // GALAT: [x] tidak terdefinisikan
isi x = 5;         // benar
```

Beberapa deklarasi bisa digabung dengan koma:

```javascript
isi a = 1, b = 2, c;     // c bernilai takterdefinisi
```

Lingkup (scope) mengikuti **blok** `{ ... }`, mirip `let` di JavaScript:

```javascript
isi i = 1;
jika (benar) {
  isi i = 2;       // variabel baru, hanya hidup di dalam blok ini
  tulis(i);        // 2
}
tulis(i);          // 1
```

Variabel yang dideklarasikan di tingkat teratas skrip bersifat global untuk sesi
interpreter itu, dan tetap terlihat dari dalam fungsi:

```javascript
isi ambang = 512;
fungsi lewat(nilai) {
  balik nilai > ambang;     // ambang terlihat dari sini
}
tulis(lewat(600));          // benar
```

---

## Percabangan dan perulangan

### `jika` / `lainnya`

```javascript
isi suhu = 31;

jika (suhu > 35) {
  tulis("panas");
} lainnya {
  tulis("normal");
}
```

Rantai kondisi ditulis dengan `lainnya jika`:

```javascript
jika (suhu > 35) {
  tulis("panas");
} lainnya jika (suhu > 25) {
  tulis("hangat");
} lainnya {
  tulis("sejuk");
}
```

> **Selalu pakai kurung kurawal.** Bentuk `jika (x) tulis(1); lainnya tulis(2);`
> (tanpa kurawal, dengan `lainnya`) tidak diterima parser dan menghasilkan galat sintaks.
> Satu-satunya pengecualian adalah `lainnya jika`, seperti contoh di atas.
>
> Tidak ada `pilih`/`sama`/`standar` (switch/case/default) — rantai `lainnya jika`
> inilah penggantinya.

### `ulang` (while)

```javascript
isi i = 0;
ulang (i < 3) {
  tulis("ulang", i);
  i++;
}
```

> Di ESP32, `ulang (benar) { ... }` akan **memblokir REPL selamanya**. Gunakan loop
> berbatas, atau jalankan lewat `run` dan siapkan cara keluar (mis. tombol BOOT).

### `untuk` (for)

```javascript
untuk (isi i = 0; i < 5; i++) {
  jika (i === 2) { teruskan; }   // lewati 2
  jika (i === 4) { berhenti; }   // hentikan di 4
  tulis("untuk", i);
}
```

### `untuk ... in`

Menelusuri kunci objek atau indeks array. Urutannya **tidak dijamin** sama dengan
urutan penulisan:

```javascript
isi o = { a: 1, b: 2 };
untuk (isi k in o) {
  tulis(k, o[k]);
}

isi a = [10, 20];
untuk (isi i in a) {
  tulis(i, a[i]);
}
```

Untuk array, kalau urutan penting, pakai `untuk` biasa dengan `.panjang`:

```javascript
isi a = [10, 20, 30];
untuk (isi i = 0; i < a.panjang; i++) {
  tulis(i, a[i]);
}
```

---

## Fungsi

Fungsi bisa ditulis sebagai deklarasi bernama atau sebagai nilai:

```javascript
fungsi tambah(a, b) {
  balik a + b;
}

isi kali = fungsi (a, b) {
  balik a * b;
};

tulis(tambah(2, 3), kali(2, 3));   // 5 6
```

Beberapa sifat penting:

- **Argumen kurang** → parameter sisanya bernilai `takterdefinisi`.
- **Argumen lebih** → kelebihannya diabaikan. Tidak ada objek `arguments`.
- **Rekursi** bekerja normal.
- **Fungsi adalah nilai**: bisa disimpan di variabel, dimasukkan ke array/objek, dan
  dioper sebagai argumen — inilah yang dipakai `attachInterrupt`.
- `balik` tanpa nilai mengembalikan `takterdefinisi`; fungsi tanpa `balik` juga.

```javascript
fungsi faktorial(n) {
  jika (n < 2) { balik 1; }
  balik n * faktorial(n - 1);
}
tulis(faktorial(5));               // 120

fungsi pakai(cb) { cb(9); }
pakai(fungsi (v) { tulis(v); });   // 9
```

### Metode objek dan `this`

```javascript
isi pencacah = {
  nilai: 0,
  tambah: fungsi (n) {
    this.nilai = this.nilai + n;
    balik this.nilai;
  }
};
tulis(pencacah.tambah(5));   // 5
tulis(pencacah.tambah(3));   // 8
```

### Closure — dan batasnya

Fungsi bisa membaca variabel dari lingkup yang **masih hidup**:

```javascript
isi n = 5;
isi baca = fungsi () { balik n; };
tulis(baca());               // 5   — n global, masih hidup

fungsi luar() {
  isi m = 1;
  fungsi dalam() { balik m; }
  balik dalam();             // dipanggil selagi luar() masih berjalan
}
tulis(luar());               // 1   — jalan
```

Tetapi fungsi yang **dikembalikan** dari fungsi lain **tidak** membawa serta variabel
lokal induknya, karena lingkup induk sudah dihapus saat fungsi itu selesai:

```javascript
fungsi buat(a) {
  balik fungsi () { balik a; };
}
isi f = buat(7);
tulis(f());                  // GALAT: [a] tidak terdefinisikan
```

Pola "pabrik fungsi" dan pencacah berbasis closure karena itu **tidak bisa dipakai**.
Gunakan objek untuk menyimpan keadaan:

```javascript
isi pencacah = { n: 0, naik: fungsi () { this.n = this.n + 1; balik this.n; } };
tulis(pencacah.naik(), pencacah.naik());   // 1 2
```

---

## Array dan objek

### Array

```javascript
isi a = [1, 2, 3];

tulis(a.panjang);          // 3      <- BUKAN a.length
tulis(a[0]);               // 1
a[0] = 10;
a.push(4);                 // tambah di akhir
tulis(JSON.stringify(a));  // [10,2,3,4]

a.splice(1, 1);            // buang 1 elemen mulai indeks 1
tulis(JSON.stringify(a));  // [10,3,4]

a.splice(1, 0, 99);        // sisipkan tanpa membuang
tulis(JSON.stringify(a));  // [10,99,3,4]
```

Metode array yang tersedia hanya **tiga**: `.panjang`, `.push()`, `.splice()`. Tidak ada
`map`, `filter`, `forEach`, `slice`, `join`, `indexOf`, `sort`, `pop`, atau `concat` —
tulis sendiri dengan `untuk`.

```javascript
// pengganti forEach
fungsi tiap(arr, fn) {
  untuk (isi i = 0; i < arr.panjang; i++) { fn(arr[i], i); }
}
tiap([1, 2, 3], fungsi (v, i) { tulis(i, v); });
```

### String

String punya empat anggota: `.panjang`, `.slice()`, `.indexOf()`, dan
`.at()` / `.charCodeAt()` (keduanya fungsi yang sama: mengembalikan **kode karakter**,
bukan karakternya). Pengindeksan `s[i]` mengembalikan karakter sebagai string satu bita.

```javascript
isi s = "halo";
tulis(s.panjang);          // 4
tulis(s.slice(1, 3));      // al
tulis(s.indexOf("l"));     // 2
tulis(s.at(0));            // 104   <- kode karakter 'h'
tulis(s[0]);               // h
tulis(chr(65));            // A     <- kode -> karakter
```

Penyambungan string hanya boleh string + string:

```javascript
isi nama = "pin";
tulis(nama + " = " + JSON.stringify(13));   // pin = 13
```

### Objek

```javascript
isi sensor = {
  nama: "suhu",
  pin: 34,
  baca: fungsi () { balik analogRead(this.pin); }
};

tulis(sensor.nama);            // suhu
tulis(sensor["pin"]);          // 34
sensor.kalibrasi = 1.02;       // properti baru boleh ditambahkan
tulis(JSON.stringify(sensor)); // isi objek (fungsi tidak ikut tercetak)

untuk (isi k in sensor) { tulis(k); }
```

Kunci yang bukan pengenal biasa harus ditulis sebagai string dan diakses dengan `[]`:

```javascript
isi o = { "kunci berspasi": 1 };
tulis(o["kunci berspasi"]);    // 1
```

Objek tidak bisa dihapus propertinya (`delete` belum diimplementasi). Pewarisan
sederhana tersedia lewat `Object.create`:

```javascript
isi induk = { sapa: fungsi () { tulis("halo"); } };
isi anak  = Object.create(induk);
anak.sapa();                   // halo
```

---

## Fungsi bawaan

Ini seluruh isi `baik_init_builtin()` di `src/baik/baik_builtin.c` — tidak ada yang lain. (API ESP32
seperti `pinMode` dan `WiFi.begin` didaftarkan terpisah; lihat [API.md](API.md).)

| Nama | Guna |
|---|---|
| `tulis(...)` | Cetak semua argumen ke konsol, dipisah spasi, diakhiri baris baru. Padanan `print`/`console.log`. Mengembalikan `takterdefinisi`. |
| `muat(berkas)` | Baca dan jalankan berkas skrip dari sistem berkas (padanan `load`). Memakai path POSIX, jadi di ESP32 tulis `muat("/spiffs/skrip.ina")`. Untuk path SPIFFS gaya Arduino (`/skrip.ina`), pakai perintah konsol `run` atau fungsi `jalankan()`. |
| `chr(kode)` | Ubah kode 0–255 menjadi string satu karakter. `chr(65)` → `"A"`. Di luar rentang → `kosong`. |
| `mkstr(ptr, len)` | Bentuk string BAIK dari pointer memori mentah. Untuk pemakaian tingkat rendah dari sisi C. |
| `s2o(ptr, deskriptor)` | Ubah struct C menjadi objek BAIK, memakai deskriptor `baik_c_struct_member`. Tingkat rendah. |
| `getBAIK()` | Kembalikan pointer native ke interpreter itu sendiri (`struct baik *`). Dipakai oleh kode C. |
| `gc(penuh)` | Jalankan pemungut sampah. `gc(benar)` memaksa siklus penuh. |
| `die(pesan)` | Hentikan eksekusi skrip dengan galat berisi `pesan`. Pengganti `throw`. |
| `global` | Objek global itu sendiri. `global.tulis` sama dengan `tulis`. |
| `NaN` | Nilai *Not a Number*. |
| `isNaN(x)` | `benar` bila `x` adalah NaN. |
| `JSON.stringify(x)` | Ubah nilai menjadi teks JSON. Satu-satunya cara praktis melihat isi array/objek dan mengubah angka menjadi string. |
| `JSON.parse(teks)` | Urai teks JSON menjadi nilai BAIK. |
| `Object.create(induk)` | Buat objek baru dengan `induk` sebagai prototipe. |

```javascript
tulis(JSON.stringify({pin: 13, nyala: salah}));  // {"nyala":salah,"pin":13}
isi o = JSON.parse('{"x": 5}');
tulis(o.x);                                      // 5
tulis(isNaN(NaN), isNaN(1));                     // benar salah
```

Fungsi `ffi()` dan `ffi_cb_free()` **sengaja dinonaktifkan** (dikomentari di
`baik_init_builtin()`), sehingga skrip tidak bisa memanggil sembarang simbol C.

---

## Perilaku angka

Semua angka disimpan sebagai `double` IEEE-754 64-bit. Konsekuensinya:

- Tidak ada tipe bilangan bulat terpisah. `6 / 3` menghasilkan `2`, tetapi `7 / 2`
  menghasilkan `3.5`.
- Bilangan bulat tetap eksak sampai 2^53 (≈ 9 × 10^15).
- Operator bitwise (`& | ^ ~ << >> >>>`) mengubah nilai ke bilangan bulat 32-bit lebih
  dulu, lalu kembali ke `double`.
- Aritmetika pecahan punya galat pembulatan biasa: `0.1 + 0.2` bukan tepat `0.3`.

Cara pencetakannya juga khas dan sering mengejutkan:

```javascript
tulis(1.0);        // 1           <- angka bulat dicetak tanpa koma
tulis(2.5);        // 2.500000    <- pecahan selalu 6 angka di belakang koma
tulis(1 / 3);      // 0.333333
tulis(0.1 + 0.2);  // 0.300000
tulis(1e10);       // 10000000000
```

Jadi jangan kaget melihat `2.500000`; itu format `%f`, bukan nilai yang berbeda.

---

## Keterbatasan yang nyata

Ringkasan semuanya di satu tempat, agar kamu tidak membuang waktu mencari fitur yang
memang tidak ada:

**Sintaks yang belum diimplementasi**
- Dua belas kata kunci ditolak parser dengan `[kata] tidak terimplementasi`:
  `var`, `pilih`, `sama`, `kerjakan`, `try`, `catch`, `throw`, `new`, `delete`,
  `instanceof`, `void`, `with`.
- Tiga lagi menghasilkan parse error biasa: `standar`, `finally`, `debugger`.
- **Tidak ada penanganan eksepsi sama sekali** — tanpa `try`/`catch`/`throw`, galat
  dilaporkan lewat nilai balik dan pesan konsol.
- Tidak ada `pilih`/`sama`/`standar` (switch) — pakai rantai `jika` / `lainnya jika`.
- Tidak ada `kerjakan ... ulang` (do-while) — pakai `ulang` dengan penanda.
- `in` sebagai operator ekspresi (`"a" in o`) — hanya bisa di dalam `untuk (... in ...)`.
- `jika ... lainnya` tanpa kurung kurawal (kecuali bentuk `lainnya jika`).

**Semantik yang berbeda dari JavaScript**
- `==` / `!=` ditolak; wajib `===` / `!==`.
- Tanpa konversi tipe implisit: `"x" + 1` adalah galat.
- `<` `>` `<=` `>=` hanya bermakna untuk angka.
- Panjang array/string adalah `.panjang`, bukan `.length`.
- Fungsi yang dikembalikan tidak membawa lingkup lokal induknya (closure terbatas).
- Tidak ada `arguments`, tidak ada parameter bawaan, tidak ada *rest/spread*.
- Tidak ada *hoisting* dan tidak ada penugasan ke variabel yang belum dideklarasikan.

**Pustaka standar yang tidak ada**
- Tidak ada `Math` (namun API ESP32 menyediakan `sin`, `cos`, `sqrt`, `pow`, `abs`,
  `min`, `max`, `floor`, `ceil`, `round`, `log`, `exp`, `random` sebagai fungsi global
  — lihat [API.md](API.md)).
- Tidak ada `String`, `Number`, `Date`, `RegExp`, `Promise`, `Map`, `Set`, `Symbol`.
- Metode array hanya `.panjang`, `.push`, `.splice`; metode string hanya `.panjang`,
  `.slice`, `.indexOf`, `.at`/`.charCodeAt`.
- Tidak ada `class`, tidak ada modul (`import`/`export`), tidak ada *template literal*,
  tidak ada *arrow function*.

**Terkait lingkungan ESP32**
- Tidak ada `setTimeout`, `setInterval`, `Promise`, atau bentuk konkurensi apa pun.
  Skrip berjalan sinkron di task REPL; loop tak berujung memblokir konsol.
- Callback interupsi tidak dijalankan langsung dari ISR — lihat
  [ARSITEKTUR.md](ARSITEKTUR.md).
- FFI dimatikan; menambah kemampuan harus lewat fungsi native baru di `src/esp32/`.

---

## Kalau kamu tahu JavaScript

Tabel padanan cepat:

| JavaScript | BAIK | Catatan |
|---|---|---|
| `let x = 1;` | `isi x = 1;` | `isi` adalah satu-satunya kata deklarasi yang jalan |
| `var x = 1;` | `isi x = 1;` | **`var` TIDAK didukung** — parser menolaknya |
| `const x = 1;` | `isi x = 1;` | tidak ada penanda konstanta |
| `function f(a) {}` | `fungsi f(a) {}` | |
| `return v;` | `balik v;` | |
| `if (c) {} else {}` | `jika (c) {} lainnya {}` | kurung kurawal wajib |
| `while (c) {}` | `ulang (c) {}` | |
| `for (let i=0;...)` | `untuk (isi i=0;...)` | |
| `for (const k in o)` | `untuk (isi k in o)` | |
| `else if (c) {}` | `lainnya jika (c) {}` | boleh dirantai |
| `do {} while (c)` | — | `kerjakan` ditolak parser; pakai `ulang` + penanda |
| `switch/case/default` | — | `pilih`/`sama`/`standar` ditolak parser; pakai rantai `jika`/`lainnya jika` |
| `break;` | `berhenti;` | |
| `continue;` | `teruskan;` | |
| `true` / `false` | `benar` / `salah` | |
| `null` | `kosong` | |
| `undefined` | `takterdefinisi` | |
| `typeof x` | `tipe x` | hasil berbahasa Indonesia |
| `x === y` | `x === y` | `==` dilarang |
| `console.log(a, b)` | `tulis(a, b)` | |
| `arr.length` | `arr.panjang` | |
| `str.length` | `str.panjang` | |
| `str.charCodeAt(i)` | `str.at(i)` atau `str.charCodeAt(i)` | keduanya sama |
| `String.fromCharCode(n)` | `chr(n)` | |
| `"x" + 1` | `"x" + JSON.stringify(1)` | tanpa konversi implisit |
| `throw new Error("x")` | `die("x")` | |
| `try/catch` | — | belum ada; periksa nilai balik |
| `arr.forEach(f)` | `untuk (isi i=0; i<arr.panjang; i++) { f(arr[i]); }` | |
| `arr.map(f)` | tulis sendiri dengan `untuk` + `push` | |
| `Object.keys(o)` | `untuk (isi k in o)` | |
| `Object.create(p)` | `Object.create(p)` | sama |
| `JSON.stringify` / `JSON.parse` | sama | |
| `setTimeout(f, ms)` | — | tidak ada; pakai `delay(ms)` lalu panggil `f()` |
| `class A {}` | objek literal + `Object.create` | tidak ada `class` |

---

## Lihat juga

- [API.md](API.md) — seluruh fungsi ESP32 yang bisa dipanggil dari BAIK.
- [PINOUT.md](PINOUT.md) — pin mana yang boleh dipakai untuk apa.
- [MULAI-CEPAT.md](MULAI-CEPAT.md) — panduan langkah pertama.
- [ARSITEKTUR.md](ARSITEKTUR.md) — cara interpreter ini tertanam di firmware.
- `src/baik/` — sumber kebenaran. Cari `is_reserved_word_token` (baik_tokenizer.c), `parse_statement` (baik_parser.c),
  dan `baik_init_builtin` bila ragu.
