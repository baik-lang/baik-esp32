#include <SPIFFS.h>

#include "./console.h"
#include "soc/soc_caps.h"
#include "esp_err.h"
#include "ESP32Console/Commands/CoreCommands.h"
#include "ESP32Console/Commands/SystemCommands.h"
#include "ESP32Console/Commands/NetworkCommands.h"
#include "ESP32Console/Commands/VFSCommands.h"
#include "ESP32Console/Commands/GPIOCommands.h"
#include "driver/uart.h"
#include "esp_vfs_dev.h"
#include "linenoise/linenoise.h"
#include "ESP32Console/Helpers/PWDHelpers.h"
#include "ESP32Console/Helpers/InputParser.h"
#include "strings.h"
#include "baik.h"
#include "esp32/baik_esp32.h"

#include <stdlib.h>
#include <string.h>
#include <new>

static const char *TAG = "ESP32Console";

/* Pointer interpreter BAIK milik task REPL.
 *
 * Fungsi perintah esp_console bertanda tangan `int(int argc, char **argv)` dan
 * TIDAK menerima konteks apa pun, sehingga pointer interpreter harus disimpan
 * di lingkup berkas agar perintah seperti `run` bisa memakainya. Diisi sekali
 * di Console::repl_task() sebelum REPL berjalan. */
static struct baik *s_baik = NULL;

using namespace ESP32Console::Commands;

namespace ESP32Console
{
    void Console::registerCoreCommands()
    {
        registerCommand(getClearCommand());
        registerCommand(getHistoryCommand());
        // registerCommand(getEchoCommand());
        // registerCommand(getSetMultilineCommand());
        // registerCommand(getEnvCommand());
        // registerCommand(getDeclareCommand());
    }

    void Console::registerSystemCommands()
    {
        registerCommand(getSysInfoCommand());
        registerCommand(getRestartCommand());
        registerCommand(getMemInfoCommand());
        // registerCommand(getDateCommand());
    }

    void ESP32Console::Console::registerNetworkCommands()
    {
        registerCommand(getPingCommand());
        registerCommand(getIpconfigCommand());
    }

    void Console::registerVFSCommands()
    {
        registerCommand(getCatCommand());
        registerCommand(getCDCommand());
        registerCommand(getPWDCommand());
        registerCommand(getLsCommand());
        registerCommand(getMvCommand());
        registerCommand(getCPCommand());
        registerCommand(getRMCommand());
        registerCommand(getRMDirCommand());
        registerCommand(getEditCommand());
    }

    void Console::registerGPIOCommands()
    {
        registerCommand(getPinModeCommand());
        registerCommand(getDigitalReadCommand());
        registerCommand(getDigitalWriteCommand());
        registerCommand(getAnalogReadCommand());
    }

    /* ---- Daftar nama perintah konsol yang benar-benar terdaftar -------------
     *
     * esp_console tidak menyediakan API publik untuk menelusuri perintah yang
     * sudah terdaftar, jadi kita mencatatnya sendiri. Semua registrasi melewati
     * Console::registerCommand() (lihat console.h), sehingga daftar ini otomatis
     * ikut bertambah saat registerVFSCommands()/registerGPIOCommands()/
     * registerNetworkCommands()/registerBaikCommands() dipanggil. Tidak ada lagi
     * tabel nama yang ditulis tangan dan gampang basi. */
#define BAIK_MAKS_PERINTAH 64
    static const char *s_daftar_perintah[BAIK_MAKS_PERINTAH];
    static size_t s_jumlah_perintah = 0;

    bool Console::adalahPerintah(const char *token)
    {
        if (token == nullptr || token[0] == '\0')
        {
            return false;
        }

        for (size_t i = 0; i < s_jumlah_perintah; i++)
        {
            if (strcmp(s_daftar_perintah[i], token) == 0)
            {
                return true;
            }
        }
        return false;
    }

    void Console::catatPerintah(const char *command)
    {
        if (command == nullptr || command[0] == '\0' || adalahPerintah(command))
        {
            return;
        }

        if (s_jumlah_perintah >= BAIK_MAKS_PERINTAH)
        {
            log_e("Daftar perintah penuh, '%s' tidak dicatat", command);
            return;
        }

        /* Salin namanya: struct esp_console_cmd_t yang dipakai pemanggil bisa
         * berupa objek sementara, jadi pointer aslinya belum tentu awet. */
        char *salinan = strdup(command);
        if (salinan == nullptr)
        {
            log_e("Kehabisan memori saat mencatat perintah '%s'", command);
            return;
        }
        s_daftar_perintah[s_jumlah_perintah++] = salinan;
    }

    void Console::beginCommon()
    {
        /* Tell linenoise where to get command completions and hints */
        linenoiseSetCompletionCallback(&esp_console_get_completion);
        linenoiseSetHintsCallback((linenoiseHintsCallback *)&esp_console_get_hint);

        /* Set command history size */
        linenoiseHistorySetMaxLen(max_history_len_);

        /* Set command maximum length */
        linenoiseSetMaxLineLen(max_cmdline_len_);

        // Load history if defined
        if (history_save_path_)
        {
            linenoiseHistoryLoad(history_save_path_);
        }

        // Register core commands like echo
        esp_console_register_help_command();
        /* `help` didaftarkan langsung oleh esp_console (tidak lewat
         * registerCommand()), jadi catat namanya secara manual. */
        catatPerintah("help");
        registerCoreCommands();
    }

    void Console::begin(int baud, int rxPin, int txPin, uint8_t channel)
    {
        log_d("Initialize console");

        if (channel >= SOC_UART_NUM)
        {
            log_e("Serial number is invalid, please use numers from 0 to %u", SOC_UART_NUM - 1);
            return;
        }

        this->uart_channel_ = channel;

        // Reinit the UART driver if the channel was already in use
        if (uart_is_driver_installed(channel))
        {
            uart_driver_delete(channel);
        }

        /* Drain stdout before reconfiguring it */
        fflush(stdout);
        fsync(fileno(stdout));

        /* Disable buffering on stdin */
        setvbuf(stdin, NULL, _IONBF, 0);

        /* Minicom, screen, idf_monitor send CR when ENTER key is pressed */
        esp_vfs_dev_uart_port_set_rx_line_endings(channel, ESP_LINE_ENDINGS_CR);
        /* Move the caret to the beginning of the next line on '\n' */
        esp_vfs_dev_uart_port_set_tx_line_endings(channel, ESP_LINE_ENDINGS_CRLF);

        /* Configure UART. Note that REF_TICK is used so that the baud rate remains
         * correct while APB frequency is changing in light sleep mode.
         */
        const uart_config_t uart_config = {
            .baud_rate = baud,
            .data_bits = UART_DATA_8_BITS,
            .parity = UART_PARITY_DISABLE,
            .stop_bits = UART_STOP_BITS_1,
#if SOC_UART_SUPPORT_REF_TICK
            .source_clk = UART_SCLK_REF_TICK,
#elif SOC_UART_SUPPORT_XTAL_CLK
            .source_clk = UART_SCLK_XTAL,
#endif
        };

        ESP_ERROR_CHECK(uart_param_config(channel, &uart_config));

        // Set the correct pins for the UART of needed
        if (rxPin > 0 || txPin > 0)
        {
            if (rxPin < 0 || txPin < 0)
            {
                log_e("Both rxPin and txPin has to be passed!");
            }
            uart_set_pin(channel, txPin, rxPin, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
        }

        /* Install UART driver for interrupt-driven reads and writes */
        ESP_ERROR_CHECK(uart_driver_install(channel, 256, 0, 0, NULL, 0));

        /* Tell VFS to use UART driver */
        esp_vfs_dev_uart_use_driver(channel);

        esp_console_config_t console_config = {
            .max_cmdline_length = max_cmdline_len_,
            .max_cmdline_args = max_cmdline_args_,
            .hint_color = 333333};

        ESP_ERROR_CHECK(esp_console_init(&console_config));

        beginCommon();

        // Start REPL task
        if (xTaskCreate(&Console::repl_task, "console_repl", 4096, this, 2, &task_) != pdTRUE)
        {
            log_e("Could not start REPL task!");
        }
    }

    static void resetAfterCommands()
    {
        // Reset all global states a command could change

        // Reset getopt parameters
        optind = 0;
    }

    /* Baca seluruh isi berkas SPIFFS ke dalam `content`.
     * Sengaja TIDAK mencetak apa pun: pemanggillah yang tahu apakah berkas yang
     * hilang itu wajar (autorun /baik.ina) atau sebuah galat (perintah `run`). */
    bool readFileToCStr(const char *path, String &content)
    {
        File file = SPIFFS.open(path, "r");
        if (!file || file.isDirectory())
        {
            return false;
        }

        size_t fileSize = file.size();
        char *fileBuffer = new (std::nothrow) char[fileSize + 1]; // +1 untuk NUL
        if (fileBuffer == nullptr)
        {
            file.close();
            return false;
        }

        size_t dibaca = file.readBytes(fileBuffer, fileSize);
        fileBuffer[dibaca] = '\0';

        content = String(fileBuffer);

        delete[] fileBuffer;
        file.close();
        return true;
    }

    /* ---- Perintah konsol khusus BAIK ---------------------------------------
     *
     * Tanda tangan esp_console adalah int(int argc, char **argv) dan tidak
     * membawa konteks, jadi pointer interpreter diambil dari s_baik. */

    static int cmdPinout(int argc, char **argv)
    {
        (void) argc;
        (void) argv;
        e32_print_pinout();
        return EXIT_SUCCESS;
    }

    static int cmdApi(int argc, char **argv)
    {
        (void) argc;
        (void) argv;
        e32_print_api();
        return EXIT_SUCCESS;
    }

    static int cmdRun(int argc, char **argv)
    {
        if (argc < 2)
        {
            printf("Pemakaian: run <berkas>\n"
                   "Contoh   : run /baik.ina\n");
            return EXIT_FAILURE;
        }

        if (s_baik == nullptr)
        {
            printf("Galat: interpreter BAIK belum siap.\n");
            return EXIT_FAILURE;
        }

        /* Terima "baik.ina", "/baik.ina", maupun "/spiffs/baik.ina".
         * API Arduino SPIFFS memakai path tanpa awalan mount "/spiffs". */
        String path = argv[1];
        if (path.startsWith("/spiffs"))
        {
            path = path.substring(strlen("/spiffs"));
        }
        if (!path.startsWith("/"))
        {
            path = "/" + path;
        }

        String isi;
        if (!readFileToCStr(path.c_str(), isi))
        {
            printf("Galat: berkas '%s' tidak ditemukan atau tidak bisa dibaca.\n",
                   path.c_str());
            return EXIT_FAILURE;
        }

        baik_val_t hasil = 0;
        baik_err_t err = baik_exec(s_baik, isi.c_str(), &hasil);
        if (err != BAIK_OK)
        {
            baik_print_error(s_baik, stdout, NULL, 1);
            return EXIT_FAILURE;
        }

        return EXIT_SUCCESS;
    }

    void Console::registerBaikCommands()
    {
        registerCommand("pinout", &cmdPinout,
                        "Tampilkan tabel pinout papan ini beserta kapabilitas tiap GPIO");
        registerCommand("api", &cmdApi,
                        "Tampilkan ringkasan seluruh fungsi BAIK-ESP32 yang tersedia");
        registerCommand("run", &cmdRun,
                        "Jalankan berkas skrip BAIK dari SPIFFS", "<berkas.ina>");
    }

    void Console::repl_task(void *args)
    {
        Console const &console = *(static_cast<Console *>(args));

        /* Change standard input and output of the task if the requested UART is
         * NOT the default one. This block will replace stdin, stdout and stderr.
         * We have to do this in the repl task (not in the begin, as these settings are only valid for the current task)
         */
        if (console.uart_channel_ != CONFIG_ESP_CONSOLE_UART_NUM)
        {
            char path[13] = {0};
            snprintf(path, 13, "/dev/uart/%d", console.uart_channel_);

            stdin = fopen(path, "r");
            stdout = fopen(path, "w");
            stderr = stdout;
        }

        setvbuf(stdin, NULL, _IONBF, 0);

        // ---- Inisialisasi interpreter BAIK ---------------------------------
        struct baik *baik = baik_create();
        s_baik = baik; /* dipakai perintah konsol `run` dan poll interupsi */

        /* Daftarkan SELURUH API ESP32 (konstanta, GPIO, sistem, bus, jaringan,
         * berkas) ke object global interpreter. Harus dilakukan sebelum skrip
         * apa pun dijalankan, termasuk sebelum autorun /baik.ina. */
        baik_esp32_register(baik);

        baik_err_t err = BAIK_OK;
        baik_val_t res = 0;

        // ---- Autorun berkas /baik.ina bila ada ------------------------------
        String fileContent;
        if (readFileToCStr("/baik.ina", fileContent))
        {
            printf("\r\n"
                   "Kode BAIK ditemukan dan dijalankan.....\r\n"
                   "---------------------------------------\r\n");

            err = baik_exec(baik, fileContent.c_str(), &res);
            if (err != BAIK_OK)
            {
                baik_print_error(baik, stdout, NULL, 1);
                /* Galat pada autorun tidak boleh mematikan REPL. */
                err = BAIK_OK;
            }

            printf("\r\n"
                   "---------------------------------------\r\n");
        }

        /* This message shall be printed here and not earlier as the stdout
         * has just been set above. */
        printf("\r\n"
                "Selamat datang di BAIK X.\r\n"
                "Ketik 'help' untuk melihat daftar perintah.\r\n"
                "Ketik 'api' untuk daftar fungsi ESP32, 'pinout' untuk peta pin.\r\n"
                "Gunakan tombol UP/DOWN untuk navigasi histori perintah.\r\n");

        // Probe terminal status
        int probe_status = linenoiseProbe();
        if (probe_status)
        {
            linenoiseSetDumbMode(1);
        }

        if (linenoiseIsDumbMode())
        {
            printf("\r\n"
                   "Your terminal application does not support escape sequences.\n\n"
                   "Line editing and history features are disabled.\n\n"
                   "On Windows, try using Putty instead.\r\n");
        }

        linenoiseSetMaxLineLen(console.max_cmdline_len_);

        while (1)
        {
            /* Jalankan callback interupsi/sentuh yang tertunda. WAJIB dari task
             * konsol (bukan dari ISR) karena interpreter BAIK tidak reentrant.
             * Dipanggil tepat sebelum menunggu baris berikutnya. */
            baik_esp32_poll_interrupts(s_baik);

            String prompt = console.prompt_;

            // Insert current PWD into prompt if needed
            prompt.replace("%pwd%", console_getpwd());

            char *line = linenoise(prompt.c_str());
            if (line == NULL)
            {
                ESP_LOGD(TAG, "empty line");
                /* Ignore empty lines */
                continue;
            }

            log_v("Line received from linenoise: %s\n", line);

            /* Add the command to the history */
            linenoiseHistoryAdd(line);

            /* Save command history to filesystem */
            if (console.history_save_path_)
            {
                linenoiseHistorySave(console.history_save_path_);
            }

            // Interpolate the input line
            String interpolated_line = interpolateLine(line);

            /* Lewati baris kosong / hanya spasi. */
            String baris = interpolated_line;
            baris.trim();
            if (baris.length() == 0)
            {
                linenoiseFree(line);
                continue;
            }

            /* Deteksi perintah konsol memakai KATA PERTAMA saja.
             * Sebelumnya seluruh baris dibandingkan, sehingga "help gpio" atau
             * "cat /x" tidak pernah dikenali sebagai perintah dan malah
             * dilempar ke interpreter BAIK. */
            int batas = baris.indexOf(' ');
            int batasTab = baris.indexOf('\t');
            if (batasTab >= 0 && (batas < 0 || batasTab < batas))
            {
                batas = batasTab;
            }
            String kataPertama = (batas < 0) ? baris : baris.substring(0, batas);

            /* Daftar perintah diambil dari registrasi esp_console yang nyata
             * (lihat Console::catatPerintah), jadi perintah VFS/GPIO/jaringan/
             * BAIK ikut terdeteksi begitu diaktifkan. */
            bool isCommand = Console::adalahPerintah(kataPertama.c_str());

            if (isCommand)
            {
                /* Jalankan sebagai perintah konsol. */
                int ret = 0;
                esp_err_t esp_err = esp_console_run(interpolated_line.c_str(), &ret);

                // Reset global state
                resetAfterCommands();

                if (esp_err == ESP_ERR_NOT_FOUND)
                {
                    printf("Perintah tidak dikenal: %s\n", kataPertama.c_str());
                }
                else if (esp_err == ESP_ERR_INVALID_ARG)
                {
                    // perintah kosong, tidak perlu pesan
                }
                else if (esp_err == ESP_OK && ret != ESP_OK)
                {
                    printf("Perintah mengembalikan kode galat: 0x%x (%s)\n",
                           ret, esp_err_to_name(ret));
                }
                else if (esp_err != ESP_OK)
                {
                    printf("Galat internal: %s\n", esp_err_to_name(esp_err));
                }
            }
            else
            {
                /* Jalankan sebagai kode BAIK dan LAPORKAN galatnya. */
                err = baik_exec(baik, interpolated_line.c_str(), &res);
                if (err != BAIK_OK)
                {
                    baik_print_error(baik, stdout, NULL, 1);
                    /* Bersihkan status galat supaya REPL tetap hidup. */
                    err = BAIK_OK;
                }

                // Reset global state
                resetAfterCommands();
            }

            linenoiseFree(line);
        }

        /* Tidak ada kode setelah while(1) di atas: loop REPL tidak punya jalan
         * keluar, sehingga baik_destroy(baik) yang dulu diletakkan di sini tidak
         * pernah tercapai (dead code). Interpreter memang hidup selama papan
         * menyala, jadi tidak ada yang perlu dibebaskan di titik ini. */
    }

    void Console::end()
    {
    }
};