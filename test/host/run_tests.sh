#!/usr/bin/env bash
#
# run_tests.sh - Bangun penjalan BAIK untuk PC lalu jalankan seluruh kasus uji.
#
# Setiap berkas cases/<nama>.ina dijalankan, keluarannya (stdout+stderr,
# ditambah baris "[keluar=N]" berisi kode keluar) dibandingkan dengan
# cases/<nama>.expected.
#
# Pemakaian:
#   test/host/run_tests.sh              # jalankan semua kasus
#   test/host/run_tests.sh 03 json      # hanya kasus yang namanya cocok
#   test/host/run_tests.sh --rekam      # REKAM ulang berkas .expected
#   test/host/run_tests.sh --verbose    # tampilkan keluaran penuh tiap kasus
#
# Sebelum membangun, skrip memastikan berkas lama src/baik.c TIDAK ada lagi
# (intinya kini tersebar di src/baik/*.c); bila ia kembali, penautan akan
# gagal dengan "multiple definition of ...".
#
# Kode keluar: 0 bila semua lulus, 1 bila ada yang gagal.
#
set -u -o pipefail

DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
CASES="$DIR/cases"
BIN="$DIR/build/baik-host"

REKAM=0
VERBOSE=0
POLA=()

while [ $# -gt 0 ]; do
  case "$1" in
    --rekam|--record) REKAM=1 ;;
    --verbose|-v)     VERBOSE=1 ;;
    -h|--help)
      sed -n '2,20p' "${BASH_SOURCE[0]}" | sed 's/^# \{0,1\}//'
      exit 0 ;;
    *) POLA+=("$1") ;;
  esac
  shift
done

# --- warna (hanya bila stdout adalah terminal) ---------------------------
if [ -t 1 ]; then
  C_OK=$'\033[32m'; C_NO=$'\033[31m'; C_HL=$'\033[36m'; C_0=$'\033[0m'
else
  C_OK=''; C_NO=''; C_HL=''; C_0=''
fi

# --- pemeriksaan susunan sumber ------------------------------------------
# Inti interpreter sudah dipecah dari satu berkas src/baik.c menjadi modul
# src/baik/*.c. Kalau src/baik.c muncul lagi (mis. karena revert, merge, atau
# salin-balik), Makefile akan tetap mengompilasi src/baik/*.c SEKALIGUS ada
# berkas lama yang isinya sama; begitu ia ikut terkompilasi, penaut gagal
# dengan ratusan "multiple definition of ...". Lebih baik berhenti di sini
# dengan pesan yang jelas daripada menerima banjir galat penaut.
ROOT="$(cd "$DIR/../.." && pwd)"
if [ -e "$ROOT/src/baik.c" ]; then
  cat >&2 <<PESAN
${C_NO}GAGAL: $ROOT/src/baik.c masih ada.${C_0}

Inti interpreter kini tersebar di src/baik/*.c (lihat src/baik/baik_internal.h).
Berkas tunggal src/baik.c yang lama HARUS sudah terhapus; bila ia kembali,
simbol yang sama terdefinisi dua kali dan penautan akan gagal.

Perbaiki dengan menghapus berkas itu:
    git rm src/baik.c        # bila ia terlacak git
    rm     src/baik.c        # bila ia hanya berkas lepas
PESAN
  exit 1
fi

# --- bangun dulu ---------------------------------------------------------
echo "${C_HL}==> Membangun penjalan BAIK untuk PC...${C_0}"
if ! make -s -C "$DIR" 2>&1 | sed 's/^/    /'; then
  echo "${C_NO}GAGAL: kompilasi penjalan gagal.${C_0}" >&2
  exit 1
fi
if [ ! -x "$BIN" ]; then
  echo "${C_NO}GAGAL: $BIN tidak terbentuk.${C_0}" >&2
  exit 1
fi

# --- kumpulkan kasus -----------------------------------------------------
mapfile -t SEMUA < <(find "$CASES" -maxdepth 1 -name '*.ina' | sort)

DAFTAR=()
if [ ${#POLA[@]} -eq 0 ]; then
  DAFTAR=("${SEMUA[@]}")
else
  for f in "${SEMUA[@]}"; do
    for p in "${POLA[@]}"; do
      case "$(basename "$f")" in *"$p"*) DAFTAR+=("$f"); break ;; esac
    done
  done
fi

if [ ${#DAFTAR[@]} -eq 0 ]; then
  echo "${C_NO}Tidak ada kasus uji yang cocok.${C_0}" >&2
  exit 1
fi

echo "${C_HL}==> Menjalankan ${#DAFTAR[@]} kasus uji${C_0}"
echo

LULUS=0
GAGAL=0
TEREKAM=0
NAMA_GAGAL=()
TMP="$(mktemp -d)"
trap 'rm -rf "$TMP"' EXIT

for ina in "${DAFTAR[@]}"; do
  nama="$(basename "$ina" .ina)"
  exp="${ina%.ina}.expected"
  out="$TMP/$nama.out"

  # Jalankan dari direktori cases DAN serahkan hanya nama berkasnya.
  # Penting: saat terjadi galat runtime, interpreter mencetak jejak
  # "  at <berkas>:<baris>" memakai path PERSIS seperti yang kita berikan.
  # Kalau kita menyerahkan path absolut, berkas .expected ikut memuat path
  # absolut mesin perekam dan kasus itu hanya lulus di mesin tersebut.
  ( cd "$CASES" && "$BIN" "$nama.ina" ) >"$out" 2>&1
  rc=$?
  echo "[keluar=$rc]" >>"$out"

  if [ "$REKAM" -eq 1 ]; then
    cp "$out" "$exp"
    printf '  %-28s %sREKAM%s\n' "$nama" "$C_HL" "$C_0"
    TEREKAM=$((TEREKAM + 1))
    continue
  fi

  if [ ! -f "$exp" ]; then
    printf '  %-28s %sGAGAL%s (tidak ada berkas .expected)\n' "$nama" "$C_NO" "$C_0"
    GAGAL=$((GAGAL + 1)); NAMA_GAGAL+=("$nama")
    continue
  fi

  if diff -q "$exp" "$out" >/dev/null 2>&1; then
    printf '  %-28s %sLULUS%s\n' "$nama" "$C_OK" "$C_0"
    LULUS=$((LULUS + 1))
    [ "$VERBOSE" -eq 1 ] && sed 's/^/        | /' "$out"
  else
    printf '  %-28s %sGAGAL%s\n' "$nama" "$C_NO" "$C_0"
    GAGAL=$((GAGAL + 1)); NAMA_GAGAL+=("$nama")
    echo "    --- beda (- diharapkan / + didapat) ---"
    diff -u "$exp" "$out" | tail -n +3 | sed 's/^/    /'
    echo "    ---------------------------------------"
  fi
done

echo
echo "==================== RINGKASAN UJI HOST ===================="
if [ "$REKAM" -eq 1 ]; then
  echo "  Berkas .expected direkam ulang : $TEREKAM"
  echo "============================================================"
  exit 0
fi
echo "  Total  : $((LULUS + GAGAL))"
echo "  LULUS  : $LULUS"
echo "  GAGAL  : $GAGAL"
if [ "$GAGAL" -gt 0 ]; then
  echo "  Yang gagal:"
  for n in "${NAMA_GAGAL[@]}"; do echo "    - $n"; done
fi
echo "============================================================"

[ "$GAGAL" -eq 0 ] || exit 1
echo "${C_OK}Semua kasus uji LULUS.${C_0}"
exit 0
