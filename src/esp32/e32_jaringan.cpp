/*
 * e32_jaringan.cpp - Modul 5: JARINGAN untuk bahasa BAIK di ESP32.
 *
 * Mendaftarkan object:
 *   WiFi  - sambungan WiFi station / access point + pemindaian jaringan
 *   HTTP  - klien HTTP/HTTPS sederhana (GET / POST / getJSON)
 *   NTP   - waktu jaringan (alias object: `Waktu`)
 *   MDNS  - penamaan .local di jaringan lokal
 * beserta konstanta status (WL_*) dan mode (WIFI_*) WiFi pada object global.
 *
 * Aturan modul ini:
 *   - Seluruh simbol internal bersifat `static`.
 *   - Tidak ada fungsi yang boleh menggantung (hang) selamanya. Setiap
 *     penungguan memakai batas waktu (timeout) yang jelas.
 *   - Semua fungsi aman dipanggil walaupun WiFi belum tersambung: yang
 *     terjadi hanyalah nilai balik galat + pesan berbahasa Indonesia.
 *   - Galat dilaporkan lewat nilai balik (`kosong` / `salah` / `-1`) plus
 *     pesan ke konsol, sebab bahasa BAIK tidak punya exception.
 *
 * Catatan keluaran: kita memakai printf() (bukan Serial.print) supaya pesan
 * galat memakai aliran keluaran yang sama dengan `tulis()` milik BAIK,
 * sehingga urutan cetakan di konsol tetap rapi.
 */

#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <ESPmDNS.h>

#include <time.h>
#include <string.h>
#include <stdio.h>

#include "baik_esp32.h"

/* ======================================================================== */
/* Keadaan modul (state)                                                    */
/* ======================================================================== */

/* Kode status HTTP terakhir. Positif = kode HTTP (200, 404, ...),
 * negatif = galat transport HTTPClient, 0 = belum ada permintaan. */
static int s_http_last_status = 0;

/* Batas waktu klien HTTP (milidetik). Sengaja tidak terlalu besar supaya
 * skrip tidak terasa menggantung bila server tidak menjawab. */
static const uint16_t E32_HTTP_TIMEOUT_MS = 10000;

/* Pengaturan NTP. Bawaan: WIB (UTC+7) = 7 * 3600 = 25200 detik. */
#define E32_NTP_OFFSET_WIB 25200L

static char s_ntp_server[64] = "pool.ntp.org";
static long s_ntp_offset     = E32_NTP_OFFSET_WIB;
static int  s_ntp_dst        = 0;
static int  s_ntp_begun      = 0;

/* ======================================================================== */
/* Utilitas internal                                                        */
/* ======================================================================== */

/* Salin string argumen ke buffer milik kita sendiri (aman dari relokasi
 * buffer string interpreter). Selalu diakhiri NUL. */
static void e32_copy_arg(struct baik *baik, int n, const char *def,
                         char *buf, size_t buflen) {
  const char *s = e32_arg_str(baik, n, def);
  if (buf == NULL || buflen == 0) return;
  if (s == NULL) {
    buf[0] = '\0';
    return;
  }
  snprintf(buf, buflen, "%s", s);
}

/* Kembalikan `kosong` (null) ke skrip BAIK. */
static void e32_ret_null(struct baik *baik) {
  baik_return(baik, baik_mk_null());
}

/* Terjemahan wl_status_t ke bahasa Indonesia. */
static const char *e32_wifi_status_text(int st) {
  switch (st) {
    case WL_IDLE_STATUS:     return "menganggur";
    case WL_NO_SSID_AVAIL:   return "SSID tidak ditemukan";
    case WL_SCAN_COMPLETED:  return "pemindaian selesai";
    case WL_CONNECTED:       return "tersambung";
    case WL_CONNECT_FAILED:  return "gagal tersambung";
    case WL_CONNECTION_LOST: return "koneksi terputus";
    case WL_DISCONNECTED:    return "tidak tersambung";
    case WL_NO_SHIELD:       return "perangkat WiFi tidak aktif";
    default:                 return "status tidak dikenal";
  }
}

/* Terjemahan jenis enkripsi hasil pemindaian ke string yang mudah dibaca. */
static const char *e32_enc_text(int enc) {
  switch (enc) {
    case WIFI_AUTH_OPEN:            return "terbuka";
    case WIFI_AUTH_WEP:             return "WEP";
    case WIFI_AUTH_WPA_PSK:         return "WPA-PSK";
    case WIFI_AUTH_WPA2_PSK:        return "WPA2-PSK";
    case WIFI_AUTH_WPA_WPA2_PSK:    return "WPA/WPA2-PSK";
    case WIFI_AUTH_WPA2_ENTERPRISE: return "WPA2-Enterprise";
    case WIFI_AUTH_WPA3_PSK:        return "WPA3-PSK";
    case WIFI_AUTH_WPA2_WPA3_PSK:   return "WPA2/WPA3-PSK";
    case WIFI_AUTH_WAPI_PSK:        return "WAPI-PSK";
    default:                        return "tidak diketahui";
  }
}

/* 1 bila WiFi station sudah tersambung. Tidak pernah memblokir. */
static int e32_wifi_siap(const char *fname) {
  if (WiFi.status() == WL_CONNECTED) return 1;
  printf("Galat %s: WiFi belum tersambung (%s). "
         "Panggil WiFi.begin(ssid, sandi) lalu WiFi.tungguKoneksi(15000).\n",
         fname, e32_wifi_status_text((int) WiFi.status()));
  return 0;
}

/* ======================================================================== */
/* Object WiFi                                                              */
/* ======================================================================== */

/* WiFi.begin(ssid[, sandi]) -> angka status awal */
static void bk_wifi_begin(struct baik *baik) {
  char ssid[64];
  char pass[80];

  e32_copy_arg(baik, 0, "", ssid, sizeof(ssid));
  e32_copy_arg(baik, 1, "", pass, sizeof(pass));

  if (ssid[0] == '\0') {
    printf("Galat WiFi.begin: nama jaringan (ssid) kosong.\n");
    e32_ret_int(baik, (long) WL_CONNECT_FAILED);
    return;
  }

  /* Sandi kosong berarti jaringan terbuka -> kirim NULL, bukan "". */
  if (pass[0] == '\0') {
    WiFi.begin(ssid);
  } else {
    WiFi.begin(ssid, pass);
  }
  e32_ret_int(baik, (long) WiFi.status());
}

/* WiFi.disconnect([matikanRadio]) -> boolean */
static void bk_wifi_disconnect(struct baik *baik) {
  int off = e32_arg_bool(baik, 0, 0);
  bool ok = WiFi.disconnect(off ? true : false);
  e32_ret_bool(baik, ok ? 1 : 0);
}

/* WiFi.status() -> angka (wl_status_t) */
static void bk_wifi_status(struct baik *baik) {
  e32_ret_int(baik, (long) WiFi.status());
}

/* WiFi.statusText() -> string bahasa Indonesia */
static void bk_wifi_status_text(struct baik *baik) {
  e32_ret_str(baik, e32_wifi_status_text((int) WiFi.status()));
}

/* WiFi.isConnected() -> boolean */
static void bk_wifi_is_connected(struct baik *baik) {
  e32_ret_bool(baik, WiFi.status() == WL_CONNECTED ? 1 : 0);
}

/* WiFi.localIP() -> string */
static void bk_wifi_local_ip(struct baik *baik) {
  String s = WiFi.localIP().toString();
  e32_ret_str(baik, s.c_str());
}

/* WiFi.gatewayIP() -> string */
static void bk_wifi_gateway_ip(struct baik *baik) {
  String s = WiFi.gatewayIP().toString();
  e32_ret_str(baik, s.c_str());
}

/* WiFi.subnetMask() -> string */
static void bk_wifi_subnet_mask(struct baik *baik) {
  String s = WiFi.subnetMask().toString();
  e32_ret_str(baik, s.c_str());
}

/* WiFi.dnsIP([nomor]) -> string */
static void bk_wifi_dns_ip(struct baik *baik) {
  int idx = e32_arg_int(baik, 0, 0);
  if (idx < 0) idx = 0;
  if (idx > 2) idx = 2;
  String s = WiFi.dnsIP((uint8_t) idx).toString();
  e32_ret_str(baik, s.c_str());
}

/* WiFi.macAddress() -> string */
static void bk_wifi_mac_address(struct baik *baik) {
  String s = WiFi.macAddress();
  e32_ret_str(baik, s.c_str());
}

/* WiFi.RSSI() -> angka (dBm) */
static void bk_wifi_rssi(struct baik *baik) {
  e32_ret_int(baik, (long) WiFi.RSSI());
}

/* WiFi.SSID() -> string nama jaringan yang sedang dipakai */
static void bk_wifi_ssid(struct baik *baik) {
  String s = WiFi.SSID();
  e32_ret_str(baik, s.c_str());
}

/* WiFi.channel() -> angka kanal */
static void bk_wifi_channel(struct baik *baik) {
  e32_ret_int(baik, (long) WiFi.channel());
}

/* WiFi.mode(m) -> boolean; m = WIFI_OFF/WIFI_STA/WIFI_AP/WIFI_AP_STA */
static void bk_wifi_mode(struct baik *baik) {
  int m = e32_arg_int(baik, 0, (int) WIFI_STA);
  if (m < 0 || m > 3) {
    printf("Galat WiFi.mode: mode %d tidak sah. "
           "Pakai WIFI_OFF, WIFI_STA, WIFI_AP, atau WIFI_AP_STA.\n", m);
    e32_ret_bool(baik, 0);
    return;
  }
  e32_ret_bool(baik, WiFi.mode((wifi_mode_t) m) ? 1 : 0);
}

/* WiFi.setHostname(nama) -> boolean */
static void bk_wifi_set_hostname(struct baik *baik) {
  char nama[64];
  e32_copy_arg(baik, 0, "", nama, sizeof(nama));
  if (nama[0] == '\0') {
    printf("Galat WiFi.setHostname: nama host kosong.\n");
    e32_ret_bool(baik, 0);
    return;
  }
  e32_ret_bool(baik, WiFi.setHostname(nama) ? 1 : 0);
}

/* WiFi.softAP(ssid[, sandi[, kanal]]) -> boolean */
static void bk_wifi_soft_ap(struct baik *baik) {
  char ssid[64];
  char pass[80];
  int kanal;

  e32_copy_arg(baik, 0, "", ssid, sizeof(ssid));
  e32_copy_arg(baik, 1, "", pass, sizeof(pass));
  kanal = e32_arg_int(baik, 2, 1);

  if (ssid[0] == '\0') {
    printf("Galat WiFi.softAP: nama jaringan (ssid) kosong.\n");
    e32_ret_bool(baik, 0);
    return;
  }
  if (kanal < 1 || kanal > 13) {
    printf("Galat WiFi.softAP: kanal %d di luar jangkauan 1..13.\n", kanal);
    e32_ret_bool(baik, 0);
    return;
  }
  /* WPA2 minimal 8 karakter; sandi lebih pendek dianggap AP terbuka. */
  if (pass[0] != '\0' && strlen(pass) < 8) {
    printf("Galat WiFi.softAP: sandi minimal 8 karakter. "
           "AP dibuat TANPA sandi.\n");
    pass[0] = '\0';
  }

  if (pass[0] == '\0') {
    e32_ret_bool(baik, WiFi.softAP(ssid, NULL, kanal) ? 1 : 0);
  } else {
    e32_ret_bool(baik, WiFi.softAP(ssid, pass, kanal) ? 1 : 0);
  }
}

/* WiFi.softAPIP() -> string */
static void bk_wifi_soft_ap_ip(struct baik *baik) {
  String s = WiFi.softAPIP().toString();
  e32_ret_str(baik, s.c_str());
}

/* WiFi.softAPgetStationNum() -> angka klien yang menempel */
static void bk_wifi_soft_ap_station_num(struct baik *baik) {
  e32_ret_int(baik, (long) WiFi.softAPgetStationNum());
}

/*
 * WiFi.scanNetworks() -> array object {ssid, rssi, kanal, enkripsi, bssid}
 *
 * Pemindaian dilakukan secara sinkron (blocking) tetapi dibatasi oleh
 * driver WiFi sendiri, jadi tidak menggantung selamanya. Hasil pemindaian
 * disimpan di dalam driver dan WAJIB dibebaskan dengan WiFi.scanDelete().
 */
static void bk_wifi_scan_networks(struct baik *baik) {
  baik_val_t arr;
  int n, i;

  /* Array di-own supaya tidak tersapu pengumpul sampah (GC) saat kita
   * masih membangun isinya. */
  arr = baik_mk_array(baik);
  baik_own(baik, &arr);

  n = (int) WiFi.scanNetworks();
  if (n < 0) {
    printf("Galat WiFi.scanNetworks: pemindaian gagal (kode %d). "
           "Pastikan mode WiFi aktif (WiFi.mode(WIFI_STA)).\n", n);
    WiFi.scanDelete();
    baik_disown(baik, &arr);
    baik_return(baik, arr);
    return;
  }

  for (i = 0; i < n; i++) {
    uint8_t idx  = (uint8_t) i;
    baik_val_t o = baik_mk_object(baik);
    String ssid  = WiFi.SSID(idx);
    String bssid = WiFi.BSSIDstr(idx);

    /* Masukkan dulu ke array (supaya object terjangkau dari nilai yang
     * sudah di-own), baru diisi propertinya. */
    baik_array_push(baik, arr, o);

    baik_set(baik, o, "ssid", ~0,
             baik_mk_string(baik, ssid.c_str(), ~(size_t) 0, 1));
    baik_set(baik, o, "rssi", ~0,
             baik_mk_number(baik, (double) WiFi.RSSI(idx)));
    baik_set(baik, o, "kanal", ~0,
             baik_mk_number(baik, (double) WiFi.channel(idx)));
    baik_set(baik, o, "enkripsi", ~0,
             baik_mk_string(baik, e32_enc_text((int) WiFi.encryptionType(idx)),
                            ~(size_t) 0, 1));
    baik_set(baik, o, "bssid", ~0,
             baik_mk_string(baik, bssid.c_str(), ~(size_t) 0, 1));
  }

  /* Bebaskan memori hasil pemindaian di dalam driver WiFi. */
  WiFi.scanDelete();

  baik_disown(baik, &arr);
  baik_return(baik, arr);
}

/* WiFi.sleep(aktif) -> boolean; hemat daya modem */
static void bk_wifi_sleep(struct baik *baik) {
  int on = e32_arg_bool(baik, 0, 1);
  WiFi.setSleep(on ? true : false);
  e32_ret_bool(baik, 1);
}

/*
 * WiFi.setTxPower(dbm) -> boolean
 *
 * API Arduino memakai enum wifi_power_t yang bersatuan 1/4 dBm. Kita
 * memakai daftar nilai mentah (bukan nama enum) supaya tetap kompatibel
 * antara arduino-esp32 2.x dan 3.x, lalu memilih nilai sah yang paling
 * dekat dengan permintaan pengguna.
 */
static void bk_wifi_set_tx_power(struct baik *baik) {
  /* Nilai sah wifi_power_t dalam satuan seperempat dBm. */
  static const int kQuarter[] = {-4, 8, 20, 28, 34, 44, 52, 60, 68, 74, 76, 78};
  const int kCount = (int) (sizeof(kQuarter) / sizeof(kQuarter[0]));
  double dbm = e32_arg_num(baik, 0, 19.5);
  int want = (int) (dbm * 4.0);
  int best = kQuarter[0];
  int bestDiff = -1;
  int i;

  for (i = 0; i < kCount; i++) {
    int diff = kQuarter[i] - want;
    if (diff < 0) diff = -diff;
    if (bestDiff < 0 || diff < bestDiff) {
      bestDiff = diff;
      best = kQuarter[i];
    }
  }
  e32_ret_bool(baik, WiFi.setTxPower((wifi_power_t) best) ? 1 : 0);
}

/*
 * WiFi.tungguKoneksi([timeoutMs]) -> boolean   (alias WiFi.waitConnected)
 *
 * Menunggu sampai status WL_CONNECTED atau batas waktu habis. SELALU
 * memakai batas waktu -- tidak pernah mengulang tanpa batas.
 */
static void bk_wifi_tunggu_koneksi(struct baik *baik) {
  long timeout = (long) e32_arg_int(baik, 0, 15000);
  unsigned long mulai;

  if (timeout < 0) timeout = 0;
  if (timeout > 120000L) timeout = 120000L; /* batas atas pengaman */

  mulai = millis();
  while (WiFi.status() != WL_CONNECTED) {
    if ((long) (millis() - mulai) >= timeout) {
      printf("WiFi.tungguKoneksi: batas waktu %ld ms habis (%s).\n",
             timeout, e32_wifi_status_text((int) WiFi.status()));
      e32_ret_bool(baik, 0);
      return;
    }
    delay(100);
  }
  e32_ret_bool(baik, 1);
}

/* ======================================================================== */
/* Object HTTP                                                              */
/* ======================================================================== */

/*
 * Inti permintaan HTTP/HTTPS.
 *
 * metode  : "GET" atau "POST"
 * body    : isi untuk POST (boleh NULL)
 * ctype   : Content-Type untuk POST (boleh NULL -> bawaan)
 * keluar  : diisi badan jawaban bila berhasil
 *
 * Kembalikan 1 bila berhasil (ada jawaban HTTP), 0 bila gagal.
 * Kode status terakhir selalu disimpan di s_http_last_status.
 *
 * TLS / HTTPS: memakai WiFiClientSecure dengan setInsecure().
 *   PERINGATAN KEAMANAN: setInsecure() membuat sambungan TLS TIDAK
 *   memverifikasi sertifikat server sama sekali. Sambungan tetap
 *   terenkripsi, tetapi TIDAK terlindung dari serangan man-in-the-middle.
 *   Ini dipilih supaya skrip BAIK tidak perlu menanam sertifikat CA di
 *   dalam flash. Untuk keperluan yang menuntut keamanan sungguhan,
 *   gunakan pustaka/firmware yang menanam CA root sendiri.
 */
static int e32_http_request(const char *url, const char *metode,
                            const char *body, const char *ctype,
                            String &keluar) {
  HTTPClient http;
  WiFiClient plain;
  WiFiClientSecure *tls = NULL;
  WiFiClient *client = NULL;
  String u;
  int code;
  int aman;

  keluar = "";
  s_http_last_status = 0;

  if (url == NULL || url[0] == '\0') {
    printf("Galat HTTP: alamat (URL) kosong.\n");
    s_http_last_status = -1;
    return 0;
  }
  if (!e32_wifi_siap("HTTP")) {
    s_http_last_status = -1;
    return 0;
  }

  u = String(url);
  if (!u.startsWith("http://") && !u.startsWith("https://")) {
    printf("Galat HTTP: alamat harus diawali http:// atau https:// -> %s\n",
           url);
    s_http_last_status = -1;
    return 0;
  }
  aman = u.startsWith("https://") ? 1 : 0;

  if (aman) {
    tls = new WiFiClientSecure();
    if (tls == NULL) {
      printf("Galat HTTP: memori tidak cukup untuk sambungan TLS.\n");
      s_http_last_status = -1;
      return 0;
    }
    /* Sertifikat server TIDAK diverifikasi -- lihat catatan di atas. */
    tls->setInsecure();
    client = tls;
  } else {
    client = &plain;
  }

  http.setConnectTimeout(E32_HTTP_TIMEOUT_MS);
  http.setTimeout(E32_HTTP_TIMEOUT_MS);
  http.setReuse(false);
  http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);

  if (!http.begin(*client, u)) {
    printf("Galat HTTP: alamat tidak bisa diurai -> %s\n", url);
    s_http_last_status = -1;
    if (tls != NULL) delete tls;
    return 0;
  }

  if (strcmp(metode, "POST") == 0) {
    http.addHeader("Content-Type",
                   (ctype != NULL && ctype[0] != '\0')
                       ? ctype
                       : "application/x-www-form-urlencoded");
    code = http.POST(String(body != NULL ? body : ""));
  } else {
    code = http.GET();
  }

  s_http_last_status = code;

  if (code <= 0) {
    printf("Galat HTTP %s: permintaan gagal (kode %d: %s) -> %s\n",
           metode, code, http.errorToString(code).c_str(), url);
    http.end();                       /* selalu tutup sambungan */
    if (tls != NULL) delete tls;
    return 0;
  }

  keluar = http.getString();
  if (code >= 400) {
    printf("Peringatan HTTP %s: server menjawab kode %d -> %s\n",
           metode, code, url);
  }

  http.end();                         /* selalu tutup sambungan */
  if (tls != NULL) delete tls;
  return 1;
}

/* HTTP.get(url) -> string badan jawaban, atau `kosong` bila gagal */
static void bk_http_get(struct baik *baik) {
  char url[256];
  String jawab;

  e32_copy_arg(baik, 0, "", url, sizeof(url));
  if (!e32_http_request(url, "GET", NULL, NULL, jawab)) {
    e32_ret_null(baik);
    return;
  }
  baik_return(baik,
              baik_mk_string(baik, jawab.c_str(), jawab.length(), 1));
}

/* HTTP.post(url, body[, contentType]) -> string, atau `kosong` bila gagal */
static void bk_http_post(struct baik *baik) {
  char url[256];
  char ctype[64];
  const char *body;
  String isi;
  String jawab;

  e32_copy_arg(baik, 0, "", url, sizeof(url));
  e32_copy_arg(baik, 2, "application/x-www-form-urlencoded",
               ctype, sizeof(ctype));

  /* Badan permintaan bisa panjang, jadi disalin ke String, bukan buffer
   * tetap seperti argumen lain. */
  body = e32_arg_str(baik, 1, "");
  isi = String(body != NULL ? body : "");

  if (!e32_http_request(url, "POST", isi.c_str(), ctype, jawab)) {
    e32_ret_null(baik);
    return;
  }
  baik_return(baik,
              baik_mk_string(baik, jawab.c_str(), jawab.length(), 1));
}

/* HTTP.statusTerakhir() -> angka  (alias HTTP.lastStatus) */
static void bk_http_status_terakhir(struct baik *baik) {
  e32_ret_int(baik, (long) s_http_last_status);
}

/*
 * Pembungkus BAIK untuk JSON.parse.
 *
 * CATATAN TEKNIS PENTING: di src/baik.c, cabang pemanggilan fungsi native
 * (foreign) di dalam baik_apply() DIKOMENTARI -- baik_apply() hanya
 * menjalankan bytecode, jadi ia hanya aman untuk fungsi yang ditulis dalam
 * bahasa BAIK. Padahal JSON.parse didaftarkan sebagai fungsi native
 * (baik_mk_foreign_func). Memanggilnya lewat baik_apply() akan melompat ke
 * alamat bytecode palsu dan membuat papan reset.
 *
 * Supaya tetap memakai baik_apply() seperti rencana TANPA menyentuh
 * src/baik.c, kita membuat SEKALI sebuah fungsi BAIK pembungkus:
 *     fungsi(t) { balik JSON.parse(t); }
 * Fungsi ini adalah fungsi BAIK sejati sehingga aman dipanggil
 * baik_apply(); pemanggilan JSON.parse di dalamnya dikerjakan oleh mesin
 * virtual BAIK sendiri (OP_CALL), yang memang mendukung fungsi native.
 */
static baik_val_t s_json_wrapper;
static int        s_json_wrapper_ok = 0;

static void e32_init_json_wrapper(struct baik *baik) {
  static const char kSrc1[] = "fungsi(t) { balik JSON.parse(t); }";
  static const char kSrc2[] =
      "isi __baikUraiJSON = fungsi(t) { balik JSON.parse(t); };";
  baik_val_t f = baik_mk_undefined();

  if (s_json_wrapper_ok) return;

  /* Cara 1: nilai program = fungsi pembungkus itu sendiri. */
  if (baik_exec(baik, kSrc1, &f) == BAIK_OK && baik_is_function(f)) {
    s_json_wrapper = f;
    baik_own(baik, &s_json_wrapper);
    s_json_wrapper_ok = 1;
    return;
  }
  /* Bersihkan keadaan galat supaya tidak mengganggu skrip berikutnya. */
  baik_set_errorf(baik, BAIK_OK, NULL);

  /* Cara 2: simpan di global lalu ambil kembali. */
  f = baik_mk_undefined();
  if (baik_exec(baik, kSrc2, &f) == BAIK_OK) {
    f = baik_get(baik, baik_get_global(baik), "__baikUraiJSON", ~0);
    if (baik_is_function(f)) {
      s_json_wrapper = f;
      baik_own(baik, &s_json_wrapper);
      s_json_wrapper_ok = 1;
      return;
    }
  }
  baik_set_errorf(baik, BAIK_OK, NULL);
  printf("Peringatan: HTTP.getJSON tidak siap "
         "(pembungkus JSON.parse gagal dibuat).\n");
}

/*
 * HTTP.getJSON(url) -> hasil JSON.parse, atau `kosong` bila gagal.
 *
 * Kita tidak menulis pengurai JSON sendiri: object global -> properti
 * "JSON" -> properti "parse" -> baik_apply(). Lihat catatan pembungkus di
 * atas untuk alasan adanya jalur pembungkus.
 */
static void bk_http_get_json(struct baik *baik) {
  char url[256];
  String jawab;
  baik_val_t g, json, parse, fn, ini, arg, hasil;
  baik_err_t err;

  e32_copy_arg(baik, 0, "", url, sizeof(url));
  if (!e32_http_request(url, "GET", NULL, NULL, jawab)) {
    e32_ret_null(baik);
    return;
  }

  g = baik_get_global(baik);
  json = baik_get(baik, g, "JSON", ~0);
  if (!baik_is_object(json)) {
    printf("Galat HTTP.getJSON: object JSON tidak tersedia.\n");
    e32_ret_null(baik);
    return;
  }
  parse = baik_get(baik, json, "parse", ~0);
  if (baik_is_undefined(parse)) {
    printf("Galat HTTP.getJSON: JSON.parse tidak tersedia.\n");
    e32_ret_null(baik);
    return;
  }

  if (baik_is_function(parse)) {
    /* JSON.parse berupa fungsi BAIK -> aman dipanggil langsung. */
    fn  = parse;
    ini = json;
  } else {
    /* JSON.parse berupa fungsi native -> lewat pembungkus BAIK. */
    e32_init_json_wrapper(baik);
    if (!s_json_wrapper_ok) {
      printf("Galat HTTP.getJSON: pengurai JSON tidak tersedia.\n");
      e32_ret_null(baik);
      return;
    }
    fn  = s_json_wrapper;
    ini = g;
  }

  arg   = baik_mk_string(baik, jawab.c_str(), jawab.length(), 1);
  hasil = baik_mk_undefined();

  /* Memanggil kembali ke dalam interpreter dapat memicu pengumpul sampah,
   * jadi argumen dan hasil harus di-own selama pemanggilan. */
  baik_own(baik, &arg);
  baik_own(baik, &hasil);
  err = baik_apply(baik, &hasil, fn, ini, 1, &arg);
  baik_disown(baik, &hasil);
  baik_disown(baik, &arg);

  if (err != BAIK_OK || baik_is_undefined(hasil)) {
    /* Bersihkan keadaan galat interpreter supaya REPL tetap sehat. */
    baik_set_errorf(baik, BAIK_OK, NULL);
    printf("Galat HTTP.getJSON: jawaban dari %s bukan JSON yang sah.\n", url);
    e32_ret_null(baik);
    return;
  }
  baik_return(baik, hasil);
}

/* ======================================================================== */
/* Object NTP (alias Waktu)                                                 */
/* ======================================================================== */

/* Terapkan pengaturan NTP yang tersimpan ke SNTP. */
static void e32_ntp_apply(void) {
  configTime(s_ntp_offset, s_ntp_dst, s_ntp_server);
  s_ntp_begun = 1;
}

/*
 * Ambil waktu lokal saat ini. `ms` adalah batas waktu menunggu waktu
 * tersinkronisasi. Kembalikan 1 bila waktu sudah sah.
 */
static int e32_ntp_now(struct tm *ti, uint32_t ms) {
  if (!s_ntp_begun) e32_ntp_apply();
  memset(ti, 0, sizeof(*ti));
  return getLocalTime(ti, ms) ? 1 : 0;
}

/* NTP.begin([server, offsetDetik, offsetDst]) -> boolean */
static void bk_ntp_begin(struct baik *baik) {
  char server[64];

  e32_copy_arg(baik, 0, "pool.ntp.org", server, sizeof(server));
  if (server[0] == '\0') strcpy(server, "pool.ntp.org");

  snprintf(s_ntp_server, sizeof(s_ntp_server), "%s", server);

  s_ntp_offset = (long) e32_arg_int(baik, 1, (int) E32_NTP_OFFSET_WIB);
  s_ntp_dst    = e32_arg_int(baik, 2, 0);

  e32_ntp_apply();

  if (WiFi.status() != WL_CONNECTED) {
    printf("Peringatan NTP.begin: WiFi belum tersambung, "
           "waktu baru akan sinkron setelah jaringan siap.\n");
  }
  e32_ret_bool(baik, 1);
}

/* NTP.sync([timeoutMs]) -> boolean */
static void bk_ntp_sync(struct baik *baik) {
  long timeout = (long) e32_arg_int(baik, 0, 10000);
  struct tm ti;

  if (timeout < 100) timeout = 100;
  if (timeout > 60000L) timeout = 60000L; /* batas atas pengaman */

  if (WiFi.status() != WL_CONNECTED) {
    printf("Galat NTP.sync: WiFi belum tersambung.\n");
    e32_ret_bool(baik, 0);
    return;
  }
  if (!s_ntp_begun) e32_ntp_apply();

  if (!e32_ntp_now(&ti, (uint32_t) timeout)) {
    printf("Galat NTP.sync: gagal menyinkronkan waktu dari %s "
           "dalam %ld ms.\n", s_ntp_server, timeout);
    e32_ret_bool(baik, 0);
    return;
  }
  e32_ret_bool(baik, 1);
}

/* NTP.epoch() -> detik sejak 1970 (0 bila waktu belum sah) */
static void bk_ntp_epoch(struct baik *baik) {
  struct tm ti;
  if (!e32_ntp_now(&ti, 100)) {
    e32_ret_num(baik, 0.0);
    return;
  }
  e32_ret_num(baik, (double) time(NULL));
}

/* NTP.format(fmt) -> string strftime, atau `kosong` bila waktu belum sah */
static void bk_ntp_format(struct baik *baik) {
  char fmt[64];
  char buf[128];
  struct tm ti;
  size_t n;

  e32_copy_arg(baik, 0, "%Y-%m-%d %H:%M:%S", fmt, sizeof(fmt));
  if (fmt[0] == '\0') strcpy(fmt, "%Y-%m-%d %H:%M:%S");

  if (!e32_ntp_now(&ti, 100)) {
    printf("Galat NTP.format: waktu belum tersinkronisasi. "
           "Panggil NTP.begin() lalu NTP.sync().\n");
    e32_ret_null(baik);
    return;
  }

  n = strftime(buf, sizeof(buf), fmt, &ti);
  if (n == 0) {
    printf("Galat NTP.format: format \"%s\" tidak menghasilkan apa-apa "
           "atau terlalu panjang.\n", fmt);
    e32_ret_null(baik);
    return;
  }
  buf[sizeof(buf) - 1] = '\0';
  e32_ret_str(baik, buf);
}

/* NTP.jam() / menit() / detik() / tanggal() / bulan() / tahun() -> angka
 * Kembalikan -1 bila waktu belum tersinkronisasi. */
static void bk_ntp_jam(struct baik *baik) {
  struct tm ti;
  e32_ret_int(baik, e32_ntp_now(&ti, 100) ? (long) ti.tm_hour : -1L);
}

static void bk_ntp_menit(struct baik *baik) {
  struct tm ti;
  e32_ret_int(baik, e32_ntp_now(&ti, 100) ? (long) ti.tm_min : -1L);
}

static void bk_ntp_detik(struct baik *baik) {
  struct tm ti;
  e32_ret_int(baik, e32_ntp_now(&ti, 100) ? (long) ti.tm_sec : -1L);
}

static void bk_ntp_tanggal(struct baik *baik) {
  struct tm ti;
  e32_ret_int(baik, e32_ntp_now(&ti, 100) ? (long) ti.tm_mday : -1L);
}

static void bk_ntp_bulan(struct baik *baik) {
  struct tm ti;
  e32_ret_int(baik, e32_ntp_now(&ti, 100) ? (long) (ti.tm_mon + 1) : -1L);
}

static void bk_ntp_tahun(struct baik *baik) {
  struct tm ti;
  e32_ret_int(baik, e32_ntp_now(&ti, 100) ? (long) (ti.tm_year + 1900) : -1L);
}

/* ======================================================================== */
/* Object MDNS                                                              */
/* ======================================================================== */

static int s_mdns_begun = 0;

/* MDNS.begin(nama) -> boolean; perangkat bisa dipanggil <nama>.local */
static void bk_mdns_begin(struct baik *baik) {
  char nama[64];

  e32_copy_arg(baik, 0, "baik", nama, sizeof(nama));
  if (nama[0] == '\0') strcpy(nama, "baik");

  if (WiFi.status() != WL_CONNECTED && WiFi.getMode() == WIFI_MODE_NULL) {
    printf("Peringatan MDNS.begin: jaringan belum aktif, "
           "nama .local mungkin belum bisa dipakai.\n");
  }

  if (!MDNS.begin(nama)) {
    printf("Galat MDNS.begin: gagal memulai mDNS untuk nama \"%s\".\n", nama);
    s_mdns_begun = 0;
    e32_ret_bool(baik, 0);
    return;
  }
  s_mdns_begun = 1;
  e32_ret_bool(baik, 1);
}

/* MDNS.addService(layanan, protokol, port) -> boolean */
static void bk_mdns_add_service(struct baik *baik) {
  char svc[32];
  char proto[16];
  int port;

  e32_copy_arg(baik, 0, "http", svc, sizeof(svc));
  e32_copy_arg(baik, 1, "tcp", proto, sizeof(proto));
  port = e32_arg_int(baik, 2, 80);

  if (!s_mdns_begun) {
    printf("Galat MDNS.addService: panggil MDNS.begin(nama) terlebih dulu.\n");
    e32_ret_bool(baik, 0);
    return;
  }
  if (svc[0] == '\0' || proto[0] == '\0') {
    printf("Galat MDNS.addService: nama layanan/protokol kosong.\n");
    e32_ret_bool(baik, 0);
    return;
  }
  if (port < 1 || port > 65535) {
    printf("Galat MDNS.addService: port %d di luar jangkauan 1..65535.\n",
           port);
    e32_ret_bool(baik, 0);
    return;
  }

  MDNS.addService(String(svc), String(proto), (uint16_t) port);
  e32_ret_bool(baik, 1);
}

/* ======================================================================== */
/* Registrasi                                                               */
/* ======================================================================== */

void baik_esp32_register_jaringan(struct baik *baik, baik_val_t g) {
  baik_val_t wifi, http, ntp, mdns;

  /* ---------------- object WiFi ---------------- */
  wifi = e32_ns(baik, g, "WiFi");
  e32_fn(baik, wifi, "begin",                bk_wifi_begin);
  e32_fn(baik, wifi, "disconnect",           bk_wifi_disconnect);
  e32_fn(baik, wifi, "status",               bk_wifi_status);
  e32_fn(baik, wifi, "statusText",           bk_wifi_status_text);
  e32_fn(baik, wifi, "statusTeks",           bk_wifi_status_text); /* alias ID */
  e32_fn(baik, wifi, "isConnected",          bk_wifi_is_connected);
  e32_fn(baik, wifi, "tersambung",           bk_wifi_is_connected); /* alias ID */
  e32_fn(baik, wifi, "localIP",              bk_wifi_local_ip);
  e32_fn(baik, wifi, "gatewayIP",            bk_wifi_gateway_ip);
  e32_fn(baik, wifi, "subnetMask",           bk_wifi_subnet_mask);
  e32_fn(baik, wifi, "dnsIP",                bk_wifi_dns_ip);
  e32_fn(baik, wifi, "macAddress",           bk_wifi_mac_address);
  e32_fn(baik, wifi, "RSSI",                 bk_wifi_rssi);
  e32_fn(baik, wifi, "SSID",                 bk_wifi_ssid);
  e32_fn(baik, wifi, "channel",              bk_wifi_channel);
  e32_fn(baik, wifi, "mode",                 bk_wifi_mode);
  e32_fn(baik, wifi, "setHostname",          bk_wifi_set_hostname);
  e32_fn(baik, wifi, "softAP",               bk_wifi_soft_ap);
  e32_fn(baik, wifi, "softAPIP",             bk_wifi_soft_ap_ip);
  e32_fn(baik, wifi, "softAPgetStationNum",  bk_wifi_soft_ap_station_num);
  e32_fn(baik, wifi, "scanNetworks",         bk_wifi_scan_networks);
  e32_fn(baik, wifi, "pindaiJaringan",       bk_wifi_scan_networks); /* alias ID */
  e32_fn(baik, wifi, "sleep",                bk_wifi_sleep);
  e32_fn(baik, wifi, "setTxPower",           bk_wifi_set_tx_power);
  e32_fn(baik, wifi, "tungguKoneksi",        bk_wifi_tunggu_koneksi);
  e32_fn(baik, wifi, "waitConnected",        bk_wifi_tunggu_koneksi); /* alias EN */

  /* ---------------- object HTTP ---------------- */
  http = e32_ns(baik, g, "HTTP");
  e32_fn(baik, http, "get",             bk_http_get);
  e32_fn(baik, http, "post",            bk_http_post);
  e32_fn(baik, http, "statusTerakhir",  bk_http_status_terakhir);
  e32_fn(baik, http, "lastStatus",      bk_http_status_terakhir); /* alias EN */
  e32_fn(baik, http, "getJSON",         bk_http_get_json);

  /* ---------------- object NTP (alias Waktu) ---------------- */
  ntp = e32_ns(baik, g, "NTP");
  e32_fn(baik, ntp, "begin",   bk_ntp_begin);
  e32_fn(baik, ntp, "sync",    bk_ntp_sync);
  e32_fn(baik, ntp, "sinkron", bk_ntp_sync);   /* alias ID */
  e32_fn(baik, ntp, "epoch",   bk_ntp_epoch);
  e32_fn(baik, ntp, "format",  bk_ntp_format);
  e32_fn(baik, ntp, "jam",     bk_ntp_jam);
  e32_fn(baik, ntp, "menit",   bk_ntp_menit);
  e32_fn(baik, ntp, "detik",   bk_ntp_detik);
  e32_fn(baik, ntp, "tanggal", bk_ntp_tanggal);
  e32_fn(baik, ntp, "bulan",   bk_ntp_bulan);
  e32_fn(baik, ntp, "tahun",   bk_ntp_tahun);
  /* `Waktu` menunjuk ke OBJECT YANG SAMA, bukan salinan. */
  baik_set(baik, g, "Waktu", ~0, ntp);

  /* ---------------- object MDNS ---------------- */
  mdns = e32_ns(baik, g, "MDNS");
  e32_fn(baik, mdns, "begin",       bk_mdns_begin);
  e32_fn(baik, mdns, "addService",  bk_mdns_add_service);

  /* ---------------- konstanta status & mode WiFi ----------------
   * Didaftarkan di global (supaya bisa ditulis apa adanya, mis.
   * `jika (WiFi.status() == WL_CONNECTED)`) dan juga pada object WiFi. */
  {
    static const struct { const char *nama; double nilai; } kKonst[] = {
      /* status (wl_status_t) */
      { "WL_IDLE_STATUS",     (double) WL_IDLE_STATUS     },
      { "WL_NO_SSID_AVAIL",   (double) WL_NO_SSID_AVAIL   },
      { "WL_SCAN_COMPLETED",  (double) WL_SCAN_COMPLETED  },
      { "WL_CONNECTED",       (double) WL_CONNECTED       },
      { "WL_CONNECT_FAILED",  (double) WL_CONNECT_FAILED  },
      { "WL_CONNECTION_LOST", (double) WL_CONNECTION_LOST },
      { "WL_DISCONNECTED",    (double) WL_DISCONNECTED    },
      { "WL_NO_SHIELD",       (double) WL_NO_SHIELD       },
      /* mode (wifi_mode_t) */
      { "WIFI_OFF",           (double) WIFI_OFF           },
      { "WIFI_STA",           (double) WIFI_STA           },
      { "WIFI_AP",            (double) WIFI_AP            },
      { "WIFI_AP_STA",        (double) WIFI_AP_STA        }
    };
    const int n = (int) (sizeof(kKonst) / sizeof(kKonst[0]));
    int i;
    for (i = 0; i < n; i++) {
      e32_const(baik, g,    kKonst[i].nama, kKonst[i].nilai);
      e32_const(baik, wifi, kKonst[i].nama, kKonst[i].nilai);
    }
  }

  /* Terakhir: siapkan pembungkus JSON.parse untuk HTTP.getJSON. Sengaja
   * dilakukan di akhir karena baik_exec() menjalankan mesin virtual dan
   * dapat memicu pengumpul sampah; semua object modul ini sudah terpasang
   * di global pada titik ini sehingga aman. */
  e32_init_json_wrapper(baik);
}
