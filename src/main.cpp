#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <SPIFFS.h>
#include <Preferences.h>
#include <Arduino.h>

#include "console.h"
#include "strings.h"

using namespace ESP32Console;

Console console;

const char *default_ssid = "ESP32_AP";
const char *default_password = "12345678";

AsyncWebServer server(80);
Preferences preferences;

/* Nama berkas unggahan datang dari jaringan, jadi tidak boleh dipercaya.
 * SPIFFS itu datar (tanpa direktori sungguhan), tetapi nama yang mengandung
 * '/' atau '..' tetap bisa menghasilkan path yang mengejutkan - dan nama
 * lebih dari 31 karakter ditolak diam-diam oleh SPIFFS. Tolak lebih awal. */
static bool namaBerkasAman(const String &nama)
{
    if (nama.isEmpty() || nama.length() > 30)
    {
        return false;
    }
    if (nama.indexOf('/') >= 0 || nama.indexOf('\\') >= 0 || nama.indexOf("..") >= 0)
    {
        return false;
    }
    return true;
}

void handleFileUpload(AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final)
{
    if (!index)
    {
        if (!namaBerkasAman(filename))
        {
            Serial.printf("Unggahan ditolak, nama berkas tidak aman: %s\n", filename.c_str());
            return request->send(400, "text/plain",
                                 "Nama berkas tidak boleh kosong, mengandung '/' atau '..', "
                                 "atau lebih dari 30 karakter.");
        }

        Serial.printf("Mulai unggah: %s\n", filename.c_str());

        /* Potong berkas lama. Sebelumnya handle hasil SPIFFS.open() di sini
         * tidak pernah ditutup, sehingga satu handle bocor pada SETIAP
         * unggahan sampai papan di-restart. */
        File awal = SPIFFS.open("/" + filename, FILE_WRITE);
        if (!awal)
        {
            return request->send(500, "text/plain", "Gagal membuka berkas untuk ditulis");
        }
        awal.close();
    }

    if (len > 0)
    {
        File file = SPIFFS.open("/" + filename, FILE_APPEND);
        if (!file)
        {
            return request->send(500, "text/plain", "Gagal membuka berkas untuk ditambahkan");
        }
        size_t ditulis = file.write(data, len);
        file.close();
        if (ditulis != len)
        {
            return request->send(500, "text/plain",
                                 "Gagal menulis berkas - penyimpanan SPIFFS mungkin penuh");
        }
    }

    if (final)
    {
        Serial.printf("Selesai unggah: %s, %u bita\n", filename.c_str(), (unsigned) (index + len));
        request->send(200, "text/plain", "Berkas berhasil diunggah. Papan akan dimulai ulang...");
        delay(2000);
        ESP.restart();
    }
}

void baikRun()
{
    // Siapkan REPL BAIK di UART0.
    console.setPrompt("Baik> ");

    // begin() menginisialisasi esp_console + UART, lalu memulai task REPL.
    // Seluruh pendaftaran perintah harus dilakukan SESUDAH ini, karena
    // esp_console_cmd_register() menuntut esp_console_init() sudah jalan.
    console.begin(115200);

    // sysinfo, meminfo, restart
    console.registerSystemCommands();
    // ls, cat, cd, pwd, mv, cp, rm, rmdir, edit
    console.registerVFSCommands();
    /* registerGPIOCommands() SENGAJA TIDAK dipanggil.
     *
     * Ia mendaftarkan perintah konsol bernama `pinMode`, `digitalRead`,
     * `digitalWrite`, dan `analogRead` - persis sama dengan nama fungsi BAIK.
     * Akibatnya satu nama punya dua perilaku yang berbeda: `digitalWrite 2 1`
     * (dipisah spasi) masuk ke perintah konsol, sedangkan `digitalWrite(2, 1)`
     * masuk ke BAIK. Yang berbahaya, jalur perintah konsol TIDAK melewati
     * validasi pin, sehingga `digitalWrite 34 1` pada pin input-only lolos
     * begitu saja - padahal justru validasi itulah nilai utama binding BAIK.
     *
     * GPIO dikerjakan lewat fungsi BAIK saja. Method-nya tetap ada di kelas
     * Console bila suatu saat dibutuhkan.
     */
    // ping, ipconfig
    console.registerNetworkCommands();
    // pinout, api, run <berkas>
    console.registerBaikCommands();
}

void setup()
{
    Serial.begin(115200);

    // Initialize SPIFFS
    if (!SPIFFS.begin(true))
    {
        Serial.println("An Error has occurred while mounting SPIFFS");
        return;
    }

    // Load Wi-Fi credentials from Preferences
    preferences.begin("wifi", false);
    String ssid = preferences.getString("ssid", "");
    String password = preferences.getString("password", "");

    if (ssid.isEmpty())
    {
        // Start Access Point mode if no credentials are stored
        WiFi.softAP(default_ssid, default_password);
        Serial.println("Started AP mode");
    }
    else
    {
        /* Sambungkan ke jaringan tersimpan, TAPI dengan batas waktu.
         *
         * Sebelumnya gelung ini menunggu WL_CONNECTED tanpa batas: bila sandi
         * salah atau router mati, setup() tidak pernah selesai sehingga REPL
         * serial tidak pernah dimulai dan papan tampak "mati" - padahal itulah
         * satu-satunya cara pengguna memperbaiki kredensialnya. Sekarang kita
         * menyerah setelah batas waktu dan kembali ke mode Access Point,
         * persis seperti yang dijanjikan halaman konfigurasi. */
        const unsigned long BATAS_SAMBUNG_MS = 20000;

        WiFi.begin(ssid.c_str(), password.c_str());
        Serial.printf("Menyambung ke WiFi \"%s\"", ssid.c_str());

        unsigned long mulai = millis();
        while (WiFi.status() != WL_CONNECTED && (millis() - mulai) < BATAS_SAMBUNG_MS)
        {
            delay(500);
            Serial.print(".");
        }
        Serial.println();

        if (WiFi.status() == WL_CONNECTED)
        {
            Serial.println("Tersambung ke WiFi");
            Serial.print("Alamat IP: ");
            Serial.println(WiFi.localIP());
            Serial.println("Buka editor di: http://" + WiFi.localIP().toString() + "/");
        }
        else
        {
            Serial.printf("Gagal menyambung ke \"%s\" dalam %lu detik.\n",
                          ssid.c_str(), BATAS_SAMBUNG_MS / 1000);
            Serial.println("Kembali ke mode Access Point supaya papan tetap bisa diakses.");

            WiFi.disconnect(true);
            WiFi.mode(WIFI_AP);
            WiFi.softAP(default_ssid, default_password);

            Serial.printf("Titik akses \"%s\" aktif, sandi \"%s\"\n",
                          default_ssid, default_password);
            Serial.print("Buka konfigurasi di: http://");
            Serial.print(WiFi.softAPIP());
            Serial.println("/ap");
        }
    }

    baikRun();

    // Serve the editor HTML file
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
              { request->send(SPIFFS, "/index.html", "text/html"); });

    // Serve the editor HTML file
    server.on("/ap", HTTP_GET, [](AsyncWebServerRequest *request)
              { request->send(SPIFFS, "/config.html", "text/html"); });

    // Handle file uploads
    server.on("/upload", HTTP_POST, [](AsyncWebServerRequest *request)
              { request->send(200); }, handleFileUpload);

    // Handle Wi-Fi configuration
    server.on("/config", HTTP_POST, [](AsyncWebServerRequest *request)
              {
    /* getParam() mengembalikan nullptr bila field tidak ada. Men-dereferensi
     * hasilnya langsung membuat papan crash hanya karena ada yang mengirim
     * POST tanpa field - mudah terjadi dari curl atau form yang tidak lengkap. */
    const AsyncWebParameter *pSsid = request->getParam("ssid", true);
    const AsyncWebParameter *pPass = request->getParam("password", true);

    if (pSsid == nullptr || pPass == nullptr)
    {
        return request->send(400, "text/plain",
                             "Permintaan tidak lengkap: field 'ssid' dan 'password' wajib diisi.");
    }

    String ssid = pSsid->value();
    String password = pPass->value();

    if (ssid.isEmpty())
    {
        return request->send(400, "text/plain", "SSID tidak boleh kosong.");
    }

    preferences.putString("ssid", ssid);
    preferences.putString("password", password);
    request->send(200, "text/plain", "Kredensial WiFi tersimpan. Papan akan dimulai ulang...");
    delay(2000);
    ESP.restart(); });

    // Start server
    server.begin();
}

void loop()
{
    // Nothing to do here
}