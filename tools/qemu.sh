#!/usr/bin/env bash
#
# tools/qemu.sh - Jalankan firmware BAIK-ESP32 di emulator QEMU.
#
# Skrip ini menggabungkan bootloader + tabel partisi + boot_app0 + firmware
# (+ SPIFFS bila ada) menjadi SATU berkas image flash, lalu menjalankannya di
# qemu-system-xtensa dengan serial disambungkan ke stdio. Dengan begitu REPL
# BAIK bisa dicoba tanpa papan fisik.
#
# Pemakaian:
#   tools/qemu.sh --pasang                  # unduh & pasang QEMU Espressif
#   tools/qemu.sh                           # jalankan env bawaan (esp32)
#   tools/qemu.sh -e esp32-s3-devkitc-1     # jalankan env lain
#   tools/qemu.sh --image-saja              # hanya buat flash.bin, jangan jalan
#   tools/qemu.sh --tanpa-wifi              # build khusus QEMU, WiFi dimatikan
#   tools/qemu.sh --gdb                     # tunggu gdb di :1234 (pio debug)
#
# >>> STATUS --tanpa-wifi: EKSPERIMENTAL, BELUM BERHASIL BOOT. <<<
#     Build normal SELALU menyalakan WiFi di setup(); QEMU tidak
#     mengemulasikan radio, jadi firmware panik di blob PHY lalu boot-loop.
#     Opsi --tanpa-wifi membangun firmware pengganti tanpa WiFi (berhasil
#     dikompilasi), TETAPI di QEMU 9.2.2 firmware itu ter-reset oleh
#     watchdog timer-group sebelum setup() sempat mencetak apa pun.
#     Penyebabnya BELUM ditemukan. Lihat docs/PENGUJIAN.md bagian QEMU.
#
# Keluar dari QEMU: Ctrl-A lalu X.
#
# ---------------------------------------------------------------------------
# CATATAN PENTING TENTANG QEMU (baca sebelum menyalahkan firmware):
#
#  * QEMU untuk Xtensa TIDAK tersedia di apt Ubuntu dan TIDAK ada di registry
#    PlatformIO. Paket "platformio/tool-qemu-xtensa" TIDAK ADA (registry hanya
#    punya "platformio/tool-qemu-riscv" terbitan 2019). Jadi satu-satunya
#    jalan adalah memakai fork resmi Espressif:
#        https://github.com/espressif/qemu/releases
#    Opsi `board_build.qemu` juga bukan fitur PlatformIO resmi.
#
#  * WiFi TIDAK diemulasikan. Semua fungsi WiFi.*, HTTP.*, NTP.*, MDNS.*
#    akan gagal / menggantung di QEMU. Uji itu HARUS di papan sungguhan.
#
#  * Dukungan ESP32-S3 di QEMU masih jauh lebih muda daripada ESP32 klasik.
#    Bila S3 tidak mau boot, coba env esp32doit-devkit-v1 lebih dulu.
#
#  * GPIO diemulasikan seadanya: tidak ada LED/tombol sungguhan. digitalWrite
#    akan berjalan tanpa galat tetapi tidak ada yang bisa diamati. Untuk
#    menguji LOGIKA skrip .ina, pakai uji host (test/host/run_tests.sh) yang
#    jauh lebih cepat dan punya shim GPIO yang mencatat setiap aksi.
#
#  * Yang PALING berguna diuji di QEMU: proses boot, inisialisasi konsol,
#    REPL BAIK, parser/eksekutor bahasa di atas Xtensa sungguhan, dan
#    SPIFFS. Bukan perangkat kerasnya.
# ---------------------------------------------------------------------------
set -u -o pipefail

REPO_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
PIO="${PIO:-$HOME/.baik-pio-venv/bin/pio}"
QEMU_DIR="${QEMU_DIR:-$HOME/.baik-qemu}"
QEMU_BIN="${QEMU_BIN:-$QEMU_DIR/bin/qemu-system-xtensa}"

# Rilis QEMU Espressif yang dipakai (ubah bila ingin versi lain).
QEMU_REL="${QEMU_REL:-esp-develop-9.2.2-20260417}"
QEMU_VER="${QEMU_VER:-esp_develop_9.2.2_20260417}"

ENVNAME="esp32doit-devkit-v1"
PASANG=0
IMAGE_SAJA=0
GDB=0
TANPA_WIFI=0
FLASH_MB="${FLASH_MB:-4}"

while [ $# -gt 0 ]; do
  case "$1" in
    -e|--env)      ENVNAME="$2"; shift ;;
    --pasang|--install) PASANG=1 ;;
    --image-saja|--image-only) IMAGE_SAJA=1 ;;
    --tanpa-wifi|--no-wifi) TANPA_WIFI=1 ;;
    --gdb)         GDB=1 ;;
    -h|--help)     sed -n '2,50p' "${BASH_SOURCE[0]}" | sed 's/^# \{0,1\}//'; exit 0 ;;
    *) echo "Opsi tidak dikenal: $1" >&2; exit 2 ;;
  esac
  shift
done

# ===========================================================================
# 1. Pasang QEMU (fork Espressif) bila diminta / belum ada
# ===========================================================================
pasang_qemu() {
  local arch tarball url tmp
  arch="$(uname -m)"
  case "$arch" in
    x86_64)  arch="x86_64-linux-gnu" ;;
    aarch64) arch="aarch64-linux-gnu" ;;
    *) echo "GAGAL: arsitektur $arch tidak didukung rilis QEMU Espressif." >&2
       return 1 ;;
  esac
  tarball="qemu-xtensa-softmmu-${QEMU_VER}-${arch}.tar.xz"
  url="https://github.com/espressif/qemu/releases/download/${QEMU_REL}/${tarball}"

  echo "==> Mengunduh $tarball"
  tmp="$(mktemp -d)"
  if ! curl -fL --progress-bar -o "$tmp/q.tar.xz" "$url"; then
    echo "GAGAL: unduhan QEMU gagal dari $url" >&2
    rm -rf "$tmp"; return 1
  fi
  mkdir -p "$QEMU_DIR"
  tar xf "$tmp/q.tar.xz" -C "$tmp"
  cp -r "$tmp/qemu/bin" "$tmp/qemu/share" "$QEMU_DIR/"
  rm -rf "$tmp"

  # QEMU Espressif ditaut dinamis ke beberapa pustaka yang sering tidak
  # terpasang (SDL2, slirp, samplerate, Xss, decor). Kita ambil versi
  # runtime-nya ke $QEMU_DIR/lib supaya TIDAK perlu mengubah sistem.
  echo "==> Melengkapi pustaka pendukung di $QEMU_DIR/lib"
  mkdir -p "$QEMU_DIR/lib"
  local work; work="$(mktemp -d)"
  ( cd "$work"
    local i miss base ver lbase cand pkg
    for i in $(seq 1 30); do
      miss="$(LD_LIBRARY_PATH="$QEMU_DIR/lib" "$QEMU_BIN" --version 2>&1 \
              | grep -oP 'lib[^:]*\.so[0-9.]*(?=: cannot open)' | head -1)"
      [ -z "$miss" ] && break
      base="${miss%%.so*}"; ver="${miss##*.so.}"
      lbase="$(echo "$base" | tr 'A-Z' 'a-z')"
      pkg=""
      for cand in "${base}${ver}" "${lbase}${ver}" "${base}-${ver}" \
                  "${lbase}-${ver}" "$lbase"; do
        apt-cache show "$cand" >/dev/null 2>&1 && { pkg="$cand"; break; }
      done
      if [ -z "$pkg" ]; then
        echo "   ! tidak tahu paket untuk $miss — pasang manual" >&2; break
      fi
      echo "   - $miss  (paket $pkg)"
      apt-get download "$pkg" >/dev/null 2>&1 || {
        echo "   ! gagal mengunduh $pkg" >&2; break; }
      for d in ./*.deb; do dpkg-deb -x "$d" . 2>/dev/null; done
      rm -f ./*.deb
      cp -a ./usr/lib/*/. "$QEMU_DIR/lib/" 2>/dev/null
    done )
  rm -rf "$work"

  if LD_LIBRARY_PATH="$QEMU_DIR/lib" "$QEMU_BIN" --version >/dev/null 2>&1; then
    echo "==> QEMU siap: $(LD_LIBRARY_PATH="$QEMU_DIR/lib" "$QEMU_BIN" --version | head -1)"
    return 0
  fi
  echo "GAGAL: QEMU terpasang tetapi tidak bisa dijalankan:" >&2
  LD_LIBRARY_PATH="$QEMU_DIR/lib" "$QEMU_BIN" --version >&2
  return 1
}

if [ "$PASANG" -eq 1 ]; then
  pasang_qemu || exit 1
  [ "$IMAGE_SAJA" -eq 0 ] || exit 0
fi

if [ ! -x "$QEMU_BIN" ]; then
  echo "QEMU belum terpasang di $QEMU_BIN."
  echo "Jalankan dulu:  tools/qemu.sh --pasang"
  exit 1
fi
export LD_LIBRARY_PATH="$QEMU_DIR/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"

# ===========================================================================
# 2. Pastikan artefak firmware ada
# ===========================================================================
# Build "tanpa WiFi" memakai DIREKTORI BUILD TERPISAH (.pio/build-qemu).
# Ini penting: PlatformIO membersihkan direktori build setiap kali flag
# kompilasi berubah, jadi kalau build QEMU dan build normal berbagi direktori,
# keduanya akan saling menghapus artefak.
if [ "$TANPA_WIFI" -eq 1 ]; then
  export PLATFORMIO_BUILD_DIR="$REPO_DIR/.pio/build-qemu"
  # Tukar src/main.cpp dengan test/qemu/main_qemu.cpp — TANPA menyentuh
  # platformio.ini maupun src/.
  export PLATFORMIO_BUILD_SRC_FILTER="+<*> -<main.cpp> +<../test/qemu/main_qemu.cpp>"
  # Karena main_qemu.cpp tidak memakai WiFi/WebServer, pencari pustaka (LDF)
  # PlatformIO berhenti menyediakan header pustaka framework yang masih
  # dibutuhkan ESP32Console. Kita tambahkan semuanya ke include path.
  FW_LIBS="$HOME/.platformio/packages/framework-arduinoespressif32/libraries"
  INC=""
  for d in "$FW_LIBS"/*/src; do [ -d "$d" ] && INC="$INC -I$d"; done
  export PLATFORMIO_BUILD_FLAGS="$INC"
  BUILD_DIR="$PLATFORMIO_BUILD_DIR/$ENVNAME"
  echo "==> Mode TANPA WiFi (build terpisah di $PLATFORMIO_BUILD_DIR)"
else
  BUILD_DIR="$REPO_DIR/.pio/build/$ENVNAME"
fi

BOOTLOADER="$BUILD_DIR/bootloader.bin"
PARTITIONS="$BUILD_DIR/partitions.bin"
FIRMWARE="$BUILD_DIR/firmware.bin"
SPIFFS="$BUILD_DIR/spiffs.bin"

# SPIFFS dibangun DULU (target buildfs), baru firmware. Urutan ini penting:
# `pio run -t buildfs` sesudah `pio run` bisa memicu pembersihan direktori.
if [ -d "$REPO_DIR/data" ]; then
  echo "==> Membangun image SPIFFS dari data/"
  "$PIO" run -d "$REPO_DIR" -e "$ENVNAME" -t buildfs >/dev/null 2>&1 || \
    echo "   ! buildfs gagal — lanjut tanpa SPIFFS"
fi

echo "==> Membangun firmware env $ENVNAME"
"$PIO" run -d "$REPO_DIR" -e "$ENVNAME" || {
  echo "GAGAL: build firmware gagal — perbaiki dulu galat kompilasinya." >&2
  exit 1; }

for f in "$BOOTLOADER" "$PARTITIONS" "$FIRMWARE"; do
  [ -f "$f" ] || { echo "GAGAL: berkas tidak ada: $f" >&2; exit 1; }
done

# boot_app0 (penanda partisi OTA) ikut di-flash oleh PlatformIO pada 0xe000.
BOOTAPP0="$(find "$HOME/.platformio/packages/framework-arduinoespressif32" \
            -name boot_app0.bin 2>/dev/null | head -1)"

# ===========================================================================
# 3. Gabungkan jadi satu image flash
# ===========================================================================
# Offset berbeda antar chip: ESP32 klasik menaruh bootloader di 0x1000,
# sedangkan ESP32-S3 (dan C3) di 0x0.
case "$ENVNAME" in
  *s3*) MACHINE="esp32s3"; OFF_BOOTLOADER="0x0" ;;
  *)    MACHINE="esp32";   OFF_BOOTLOADER="0x1000" ;;
esac

OUT="$BUILD_DIR/flash-qemu.bin"
echo "==> Menggabungkan image flash ${FLASH_MB}MB -> $OUT"

ARGS=( "$OFF_BOOTLOADER" "$BOOTLOADER" 0x8000 "$PARTITIONS" )
[ -n "$BOOTAPP0" ] && [ -f "$BOOTAPP0" ] && ARGS+=( 0xe000 "$BOOTAPP0" )
ARGS+=( 0x10000 "$FIRMWARE" )

# Offset SPIFFS dibaca dari tabel partisi, bukan ditebak.
if [ -f "$SPIFFS" ]; then
  SPIFFS_OFF="$(python3 - "$PARTITIONS" <<'PY'
import struct, sys
# Tabel partisi ESP-IDF: entri 32 byte, magic 0xAA50
data = open(sys.argv[1], 'rb').read()
for i in range(0, len(data), 32):
    e = data[i:i+32]
    if len(e) < 32 or e[0:2] != b'\xaa\x50':
        continue
    typ, sub = e[2], e[3]
    off, size = struct.unpack('<II', e[4:12])
    name = e[12:28].rstrip(b'\x00').decode('ascii', 'replace')
    if name == 'spiffs' or (typ == 1 and sub == 0x82):
        print(hex(off)); break
PY
)"
  if [ -n "$SPIFFS_OFF" ]; then
    echo "    SPIFFS pada offset $SPIFFS_OFF"
    ARGS+=( "$SPIFFS_OFF" "$SPIFFS" )
  else
    echo "    ! offset SPIFFS tidak ditemukan di tabel partisi — dilewati"
  fi
fi

# esptool.py ikut terpasang bersama PlatformIO.
ESPTOOL="$(find "$HOME/.platformio/packages/tool-esptoolpy" -name 'esptool.py' \
           2>/dev/null | head -1)"
PYBIN="$(dirname "$PIO")/python"
[ -x "$PYBIN" ] || PYBIN="python3"

if [ -n "$ESPTOOL" ]; then
  "$PYBIN" "$ESPTOOL" --chip "$MACHINE" merge_bin \
      --fill-flash-size "${FLASH_MB}MB" --output "$OUT" "${ARGS[@]}" \
    || { echo "GAGAL: esptool merge_bin gagal." >&2; exit 1; }
else
  # Cadangan: rakit sendiri dengan dd bila esptool tidak ketemu.
  echo "    esptool.py tidak ketemu — merakit image dengan dd"
  dd if=/dev/zero of="$OUT" bs=1M count="$FLASH_MB" status=none
  # flash kosong bernilai 0xFF
  "$PYBIN" - "$OUT" "$FLASH_MB" <<'PY'
import sys
p, mb = sys.argv[1], int(sys.argv[2])
open(p, 'wb').write(b'\xff' * (mb * 1024 * 1024))
PY
  i=0
  while [ $i -lt ${#ARGS[@]} ]; do
    off="${ARGS[$i]}"; file="${ARGS[$((i+1))]}"
    dd if="$file" of="$OUT" bs=1 seek="$((off))" conv=notrunc status=none
    i=$((i+2))
  done
fi

ls -la "$OUT"
[ "$IMAGE_SAJA" -eq 1 ] && { echo "Selesai (image saja)."; exit 0; }

# ===========================================================================
# 4. Jalankan QEMU
# ===========================================================================
echo
echo "==================================================================="
echo " Menjalankan $MACHINE di QEMU. Serial tersambung ke terminal ini."
echo " Keluar: tekan Ctrl-A lalu X."
echo " Ingat: WiFi TIDAK diemulasikan; GPIO tidak terhubung ke apa pun."
echo "==================================================================="
echo

QARGS=( -nographic -machine "$MACHINE"
        -drive "file=$OUT,if=mtd,format=raw"
        -serial mon:stdio )

# ESP32 klasik butuh 4MB flash + efuse default; S3 memakai ROM bawaan QEMU.
if [ "$GDB" -eq 1 ]; then
  echo "(menunggu gdb pada :1234 — sambungkan dengan 'pio debug' atau xtensa-gdb)"
  QARGS+=( -s -S )
fi

exec "$QEMU_BIN" "${QARGS[@]}"
