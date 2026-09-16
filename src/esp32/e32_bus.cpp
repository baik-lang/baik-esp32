/*
 * e32_bus.cpp - Modul 4: BUS  (I2C / SPI / UART)
 *
 * Mendaftarkan object berikut ke bahasa BAIK:
 *   Wire, Wire1   -> I2C  (alias global: I2C, i2cScan, pindaiI2C)
 *   SPI           -> SPI  (konstanta SPI_MODE0..3, VSPI/HSPI/FSPI)
 *   Serial1       -> UART1
 *   Serial2       -> UART2 (hanya pada papan yang punya 3 UART, mis. ESP32 klasik)
 *
 * Catatan kompatibilitas:
 *   - arduino-esp32 2.x maupun 3.x: kita hanya memakai jalur API yang ada di
 *     keduanya. Contohnya Wire.begin(sda, scl, freq) dipakai sebagai ganti
 *     Wire.setPins() yang baru ada sejak 2.0.5.
 *   - ESP32-S3 tidak punya VSPI; yang tersedia FSPI dan HSPI.
 *   - Jumlah UART ditentukan oleh SOC_UART_NUM, bukan asumsi per papan.
 *     ESP32 klasik maupun ESP32-S3 sama-sama punya 3 UART, jadi Serial2
 *     tersedia di keduanya. Pada SoC yang hanya punya 2 UART, nama Serial2
 *     tetap didaftarkan tetapi memberi galat yang menjelaskan.
 */

#include <Arduino.h>
#include <Wire.h>
#include <SPI.h>
#include <HardwareSerial.h>
#include <soc/soc_caps.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "baik_esp32.h"

/* ------------------------------------------------------------------------ */
/* Deteksi kemampuan papan                                                   */
/* ------------------------------------------------------------------------ */

#ifndef SOC_I2C_NUM
#define SOC_I2C_NUM 2
#endif

#ifndef SOC_UART_NUM
#define SOC_UART_NUM 2
#endif

#if SOC_I2C_NUM > 1
#define E32_HAS_WIRE1 1
#else
#define E32_HAS_WIRE1 0
#endif

#if SOC_UART_NUM > 2
#define E32_HAS_SERIAL2 1
#else
#define E32_HAS_SERIAL2 0
#endif

/* Nilai bawaan konstanta SPI_MODEx bila header inti tidak menyediakannya. */
#ifndef SPI_MODE0
#define SPI_MODE0 0x00
#endif
#ifndef SPI_MODE1
#define SPI_MODE1 0x01
#endif
#ifndef SPI_MODE2
#define SPI_MODE2 0x02
#endif
#ifndef SPI_MODE3
#define SPI_MODE3 0x03
#endif

/* Frekuensi bawaan */
#define E32_I2C_FREQ_DEFAULT 100000UL
#define E32_SPI_FREQ_DEFAULT 1000000UL

/* Batas aman jumlah byte sekali baca/tulis (buffer Wire ESP32 = 128 byte). */
#define E32_BUS_MAX_BYTES 512

/* ------------------------------------------------------------------------ */
/* Utilitas kecil                                                            */
/* ------------------------------------------------------------------------ */

/* 1 bila argumen ke-n benar-benar diberikan pengguna (bukan takterdefinisi). */
static int bus_arg_given(struct baik *baik, int n) {
  return (baik_nargs(baik) > n) && !baik_is_undefined(baik_arg(baik, n));
}

/*
 * Ambil nomor pin dari argumen ke-n. Bila argumen tidak diberikan, pakai
 * `def` (pin bawaan papan) tanpa validasi. Bila diberikan pengguna, pin WAJIB
 * lolos e32_pin_ok(). Mengembalikan 1 bila aman dipakai, 0 bila gagal.
 */
static int bus_pin_arg(struct baik *baik, int n, int def, uint32_t caps,
                       const char *fname, int *out) {
  if (!bus_arg_given(baik, n)) {
    *out = def;
    return 1;
  }
  *out = e32_arg_int(baik, n, def);
  if (!e32_pin_ok(baik, *out, caps, fname)) {
    return 0;
  }
  return 1;
}

/*
 * Ambil deretan byte dari argumen ke-n. Argumen boleh berupa:
 *   - array BAIK berisi angka
 *   - string  (semua byte-nya dipakai)
 *   - angka   (satu byte)
 * Hasil dialokasikan dengan malloc(); pemanggil WAJIB free().
 * Mengembalikan NULL bila tipe tidak dikenali atau panjang 0.
 */
static uint8_t *bus_bytes_arg(struct baik *baik, int n, size_t *len_out) {
  baik_val_t v = baik_arg(baik, n);
  uint8_t *buf = NULL;
  size_t i, len = 0;

  *len_out = 0;

  if (baik_is_array(v)) {
    len = (size_t) baik_array_length(baik, v);
    if (len == 0 || len > E32_BUS_MAX_BYTES) return NULL;
    buf = (uint8_t *) malloc(len);
    if (buf == NULL) return NULL;
    for (i = 0; i < len; i++) {
      baik_val_t e = baik_array_get(baik, v, (unsigned long) i);
      buf[i] = (uint8_t) (baik_is_number(e) ? baik_get_int(baik, e) : 0);
    }
  } else if (baik_is_string(v)) {
    size_t slen = 0;
    const char *s = baik_get_string(baik, &v, &slen);
    if (s == NULL || slen == 0 || slen > E32_BUS_MAX_BYTES) return NULL;
    buf = (uint8_t *) malloc(slen);
    if (buf == NULL) return NULL;
    memcpy(buf, s, slen);
    len = slen;
  } else if (baik_is_number(v)) {
    buf = (uint8_t *) malloc(1);
    if (buf == NULL) return NULL;
    buf[0] = (uint8_t) baik_get_int(baik, v);
    len = 1;
  } else {
    return NULL;
  }

  *len_out = len;
  return buf;
}

/* Kembalikan array BAIK berisi `len` byte dari `buf`. */
static void bus_ret_bytes(struct baik *baik, const uint8_t *buf, size_t len) {
  baik_val_t arr = baik_mk_array(baik);
  size_t i;

  baik_own(baik, &arr);
  for (i = 0; i < len; i++) {
    baik_array_push(baik, arr, baik_mk_number(baik, (double) buf[i]));
  }
  baik_disown(baik, &arr);
  baik_return(baik, arr);
}

/* Kembalikan array kosong (dipakai saat galat agar skrip tetap aman). */
static void bus_ret_empty_array(struct baik *baik) {
  baik_val_t arr = baik_mk_array(baik);
  baik_return(baik, arr);
}

/* Cetak isi argumen ke-n ke sebuah Print (dipakai .print / .println). */
static void bus_print_arg(struct baik *baik, Print *out, int n, int newline) {
  baik_val_t v = baik_arg(baik, n);

  if (baik_is_string(v)) {
    size_t len = 0;
    const char *s = baik_get_string(baik, &v, &len);
    if (s != NULL) out->write((const uint8_t *) s, len);
  } else if (baik_is_number(v)) {
    double d = baik_get_double(baik, v);
    if (d == (double) (long) d) {
      out->print((long) d);
    } else {
      out->print(d);
    }
  } else if (baik_is_boolean(v)) {
    out->print(baik_get_bool(baik, v) ? "benar" : "salah");
  }

  if (newline) out->print("\r\n");
}

/* ======================================================================== */
/* I2C  (TwoWire)                                                            */
/* ======================================================================== */

/* Wire.begin([sda, scl, freq]) -> boolean */
static void i2c_begin(struct baik *baik, TwoWire *w, const char *ns) {
  int sda = (int) SDA, scl = (int) SCL;
  unsigned long freq = E32_I2C_FREQ_DEFAULT;
  char fname[24];
  bool ok;

  snprintf(fname, sizeof(fname), "%s.begin", ns);

  if (!bus_pin_arg(baik, 0, (int) SDA,
                   E32_CAP_INPUT | E32_CAP_OUTPUT, fname, &sda)) {
    e32_ret_undef(baik);
    return;
  }
  if (!bus_pin_arg(baik, 1, (int) SCL, E32_CAP_OUTPUT, fname, &scl)) {
    e32_ret_undef(baik);
    return;
  }
  if (bus_arg_given(baik, 2)) {
    freq = (unsigned long) e32_arg_num(baik, 2, (double) E32_I2C_FREQ_DEFAULT);
    if (freq == 0) freq = E32_I2C_FREQ_DEFAULT;
  }

  /* Jalur begin(sda, scl, freq) tersedia di arduino-esp32 2.x maupun 3.x,
   * jadi kita tidak memakai setPins() yang baru ada sejak 2.0.5. */
  ok = w->begin((int) sda, (int) scl, (uint32_t) freq);
  if (!ok) {
    printf("Galat: %s gagal memulai bus I2C pada SDA=%d SCL=%d.\r\n",
           fname, sda, scl);
  }
  e32_ret_bool(baik, ok ? 1 : 0);
}

/* Wire.setClock(hz) */
static void i2c_set_clock(struct baik *baik, TwoWire *w) {
  unsigned long hz = (unsigned long) e32_arg_num(baik, 0,
                                                 (double) E32_I2C_FREQ_DEFAULT);
  if (hz == 0) hz = E32_I2C_FREQ_DEFAULT;
  w->setClock((uint32_t) hz);
  e32_ret_undef(baik);
}

/* Wire.beginTransmission(addr) */
static void i2c_begin_transmission(struct baik *baik, TwoWire *w) {
  int addr = e32_arg_int(baik, 0, 0);
  w->beginTransmission((int) (addr & 0x7F));
  e32_ret_undef(baik);
}

/* Wire.write(byteAtauString) -> jumlah byte yang ditulis */
static void i2c_write(struct baik *baik, TwoWire *w, const char *ns) {
  baik_val_t v = baik_arg(baik, 0);
  size_t n = 0;

  if (baik_is_number(v)) {
    n = w->write((uint8_t) baik_get_int(baik, v));
  } else if (baik_is_string(v)) {
    size_t len = 0;
    const char *s = baik_get_string(baik, &v, &len);
    if (s != NULL && len > 0) n = w->write((const uint8_t *) s, len);
  } else if (baik_is_array(v)) {
    size_t len = 0;
    uint8_t *buf = bus_bytes_arg(baik, 0, &len);
    if (buf != NULL) {
      n = w->write(buf, len);
      free(buf);
    }
  } else {
    printf("Galat: %s.write() butuh angka (satu byte), string, atau array.\r\n",
           ns);
    e32_ret_int(baik, 0);
    return;
  }

  e32_ret_int(baik, (long) n);
}

/* Wire.endTransmission([stop]) -> 0 = OK */
static void i2c_end_transmission(struct baik *baik, TwoWire *w) {
  int stop = e32_arg_bool(baik, 0, 1);
  uint8_t err = w->endTransmission((bool) (stop != 0));
  e32_ret_int(baik, (long) err);
}

/* Wire.requestFrom(addr, n[, stop]) -> jumlah byte yang diterima */
static void i2c_request_from(struct baik *baik, TwoWire *w) {
  int addr = e32_arg_int(baik, 0, 0) & 0x7F;
  int n = e32_arg_int(baik, 1, 1);
  int stop = e32_arg_bool(baik, 2, 1);
  int got;

  if (n < 0) n = 0;
  if (n > E32_BUS_MAX_BYTES) n = E32_BUS_MAX_BYTES;

  got = (int) w->requestFrom((int) addr, (int) n, (int) (stop != 0));
  e32_ret_int(baik, (long) got);
}

/* Wire.available() */
static void i2c_available(struct baik *baik, TwoWire *w) {
  e32_ret_int(baik, (long) w->available());
}

/* Wire.read() -> byte, atau -1 bila kosong */
static void i2c_read(struct baik *baik, TwoWire *w) {
  e32_ret_int(baik, (long) w->read());
}

/* Wire.readBytes(n) -> array */
static void i2c_read_bytes(struct baik *baik, TwoWire *w) {
  int n = e32_arg_int(baik, 0, w->available());
  baik_val_t arr;
  int i;

  if (n < 0) n = 0;
  if (n > E32_BUS_MAX_BYTES) n = E32_BUS_MAX_BYTES;

  arr = baik_mk_array(baik);
  baik_own(baik, &arr);
  for (i = 0; i < n; i++) {
    int b = w->read();
    if (b < 0) break;
    baik_array_push(baik, arr, baik_mk_number(baik, (double) (b & 0xFF)));
  }
  baik_disown(baik, &arr);
  baik_return(baik, arr);
}

/* Wire.end() */
static void i2c_end(struct baik *baik, TwoWire *w) {
  w->end();
  e32_ret_undef(baik);
}

/* Wire.scan() -> array alamat + tabel gaya i2cdetect */
static void i2c_scan(struct baik *baik, TwoWire *w, const char *ns) {
  baik_val_t arr = baik_mk_array(baik);
  int base, col, found = 0;

  baik_own(baik, &arr);

  printf("\r\nPindai I2C pada %s (0x01..0x7F)\r\n", ns);
  printf("     0  1  2  3  4  5  6  7  8  9  a  b  c  d  e  f\r\n");

  for (base = 0x00; base < 0x80; base += 0x10) {
    printf("%02x:", base);
    for (col = 0; col < 16; col++) {
      int addr = base + col;
      uint8_t err;

      if (addr < 0x01 || addr > 0x7F) {
        printf("   ");
        continue;
      }

      w->beginTransmission((int) addr);
      err = w->endTransmission(true);

      if (err == 0) {
        printf(" %02x", addr);
        baik_array_push(baik, arr, baik_mk_number(baik, (double) addr));
        found++;
      } else if (err == 4) {
        printf(" ??"); /* galat tak dikenal pada alamat ini */
      } else {
        printf(" --");
      }
    }
    printf("\r\n");
  }

  if (found == 0) {
    printf("Tidak ada perangkat I2C yang menjawab. "
           "Periksa kabel, catu daya, dan resistor pull-up.\r\n");
  } else {
    printf("Ditemukan %d perangkat I2C.\r\n", found);
  }

  baik_disown(baik, &arr);
  baik_return(baik, arr);
}

/* Wire.writeTo(addr, arrayByte) -> kode endTransmission (0 = OK) */
static void i2c_write_to(struct baik *baik, TwoWire *w, const char *ns) {
  int addr = e32_arg_int(baik, 0, 0) & 0x7F;
  size_t len = 0;
  uint8_t *buf = bus_bytes_arg(baik, 1, &len);
  uint8_t err;

  if (buf == NULL) {
    printf("Galat: %s.writeTo() butuh array angka (atau string) "
           "berisi 1..%d byte.\r\n", ns, E32_BUS_MAX_BYTES);
    e32_ret_int(baik, -1);
    return;
  }

  w->beginTransmission((int) addr);
  w->write(buf, len);
  err = w->endTransmission(true);
  free(buf);

  e32_ret_int(baik, (long) err);
}

/* Wire.readFrom(addr, n) -> array */
static void i2c_read_from(struct baik *baik, TwoWire *w) {
  int addr = e32_arg_int(baik, 0, 0) & 0x7F;
  int n = e32_arg_int(baik, 1, 1);
  baik_val_t arr;
  int i, got;

  if (n < 0) n = 0;
  if (n > E32_BUS_MAX_BYTES) n = E32_BUS_MAX_BYTES;

  got = (int) w->requestFrom((int) addr, (int) n, (int) 1);

  arr = baik_mk_array(baik);
  baik_own(baik, &arr);
  for (i = 0; i < got; i++) {
    int b = w->read();
    if (b < 0) break;
    baik_array_push(baik, arr, baik_mk_number(baik, (double) (b & 0xFF)));
  }
  baik_disown(baik, &arr);
  baik_return(baik, arr);
}

/* Wire.writeReg(addr, reg, nilai) -> kode endTransmission (0 = OK) */
static void i2c_write_reg(struct baik *baik, TwoWire *w) {
  int addr = e32_arg_int(baik, 0, 0) & 0x7F;
  int reg = e32_arg_int(baik, 1, 0) & 0xFF;
  size_t len = 0;
  uint8_t *buf = bus_bytes_arg(baik, 2, &len);
  uint8_t err;

  w->beginTransmission((int) addr);
  w->write((uint8_t) reg);
  if (buf != NULL) {
    w->write(buf, len);
    free(buf);
  }
  err = w->endTransmission(true);

  e32_ret_int(baik, (long) err);
}

/* Wire.readReg(addr, reg[, n]) -> angka (n<=1) atau array (n>1) */
static void i2c_read_reg(struct baik *baik, TwoWire *w) {
  int addr = e32_arg_int(baik, 0, 0) & 0x7F;
  int reg = e32_arg_int(baik, 1, 0) & 0xFF;
  int n = e32_arg_int(baik, 2, 1);
  int got, i;
  uint8_t err;

  if (n < 1) n = 1;
  if (n > E32_BUS_MAX_BYTES) n = E32_BUS_MAX_BYTES;

  w->beginTransmission((int) addr);
  w->write((uint8_t) reg);
  /* stop = salah -> repeated start pada requestFrom berikutnya */
  err = w->endTransmission(false);
  if (err != 0) {
    printf("Galat: readReg() gagal menulis alamat register "
           "(kode %u pada 0x%02x).\r\n", (unsigned) err, addr);
    if (n <= 1) {
      e32_ret_int(baik, -1);
    } else {
      bus_ret_empty_array(baik);
    }
    return;
  }

  got = (int) w->requestFrom((int) addr, (int) n, (int) 1);

  if (n <= 1) {
    int b = (got > 0) ? w->read() : -1;
    e32_ret_int(baik, (long) b);
  } else {
    baik_val_t arr = baik_mk_array(baik);
    baik_own(baik, &arr);
    for (i = 0; i < got; i++) {
      int b = w->read();
      if (b < 0) break;
      baik_array_push(baik, arr, baik_mk_number(baik, (double) (b & 0xFF)));
    }
    baik_disown(baik, &arr);
    baik_return(baik, arr);
  }
}

/* ---- Pembungkus per-bus (Wire / Wire1) --------------------------------- */

#define E32_I2C_BINDINGS(PFX, BUSPTR, BUSNAME)                                \
  static void bk_##PFX##_begin(struct baik *baik) {                           \
    i2c_begin(baik, BUSPTR, BUSNAME);                                         \
  }                                                                           \
  static void bk_##PFX##_setClock(struct baik *baik) {                        \
    i2c_set_clock(baik, BUSPTR);                                              \
  }                                                                           \
  static void bk_##PFX##_beginTransmission(struct baik *baik) {               \
    i2c_begin_transmission(baik, BUSPTR);                                     \
  }                                                                           \
  static void bk_##PFX##_write(struct baik *baik) {                           \
    i2c_write(baik, BUSPTR, BUSNAME);                                         \
  }                                                                           \
  static void bk_##PFX##_endTransmission(struct baik *baik) {                 \
    i2c_end_transmission(baik, BUSPTR);                                       \
  }                                                                           \
  static void bk_##PFX##_requestFrom(struct baik *baik) {                     \
    i2c_request_from(baik, BUSPTR);                                           \
  }                                                                           \
  static void bk_##PFX##_available(struct baik *baik) {                       \
    i2c_available(baik, BUSPTR);                                              \
  }                                                                           \
  static void bk_##PFX##_read(struct baik *baik) {                            \
    i2c_read(baik, BUSPTR);                                                   \
  }                                                                           \
  static void bk_##PFX##_readBytes(struct baik *baik) {                       \
    i2c_read_bytes(baik, BUSPTR);                                             \
  }                                                                           \
  static void bk_##PFX##_end(struct baik *baik) {                             \
    i2c_end(baik, BUSPTR);                                                    \
  }                                                                           \
  static void bk_##PFX##_scan(struct baik *baik) {                            \
    i2c_scan(baik, BUSPTR, BUSNAME);                                          \
  }                                                                           \
  static void bk_##PFX##_writeTo(struct baik *baik) {                         \
    i2c_write_to(baik, BUSPTR, BUSNAME);                                      \
  }                                                                           \
  static void bk_##PFX##_readFrom(struct baik *baik) {                        \
    i2c_read_from(baik, BUSPTR);                                              \
  }                                                                           \
  static void bk_##PFX##_writeReg(struct baik *baik) {                        \
    i2c_write_reg(baik, BUSPTR);                                              \
  }                                                                           \
  static void bk_##PFX##_readReg(struct baik *baik) {                         \
    i2c_read_reg(baik, BUSPTR);                                               \
  }

#define E32_I2C_REGISTER(BAIK, OBJ, PFX)                                      \
  do {                                                                        \
    e32_fn(BAIK, OBJ, "begin", bk_##PFX##_begin);                             \
    e32_fn(BAIK, OBJ, "setClock", bk_##PFX##_setClock);                       \
    e32_fn(BAIK, OBJ, "beginTransmission", bk_##PFX##_beginTransmission);     \
    e32_fn(BAIK, OBJ, "write", bk_##PFX##_write);                             \
    e32_fn(BAIK, OBJ, "endTransmission", bk_##PFX##_endTransmission);         \
    e32_fn(BAIK, OBJ, "requestFrom", bk_##PFX##_requestFrom);                 \
    e32_fn(BAIK, OBJ, "available", bk_##PFX##_available);                     \
    e32_fn(BAIK, OBJ, "read", bk_##PFX##_read);                               \
    e32_fn(BAIK, OBJ, "readBytes", bk_##PFX##_readBytes);                     \
    e32_fn(BAIK, OBJ, "end", bk_##PFX##_end);                                 \
    e32_fn(BAIK, OBJ, "scan", bk_##PFX##_scan);                               \
    e32_fn(BAIK, OBJ, "writeTo", bk_##PFX##_writeTo);                         \
    e32_fn(BAIK, OBJ, "readFrom", bk_##PFX##_readFrom);                       \
    e32_fn(BAIK, OBJ, "writeReg", bk_##PFX##_writeReg);                       \
    e32_fn(BAIK, OBJ, "readReg", bk_##PFX##_readReg);                         \
  } while (0)

E32_I2C_BINDINGS(wire, &Wire, "Wire")

#if E32_HAS_WIRE1
E32_I2C_BINDINGS(wire1, &Wire1, "Wire1")
#endif

/* ======================================================================== */
/* SPI                                                                       */
/* ======================================================================== */

/* SPI.begin([sck, miso, mosi, ss]) */
static void bk_spi_begin(struct baik *baik) {
  int sck = (int) SCK, miso = (int) MISO, mosi = (int) MOSI, ss = (int) SS;

  if (!bus_pin_arg(baik, 0, (int) SCK, E32_CAP_OUTPUT, "SPI.begin", &sck)) {
    e32_ret_undef(baik);
    return;
  }
  if (!bus_pin_arg(baik, 1, (int) MISO, E32_CAP_INPUT, "SPI.begin", &miso)) {
    e32_ret_undef(baik);
    return;
  }
  if (!bus_pin_arg(baik, 2, (int) MOSI, E32_CAP_OUTPUT, "SPI.begin", &mosi)) {
    e32_ret_undef(baik);
    return;
  }
  if (!bus_pin_arg(baik, 3, (int) SS, E32_CAP_OUTPUT, "SPI.begin", &ss)) {
    e32_ret_undef(baik);
    return;
  }

  SPI.begin((int8_t) sck, (int8_t) miso, (int8_t) mosi, (int8_t) ss);
  e32_ret_undef(baik);
}

/* SPI.end() */
static void bk_spi_end(struct baik *baik) {
  SPI.end();
  e32_ret_undef(baik);
}

/* SPI.setFrequency(hz) */
static void bk_spi_set_frequency(struct baik *baik) {
  unsigned long hz =
      (unsigned long) e32_arg_num(baik, 0, (double) E32_SPI_FREQ_DEFAULT);
  if (hz == 0) hz = E32_SPI_FREQ_DEFAULT;
  SPI.setFrequency((uint32_t) hz);
  e32_ret_undef(baik);
}

/* SPI.setDataMode(mode) */
static void bk_spi_set_data_mode(struct baik *baik) {
  int mode = e32_arg_int(baik, 0, 0);
  if (mode < 0 || mode > 3) {
    printf("Galat: SPI.setDataMode() butuh 0..3 "
           "(SPI_MODE0..SPI_MODE3), diterima %d.\r\n", mode);
    e32_ret_undef(baik);
    return;
  }
  SPI.setDataMode((uint8_t) mode);
  e32_ret_undef(baik);
}

/* SPI.setBitOrder(urutan) */
static void bk_spi_set_bit_order(struct baik *baik) {
  int order = e32_arg_int(baik, 0, (int) MSBFIRST);
  if (order != (int) LSBFIRST && order != (int) MSBFIRST) {
    printf("Galat: SPI.setBitOrder() butuh LSBFIRST atau MSBFIRST, "
           "diterima %d.\r\n", order);
    e32_ret_undef(baik);
    return;
  }
  SPI.setBitOrder((uint8_t) order);
  e32_ret_undef(baik);
}

/* SPI.beginTransaction(hz, urutan, mode) */
static void bk_spi_begin_transaction(struct baik *baik) {
  unsigned long hz =
      (unsigned long) e32_arg_num(baik, 0, (double) E32_SPI_FREQ_DEFAULT);
  int order = e32_arg_int(baik, 1, (int) MSBFIRST);
  int mode = e32_arg_int(baik, 2, 0);

  if (hz == 0) hz = E32_SPI_FREQ_DEFAULT;
  if (mode < 0 || mode > 3) mode = 0;
  if (order != (int) LSBFIRST && order != (int) MSBFIRST) {
    order = (int) MSBFIRST;
  }

  SPI.beginTransaction(SPISettings((uint32_t) hz, (uint8_t) order,
                                   (uint8_t) mode));
  e32_ret_undef(baik);
}

/* SPI.endTransaction() */
static void bk_spi_end_transaction(struct baik *baik) {
  SPI.endTransaction();
  e32_ret_undef(baik);
}

/* SPI.transfer(byte) -> byte */
static void bk_spi_transfer(struct baik *baik) {
  uint8_t out = (uint8_t) e32_arg_int(baik, 0, 0);
  e32_ret_int(baik, (long) SPI.transfer(out));
}

/* SPI.transfer16(word) -> word */
static void bk_spi_transfer16(struct baik *baik) {
  uint16_t out = (uint16_t) e32_arg_int(baik, 0, 0);
  e32_ret_int(baik, (long) SPI.transfer16(out));
}

/* SPI.transferBytes(array) -> array hasil */
static void bk_spi_transfer_bytes(struct baik *baik) {
  size_t len = 0;
  uint8_t *in = bus_bytes_arg(baik, 0, &len);
  uint8_t *out;

  if (in == NULL) {
    printf("Galat: SPI.transferBytes() butuh array angka (atau string) "
           "berisi 1..%d byte.\r\n", E32_BUS_MAX_BYTES);
    bus_ret_empty_array(baik);
    return;
  }

  out = (uint8_t *) malloc(len);
  if (out == NULL) {
    free(in);
    printf("Galat: SPI.transferBytes() kehabisan memori "
           "untuk %u byte.\r\n", (unsigned) len);
    bus_ret_empty_array(baik);
    return;
  }
  memset(out, 0, len);

  SPI.transferBytes((const uint8_t *) in, out, (uint32_t) len);

  bus_ret_bytes(baik, out, len);

  free(in);
  free(out);
}

/* SPI.write(byte) */
static void bk_spi_write(struct baik *baik) {
  SPI.write((uint8_t) e32_arg_int(baik, 0, 0));
  e32_ret_undef(baik);
}

/* SPI.writeBytes(array) -> jumlah byte yang dikirim */
static void bk_spi_write_bytes(struct baik *baik) {
  size_t len = 0;
  uint8_t *buf = bus_bytes_arg(baik, 0, &len);

  if (buf == NULL) {
    printf("Galat: SPI.writeBytes() butuh array angka (atau string) "
           "berisi 1..%d byte.\r\n", E32_BUS_MAX_BYTES);
    e32_ret_int(baik, 0);
    return;
  }

  SPI.writeBytes((const uint8_t *) buf, (uint32_t) len);
  free(buf);
  e32_ret_int(baik, (long) len);
}

/* ======================================================================== */
/* UART  (HardwareSerial)                                                    */
/* ======================================================================== */

static void uart_begin(struct baik *baik, HardwareSerial *port,
                       const char *ns) {
  unsigned long baud = (unsigned long) e32_arg_num(baik, 0, 115200.0);
  int rx = -1, tx = -1;
  char fname[24];

  snprintf(fname, sizeof(fname), "%s.begin", ns);

  if (baud == 0) baud = 115200;

  if (bus_arg_given(baik, 1)) {
    rx = e32_arg_int(baik, 1, -1);
    if (!e32_pin_ok(baik, rx, E32_CAP_INPUT, fname)) {
      e32_ret_undef(baik);
      return;
    }
  }
  if (bus_arg_given(baik, 2)) {
    tx = e32_arg_int(baik, 2, -1);
    if (!e32_pin_ok(baik, tx, E32_CAP_OUTPUT, fname)) {
      e32_ret_undef(baik);
      return;
    }
  }

  /* rx/tx = -1 berarti pakai pin bawaan papan untuk UART ini. */
  port->begin((unsigned long) baud, (uint32_t) SERIAL_8N1, (int8_t) rx,
              (int8_t) tx);
  e32_ret_undef(baik);
}

static void uart_available(struct baik *baik, HardwareSerial *port) {
  e32_ret_int(baik, (long) port->available());
}

static void uart_read(struct baik *baik, HardwareSerial *port) {
  e32_ret_int(baik, (long) port->read());
}

static void uart_read_string(struct baik *baik, HardwareSerial *port) {
  String s = port->readString();
  e32_ret_str(baik, s.c_str());
}

static void uart_write(struct baik *baik, HardwareSerial *port) {
  baik_val_t v = baik_arg(baik, 0);
  size_t n = 0;

  if (baik_is_string(v)) {
    size_t len = 0;
    const char *s = baik_get_string(baik, &v, &len);
    if (s != NULL && len > 0) n = port->write((const uint8_t *) s, len);
  } else if (baik_is_array(v)) {
    size_t len = 0;
    uint8_t *buf = bus_bytes_arg(baik, 0, &len);
    if (buf != NULL) {
      n = port->write(buf, len);
      free(buf);
    }
  } else {
    n = port->write((uint8_t) e32_arg_int(baik, 0, 0));
  }

  e32_ret_int(baik, (long) n);
}

static void uart_print(struct baik *baik, HardwareSerial *port) {
  bus_print_arg(baik, port, 0, 0);
  e32_ret_undef(baik);
}

static void uart_println(struct baik *baik, HardwareSerial *port) {
  bus_print_arg(baik, port, 0, 1);
  e32_ret_undef(baik);
}

static void uart_flush(struct baik *baik, HardwareSerial *port) {
  port->flush();
  e32_ret_undef(baik);
}

static void uart_end(struct baik *baik, HardwareSerial *port) {
  port->end();
  e32_ret_undef(baik);
}

#define E32_UART_BINDINGS(PFX, PORTPTR, PORTNAME)                             \
  static void bk_##PFX##_begin(struct baik *baik) {                           \
    uart_begin(baik, PORTPTR, PORTNAME);                                      \
  }                                                                           \
  static void bk_##PFX##_available(struct baik *baik) {                       \
    uart_available(baik, PORTPTR);                                            \
  }                                                                           \
  static void bk_##PFX##_read(struct baik *baik) {                            \
    uart_read(baik, PORTPTR);                                                 \
  }                                                                           \
  static void bk_##PFX##_readString(struct baik *baik) {                      \
    uart_read_string(baik, PORTPTR);                                          \
  }                                                                           \
  static void bk_##PFX##_write(struct baik *baik) {                           \
    uart_write(baik, PORTPTR);                                                \
  }                                                                           \
  static void bk_##PFX##_print(struct baik *baik) {                           \
    uart_print(baik, PORTPTR);                                                \
  }                                                                           \
  static void bk_##PFX##_println(struct baik *baik) {                         \
    uart_println(baik, PORTPTR);                                              \
  }                                                                           \
  static void bk_##PFX##_flush(struct baik *baik) {                           \
    uart_flush(baik, PORTPTR);                                                \
  }                                                                           \
  static void bk_##PFX##_end(struct baik *baik) {                             \
    uart_end(baik, PORTPTR);                                                  \
  }

#define E32_UART_REGISTER(BAIK, OBJ, PFX)                                     \
  do {                                                                        \
    e32_fn(BAIK, OBJ, "begin", bk_##PFX##_begin);                             \
    e32_fn(BAIK, OBJ, "available", bk_##PFX##_available);                     \
    e32_fn(BAIK, OBJ, "read", bk_##PFX##_read);                               \
    e32_fn(BAIK, OBJ, "readString", bk_##PFX##_readString);                   \
    e32_fn(BAIK, OBJ, "write", bk_##PFX##_write);                             \
    e32_fn(BAIK, OBJ, "print", bk_##PFX##_print);                             \
    e32_fn(BAIK, OBJ, "println", bk_##PFX##_println);                         \
    e32_fn(BAIK, OBJ, "flush", bk_##PFX##_flush);                             \
    e32_fn(BAIK, OBJ, "end", bk_##PFX##_end);                                 \
  } while (0)

#if SOC_UART_NUM > 1
E32_UART_BINDINGS(serial1, &Serial1, "Serial1")
#endif

#if E32_HAS_SERIAL2
E32_UART_BINDINGS(serial2, &Serial2, "Serial2")
#else
/* Papan ini (mis. ESP32-S3) tidak punya UART2 untuk pengguna. Semua fungsi
 * tetap didaftarkan supaya skrip tidak gagal saat parsing, tetapi memberi
 * pesan galat yang jelas. */
static void bk_serial2_unavailable(struct baik *baik) {
  printf("Galat: Serial2 tidak tersedia pada papan %s (%s). "
         "Papan ini hanya punya Serial0 (konsol) dan Serial1. "
         "Pakai Serial1 dengan pin RX/TX pilihanmu.\r\n",
         e32_board_name(), e32_board_chip());
  e32_ret_undef(baik);
}
#endif

/* ======================================================================== */
/* Registrasi modul                                                          */
/* ======================================================================== */

void baik_esp32_register_bus(struct baik *baik, baik_val_t g) {
  baik_val_t wire, spi, ser;

  /* ---- I2C: Wire ------------------------------------------------------ */
  wire = e32_ns(baik, g, "Wire");
  E32_I2C_REGISTER(baik, wire, wire);

  /* Alias global: I2C -> object Wire yang sama persis. */
  baik_set(baik, g, "I2C", strlen("I2C"), wire);

  /* Alias global untuk pemindai I2C. */
  e32_fn(baik, g, "i2cScan", bk_wire_scan);
  e32_fn(baik, g, "pindaiI2C", bk_wire_scan);

  /* ---- I2C: Wire1 ----------------------------------------------------- */
#if E32_HAS_WIRE1
  {
    baik_val_t wire1 = e32_ns(baik, g, "Wire1");
    E32_I2C_REGISTER(baik, wire1, wire1);
  }
#endif

  /* ---- SPI ------------------------------------------------------------ */
  spi = e32_ns(baik, g, "SPI");
  e32_fn(baik, spi, "begin", bk_spi_begin);
  e32_fn(baik, spi, "end", bk_spi_end);
  e32_fn(baik, spi, "setFrequency", bk_spi_set_frequency);
  e32_fn(baik, spi, "setDataMode", bk_spi_set_data_mode);
  e32_fn(baik, spi, "setBitOrder", bk_spi_set_bit_order);
  e32_fn(baik, spi, "beginTransaction", bk_spi_begin_transaction);
  e32_fn(baik, spi, "endTransaction", bk_spi_end_transaction);
  e32_fn(baik, spi, "transfer", bk_spi_transfer);
  e32_fn(baik, spi, "transfer16", bk_spi_transfer16);
  e32_fn(baik, spi, "transferBytes", bk_spi_transfer_bytes);
  e32_fn(baik, spi, "write", bk_spi_write);
  e32_fn(baik, spi, "writeBytes", bk_spi_write_bytes);

  /* Konstanta mode SPI (nilainya sama di semua papan). */
  e32_const(baik, g, "SPI_MODE0", (double) SPI_MODE0);
  e32_const(baik, g, "SPI_MODE1", (double) SPI_MODE1);
  e32_const(baik, g, "SPI_MODE2", (double) SPI_MODE2);
  e32_const(baik, g, "SPI_MODE3", (double) SPI_MODE3);

  /* Nama bus SPI. ESP32 klasik punya FSPI/HSPI/VSPI; ESP32-S3 hanya
   * punya FSPI/HSPI (VSPI TIDAK ada, jadi tidak didaftarkan). */
#ifdef FSPI
  e32_const(baik, g, "FSPI", (double) FSPI);
#endif
#ifdef HSPI
  e32_const(baik, g, "HSPI", (double) HSPI);
#endif
#if CONFIG_IDF_TARGET_ESP32S3
  /* VSPI tidak tersedia pada ESP32-S3. */
#else
#ifdef VSPI
  e32_const(baik, g, "VSPI", (double) VSPI);
#endif
#endif

  /* ---- UART ----------------------------------------------------------- */
#if SOC_UART_NUM > 1
  ser = e32_ns(baik, g, "Serial1");
  E32_UART_REGISTER(baik, ser, serial1);
#endif

#if E32_HAS_SERIAL2
  ser = e32_ns(baik, g, "Serial2");
  E32_UART_REGISTER(baik, ser, serial2);
#else
  ser = e32_ns(baik, g, "Serial2");
  e32_fn(baik, ser, "begin", bk_serial2_unavailable);
  e32_fn(baik, ser, "available", bk_serial2_unavailable);
  e32_fn(baik, ser, "read", bk_serial2_unavailable);
  e32_fn(baik, ser, "readString", bk_serial2_unavailable);
  e32_fn(baik, ser, "write", bk_serial2_unavailable);
  e32_fn(baik, ser, "print", bk_serial2_unavailable);
  e32_fn(baik, ser, "println", bk_serial2_unavailable);
  e32_fn(baik, ser, "flush", bk_serial2_unavailable);
  e32_fn(baik, ser, "end", bk_serial2_unavailable);
#endif

  (void) ser;
}
