# Makefile - Bangun penjalan BAIK untuk PC (uji host).
#
# Hasil: test/host/build/baik-host
#
#   make            # bangun
#   make test       # bangun lalu jalankan seluruh kasus uji
#   make bersih     # hapus seluruh hasil kompilasi (alias: make clean)
#
# Catatan:
#   * -DBAIK_GENERATE_INAC menyamakan konfigurasi dengan platformio.ini.
#   * BAIK_MAIN sengaja TIDAK didefinisikan: blok itu (src/baik/baik_repl.c)
#     memakai GNU readline (readline/add_history/rl_bind_key) yang tidak kita
#     butuhkan untuk uji; main() kita sendiri ada di runner.c.
#   * BAIK_EXPORT_INTERNAL_HEADERS sengaja TIDAK didefinisikan: antarmuka
#     publik src/baik.h sudah cukup untuk runner dan shim.
#   * Inti interpreter TIDAK LAGI satu berkas. Sejak dipecah, sumbernya adalah
#     seluruh src/baik/*.c yang saling berbagi src/baik/baik_internal.h.
#     Karena modul menyertakannya dengan path relatif ("baik_internal.h"),
#     -Isrc/baik wajib ada; -Isrc dibutuhkan karena baik_internal.h
#     menyertakan "baik.h".
#   * Inti BAIK ditulis dalam C89/C99 campuran dan memakai strdup(), maka
#     kita memakai -std=gnu99 supaya deklarasi POSIX ikut terlihat.

CC      ?= cc
DIR     := $(dir $(abspath $(lastword $(MAKEFILE_LIST))))
ROOT    := $(DIR)../..
BUILD   := $(DIR)build
COREDIR := $(ROOT)/src/baik
BIN     := $(BUILD)/baik-host

CFLAGS  ?= -O1 -g
CFLAGS  += -std=gnu99 -DBAIK_GENERATE_INAC -DCS_MMAP
CFLAGS  += -I$(ROOT)/src -I$(COREDIR)
# Catatan: -fcommon TIDAK diperlukan. Sempat dibutuhkan ketika
# baik_internal.h masih memuat definisi tentatif `const char *baik_dlsym_file;`
# di dalam header, yang membuat setiap modul mendefinisikannya ulang. Sekarang
# deklarasinya `extern` dengan satu definisi di baik_ffi.c, jadi berkas ini
# membangun bersih dengan -fno-common bawaan GCC modern.
# Peringatan dari inti BAIK (bukan milik kita) tidak boleh membanjiri layar.
CFLAGS  += -Wall -Wno-unused-function -Wno-discarded-qualifiers \
           -Wno-unused-but-set-variable
LDFLAGS ?=
LDLIBS  += -lm

# Seluruh modul inti interpreter (src/baik/*.c), dicari otomatis supaya
# penambahan/penghapusan modul tidak perlu menyunting Makefile ini.
CORE_SRC := $(sort $(wildcard $(COREDIR)/*.c))
CORE_OBJ := $(patsubst $(COREDIR)/%.c,$(BUILD)/baik/%.o,$(CORE_SRC))
CORE_HDR := $(ROOT)/src/baik.h $(COREDIR)/baik_internal.h

OBJ := $(CORE_OBJ) $(BUILD)/runner.o $(BUILD)/e32_shim.o

.PHONY: all test clean bersih uji

all: $(BIN)

$(BUILD) $(BUILD)/baik:
	@mkdir -p $@

# Aturan pola untuk setiap modul inti.
$(BUILD)/baik/%.o: $(COREDIR)/%.c $(CORE_HDR) | $(BUILD)/baik
	$(CC) $(CFLAGS) -c -o $@ $<

$(BUILD)/runner.o: $(DIR)runner.c $(DIR)shim/e32_shim.h $(ROOT)/src/baik.h | $(BUILD)
	$(CC) $(CFLAGS) -c -o $@ $<

$(BUILD)/e32_shim.o: $(DIR)shim/e32_shim.c $(DIR)shim/e32_shim.h $(ROOT)/src/baik.h | $(BUILD)
	$(CC) $(CFLAGS) -c -o $@ $<

$(BIN): $(OBJ)
	$(CC) $(LDFLAGS) -o $@ $(OBJ) $(LDLIBS)
	@echo "Selesai: $(BIN)"

test: $(BIN)
	@$(DIR)run_tests.sh

# Alias berbahasa Indonesia.
uji: test

bersih clean:
	rm -rf $(BUILD)
