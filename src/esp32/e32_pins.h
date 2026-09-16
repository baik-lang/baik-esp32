/*
 * e32_pins.h - Tabel pinout & kapabilitas GPIO ESP32 / ESP32-S3.
 *
 * Tabel ini adalah sumber kebenaran tunggal (single source of truth) untuk:
 *   - validasi pin saat runtime (BAIK akan menolak pin yang tidak mampu)
 *   - perintah konsol `pinout`
 *   - konstanta pin yang diekspor ke bahasa BAIK
 *   - dokumen docs/PINOUT.md
 */
#ifndef BAIK_E32_PINS_H_
#define BAIK_E32_PINS_H_

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Kapabilitas per pin (bit flags) */
#define E32_CAP_DIGITAL   (1u << 0)  /* bisa digitalRead/digitalWrite      */
#define E32_CAP_INPUT     (1u << 1)  /* bisa dipakai sebagai input         */
#define E32_CAP_OUTPUT    (1u << 2)  /* bisa dipakai sebagai output        */
#define E32_CAP_PULL      (1u << 3)  /* punya pull-up/pull-down internal   */
#define E32_CAP_ADC1      (1u << 4)  /* ADC unit 1 (aman dipakai + WiFi)   */
#define E32_CAP_ADC2      (1u << 5)  /* ADC unit 2 (bentrok dengan WiFi)   */
#define E32_CAP_DAC       (1u << 6)  /* DAC 8-bit                          */
#define E32_CAP_TOUCH     (1u << 7)  /* sensor sentuh kapasitif            */
#define E32_CAP_RTC       (1u << 8)  /* RTC GPIO -> bisa wake dari sleep   */
#define E32_CAP_PWM       (1u << 9)  /* bisa dipakai LEDC/PWM              */
#define E32_CAP_STRAP     (1u << 10) /* strapping pin, hati-hati saat boot */
#define E32_CAP_FLASH     (1u << 11) /* terpakai SPI flash/PSRAM: JANGAN   */
#define E32_CAP_USB       (1u << 12) /* USB D-/D+ (USB-JTAG/CDC)           */
#define E32_CAP_UART0     (1u << 13) /* UART0 konsol (TX0/RX0)             */
#define E32_CAP_I2C       (1u << 14) /* pin I2C default (SDA/SCL)          */
#define E32_CAP_SPI       (1u << 15) /* pin SPI default                    */
#define E32_CAP_BOOT      (1u << 16) /* tombol BOOT pada devkit            */
#define E32_CAP_LED       (1u << 17) /* LED onboard                        */

/* Kombinasi yang sering dipakai */
#define E32_CAP_ADC       (E32_CAP_ADC1 | E32_CAP_ADC2)
#define E32_CAP_IO        (E32_CAP_DIGITAL | E32_CAP_INPUT | E32_CAP_OUTPUT)

typedef struct e32_pin_info {
  int16_t  gpio;           /* nomor GPIO                                   */
  uint32_t caps;           /* bit flags E32_CAP_*                          */
  int8_t   adc_unit;       /* 1 / 2, atau -1                               */
  int8_t   adc_channel;    /* nomor channel ADC, atau -1                   */
  int8_t   touch_channel;  /* T0..T14, atau -1                             */
  int8_t   dac_channel;    /* 1 / 2, atau -1                               */
  int8_t   rtc_gpio;       /* nomor RTC GPIO, atau -1                      */
  const char *label;       /* label sablon di devkit, mis. "D23"           */
  const char *note;        /* catatan pemakaian / peringatan (boleh NULL)  */
} e32_pin_info_t;

/* Seluruh tabel pin papan yang sedang dikompilasi. */
const e32_pin_info_t *e32_pin_table(size_t *count);

/* Cari info satu pin berdasarkan nomor GPIO. NULL jika pin tidak ada. */
const e32_pin_info_t *e32_pin_find(int gpio);

/* 1 bila `gpio` ada dan memiliki SEMUA kapabilitas pada `caps`. */
int e32_pin_has(int gpio, uint32_t caps);

/* Nama kapabilitas untuk satu bit flag, mis. E32_CAP_ADC1 -> "ADC1". */
const char *e32_cap_name(uint32_t single_cap);

/* Tulis daftar kapabilitas pin ke buf, dipisah koma. Mengembalikan buf. */
char *e32_caps_to_string(uint32_t caps, char *buf, size_t buflen);

/* Identitas papan */
const char *e32_board_name(void);   /* mis. "esp32doit-devkit-v1"          */
const char *e32_board_chip(void);   /* mis. "ESP32-D0WD" / "ESP32-S3"      */

#ifdef __cplusplus
}
#endif
#endif /* BAIK_E32_PINS_H_ */
