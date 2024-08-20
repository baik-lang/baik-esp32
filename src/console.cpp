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

static const char *TAG = "ESP32Console";

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

    bool stringExistsInArray(const char *arr[], int size, const char *target)
    {
        for (int i = 0; i < size; i++)
        {
            if (strcmp(arr[i], target) == 0)
            {
                return true; // String exists in the array
            }
        }
        return false; // String does not exist in the array
    };

    bool readFileToCStr(const char *path, String &content)
    {
        // Open file for reading
        File file = SPIFFS.open(path, "r");
        if (!file)
        {
            Serial.println("Failed to open file for reading");
            return false;
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
        return true;
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

        // BAIK Code Initialize 
        struct baik *baik = baik_create();
        baik_err_t err = BAIK_OK;
        baik_val_t res = 0;

        // Variable to hold the file content
        String fileContent;
        // Read the file and store the content in the variable
        bool isBaikFileExists = readFileToCStr("/baik.ina", fileContent);
        if (isBaikFileExists)
        {
            printf("\r\n"
                    "Kode BAIK ditemukan dan dijalankan.....\r\n"
                    "---------------------------------------\r\n");
            if (err == BAIK_OK)
            {
                if (res == 0)
                    baik_exec(baik, fileContent.c_str(), NULL);
            }
            else
            {
                baik_print_error(baik, stdout, NULL, 1);
            }
            printf("\r\n"
                    "---------------------------------------\r\n");
        }

        /* This message shall be printed here and not earlier as the stdout
         * has just been set above. */
        printf("\r\n"
                "Selamat datang di BAIK X.\r\n"
                "Ketik 'help' untuk melihat daftar perintah.\r\n"
                "Gunakan tombol UP/DOWN untuk navigasi histori penrintah.\r\n");

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

            // command register
            const char *arr[] = {"help", "meminfo", "sysinfo", "clear", "history", "restart"};
            int size = sizeof(arr) / sizeof(arr[0]);

            // char inputString[sizeof(interpolated_line.length() + 1)];
            // strcpy(inputString, interpolated_line.c_str());
            // const char *target = strtok(inputString, " ");
            const char *target = interpolated_line.c_str();
            bool isCommand = stringExistsInArray(arr, size, target);

            /* Try to run the command */
            int ret;
            esp_err_t esp_err;

            // printf("%s",isCommand ? "true" : "false");

            if (isCommand)
            {
                esp_err = esp_console_run(interpolated_line.c_str(), &ret);
            }
            else
            {
                if (err == BAIK_OK)
                {
                    if (res == 0)
                        baik_exec(baik, interpolated_line.c_str(), NULL);
                }
                else
                {
                    baik_print_error(baik, stdout, NULL, 1);
                }
            }

            // Reset global state
            resetAfterCommands();

            if (isCommand)
            {
                if (esp_err == ESP_ERR_NOT_FOUND)
                {
                    printf("Unrecognized command\n");
                }
                else if (esp_err == ESP_ERR_INVALID_ARG)
                {
                    // command was empty
                }
                else if (esp_err == ESP_OK && ret != ESP_OK)
                {
                    printf("Command returned non-zero error code: 0x%x (%s)\n", ret, esp_err_to_name(ret));
                }
                else if (esp_err != ESP_OK)
                {
                    printf("Internal error: %s\n", esp_err_to_name(err));
                }
            }

            linenoiseFree(line);
        }
        baik_destroy(baik);
        ESP_LOGD(TAG, "REPL task ended");
        vTaskDelete(NULL);
    }

    void Console::end()
    {
    }
};