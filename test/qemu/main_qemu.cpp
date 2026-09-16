/*
 * main_qemu.cpp - Pengganti src/main.cpp KHUSUS untuk dijalankan di QEMU.
 *
 * KENAPA ADA BERKAS INI?
 * ----------------------
 * src/main.cpp selalu menyalakan WiFi saat boot (WiFi.softAP() bila belum ada
 * kredensial tersimpan, atau WiFi.begin() bila sudah). QEMU TIDAK
 * mengemulasikan radio/PHY ESP32, sehingga blob PHY tertutup milik Espressif
 * menabrak register yang tidak ada dan firmware panik:
 *
 *   Guru Meditation Error: Core 0 panic'ed (LoadStorePIFAddrError)
 *   EXCVADDR: 0x60033c00
 *   Backtrace: register_chipv7_phy <- esp_phy_enable <- wifi_hw_start <- ppTask
 *
 * Akibatnya papan virtual boot-loop dan REPL BAIK tidak pernah sempat jalan.
 *
 * Berkas ini menjalankan SEMUA hal yang sama KECUALI WiFi dan server web:
 * SPIFFS dipasang, lalu konsol + REPL BAIK dimulai. Dengan begitu bahasa BAIK
 * benar-benar bisa diuji di atas Xtensa yang diemulasikan.
 *
 * Berkas ini TIDAK ikut terbangun pada build normal. Ia hanya dipakai oleh
 * tools/qemu.sh --tanpa-wifi, yang menukarnya dengan src/main.cpp lewat
 * variabel lingkungan PLATFORMIO_BUILD_SRC_FILTER (jadi platformio.ini dan
 * src/ TIDAK diubah sama sekali).
 */
#include <Arduino.h>
#include <SPIFFS.h>
/* WiFi.h disertakan HANYA supaya pencari pustaka (LDF) PlatformIO tetap
 * menautkan pustaka WiFi — pustaka WebServer bawaan framework membutuhkannya.
 * TIDAK ADA satu pun fungsi WiFi yang dipanggil di berkas ini; radio tidak
 * pernah dinyalakan, dan itulah inti dari build QEMU ini. */
#include <WiFi.h>

#include "../../src/console.h"

using namespace ESP32Console;

static Console console;

void setup()
{
    Serial.begin(115200);
    delay(200);
    Serial.println();
    Serial.println("=========================================");
    Serial.println(" BAIK-ESP32 - build QEMU (TANPA WiFi)");
    Serial.println("=========================================");

    if (!SPIFFS.begin(true))
    {
        Serial.println("PERINGATAN: SPIFFS gagal dipasang; perintah berkas tidak akan jalan.");
    }
    else
    {
        Serial.printf("SPIFFS terpasang: %u dari %u bita terpakai\n",
                      (unsigned) SPIFFS.usedBytes(), (unsigned) SPIFFS.totalBytes());
    }

    /* Sama persis dengan baikRun() di src/main.cpp, minus jaringan. */
    console.setPrompt("Baik> ");
    console.begin(115200);
    console.registerSystemCommands();
    console.registerVFSCommands();
    console.registerGPIOCommands();
    console.registerBaikCommands();

    Serial.println("REPL BAIK siap. Coba: tulis(1+2);   atau: pinout");
}

void loop()
{
    delay(1000);
}
