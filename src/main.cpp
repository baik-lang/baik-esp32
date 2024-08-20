#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <SPIFFS.h>
#include <Preferences.h>
#include <Arduino.h>

#include "console.h"
#include "strings.h"
#include "baik.h"

using namespace ESP32Console;

Console console;

const char *default_ssid = "ESP32_AP";
const char *default_password = "12345678";

AsyncWebServer server(80);
Preferences preferences;

void handleFileUpload(AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final)
{
    if (!index)
    {
        Serial.printf("UploadStart: %s\n", filename.c_str());
        if (!SPIFFS.open("/" + filename, FILE_WRITE))
        {
            return request->send(500, "text/plain", "Failed to open file for writing");
        }
    }

    File file = SPIFFS.open("/" + filename, FILE_APPEND);
    if (file)
    {
        if (file.write(data, len) != len)
        {
            return request->send(500, "text/plain", "Failed to write file");
        }
        file.close();
    }
    else
    {
        return request->send(500, "text/plain", "Failed to open file for appending");
    }

    if (final)
    {
        Serial.printf("UploadEnd: %s, %u B\n", filename.c_str(), index + len);
        request->send(200, "text/plain", "File successfully uploaded. Restarting...");
        delay(2000);
        ESP.restart();
    }
}

void readFileToCStr(const char *path, String &content)
{
    // Open file for reading
    File file = SPIFFS.open(path, "r");
    if (!file)
    {
        Serial.println("Failed to open file for reading");
        return;
    }

    // Allocate memory for the file content
    size_t fileSize = file.size();
    char *fileBuffer = new char[fileSize + 1]; // +1 for null terminator

    // Read the file content
    file.readBytes(fileBuffer, fileSize);
    fileBuffer[fileSize] = '\0'; // Null-terminate the C-style string

    // Store the content in the provided String variable
    content = String(fileBuffer);

    // Clean up
    delete[] fileBuffer;
    file.close();
}

void baikRun()
{
    // REPL Setup
    console.setPrompt("Baik> ");
    console.begin(115200);
    console.registerSystemCommands();

    struct baik *baik = baik_create();
    baik_err_t err = BAIK_OK;
    baik_val_t res = 0;

     // Variable to hold the file content
    String fileContent;
    // Read the file and store the content in the variable
    readFileToCStr("/baik.ina", fileContent);
    if (err == BAIK_OK)
    {
        if (res == 0)
            baik_exec(baik, fileContent.c_str(), NULL);
    }
    else
    {
        baik_print_error(baik, stdout, NULL, 1);
    }

    baik_destroy(baik);
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
        // Connect to the stored Wi-Fi network
        WiFi.begin(ssid.c_str(), password.c_str());
        while (WiFi.status() != WL_CONNECTED)
        {
            delay(1000);
            Serial.println("Connecting to WiFi...");
        }
        Serial.println("Connected to WiFi");
        Serial.print("IP Address: ");
        Serial.println(WiFi.localIP());
        Serial.println("Access the editor at: http://" + WiFi.localIP().toString() + "/");
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
    String ssid = request->getParam("ssid", true)->value();
    String password = request->getParam("password", true)->value();
    preferences.putString("ssid", ssid);
    preferences.putString("password", password);
    request->send(200, "text/plain", "Wi-Fi credentials saved. Restarting...");
    delay(2000);
    ESP.restart(); });

    // Start server
    server.begin();
}

void loop()
{
    // Nothing to do here
}