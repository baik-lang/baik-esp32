#!/usr/bin/env bash
#
# tools/build.sh - Bangun firmware BAIK-ESP32 memakai PlatformIO.
#
# PlatformIO dipasang di virtualenv terpisah supaya tidak mengotori Python
# sistem:
#
#   python3 -m venv ~/.baik-pio-venv
#   ~/.baik-pio-venv/bin/pip install -U platformio
#
# Pemakaian:
#   tools/build.sh                 # bangun SEMUA environment
#   tools/build.sh esp32doit-devkit-v1
#   tools/build.sh esp32-s3-devkitc-1
#   tools/build.sh --clean <env>   # bersihkan dulu, lalu bangun
#
# Variabel lingkungan:
#   PIO        - path biner pio (bawaan: $HOME/.baik-pio-venv/bin/pio)
#   LOG_DIR    - direktori berkas log (bawaan: <repo>/.pio/build-logs)
#
set -u -o pipefail

REPO_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
PIO="${PIO:-$HOME/.baik-pio-venv/bin/pio}"
LOG_DIR="${LOG_DIR:-$REPO_DIR/.pio/build-logs}"

# Semua environment yang didukung (harus sama dengan platformio.ini).
ALL_ENVS=(esp32doit-devkit-v1 esp32-s3-devkitc-1)

CLEAN=0
ENVS=()

while [ $# -gt 0 ]; do
  case "$1" in
    --clean|-c) CLEAN=1 ;;
    -h|--help)
      sed -n '2,25p' "${BASH_SOURCE[0]}" | sed 's/^# \{0,1\}//'
      exit 0 ;;
    -*) echo "Opsi tidak dikenal: $1" >&2; exit 2 ;;
    *)  ENVS+=("$1") ;;
  esac
  shift
done

[ ${#ENVS[@]} -eq 0 ] && ENVS=("${ALL_ENVS[@]}")

if [ ! -x "$PIO" ]; then
  cat >&2 <<EOF
GALAT: PlatformIO tidak ditemukan di '$PIO'.
Pasang dulu dengan:
  python3 -m venv \$HOME/.baik-pio-venv
  \$HOME/.baik-pio-venv/bin/pip install -U platformio
Atau set variabel PIO ke path 'pio' milikmu.
EOF
  exit 1
fi

mkdir -p "$LOG_DIR"

echo "==================================================================="
echo " Build firmware BAIK-ESP32"
echo " Repo       : $REPO_DIR"
echo " PlatformIO : $("$PIO" --version 2>/dev/null || echo '?')"
echo " Log        : $LOG_DIR"
echo "==================================================================="

rc_total=0
declare -a HASIL=()

for env in "${ENVS[@]}"; do
  log="$LOG_DIR/build-$env.log"
  echo
  echo "--- [$env] mulai --------------------------------------------------"

  if [ "$CLEAN" -eq 1 ]; then
    echo "[$env] membersihkan..."
    "$PIO" run -d "$REPO_DIR" -e "$env" -t clean >>"$log" 2>&1
  fi

  # Unduhan toolchain pertama kali bisa ratusan MB; sabar.
  "$PIO" run -d "$REPO_DIR" -e "$env" 2>&1 | tee "$log"
  rc=${PIPESTATUS[0]}

  if [ "$rc" -eq 0 ]; then
    echo "[$env] BERHASIL  (log: $log)"
    HASIL+=("BERHASIL  $env")
  else
    echo "[$env] GAGAL kode=$rc  (log: $log)"
    HASIL+=("GAGAL($rc) $env")
    rc_total=1
  fi
done

echo
echo "==================== RINGKASAN BUILD ==============================="
for h in "${HASIL[@]}"; do echo "  $h"; done
echo "===================================================================="

# Artefak firmware ada di .pio/build/<env>/{firmware.bin,bootloader.bin,partitions.bin}
exit "$rc_total"
