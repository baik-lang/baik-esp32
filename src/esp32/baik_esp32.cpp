/*
 * baik_esp32.cpp - Pintu masuk tunggal jembatan BAIK x ESP32.
 *
 * Panggil baik_esp32_register(baik) SEKALI setelah baik_create(); seluruh
 * fungsi, object, dan konstanta ESP32 langsung tersedia di skrip BAIK.
 *
 * Urutan registrasi TIDAK boleh diubah: modul konstanta harus jalan lebih
 * dulu supaya modul lain boleh membaca/menimpa nilai yang sudah terpasang
 * (mis. LED_BUILTIN, SDA, SCL) dan supaya namespace yang dibuat e32_ns()
 * konsisten.
 */

#include <Arduino.h>

#include <esp_system.h>
#if defined(__has_include)
#if __has_include(<esp_idf_version.h>)
#include <esp_idf_version.h>
#endif
#if __has_include(<esp_arduino_version.h>)
#include <esp_arduino_version.h>
#endif
#endif

#include <stdio.h>

#include "baik_esp32.h"

/* Versi firmware jembatan BAIK-ESP32 (bukan versi bahasa BAIK). */
#define BAIK_ESP32_VERSION "1.0.0"

/* ======================================================================== *
 *  Fungsi global bantuan() / versi()
 * ======================================================================== */

/* bantuan()  -> cetak ringkasan seluruh API, balik takterdefinisi. */
static void e32_fn_bantuan(struct baik *baik) {
  e32_print_api();
  e32_ret_undef(baik);
}

/* versi() -> cetak identitas firmware, balik string versi BAIK-ESP32. */
static void e32_fn_versi(struct baik *baik) {
  printf("\n");
  printf("BAIK-ESP32 versi %s\n", BAIK_ESP32_VERSION);
  printf("  Papan        : %s\n", e32_board_name());
  printf("  Chip         : %s\n", e32_board_chip());
#if defined(ESP_ARDUINO_VERSION_MAJOR) && defined(ESP_ARDUINO_VERSION_MINOR) && \
    defined(ESP_ARDUINO_VERSION_PATCH)
  printf("  arduino-esp32: %d.%d.%d\n", (int) ESP_ARDUINO_VERSION_MAJOR,
         (int) ESP_ARDUINO_VERSION_MINOR, (int) ESP_ARDUINO_VERSION_PATCH);
#else
  printf("  arduino-esp32: (tidak diketahui, kemungkinan 1.x)\n");
#endif
  printf("  ESP-IDF      : %s\n", esp_get_idf_version());
  printf("  Dibangun     : %s %s\n", __DATE__, __TIME__);
  printf("\n");

  e32_ret_str(baik, BAIK_ESP32_VERSION);
}

/* ======================================================================== *
 *  Registrasi seluruh modul
 * ======================================================================== */

void baik_esp32_register(struct baik *baik) {
  baik_val_t g;

  if (baik == NULL) return;

  g = baik_get_global(baik);
  if (!baik_is_object(g)) {
    printf("GALAT baik_esp32_register(): object global tidak tersedia.\n");
    return;
  }

  /* Konstanta lebih dulu: modul lain boleh mengandalkan nilainya. */
  baik_esp32_register_konstanta(baik, g); /* e32_konstanta.cpp */
  baik_esp32_register_gpio(baik, g);      /* e32_gpio.cpp      */
  baik_esp32_register_sistem(baik, g);    /* e32_sistem.cpp    */
  baik_esp32_register_bus(baik, g);       /* e32_bus.cpp       */
  baik_esp32_register_jaringan(baik, g);  /* e32_jaringan.cpp  */
  baik_esp32_register_berkas(baik, g);    /* e32_berkas.cpp    */

  /* Fungsi bantuan global milik modul ini. */
  e32_fn(baik, g, "bantuan", e32_fn_bantuan);
  e32_fn(baik, g, "help_baik", e32_fn_bantuan); /* alias */
  e32_fn(baik, g, "versi", e32_fn_versi);

  /* Catatan GC: seluruh nilai di atas dipasang ke object global, dan object
   * global adalah akar GC (baik->scopes ditandai oleh baik_gc()). Karena itu
   * tidak ada yang perlu di-baik_own() di sini. */
}
