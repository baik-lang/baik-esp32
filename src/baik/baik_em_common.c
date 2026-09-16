/*
 * baik_em_common.c - Pustaka pendukung: log, berkas, varint, mbuf, utilitas
 * string, serta pengurai dan pencetak JSON tingkat rendah.
 *
 * Catatan: literal JSON di sini (null/true/false) sengaja TIDAK diterjemahkan.
 * JSON adalah format pertukaran data; menerjemahkannya pernah membuat
 * JSON.parse gagal untuk ketiga nilai itu, termasuk JSON dari API luar.
 *
 * Bagian dari interpreter bahasa BAIK. Deklarasi bersama ada di
 * src/baik/baik_internal.h; antarmuka publik ada di src/baik.h.
 */
#include "baik_internal.h"


#include <stdarg.h>
#include <stdio.h>
#include <string.h>

enum BAIK_EM_log_level BAIK_EM_log_level WEAK =
#if BAIK_EM_ENABLE_DEBUG
    LL_VERBOSE_DEBUG;
#else
    LL_ERROR;
#endif

#if BAIK_EM_ENABLE_STDIO
static char *s_file_level = NULL;
void BAIK_EM_log_set_file_level(const char *file_level) WEAK;
FILE *BAIK_EM_log_file WEAK = NULL;

#if BAIK_EM_LOG_ENABLE_TS_DIFF
double BAIK_EM_log_ts WEAK;
#endif

enum BAIK_EM_log_level BAIK_EM_log_cur_msg_level WEAK = LL_NONE;

void BAIK_EM_log_set_file_level(const char *file_level) {
  char *fl = s_file_level;
  if (file_level != NULL) {
    s_file_level = strdup(file_level);
  } else {
    s_file_level = NULL;
  }
  free(fl);
}

int BAIK_EM_log_print_prefix(enum BAIK_EM_log_level level, const char *file, int ln) WEAK;
int BAIK_EM_log_print_prefix(enum BAIK_EM_log_level level, const char *file, int ln) {
  char prefix[BAIK_EM_LOG_PREFIX_LEN], *q;
  const char *p;
  size_t fl = 0, ll = 0, pl = 0;

  if (level > BAIK_EM_log_level && s_file_level == NULL) return 0;
  p = file + strlen(file);

  while (p != file) {
    const char c = *(p - 1);
    if (c == '/' || c == '\\') break;
    p--;
    fl++;
  }

  ll = (ln < 10000 ? (ln < 1000 ? (ln < 100 ? (ln < 10 ? 1 : 2) : 3) : 4) : 5);
  if (fl > (sizeof(prefix) - ll - 2)) fl = (sizeof(prefix) - ll - 2);

  pl = fl + 1 + ll;
  memcpy(prefix, p, fl);
  q = prefix + pl;
  memset(q, ' ', sizeof(prefix) - pl);
  do {
    *(--q) = '0' + (ln % 10);
    ln /= 10;
  } while (ln > 0);
  *(--q) = ':';

  if (s_file_level != NULL) {
    enum BAIK_EM_log_level pll = BAIK_EM_log_level;
    struct baik_generic_str fl = baik_generic_mk_str(s_file_level), ps = GENERIC_MK_STR_N(prefix, pl);
    struct baik_generic_str k, v;
    while ((fl = baik_generic_next_comma_list_entry_n(fl, &k, &v)).p != NULL) {
      bool yes = !(!baik_generic_str_starts_with(ps, k) || v.len == 0);
      if (!yes) continue;
      pll = (enum BAIK_EM_log_level)(*v.p - '0');
      break;
    }
    if (level > pll) return 0;
  }

  if (BAIK_EM_log_file == NULL) BAIK_EM_log_file = stderr;
  BAIK_EM_log_cur_msg_level = level;
  fwrite(prefix, 1, sizeof(prefix), BAIK_EM_log_file);
#if BAIK_EM_LOG_ENABLE_TS_DIFF
  {
    double now = BAIK_EM_time();
    fprintf(BAIK_EM_log_file, "%7u ", (unsigned int) ((now - BAIK_EM_log_ts) * 1000000));
    BAIK_EM_log_ts = now;
  }
#endif
  return 1;
}

void BAIK_EM_log_printf(const char *fmt, ...) WEAK;
void BAIK_EM_log_printf(const char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  vfprintf(BAIK_EM_log_file, fmt, ap);
  va_end(ap);
  fputc('\n', BAIK_EM_log_file);
  fflush(BAIK_EM_log_file);
  BAIK_EM_log_cur_msg_level = LL_NONE;
}

void BAIK_EM_log_set_file(FILE *file) WEAK;
void BAIK_EM_log_set_file(FILE *file) {
  BAIK_EM_log_file = file;
}

#else

void BAIK_EM_log_set_file_level(const char *file_level) {
  (void) file_level;
}

#endif

void BAIK_EM_log_set_level(enum BAIK_EM_log_level level) WEAK;
void BAIK_EM_log_set_level(enum BAIK_EM_log_level level) {
  BAIK_EM_log_level = level;
#if BAIK_EM_LOG_ENABLE_TS_DIFF && BAIK_EM_ENABLE_STDIO
  BAIK_EM_log_ts = BAIK_EM_time();
#endif
}

#include <stdio.h>
#include <stdlib.h>

#ifdef BAIK_EM_MMAP
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#endif

#ifndef EXCLUDE_COMMON
char *BAIK_EM_read_file(const char *path, size_t *size) WEAK;
char *BAIK_EM_read_file(const char *path, size_t *size) {
  FILE *fp;
  char *data = NULL;
  if ((fp = fopen(path, "rb")) == NULL) {
  } else if (fseek(fp, 0, SEEK_END) != 0) {
    fclose(fp);
  } else {
    *size = ftell(fp);
    data = (char *) malloc(*size + 1);
    if (data != NULL) {
      fseek(fp, 0, SEEK_SET);
      if (fread(data, 1, *size, fp) != *size) {
        free(data);
        return NULL;
      }
      data[*size] = '\0';
    }
    fclose(fp);
  }
  return data;
}
#endif

#ifdef BAIK_EM_MMAP
char *BAIK_EM_mmap_file(const char *path, size_t *size) WEAK;
char *BAIK_EM_mmap_file(const char *path, size_t *size) {
  char *r;
  int fd = open(path, O_RDONLY, 0);
  struct stat st;
  if (fd < 0) return NULL;
  fstat(fd, &st);
  *size = (size_t) st.st_size;
  r = (char *) mmap(NULL, st.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
  if (r == MAP_FAILED) return NULL;
  return r;
}
#endif

size_t BAIK_EM_varint_llen(uint64_t num) {
  size_t llen = 0;

  do {
    llen++;
  } while (num >>= 7);

  return llen;
}

size_t BAIK_EM_varint_encode(uint64_t num, uint8_t *buf, size_t buf_size) {
  size_t llen = 0;

  do {
    uint8_t byte = num & 0x7f;
    num >>= 7;
    if (num != 0) byte |= 0x80;
    if (llen < buf_size) *buf++ = byte;
    llen++;
  } while (num != 0);

  return llen;
}

bool BAIK_EM_varint_decode(const uint8_t *buf, size_t buf_size, uint64_t *num,
                      size_t *llen) {
  size_t i = 0, shift = 0;
  uint64_t n = 0;

  do {
    if (i == buf_size || i == (8 * sizeof(*num) / 7 + 1)) return false;
   
    n |= ((uint64_t)(buf[i] & 0x7f)) << shift;
   
    i++;
    shift += 7;
  } while (shift < sizeof(uint64_t) * 8 && (buf[i - 1] & 0x80));

  *num = n;
  *llen = i;

  return true;
}

uint64_t BAIK_EM_varint_decode_unsafe(const uint8_t *buf, int *llen) {
  uint64_t v;
  size_t l;
  BAIK_EM_varint_decode(buf, ~0, &v, &l);
  *llen = l;
  return v;
}

#ifndef EXCLUDE_COMMON

#include <assert.h>
#include <string.h>

#ifndef MBUF_REALLOC
#define MBUF_REALLOC realloc
#endif

#ifndef MBUF_FREE
#define MBUF_FREE free
#endif

void mbuf_init(struct mbuf *mbuf, size_t initial_size) WEAK;
void mbuf_init(struct mbuf *mbuf, size_t initial_size) {
  mbuf->len = mbuf->size = 0;
  mbuf->buf = NULL;
  mbuf_resize(mbuf, initial_size);
}

void mbuf_free(struct mbuf *mbuf) WEAK;
void mbuf_free(struct mbuf *mbuf) {
  if (mbuf->buf != NULL) {
    MBUF_FREE(mbuf->buf);
    mbuf_init(mbuf, 0);
  }
}

void mbuf_resize(struct mbuf *a, size_t new_size) WEAK;
void mbuf_resize(struct mbuf *a, size_t new_size) {
  if (new_size > a->size || (new_size < a->size && new_size >= a->len)) {
    char *buf = (char *) MBUF_REALLOC(a->buf, new_size);

    if (buf == NULL && new_size != 0) return;
    a->buf = buf;
    a->size = new_size;
  }
}

void mbuf_trim(struct mbuf *mbuf) WEAK;
void mbuf_trim(struct mbuf *mbuf) {
  mbuf_resize(mbuf, mbuf->len);
}

size_t mbuf_insert(struct mbuf *a, size_t off, const void *buf, size_t) WEAK;
size_t mbuf_insert(struct mbuf *a, size_t off, const void *buf, size_t len) {
  char *p = NULL;

  assert(a != NULL);
  assert(a->len <= a->size);
  assert(off <= a->len);

  if (~(size_t) 0 - (size_t) a->buf < len) return 0;

  if (a->len + len <= a->size) {
    memmove(a->buf + off + len, a->buf + off, a->len - off);
    if (buf != NULL) {
      memcpy(a->buf + off, buf, len);
    }
    a->len += len;
  } else {
    size_t min_size = (a->len + len);
    size_t new_size = (size_t)(min_size * MBUF_SIZE_MULTIPLIER);
    if (new_size - min_size > MBUF_SIZE_MAX_HEADROOM) {
      new_size = min_size + MBUF_SIZE_MAX_HEADROOM;
    }
    p = (char *) MBUF_REALLOC(a->buf, new_size);
    if (p == NULL && new_size != min_size) {
      new_size = min_size;
      p = (char *) MBUF_REALLOC(a->buf, new_size);
    }
    if (p != NULL) {
      a->buf = p;
      if (off != a->len) {
        memmove(a->buf + off + len, a->buf + off, a->len - off);
      }
      if (buf != NULL) memcpy(a->buf + off, buf, len);
      a->len += len;
      a->size = new_size;
    } else {
      len = 0;
    }
  }

  return len;
}

size_t mbuf_append(struct mbuf *a, const void *buf, size_t len) WEAK;
size_t mbuf_append(struct mbuf *a, const void *buf, size_t len) {
  return mbuf_insert(a, a->len, buf, len);
}

size_t mbuf_append_and_free(struct mbuf *a, void *buf, size_t len) WEAK;
size_t mbuf_append_and_free(struct mbuf *a, void *data, size_t len) {
  size_t ret;
 
  if (a->len == 0) {
    if (a->buf != NULL) free(a->buf);
    a->buf = (char *) data;
    a->len = a->size = len;
    return len;
  }
  ret = mbuf_insert(a, a->len, data, len);
  free(data);
  return ret;
}

void mbuf_remove(struct mbuf *mb, size_t n) WEAK;
void mbuf_remove(struct mbuf *mb, size_t n) {
  if (n > 0 && n <= mb->len) {
    memmove(mb->buf, mb->buf + n, mb->len - n);
    mb->len -= n;
  }
}

void mbuf_clear(struct mbuf *mb) WEAK;
void mbuf_clear(struct mbuf *mb) {
  mb->len = 0;
}

void mbuf_move(struct mbuf *from, struct mbuf *to) WEAK;
void mbuf_move(struct mbuf *from, struct mbuf *to) {
  memcpy(to, from, sizeof(*to));
  memset(from, 0, sizeof(*from));
}

#endif

#include <ctype.h>
#include <stdlib.h>
#include <string.h>

int baik_generic_ncasecmp(const char *s1, const char *s2, size_t len) WEAK;
struct baik_generic_str baik_generic_mk_str(const char *s) WEAK;
struct baik_generic_str baik_generic_mk_str(const char *s) {
  struct baik_generic_str ret = {s, 0};
  if (s != NULL) ret.len = strlen(s);
  return ret;
}

struct baik_generic_str baik_generic_mk_str_n(const char *s, size_t len) WEAK;
struct baik_generic_str baik_generic_mk_str_n(const char *s, size_t len) {
  struct baik_generic_str ret = {s, len};
  return ret;
}

int baik_generic_vcmp(const struct baik_generic_str *str1, const char *str2) WEAK;
int baik_generic_vcmp(const struct baik_generic_str *str1, const char *str2) {
  size_t n2 = strlen(str2), n1 = str1->len;
  int r = strncmp(str1->p, str2, (n1 < n2) ? n1 : n2);
  if (r == 0) {
    return n1 - n2;
  }
  return r;
}

int baik_generic_vcasecmp(const struct baik_generic_str *str1, const char *str2) WEAK;
int baik_generic_vcasecmp(const struct baik_generic_str *str1, const char *str2) {
  size_t n2 = strlen(str2), n1 = str1->len;
  int r = baik_generic_ncasecmp(str1->p, str2, (n1 < n2) ? n1 : n2);
  if (r == 0) {
    return n1 - n2;
  }
  return r;
}

static struct baik_generic_str baik_generic_strdup_common(const struct baik_generic_str s,
                                      int nul_terminate) {
  struct baik_generic_str r = {NULL, 0};
  if (s.len > 0 && s.p != NULL) {
    char *sc = (char *) GENERIC_MALLOC(s.len + (nul_terminate ? 1 : 0));
    if (sc != NULL) {
      memcpy(sc, s.p, s.len);
      if (nul_terminate) sc[s.len] = '\0';
      r.p = sc;
      r.len = s.len;
    }
  }
  return r;
}

struct baik_generic_str baik_generic_strdup(const struct baik_generic_str s) WEAK;
struct baik_generic_str baik_generic_strdup(const struct baik_generic_str s) {
  return baik_generic_strdup_common(s, 0);
}

struct baik_generic_str baik_generic_strdup_nul(const struct baik_generic_str s) WEAK;
struct baik_generic_str baik_generic_strdup_nul(const struct baik_generic_str s) {
  return baik_generic_strdup_common(s, 1);
}

const char *baik_generic_strchr(const struct baik_generic_str s, int c) WEAK;
const char *baik_generic_strchr(const struct baik_generic_str s, int c) {
  size_t i;
  for (i = 0; i < s.len; i++) {
    if (s.p[i] == c) return &s.p[i];
  }
  return NULL;
}

int baik_generic_strcmp(const struct baik_generic_str str1, const struct baik_generic_str str2) WEAK;
int baik_generic_strcmp(const struct baik_generic_str str1, const struct baik_generic_str str2) {
  size_t i = 0;
  while (i < str1.len && i < str2.len) {
    if (str1.p[i] < str2.p[i]) return -1;
    if (str1.p[i] > str2.p[i]) return 1;
    i++;
  }
  if (i < str1.len) return 1;
  if (i < str2.len) return -1;
  return 0;
}

int baik_generic_strncmp(const struct baik_generic_str, const struct baik_generic_str, size_t n) WEAK;
int baik_generic_strncmp(const struct baik_generic_str str1, const struct baik_generic_str str2, size_t n) {
  struct baik_generic_str s1 = str1;
  struct baik_generic_str s2 = str2;

  if (s1.len > n) {
    s1.len = n;
  }
  if (s2.len > n) {
    s2.len = n;
  }
  return baik_generic_strcmp(s1, s2);
}

void baik_generic_strfree(struct baik_generic_str *s) WEAK;
void baik_generic_strfree(struct baik_generic_str *s) {
  char *sp = (char *) s->p;
  s->p = NULL;
  s->len = 0;
  if (sp != NULL) free(sp);
}

const char *baik_generic_strstr(const struct baik_generic_str haystack,
                      const struct baik_generic_str needle) WEAK;
const char *baik_generic_strstr(const struct baik_generic_str haystack,
                      const struct baik_generic_str needle) {
  size_t i;
  if (needle.len > haystack.len) return NULL;
  for (i = 0; i <= haystack.len - needle.len; i++) {
    if (memcmp(haystack.p + i, needle.p, needle.len) == 0) {
      return haystack.p + i;
    }
  }
  return NULL;
}

struct baik_generic_str baik_generic_strstrip(struct baik_generic_str s) WEAK;
struct baik_generic_str baik_generic_strstrip(struct baik_generic_str s) {
  while (s.len > 0 && isspace((int) *s.p)) {
    s.p++;
    s.len--;
  }
  while (s.len > 0 && isspace((int) *(s.p + s.len - 1))) {
    s.len--;
  }
  return s;
}

int baik_generic_str_starts_with(struct baik_generic_str s, struct baik_generic_str prefix) WEAK;
int baik_generic_str_starts_with(struct baik_generic_str s, struct baik_generic_str prefix) {
  const struct baik_generic_str sp = GENERIC_MK_STR_N(s.p, prefix.len);
  if (s.len < prefix.len) return 0;
  return (baik_generic_strcmp(sp, prefix) == 0);
}

#ifndef EXCLUDE_COMMON

#ifndef C_DISABLE_BUILTIN_SNPRINTF
#define C_DISABLE_BUILTIN_SNPRINTF 0
#endif

size_t c_strnlen(const char *s, size_t maxlen) WEAK;
size_t c_strnlen(const char *s, size_t maxlen) {
  size_t l = 0;
  for (; l < maxlen && s[l] != '\0'; l++) {
  }
  return l;
}

#define C_SNPRINTF_APPEND_CHAR(ch)       \
  do {                                   \
    if (i < (int) buf_size) buf[i] = ch; \
    i++;                                 \
  } while (0)

#define C_SNPRINTF_FLAG_ZERO 1

#if C_DISABLE_BUILTIN_SNPRINTF
int c_vsnprintf(char *buf, size_t buf_size, const char *fmt, va_list ap) WEAK;
int c_vsnprintf(char *buf, size_t buf_size, const char *fmt, va_list ap) {
  return vsnprintf(buf, buf_size, fmt, ap);
}
#else
static int c_itoa(char *buf, size_t buf_size, int64_t num, int base, int flags,
                  int field_width) {
  char tmp[40];
  int i = 0, k = 0, neg = 0;

  if (num < 0) {
    neg++;
    num = -num;
  }

  do {
    int rem = num % base;
    if (rem < 10) {
      tmp[k++] = '0' + rem;
    } else {
      tmp[k++] = 'a' + (rem - 10);
    }
    num /= base;
  } while (num > 0);

  if (flags && C_SNPRINTF_FLAG_ZERO) {
    while (k < field_width && k < (int) sizeof(tmp) - 1) {
      tmp[k++] = '0';
    }
  }

  if (neg) {
    tmp[k++] = '-';
  }

  while (--k >= 0) {
    C_SNPRINTF_APPEND_CHAR(tmp[k]);
  }

  return i;
}

int c_vsnprintf(char *buf, size_t buf_size, const char *fmt, va_list ap) WEAK;
int c_vsnprintf(char *buf, size_t buf_size, const char *fmt, va_list ap) {
  int ch, i = 0, len_mod, flags, precision, field_width;

  while ((ch = *fmt++) != '\0') {
    if (ch != '%') {
      C_SNPRINTF_APPEND_CHAR(ch);
    } else {
     
      flags = field_width = precision = len_mod = 0;

      if (*fmt == '0') {
        flags |= C_SNPRINTF_FLAG_ZERO;
      }

      while (*fmt >= '0' && *fmt <= '9') {
        field_width *= 10;
        field_width += *fmt++ - '0';
      }
     
      if (*fmt == '*') {
        field_width = va_arg(ap, int);
        fmt++;
      }

      if (*fmt == '.') {
        fmt++;
        if (*fmt == '*') {
          precision = va_arg(ap, int);
          fmt++;
        } else {
          while (*fmt >= '0' && *fmt <= '9') {
            precision *= 10;
            precision += *fmt++ - '0';
          }
        }
      }

      switch (*fmt) {
        case 'h':
        case 'l':
        case 'L':
        case 'I':
        case 'q':
        case 'j':
        case 'z':
        case 't':
          len_mod = *fmt++;
          if (*fmt == 'h') {
            len_mod = 'H';
            fmt++;
          }
          if (*fmt == 'l') {
            len_mod = 'q';
            fmt++;
          }
          break;
      }

      ch = *fmt++;
      if (ch == 's') {
        const char *s = va_arg(ap, const char *);
        int j;
        int pad = field_width - (precision >= 0 ? c_strnlen(s, precision) : 0);
        for (j = 0; j < pad; j++) {
          C_SNPRINTF_APPEND_CHAR(' ');
        }

       
        if (s != NULL) {
         
          for (j = 0; (precision <= 0 || j < precision) && s[j] != '\0'; j++) {
            C_SNPRINTF_APPEND_CHAR(s[j]);
          }
        }
      } else if (ch == 'c') {
        ch = va_arg(ap, int);
        C_SNPRINTF_APPEND_CHAR(ch);
      } else if (ch == 'd' && len_mod == 0) {
        i += c_itoa(buf + i, buf_size - i, va_arg(ap, int), 10, flags,
                    field_width);
      } else if (ch == 'd' && len_mod == 'l') {
        i += c_itoa(buf + i, buf_size - i, va_arg(ap, long), 10, flags,
                    field_width);
#ifdef SSIZE_MAX
      } else if (ch == 'd' && len_mod == 'z') {
        i += c_itoa(buf + i, buf_size - i, va_arg(ap, ssize_t), 10, flags,
                    field_width);
#endif
      } else if (ch == 'd' && len_mod == 'q') {
        i += c_itoa(buf + i, buf_size - i, va_arg(ap, int64_t), 10, flags,
                    field_width);
      } else if ((ch == 'x' || ch == 'u') && len_mod == 0) {
        i += c_itoa(buf + i, buf_size - i, va_arg(ap, unsigned),
                    ch == 'x' ? 16 : 10, flags, field_width);
      } else if ((ch == 'x' || ch == 'u') && len_mod == 'l') {
        i += c_itoa(buf + i, buf_size - i, va_arg(ap, unsigned long),
                    ch == 'x' ? 16 : 10, flags, field_width);
      } else if ((ch == 'x' || ch == 'u') && len_mod == 'z') {
        i += c_itoa(buf + i, buf_size - i, va_arg(ap, size_t),
                    ch == 'x' ? 16 : 10, flags, field_width);
      } else if (ch == 'p') {
        unsigned long num = (unsigned long) (uintptr_t) va_arg(ap, void *);
        C_SNPRINTF_APPEND_CHAR('0');
        C_SNPRINTF_APPEND_CHAR('x');
        i += c_itoa(buf + i, buf_size - i, num, 16, flags, 0);
      } else {
#ifndef NO_LIBC
       
        abort();
#endif
      }
    }
  }

  if (buf_size > 0) {
    buf[i < (int) buf_size ? i : (int) buf_size - 1] = '\0';
  }

  return i;
}
#endif

int c_snprintf(char *buf, size_t buf_size, const char *fmt, ...) WEAK;
int c_snprintf(char *buf, size_t buf_size, const char *fmt, ...) {
  int result;
  va_list ap;
  va_start(ap, fmt);
  result = c_vsnprintf(buf, buf_size, fmt, ap);
  va_end(ap);
  return result;
}

#ifdef _WIN32
int to_wchar(const char *path, wchar_t *wbuf, size_t wbuf_len) {
  int ret;
  char buf[MAX_PATH * 2], buf2[MAX_PATH * 2], *p;

  strncpy(buf, path, sizeof(buf));
  buf[sizeof(buf) - 1] = '\0';

  p = buf + strlen(buf) - 1;
  while (p > buf && p[-1] != ':' && (p[0] == '\\' || p[0] == '/')) *p-- = '\0';

  memset(wbuf, 0, wbuf_len * sizeof(wchar_t));
  ret = MultiByteToWideChar(CP_UTF8, 0, buf, -1, wbuf, (int) wbuf_len);

  WideCharToMultiByte(CP_UTF8, 0, wbuf, (int) wbuf_len, buf2, sizeof(buf2),
                      NULL, NULL);
  if (strcmp(buf, buf2) != 0) {
    wbuf[0] = L'\0';
    ret = 0;
  }

  return ret;
}
#endif


const char *c_strnstr(const char *s, const char *find, size_t slen) WEAK;
const char *c_strnstr(const char *s, const char *find, size_t slen) {
  size_t find_length = strlen(find);
  size_t i;

  for (i = 0; i < slen; i++) {
    if (i + find_length > slen) {
      return NULL;
    }

    if (strncmp(&s[i], find, find_length) == 0) {
      return &s[i];
    }
  }

  return NULL;
}

#if BAIK_EM_ENABLE_STRDUP
char *strdup(const char *src) WEAK;
char *strdup(const char *src) {
  size_t len = strlen(src) + 1;
  char *ret = GENERIC_MALLOC(len);
  if (ret != NULL) {
    strcpy(ret, src);
  }
  return ret;
}
#endif

void BAIK_EM_to_hex(char *to, const unsigned char *p, size_t len) WEAK;
void BAIK_EM_to_hex(char *to, const unsigned char *p, size_t len) {
  static const char *hex = "0123456789abcdef";

  for (; len--; p++) {
    *to++ = hex[p[0] >> 4];
    *to++ = hex[p[0] & 0x0f];
  }

  *to = '\0';
}

static int fourbit(int ch) {
  if (ch >= '0' && ch <= '9') {
    return ch - '0';
  } else if (ch >= 'a' && ch <= 'f') {
    return ch - 'a' + 10;
  } else if (ch >= 'A' && ch <= 'F') {
    return ch - 'A' + 10;
  }
  return 0;
}

void BAIK_EM_from_hex(char *to, const char *p, size_t len) WEAK;
void BAIK_EM_from_hex(char *to, const char *p, size_t len) {
  size_t i;

  for (i = 0; i < len; i += 2) {
    *to++ = (fourbit(p[i]) << 4) + fourbit(p[i + 1]);
  }
  *to = '\0';
}

#if BAIK_EM_ENABLE_TO64
int64_t BAIK_EM_to64(const char *s) WEAK;
int64_t BAIK_EM_to64(const char *s) {
  int64_t result = 0;
  int64_t neg = 1;
  while (*s && isspace((unsigned char) *s)) s++;
  if (*s == '-') {
    neg = -1;
    s++;
  }
  while (isdigit((unsigned char) *s)) {
    result *= 10;
    result += (*s - '0');
    s++;
  }
  return result * neg;
}
#endif

static int str_util_lowercase(const char *s) {
  return tolower(*(const unsigned char *) s);
}

int baik_generic_ncasecmp(const char *s1, const char *s2, size_t len) WEAK;
int baik_generic_ncasecmp(const char *s1, const char *s2, size_t len) {
  int diff = 0;

  if (len > 0) do {
      diff = str_util_lowercase(s1++) - str_util_lowercase(s2++);
    } while (diff == 0 && s1[-1] != '\0' && --len > 0);

  return diff;
}

int baik_generic_casecmp(const char *s1, const char *s2) WEAK;
int baik_generic_casecmp(const char *s1, const char *s2) {
  return baik_generic_ncasecmp(s1, s2, (size_t) ~0);
}

int baik_generic_asprintf(char **buf, size_t size, const char *fmt, ...) WEAK;
int baik_generic_asprintf(char **buf, size_t size, const char *fmt, ...) {
  int ret;
  va_list ap;
  va_start(ap, fmt);
  ret = baik_generic_avprintf(buf, size, fmt, ap);
  va_end(ap);
  return ret;
}

int baik_generic_avprintf(char **buf, size_t size, const char *fmt, va_list ap) WEAK;
int baik_generic_avprintf(char **buf, size_t size, const char *fmt, va_list ap) {
  va_list ap_copy;
  int len;

  va_copy(ap_copy, ap);
  len = vsnprintf(*buf, size, fmt, ap_copy);
  va_end(ap_copy);

  if (len < 0) {
   
    *buf = NULL;
    while (len < 0) {
      GENERIC_FREE(*buf);
      if (size == 0) {
        size = 5;
      }
      size *= 2;
      if ((*buf = (char *) GENERIC_MALLOC(size)) == NULL) {
        len = -1;
        break;
      }
      va_copy(ap_copy, ap);
      len = vsnprintf(*buf, size - 1, fmt, ap_copy);
      va_end(ap_copy);
    }

    (*buf)[len] = 0;
   
  } else if (len >= (int) size) {
   
    if ((*buf = (char *) GENERIC_MALLOC(len + 1)) == NULL) {
      len = -1;
    } else {   
      va_copy(ap_copy, ap);
      len = vsnprintf(*buf, len + 1, fmt, ap_copy);
      va_end(ap_copy);
    }
  }

  return len;
}

const char *baik_generic_next_comma_list_entry(const char *, struct baik_generic_str *,
                                     struct baik_generic_str *) WEAK;
const char *baik_generic_next_comma_list_entry(const char *list, struct baik_generic_str *val,
                                     struct baik_generic_str *eq_val) {
  struct baik_generic_str ret = baik_generic_next_comma_list_entry_n(baik_generic_mk_str(list), val, eq_val);
  return ret.p;
}
struct baik_generic_str baik_generic_next_comma_list_entry_n(struct baik_generic_str list, struct baik_generic_str *val,
                                         struct baik_generic_str *eq_val) WEAK;
struct baik_generic_str baik_generic_next_comma_list_entry_n(struct baik_generic_str list, struct baik_generic_str *val,
                                         struct baik_generic_str *eq_val) {
  if (list.len == 0) {
    list = baik_generic_mk_str(NULL);
  } else {
    const char *chr = NULL;
    *val = list;

    if ((chr = baik_generic_strchr(*val, ',')) != NULL) {
      val->len = chr - val->p;
      chr++;
      list.len -= (chr - list.p);
      list.p = chr;
    } else {
      list = baik_generic_mk_str_n(list.p + list.len, 0);
    }

    if (eq_val != NULL) {
      eq_val->len = 0;
      eq_val->p = (const char *) memchr(val->p, '=', val->len);
      if (eq_val->p != NULL) {
        eq_val->p++;
        eq_val->len = val->p + val->len - eq_val->p;
        val->len = (eq_val->p - val->p) - 1;
      }
    }
  }

  return list;
}

size_t baik_generic_match_prefix_n(const struct baik_generic_str, const struct baik_generic_str) WEAK;
size_t baik_generic_match_prefix_n(const struct baik_generic_str pattern, const struct baik_generic_str str) {
  const char *or_str;
  size_t res = 0, len = 0, i = 0, j = 0;

  if ((or_str = (const char *) memchr(pattern.p, '|', pattern.len)) != NULL ||
      (or_str = (const char *) memchr(pattern.p, ',', pattern.len)) != NULL) {
    struct baik_generic_str pstr = {pattern.p, (size_t)(or_str - pattern.p)};
    res = baik_generic_match_prefix_n(pstr, str);
    if (res > 0) return res;
    pstr.p = or_str + 1;
    pstr.len = (pattern.p + pattern.len) - (or_str + 1);
    return baik_generic_match_prefix_n(pstr, str);
  }

  for (; i < pattern.len && j < str.len; i++, j++) {
    if (pattern.p[i] == '?') {
      continue;
    } else if (pattern.p[i] == '*') {
      i++;
      if (i < pattern.len && pattern.p[i] == '*') {
        i++;
        len = str.len - j;
      } else {
        len = 0;
        while (j + len < str.len && str.p[j + len] != '/') len++;
      }
      if (i == pattern.len || (pattern.p[i] == '$' && i == pattern.len - 1))
        return j + len;
      do {
        const struct baik_generic_str pstr = {pattern.p + i, pattern.len - i};
        const struct baik_generic_str sstr = {str.p + j + len, str.len - j - len};
        res = baik_generic_match_prefix_n(pstr, sstr);
      } while (res == 0 && len != 0 && len-- > 0);
      return res == 0 ? 0 : j + res + len;
    } else if (str_util_lowercase(&pattern.p[i]) !=
               str_util_lowercase(&str.p[j])) {
      break;
    }
  }
  if (i < pattern.len && pattern.p[i] == '$') {
    return j == str.len ? str.len : 0;
  }
  return i == pattern.len ? j : 0;
}

size_t baik_generic_match_prefix(const char *, int, const char *) WEAK;
size_t baik_generic_match_prefix(const char *pattern, int pattern_len, const char *str) {
  const struct baik_generic_str pstr = {pattern, (size_t) pattern_len};
  struct baik_generic_str s = {str, 0};
  if (str != NULL) s.len = strlen(str);
  return baik_generic_match_prefix_n(pstr, s);
}

#endif

#define _CRT_SECURE_NO_WARNINGS

#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if !defined(WEAK)
#if (defined(__GNUC__) || defined(__TI_COMPILER_VERSION__)) && !defined(_WIN32)
#define WEAK __attribute__((weak))
#else
#define WEAK
#endif
#endif

#ifdef _WIN32
#undef snprintf
#undef vsnprintf
#define snprintf BAIK_EM_win_snprintf
#define vsnprintf BAIK_EM_win_vsnprintf
int BAIK_EM_win_snprintf(char *str, size_t size, const char *format, ...);
int BAIK_EM_win_vsnprintf(char *str, size_t size, const char *format, va_list ap);
#if _MSC_VER >= 1700
#include <stdint.h>
#else
typedef _int64 int64_t;
typedef unsigned _int64 uint64_t;
#endif
#define PRId64 "I64d"
#define PRIu64 "I64u"
#else

#ifndef __STDC_FORMAT_MACROS
#define __STDC_FORMAT_MACROS
#endif
#include <inttypes.h>
#endif

#ifndef INT64_FMT
#define INT64_FMT PRId64
#endif
#ifndef UINT64_FMT
#define UINT64_FMT PRIu64
#endif

#ifndef va_copy
#define va_copy(x, y) x = y
#endif

#ifndef JSON_ENABLE_ARRAY
#define JSON_ENABLE_ARRAY 1
#endif

struct frozen {
  const char *end;
  const char *cur;

  const char *cur_name;
  size_t cur_name_len;

  char path[JSON_MAX_PATH_LEN];
  size_t path_len;
  void *callback_data;
  json_walk_callback_t callback;
};

struct fstate {
  const char *ptr;
  size_t path_len;
};

#define SET_STATE(fr, ptr, str, len)              \
  struct fstate fstate = {(ptr), (fr)->path_len}; \
  json_append_to_path((fr), (str), (len));

#define CALL_BACK(fr, tok, value, len)                                        \
  do {                                                                        \
    if ((fr)->callback &&                                                     \
        ((fr)->path_len == 0 || (fr)->path[(fr)->path_len - 1] != '.')) {     \
      struct json_token t = {(value), (int) (len), (tok)};                    \
                                                                              \
                \
      (fr)->callback((fr)->callback_data, (fr)->cur_name, (fr)->cur_name_len, \
                     (fr)->path, &t);                                         \
                                                                              \
                                                         \
      (fr)->cur_name = NULL;                                                  \
      (fr)->cur_name_len = 0;                                                 \
    }                                                                         \
  } while (0)

static int json_append_to_path(struct frozen *f, const char *str, int size) {
  int n = f->path_len;
  int left = sizeof(f->path) - n - 1;
  if (size > left) size = left;
  memcpy(f->path + n, str, size);
  f->path[n + size] = '\0';
  f->path_len += size;
  return n;
}

static void json_truncate_path(struct frozen *f, size_t len) {
  f->path_len = len;
  f->path[len] = '\0';
}

static int json_parse_object(struct frozen *f);
static int json_parse_value(struct frozen *f);

#define EXPECT(cond, err_code)      \
  do {                              \
    if (!(cond)) return (err_code); \
  } while (0)

#define TRY(expr)          \
  do {                     \
    int _n = expr;         \
    if (_n < 0) return _n; \
  } while (0)

#define END_OF_STRING (-1)

static int json_left(const struct frozen *f) {
  return f->end - f->cur;
}

static int json_isspace(int ch) {
  return ch == ' ' || ch == '\t' || ch == '\r' || ch == '\n';
}

static void json_skip_whitespaces(struct frozen *f) {
  while (f->cur < f->end && json_isspace(*f->cur)) f->cur++;
}

static int json_cur(struct frozen *f) {
  json_skip_whitespaces(f);
  return f->cur >= f->end ? END_OF_STRING : *(unsigned char *) f->cur;
}

static int json_test_and_skip(struct frozen *f, int expected) {
  int ch = json_cur(f);
  if (ch == expected) {
    f->cur++;
    return 0;
  }
  return ch == END_OF_STRING ? JSON_STRING_INCOMPLETE : JSON_STRING_INVALID;
}

static int json_isalpha(int ch) {
  return (ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z');
}

static int json_isdigit(int ch) {
  return ch >= '0' && ch <= '9';
}

static int json_isxdigit(int ch) {
  return json_isdigit(ch) || (ch >= 'a' && ch <= 'f') ||
         (ch >= 'A' && ch <= 'F');
}

static int json_get_escape_len(const char *s, int len) {
  switch (*s) {
    case 'u':
      return len < 6 ? JSON_STRING_INCOMPLETE
                     : json_isxdigit(s[1]) && json_isxdigit(s[2]) &&
                               json_isxdigit(s[3]) && json_isxdigit(s[4])
                           ? 5
                           : JSON_STRING_INVALID;
    case '"':
    case '\\':
    case '/':
    case 'b':
    case 'f':
    case 'n':
    case 'r':
    case 't':
      return len < 2 ? JSON_STRING_INCOMPLETE : 1;
    default:
      return JSON_STRING_INVALID;
  }
}

static int json_parse_identifier(struct frozen *f) {
  EXPECT(json_isalpha(json_cur(f)), JSON_STRING_INVALID);
  {
    SET_STATE(f, f->cur, "", 0);
    while (f->cur < f->end &&
           (*f->cur == '_' || json_isalpha(*f->cur) || json_isdigit(*f->cur))) {
      f->cur++;
    }
    json_truncate_path(f, fstate.path_len);
    CALL_BACK(f, JSON_TYPE_STRING, fstate.ptr, f->cur - fstate.ptr);
  }
  return 0;
}

static int json_get_utf8_char_len(unsigned char ch) {
  if ((ch & 0x80) == 0) return 1;
  switch (ch & 0xf0) {
    case 0xf0:
      return 4;
    case 0xe0:
      return 3;
    default:
      return 2;
  }
}

static int json_parse_string(struct frozen *f) {
  int n, ch = 0, len = 0;
  TRY(json_test_and_skip(f, '"'));
  {
    SET_STATE(f, f->cur, "", 0);
    for (; f->cur < f->end; f->cur += len) {
      ch = *(unsigned char *) f->cur;
      len = json_get_utf8_char_len((unsigned char) ch);
      EXPECT(ch >= 32 && len > 0, JSON_STRING_INVALID);
      EXPECT(len <= json_left(f), JSON_STRING_INCOMPLETE);
      if (ch == '\\') {
        EXPECT((n = json_get_escape_len(f->cur + 1, json_left(f))) > 0, n);
        len += n;
      } else if (ch == '"') {
        json_truncate_path(f, fstate.path_len);
        CALL_BACK(f, JSON_TYPE_STRING, fstate.ptr, f->cur - fstate.ptr);
        f->cur++;
        break;
      };
    }
  }
  return ch == '"' ? 0 : JSON_STRING_INCOMPLETE;
}

static int json_parse_number(struct frozen *f) {
  int ch = json_cur(f);
  SET_STATE(f, f->cur, "", 0);
  if (ch == '-') f->cur++;
  EXPECT(f->cur < f->end, JSON_STRING_INCOMPLETE);
  if (f->cur + 1 < f->end && f->cur[0] == '0' && f->cur[1] == 'x') {
    f->cur += 2;
    EXPECT(f->cur < f->end, JSON_STRING_INCOMPLETE);
    EXPECT(json_isxdigit(f->cur[0]), JSON_STRING_INVALID);
    while (f->cur < f->end && json_isxdigit(f->cur[0])) f->cur++;
  } else {
    EXPECT(json_isdigit(f->cur[0]), JSON_STRING_INVALID);
    while (f->cur < f->end && json_isdigit(f->cur[0])) f->cur++;
    if (f->cur < f->end && f->cur[0] == '.') {
      f->cur++;
      EXPECT(f->cur < f->end, JSON_STRING_INCOMPLETE);
      EXPECT(json_isdigit(f->cur[0]), JSON_STRING_INVALID);
      while (f->cur < f->end && json_isdigit(f->cur[0])) f->cur++;
    }
    if (f->cur < f->end && (f->cur[0] == 'e' || f->cur[0] == 'E')) {
      f->cur++;
      EXPECT(f->cur < f->end, JSON_STRING_INCOMPLETE);
      if ((f->cur[0] == '+' || f->cur[0] == '-')) f->cur++;
      EXPECT(f->cur < f->end, JSON_STRING_INCOMPLETE);
      EXPECT(json_isdigit(f->cur[0]), JSON_STRING_INVALID);
      while (f->cur < f->end && json_isdigit(f->cur[0])) f->cur++;
    }
  }
  json_truncate_path(f, fstate.path_len);
  CALL_BACK(f, JSON_TYPE_NUMBER, fstate.ptr, f->cur - fstate.ptr);
  return 0;
}

#if JSON_ENABLE_ARRAY

static int json_parse_array(struct frozen *f) {
  int i = 0, current_path_len;
  char buf[20];
  CALL_BACK(f, JSON_TYPE_ARRAY_START, NULL, 0);
  TRY(json_test_and_skip(f, '['));
  {
    {
      SET_STATE(f, f->cur - 1, "", 0);
      while (json_cur(f) != ']') {
        snprintf(buf, sizeof(buf), "[%d]", i);
        i++;
        current_path_len = json_append_to_path(f, buf, strlen(buf));
        f->cur_name =
            f->path + strlen(f->path) - strlen(buf) + 1;
        f->cur_name_len = strlen(buf) - 2;
        TRY(json_parse_value(f));
        json_truncate_path(f, current_path_len);
        if (json_cur(f) == ',') f->cur++;
      }
      TRY(json_test_and_skip(f, ']'));
      json_truncate_path(f, fstate.path_len);
      CALL_BACK(f, JSON_TYPE_ARRAY_END, fstate.ptr, f->cur - fstate.ptr);
    }
  }
  return 0;
}
#endif

static int json_expect(struct frozen *f, const char *s, int len,
                       enum json_token_type tok_type) {
  int i, n = json_left(f);
  SET_STATE(f, f->cur, "", 0);
  for (i = 0; i < len; i++) {
    if (i >= n) return JSON_STRING_INCOMPLETE;
    if (f->cur[i] != s[i]) return JSON_STRING_INVALID;
  }
  f->cur += len;
  json_truncate_path(f, fstate.path_len);

  CALL_BACK(f, tok_type, fstate.ptr, f->cur - fstate.ptr);

  return 0;
}

static int json_parse_value(struct frozen *f) {
  int ch = json_cur(f);

  switch (ch) {
    case '"':
      TRY(json_parse_string(f));
      break;
    case '{':
      TRY(json_parse_object(f));
      break;
#if JSON_ENABLE_ARRAY
    case '[':
      TRY(json_parse_array(f));
      break;
#endif
    /* JSON adalah format pertukaran data: literalnya SELALU null/true/false,
     * apa pun bahasa kata kunci BAIK. Ketiga string ini pernah ikut
     * diterjemahkan, padahal pemilihan cabang di atas memakai huruf pertama
     * 'n'/'t'/'f' - sehingga perbandingannya tidak akan pernah cocok dan
     * JSON.parse gagal untuk true/false/null, termasuk JSON dari API luar.
     * Panjang 4/4/5 di bawah memang panjang kata aslinya, itu petunjuk
     * bahwa hanya stringnya yang tersentuh. */
    case 'n':
      TRY(json_expect(f, "null", 4, JSON_TYPE_NULL));
      break;
    case 't':
      TRY(json_expect(f, "true", 4, JSON_TYPE_TRUE));
      break;
    case 'f':
      TRY(json_expect(f, "false", 5, JSON_TYPE_FALSE));
      break;
    case '-':
    case '0':
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
    case '8':
    case '9':
      TRY(json_parse_number(f));
      break;
    default:
      return ch == END_OF_STRING ? JSON_STRING_INCOMPLETE : JSON_STRING_INVALID;
  }

  return 0;
}


static int json_parse_key(struct frozen *f) {
  int ch = json_cur(f);
  if (json_isalpha(ch)) {
    TRY(json_parse_identifier(f));
  } else if (ch == '"') {
    TRY(json_parse_string(f));
  } else {
    return ch == END_OF_STRING ? JSON_STRING_INCOMPLETE : JSON_STRING_INVALID;
  }
  return 0;
}


static int json_parse_pair(struct frozen *f) {
  int current_path_len;
  const char *tok;
  json_skip_whitespaces(f);
  tok = f->cur;
  TRY(json_parse_key(f));
  {
    f->cur_name = *tok == '"' ? tok + 1 : tok;
    f->cur_name_len = *tok == '"' ? f->cur - tok - 2 : f->cur - tok;
    current_path_len = json_append_to_path(f, f->cur_name, f->cur_name_len);
  }
  TRY(json_test_and_skip(f, ':'));
  TRY(json_parse_value(f));
  json_truncate_path(f, current_path_len);
  return 0;
}


static int json_parse_object(struct frozen *f) {
  CALL_BACK(f, JSON_TYPE_OBJECT_START, NULL, 0);
  TRY(json_test_and_skip(f, '{'));
  {
    SET_STATE(f, f->cur - 1, ".", 1);
    while (json_cur(f) != '}') {
      TRY(json_parse_pair(f));
      if (json_cur(f) == ',') f->cur++;
    }
    TRY(json_test_and_skip(f, '}'));
    json_truncate_path(f, fstate.path_len);
    CALL_BACK(f, JSON_TYPE_OBJECT_END, fstate.ptr, f->cur - fstate.ptr);
  }
  return 0;
}

static int json_doit(struct frozen *f) {
  if (f->cur == 0 || f->end < f->cur) return JSON_STRING_INVALID;
  if (f->end == f->cur) return JSON_STRING_INCOMPLETE;
  return json_parse_value(f);
}

int json_escape(struct json_out *out, const char *p, size_t len) WEAK;
int json_escape(struct json_out *out, const char *p, size_t len) {
  size_t i, cl, n = 0;
  const char *hex_digits = "0123456789abcdef";
  const char *specials = "btnvfr";

  for (i = 0; i < len; i++) {
    unsigned char ch = ((unsigned char *) p)[i];
    if (ch == '"' || ch == '\\') {
      n += out->printer(out, "\\", 1);
      n += out->printer(out, p + i, 1);
    } else if (ch >= '\b' && ch <= '\r') {
      n += out->printer(out, "\\", 1);
      n += out->printer(out, &specials[ch - '\b'], 1);
    } else if (isprint(ch)) {
      n += out->printer(out, p + i, 1);
    } else if ((cl = json_get_utf8_char_len(ch)) == 1) {
      n += out->printer(out, "\\u00", 4);
      n += out->printer(out, &hex_digits[(ch >> 4) % 0xf], 1);
      n += out->printer(out, &hex_digits[ch % 0xf], 1);
    } else {
      n += out->printer(out, p + i, cl);
      i += cl - 1;
    }
  }

  return n;
}

int json_printer_buf(struct json_out *out, const char *buf, size_t len) WEAK;
int json_printer_buf(struct json_out *out, const char *buf, size_t len) {
  size_t avail = out->u.buf.size - out->u.buf.len;
  size_t n = len < avail ? len : avail;
  memcpy(out->u.buf.buf + out->u.buf.len, buf, n);
  out->u.buf.len += n;
  if (out->u.buf.size > 0) {
    size_t idx = out->u.buf.len;
    if (idx >= out->u.buf.size) idx = out->u.buf.size - 1;
    out->u.buf.buf[idx] = '\0';
  }
  return len;
}

int json_printer_file(struct json_out *out, const char *buf, size_t len) WEAK;
int json_printer_file(struct json_out *out, const char *buf, size_t len) {
  return fwrite(buf, 1, len, out->u.fp);
}

#if JSON_ENABLE_BASE64
static int b64idx(int c) {
  if (c < 26) {
    return c + 'A';
  } else if (c < 52) {
    return c - 26 + 'a';
  } else if (c < 62) {
    return c - 52 + '0';
  } else {
    return c == 62 ? '+' : '/';
  }
}

static int b64rev(int c) {
  if (c >= 'A' && c <= 'Z') {
    return c - 'A';
  } else if (c >= 'a' && c <= 'z') {
    return c + 26 - 'a';
  } else if (c >= '0' && c <= '9') {
    return c + 52 - '0';
  } else if (c == '+') {
    return 62;
  } else if (c == '/') {
    return 63;
  } else {
    return 64;
  }
}

static int b64enc(struct json_out *out, const unsigned char *p, int n) {
  char buf[4];
  int i, len = 0;
  for (i = 0; i < n; i += 3) {
    int a = p[i], b = i + 1 < n ? p[i + 1] : 0, c = i + 2 < n ? p[i + 2] : 0;
    buf[0] = b64idx(a >> 2);
    buf[1] = b64idx((a & 3) << 4 | (b >> 4));
    buf[2] = b64idx((b & 15) << 2 | (c >> 6));
    buf[3] = b64idx(c & 63);
    if (i + 1 >= n) buf[2] = '=';
    if (i + 2 >= n) buf[3] = '=';
    len += out->printer(out, buf, sizeof(buf));
  }
  return len;
}

static int b64dec(const char *src, int n, char *dst) {
  const char *end = src + n;
  int len = 0;
  while (src + 3 < end) {
    int a = b64rev(src[0]), b = b64rev(src[1]), c = b64rev(src[2]),
        d = b64rev(src[3]);
    dst[len++] = (a << 2) | (b >> 4);
    if (src[2] != '=') {
      dst[len++] = (b << 4) | (c >> 2);
      if (src[3] != '=') {
        dst[len++] = (c << 6) | d;
      }
    }
    src += 4;
  }
  return len;
}
#endif

static unsigned char hexdec(const char *s) {
#define HEXTOI(x) (x >= '0' && x <= '9' ? x - '0' : x - 'W')
  int a = tolower(*(const unsigned char *) s);
  int b = tolower(*(const unsigned char *) (s + 1));
  return (HEXTOI(a) << 4) | HEXTOI(b);
}

int json_vprintf(struct json_out *out, const char *fmt, va_list xap) WEAK;
int json_vprintf(struct json_out *out, const char *fmt, va_list xap) {
  int len = 0;
  const char *quote = "\"", *null = "null";  /* literal JSON, bukan kata BAIK */
  va_list ap;
  va_copy(ap, xap);

  while (*fmt != '\0') {
    if (strchr(":, \r\n\t[]{}\"", *fmt) != NULL) {
      len += out->printer(out, fmt, 1);
      fmt++;
    } else if (fmt[0] == '%') {
      char buf[21];
      size_t skip = 2;

      if (fmt[1] == 'l' && fmt[2] == 'l' && (fmt[3] == 'd' || fmt[3] == 'u')) {
        int64_t val = va_arg(ap, int64_t);
        const char *fmt2 = fmt[3] == 'u' ? "%" UINT64_FMT : "%" INT64_FMT;
        snprintf(buf, sizeof(buf), fmt2, val);
        len += out->printer(out, buf, strlen(buf));
        skip += 2;
      } else if (fmt[1] == 'z' && fmt[2] == 'u') {
        size_t val = va_arg(ap, size_t);
        snprintf(buf, sizeof(buf), "%lu", (unsigned long) val);
        len += out->printer(out, buf, strlen(buf));
        skip += 1;
      } else if (fmt[1] == 'M') {
        json_printf_callback_t f = va_arg(ap, json_printf_callback_t);
        len += f(out, &ap);
      } else if (fmt[1] == 'B') {
        int val = va_arg(ap, int);
        const char *str = val ? "true" : "false";  /* literal JSON */
        len += out->printer(out, str, strlen(str));
      } else if (fmt[1] == 'H') {
#if JSON_ENABLE_HEX
        const char *hex = "0123456789abcdef";
        int i, n = va_arg(ap, int);
        const unsigned char *p = va_arg(ap, const unsigned char *);
        len += out->printer(out, quote, 1);
        for (i = 0; i < n; i++) {
          len += out->printer(out, &hex[(p[i] >> 4) & 0xf], 1);
          len += out->printer(out, &hex[p[i] & 0xf], 1);
        }
        len += out->printer(out, quote, 1);
#endif
      } else if (fmt[1] == 'V') {
#if JSON_ENABLE_BASE64
        const unsigned char *p = va_arg(ap, const unsigned char *);
        int n = va_arg(ap, int);
        len += out->printer(out, quote, 1);
        len += b64enc(out, p, n);
        len += out->printer(out, quote, 1);
#endif
      } else if (fmt[1] == 'Q' ||
                 (fmt[1] == '.' && fmt[2] == '*' && fmt[3] == 'Q')) {
        size_t l = 0;
        const char *p;

        if (fmt[1] == '.') {
          l = (size_t) va_arg(ap, int);
          skip += 2;
        }
        p = va_arg(ap, char *);

        if (p == NULL) {
          len += out->printer(out, null, 4);
        } else {
          if (fmt[1] == 'Q') {
            l = strlen(p);
          }
          len += out->printer(out, quote, 1);
          len += json_escape(out, p, l);
          len += out->printer(out, quote, 1);
        }
      } else {
       

        const char *end_of_format_specifier = "sdfFeEgGlhuIcx.*-0123456789";
        int n = strspn(fmt + 1, end_of_format_specifier);
        char *pbuf = buf;
        int need_len, size = sizeof(buf);
        char fmt2[20];
        va_list ap_copy;
        strncpy(fmt2, fmt,
                n + 1 > (int) sizeof(fmt2) ? sizeof(fmt2) : (size_t) n + 1);
        fmt2[n + 1] = '\0';

        va_copy(ap_copy, ap);
        need_len = vsnprintf(pbuf, size, fmt2, ap_copy);
        va_end(ap_copy);

        if (need_len < 0) {
         
          pbuf = NULL;
          while (need_len < 0) {
            free(pbuf);
            size *= 2;
            if ((pbuf = (char *) malloc(size)) == NULL) break;
            va_copy(ap_copy, ap);
            need_len = vsnprintf(pbuf, size, fmt2, ap_copy);
            va_end(ap_copy);
          }
        } else if (need_len >= (int) sizeof(buf)) {
         
          if ((pbuf = (char *) malloc(need_len + 1)) != NULL) {
            va_copy(ap_copy, ap);
            vsnprintf(pbuf, need_len + 1, fmt2, ap_copy);
            va_end(ap_copy);
          }
        }
        if (pbuf == NULL) {
          buf[0] = '\0';
          pbuf = buf;
        }

       
        if ((n + 1 == strlen("%" PRId64) && strcmp(fmt2, "%" PRId64) == 0) ||
            (n + 1 == strlen("%" PRIu64) && strcmp(fmt2, "%" PRIu64) == 0)) {
          (void) va_arg(ap, int64_t);
        } else if (strcmp(fmt2, "%.*s") == 0) {
          (void) va_arg(ap, int);
          (void) va_arg(ap, char *);
        } else {
          switch (fmt2[n]) {
            case 'u':
            case 'd':
              (void) va_arg(ap, int);
              break;
            case 'g':
            case 'f':
              (void) va_arg(ap, double);
              break;
            case 'p':
              (void) va_arg(ap, void *);
              break;
            default:
             
              (void) va_arg(ap, int);
          }
        }

        len += out->printer(out, pbuf, strlen(pbuf));
        skip = n + 1;
 
        if (pbuf != buf) {
          free(pbuf);
          pbuf = NULL;
        }
      }
      fmt += skip;
    } else if (*fmt == '_' || json_isalpha(*fmt)) {
      len += out->printer(out, quote, 1);
      while (*fmt == '_' || json_isalpha(*fmt) || json_isdigit(*fmt)) {
        len += out->printer(out, fmt, 1);
        fmt++;
      }
      len += out->printer(out, quote, 1);
    } else {
      len += out->printer(out, fmt, 1);
      fmt++;
    }
  }
  va_end(ap);

  return len;
}

int json_printf(struct json_out *out, const char *fmt, ...) WEAK;
int json_printf(struct json_out *out, const char *fmt, ...) {
  int n;
  va_list ap;
  va_start(ap, fmt);
  n = json_vprintf(out, fmt, ap);
  va_end(ap);
  return n;
}

int json_printf_array(struct json_out *out, va_list *ap) WEAK;
int json_printf_array(struct json_out *out, va_list *ap) {
  int len = 0;
  char *arr = va_arg(*ap, char *);
  size_t i, arr_size = va_arg(*ap, size_t);
  size_t elem_size = va_arg(*ap, size_t);
  const char *fmt = va_arg(*ap, char *);
  len += json_printf(out, "[", 1);
  for (i = 0; arr != NULL && i < arr_size / elem_size; i++) {
    union {
      int64_t i;
      double d;
    } val;
    memcpy(&val, arr + i * elem_size,
           elem_size > sizeof(val) ? sizeof(val) : elem_size);
    if (i > 0) len += json_printf(out, ", ");
    if (strpbrk(fmt, "efg") != NULL) {
      len += json_printf(out, fmt, val.d);
    } else {
      len += json_printf(out, fmt, val.i);
    }
  }
  len += json_printf(out, "]", 1);
  return len;
}

#ifdef _WIN32
int BAIK_EM_win_vsnprintf(char *str, size_t size, const char *format,
                     va_list ap) WEAK;
int BAIK_EM_win_vsnprintf(char *str, size_t size, const char *format, va_list ap) {
  int res = _vsnprintf(str, size, format, ap);
  va_end(ap);
  if (res >= size) {
    str[size - 1] = '\0';
  }
  return res;
}

int BAIK_EM_win_snprintf(char *str, size_t size, const char *format, ...) WEAK;
int BAIK_EM_win_snprintf(char *str, size_t size, const char *format, ...) {
  int res;
  va_list ap;
  va_start(ap, format);
  res = vsnprintf(str, size, format, ap);
  va_end(ap);
  return res;
}
#endif

int json_walk(const char *json_string, int json_string_length,
              json_walk_callback_t callback, void *callback_data) WEAK;
int json_walk(const char *json_string, int json_string_length,
              json_walk_callback_t callback, void *callback_data) {
  struct frozen frozen;

  memset(&frozen, 0, sizeof(frozen));
  frozen.end = json_string + json_string_length;
  frozen.cur = json_string;
  frozen.callback_data = callback_data;
  frozen.callback = callback;

  TRY(json_doit(&frozen));

  return frozen.cur - json_string;
}

struct scan_array_info {
  int found;
  char path[JSON_MAX_PATH_LEN];
  struct json_token *token;
};

static void json_scanf_array_elem_cb(void *callback_data, const char *name,
                                     size_t name_len, const char *path,
                                     const struct json_token *token) {
  struct scan_array_info *info = (struct scan_array_info *) callback_data;

  (void) name;
  (void) name_len;

  if (strcmp(path, info->path) == 0) {
    *info->token = *token;
    info->found = 1;
  }
}

int json_scanf_array_elem(const char *s, int len, const char *path, int idx,
                          struct json_token *token) WEAK;
int json_scanf_array_elem(const char *s, int len, const char *path, int idx,
                          struct json_token *token) {
  struct scan_array_info info;
  info.token = token;
  info.found = 0;
  memset(token, 0, sizeof(*token));
  snprintf(info.path, sizeof(info.path), "%s[%d]", path, idx);
  json_walk(s, len, json_scanf_array_elem_cb, &info);
  return info.found ? token->len : -1;
}

struct json_scanf_info {
  int num_conversions;
  char *path;
  const char *fmt;
  void *target;
  void *user_data;
  int type;
};

int json_unescape(const char *src, int slen, char *dst, int dlen) WEAK;
int json_unescape(const char *src, int slen, char *dst, int dlen) {
  char *send = (char *) src + slen, *dend = dst + dlen, *orig_dst = dst, *p;
  const char *esc1 = "\"\\/bfnrt", *esc2 = "\"\\/\b\f\n\r\t";

  while (src < send) {
    if (*src == '\\') {
      if (++src >= send) return JSON_STRING_INCOMPLETE;
      if (*src == 'u') {
        if (send - src < 5) return JSON_STRING_INCOMPLETE;
       
        if (src[1] == '0' && src[2] == '0') {
         
          if (dst < dend) *dst = hexdec(src + 3);
          src += 4;
        } else {
          return JSON_STRING_INVALID;
        }
      } else if ((p = (char *) strchr(esc1, *src)) != NULL) {
        if (dst < dend) *dst = esc2[p - esc1];
      } else {
        return JSON_STRING_INVALID;
      }
    } else {
      if (dst < dend) *dst = *src;
    }
    dst++;
    src++;
  }

  return dst - orig_dst;
}

static void json_scanf_cb(void *callback_data, const char *name,
                          size_t name_len, const char *path,
                          const struct json_token *token) {
  struct json_scanf_info *info = (struct json_scanf_info *) callback_data;
  char buf[32];

  (void) name;
  (void) name_len;

  if (token->ptr == NULL) {
    return;
  }

  if (strcmp(path, info->path) != 0) {
    return;
  }

  switch (info->type) {
    case 'B':
      info->num_conversions++;
      switch (sizeof(bool)) {
        case sizeof(char):
          *(char *) info->target = (token->type == JSON_TYPE_TRUE ? 1 : 0);
          break;
        case sizeof(int):
          *(int *) info->target = (token->type == JSON_TYPE_TRUE ? 1 : 0);
          break;
        default:
          abort();
      }
      break;
    case 'M': {
      union {
        void *p;
        json_scanner_t f;
      } u = {info->target};
      info->num_conversions++;
      u.f(token->ptr, token->len, info->user_data);
      break;
    }
    case 'Q': {
      char **dst = (char **) info->target;
      if (token->type == JSON_TYPE_NULL) {
        *dst = NULL;
      } else {
        int unescaped_len = json_unescape(token->ptr, token->len, NULL, 0);
        if (unescaped_len >= 0 &&
            (*dst = (char *) malloc(unescaped_len + 1)) != NULL) {
          info->num_conversions++;
          if (json_unescape(token->ptr, token->len, *dst, unescaped_len) ==
              unescaped_len) {
            (*dst)[unescaped_len] = '\0';
          } else {
            free(*dst);
            *dst = NULL;
          }
        }
      }
      break;
    }
    case 'H': {
#if JSON_ENABLE_HEX
      char **dst = (char **) info->user_data;
      int i, len = token->len / 2;
      *(int *) info->target = len;
      if ((*dst = (char *) malloc(len + 1)) != NULL) {
        for (i = 0; i < len; i++) {
          (*dst)[i] = hexdec(token->ptr + 2 * i);
        }
        (*dst)[len] = '\0';
        info->num_conversions++;
      }
#endif
      break;
    }
    case 'V': {
#if JSON_ENABLE_BASE64
      char **dst = (char **) info->target;
      int len = token->len * 4 / 3 + 2;
      if ((*dst = (char *) malloc(len + 1)) != NULL) {
        int n = b64dec(token->ptr, token->len, *dst);
        (*dst)[n] = '\0';
        *(int *) info->user_data = n;
        info->num_conversions++;
      }
#endif
      break;
    }
    case 'T':
      info->num_conversions++;
      *(struct json_token *) info->target = *token;
      break;
    default:
      if (token->len >= (int) sizeof(buf)) break;
     
      memcpy(buf, token->ptr, token->len);
      buf[token->len] = '\0';
     
      if (info->fmt[1] == 'd' || (info->fmt[1] == 'l' && info->fmt[2] == 'd') ||
          info->fmt[1] == 'i') {
        char *endptr = NULL;
        long r = strtol(buf, &endptr, 0);
        if (*endptr == '\0') {
          if (info->fmt[1] == 'l') {
            *((long *) info->target) = r;
          } else {
            *((int *) info->target) = (int) r;
          }
          info->num_conversions++;
        }
      } else if (info->fmt[1] == 'u' ||
                 (info->fmt[1] == 'l' && info->fmt[2] == 'u')) {
        char *endptr = NULL;
        unsigned long r = strtoul(buf, &endptr, 0);
        if (*endptr == '\0') {
          if (info->fmt[1] == 'l') {
            *((unsigned long *) info->target) = r;
          } else {
            *((unsigned int *) info->target) = (unsigned int) r;
          }
          info->num_conversions++;
        }
      } else {
#if !JSON_MINIMAL
        info->num_conversions += sscanf(buf, info->fmt, info->target);
#endif
      }
      break;
  }
}

int json_vscanf(const char *s, int len, const char *fmt, va_list ap) WEAK;
int json_vscanf(const char *s, int len, const char *fmt, va_list ap) {
  char path[JSON_MAX_PATH_LEN] = "", fmtbuf[20];
  int i = 0;
  char *p = NULL;
  struct json_scanf_info info = {0, path, fmtbuf, NULL, NULL, 0};

  while (fmt[i] != '\0') {
    if (fmt[i] == '{') {
      strcat(path, ".");
      i++;
    } else if (fmt[i] == '}') {
      if ((p = strrchr(path, '.')) != NULL) *p = '\0';
      i++;
    } else if (fmt[i] == '%') {
      info.target = va_arg(ap, void *);
      info.type = fmt[i + 1];
      switch (fmt[i + 1]) {
        case 'M':
        case 'V':
        case 'H':
          info.user_data = va_arg(ap, void *);
       
        case 'B':
        case 'Q':
        case 'T':
          i += 2;
          break;
        default: {
          const char *delims = ", \t\r\n]}";
          int conv_len = strcspn(fmt + i + 1, delims) + 1;
          memcpy(fmtbuf, fmt + i, conv_len);
          fmtbuf[conv_len] = '\0';
          i += conv_len;
          i += strspn(fmt + i, delims);
          break;
        }
      }
      json_walk(s, len, json_scanf_cb, &info);
    } else if (json_isalpha(fmt[i]) || json_get_utf8_char_len(fmt[i]) > 1) {
      char *pe;
      const char *delims = ": \r\n\t";
      int key_len = strcspn(&fmt[i], delims);
      if ((p = strrchr(path, '.')) != NULL) p[1] = '\0';
      pe = path + strlen(path);
      memcpy(pe, fmt + i, key_len);
      pe[key_len] = '\0';
      i += key_len + strspn(fmt + i + key_len, delims);
    } else {
      i++;
    }
  }
  return info.num_conversions;
}

int json_scanf(const char *str, int len, const char *fmt, ...) WEAK;
int json_scanf(const char *str, int len, const char *fmt, ...) {
  int result;
  va_list ap;
  va_start(ap, fmt);
  result = json_vscanf(str, len, fmt, ap);
  va_end(ap);
  return result;
}

int json_vfprintf(const char *file_name, const char *fmt, va_list ap) WEAK;
int json_vfprintf(const char *file_name, const char *fmt, va_list ap) {
  int res = -1;
  FILE *fp = fopen(file_name, "wb");
  if (fp != NULL) {
    struct json_out out = JSON_OUT_FILE(fp);
    res = json_vprintf(&out, fmt, ap);
    fputc('\n', fp);
    fclose(fp);
  }
  return res;
}

int json_fprintf(const char *file_name, const char *fmt, ...) WEAK;
int json_fprintf(const char *file_name, const char *fmt, ...) {
  int result;
  va_list ap;
  va_start(ap, fmt);
  result = json_vfprintf(file_name, fmt, ap);
  va_end(ap);
  return result;
}

char *json_fread(const char *path) WEAK;
char *json_fread(const char *path) {
  FILE *fp;
  char *data = NULL;
  if ((fp = fopen(path, "rb")) == NULL) {
  } else if (fseek(fp, 0, SEEK_END) != 0) {
    fclose(fp);
  } else {
    long size = ftell(fp);
    if (size > 0 && (data = (char *) malloc(size + 1)) != NULL) {
      fseek(fp, 0, SEEK_SET);
      if (fread(data, 1, size, fp) != (size_t) size) {
        free(data);
        data = NULL;
      } else {
        data[size] = '\0';
      }
    }
    fclose(fp);
  }
  return data;
}

struct json_setf_data {
  const char *json_path;
  const char *base;
  int matched;     
  int pos;         
  int end;         
  int prev;        
};

static int get_matched_prefix_len(const char *s1, const char *s2) {
  int i = 0;
  while (s1[i] && s2[i] && s1[i] == s2[i]) i++;
  return i;
}

static void json_vsetf_cb(void *userdata, const char *name, size_t name_len,
                          const char *path, const struct json_token *t) {
  struct json_setf_data *data = (struct json_setf_data *) userdata;
  int off, len = get_matched_prefix_len(path, data->json_path);
  if (t->ptr == NULL) return;
  off = t->ptr - data->base;
  if (len > data->matched) data->matched = len;

 
  if (len < data->matched && data->pos == 0 &&
      (t->type == JSON_TYPE_OBJECT_END || t->type == JSON_TYPE_ARRAY_END)) {
    data->pos = data->end = data->prev;
  }

 
  if (strcmp(path, data->json_path) == 0 && t->type != JSON_TYPE_OBJECT_START &&
      t->type != JSON_TYPE_ARRAY_START) {
    data->pos = off;
    data->end = off + t->len;
  }

  if (data->pos == 0) {
    data->prev = off + t->len;
  } else if ((t->ptr[0] == '[' || t->ptr[0] == '{') && off + 1 < data->pos &&
             off + 1 > data->prev) {
    data->prev = off + 1;
  }
  (void) name;
  (void) name_len;
}

int json_vsetf(const char *s, int len, struct json_out *out,
               const char *json_path, const char *json_fmt, va_list ap) WEAK;
int json_vsetf(const char *s, int len, struct json_out *out,
               const char *json_path, const char *json_fmt, va_list ap) {
  struct json_setf_data data;
  memset(&data, 0, sizeof(data));
  data.json_path = json_path;
  data.base = s;
  data.end = len;
  json_walk(s, len, json_vsetf_cb, &data);
  if (json_fmt == NULL) {
   
    json_printf(out, "%.*s", data.prev, s);
   
    if (s[data.prev - 1] == '{' || s[data.prev - 1] == '[') {
      int i = data.end;
      while (i < len && json_isspace(s[i])) i++;
      if (s[i] == ',') data.end = i + 1;
    }
    json_printf(out, "%.*s", len - data.end, s + data.end);
  } else {
   
    int n, off = data.matched, depth = 0;

    json_printf(out, "%.*s", data.pos, s);

    while ((n = strcspn(&json_path[off], ".[")) > 0) {
      if (s[data.prev - 1] != '{' && s[data.prev - 1] != '[' && depth == 0) {
        json_printf(out, ",");
      }
      if (off > 0 && json_path[off - 1] != '.') break;
      json_printf(out, "%.*Q:", n, json_path + off);
      off += n;
      if (json_path[off] != '\0') {
        json_printf(out, "%c", json_path[off] == '.' ? '{' : '[');
        depth++;
        off++;
      }
    }
   
    json_vprintf(out, json_fmt, ap);

    for (; off > data.matched; off--) {
      int ch = json_path[off];
      const char *p = ch == '.' ? "}" : ch == '[' ? "]" : "";
      json_printf(out, "%s", p);
    }

    json_printf(out, "%.*s", len - data.end, s + data.end);
  }
  return data.end > data.pos ? 1 : 0;
}

int json_setf(const char *s, int len, struct json_out *out,
              const char *json_path, const char *json_fmt, ...) WEAK;
int json_setf(const char *s, int len, struct json_out *out,
              const char *json_path, const char *json_fmt, ...) {
  int result;
  va_list ap;
  va_start(ap, json_fmt);
  result = json_vsetf(s, len, out, json_path, json_fmt, ap);
  va_end(ap);
  return result;
}

struct prettify_data {
  struct json_out *out;
  int level;
  int last_token;
};

static void indent(struct json_out *out, int level) {
  while (level-- > 0) out->printer(out, "  ", 2);
}

static void print_key(struct prettify_data *pd, const char *path,
                      const char *name, int name_len) {
  if (pd->last_token != JSON_TYPE_INVALID &&
      pd->last_token != JSON_TYPE_ARRAY_START &&
      pd->last_token != JSON_TYPE_OBJECT_START) {
    pd->out->printer(pd->out, ",", 1);
  }
  if (path[0] != '\0') pd->out->printer(pd->out, "\n", 1);
  indent(pd->out, pd->level);
  if (path[0] != '\0' && path[strlen(path) - 1] != ']') {
    pd->out->printer(pd->out, "\"", 1);
    pd->out->printer(pd->out, name, (int) name_len);
    pd->out->printer(pd->out, "\"", 1);
    pd->out->printer(pd->out, ": ", 2);
  }
}

static void prettify_cb(void *userdata, const char *name, size_t name_len,
                        const char *path, const struct json_token *t) {
  struct prettify_data *pd = (struct prettify_data *) userdata;
  switch (t->type) {
    case JSON_TYPE_OBJECT_START:
    case JSON_TYPE_ARRAY_START:
      print_key(pd, path, name, name_len);
      pd->out->printer(pd->out, t->type == JSON_TYPE_ARRAY_START ? "[" : "{",
                       1);
      pd->level++;
      break;
    case JSON_TYPE_OBJECT_END:
    case JSON_TYPE_ARRAY_END:
      pd->level--;
      if (pd->last_token != JSON_TYPE_INVALID &&
          pd->last_token != JSON_TYPE_ARRAY_START &&
          pd->last_token != JSON_TYPE_OBJECT_START) {
        pd->out->printer(pd->out, "\n", 1);
        indent(pd->out, pd->level);
      }
      pd->out->printer(pd->out, t->type == JSON_TYPE_ARRAY_END ? "]" : "}", 1);
      break;
    case JSON_TYPE_NUMBER:
    case JSON_TYPE_NULL:
    case JSON_TYPE_TRUE:
    case JSON_TYPE_FALSE:
    case JSON_TYPE_STRING:
      print_key(pd, path, name, name_len);
      if (t->type == JSON_TYPE_STRING) pd->out->printer(pd->out, "\"", 1);
      pd->out->printer(pd->out, t->ptr, t->len);
      if (t->type == JSON_TYPE_STRING) pd->out->printer(pd->out, "\"", 1);
      break;
    default:
      break;
  }
  pd->last_token = t->type;
}

int json_prettify(const char *s, int len, struct json_out *out) WEAK;
int json_prettify(const char *s, int len, struct json_out *out) {
  struct prettify_data pd = {out, 0, JSON_TYPE_INVALID};
  return json_walk(s, len, prettify_cb, &pd);
}

int json_prettify_file(const char *file_name) WEAK;
int json_prettify_file(const char *file_name) {
  int res = -1;
  char *s = json_fread(file_name);
  FILE *fp;
  if (s != NULL && (fp = fopen(file_name, "wb")) != NULL) {
    struct json_out out = JSON_OUT_FILE(fp);
    res = json_prettify(s, strlen(s), &out);
    if (res < 0) {
     
      fclose(fp);
      fp = fopen(file_name, "wb");
      fseek(fp, 0, SEEK_SET);
      fwrite(s, 1, strlen(s), fp);
    } else {
      fputc('\n', fp);
    }
    fclose(fp);
  }
  free(s);
  return res;
}

struct next_data {
  void *handle;            
  const char *path;        
  int path_len;            
  int found;               
  struct json_token *key;  
  struct json_token *val;  
  int *idx;                
};

static void next_set_key(struct next_data *d, const char *name, int name_len,
                         int is_array) {
  if (is_array) {
    if (d->key != NULL) {
      d->key->len = 0;
      d->key->ptr = NULL;
    }
    if (d->idx != NULL) *d->idx = atoi(name);
  } else {
   
    if (d->key != NULL) {
      d->key->ptr = name;
      d->key->len = name_len;
    }
    if (d->idx != NULL) *d->idx = -1;
  }
}

static void json_next_cb(void *userdata, const char *name, size_t name_len,
                         const char *path, const struct json_token *t) {
  struct next_data *d = (struct next_data *) userdata;
  const char *p = path + d->path_len;
  if (d->found) return;
  if (d->path_len >= (int) strlen(path)) return;
  if (strncmp(d->path, path, d->path_len) != 0) return;
  if (strchr(p + 1, '.') != NULL) return;
  if (strchr(p + 1, '[') != NULL) return;
  
  if (t->type == JSON_TYPE_OBJECT_START || t->type == JSON_TYPE_ARRAY_START) {
    next_set_key(d, name, name_len, p[0] == '[');
  } else if (d->handle == NULL || d->handle < (void *) t->ptr) {
    if (t->type != JSON_TYPE_OBJECT_END && t->type != JSON_TYPE_ARRAY_END) {
      next_set_key(d, name, name_len, p[0] == '[');
    }
    if (d->val != NULL) *d->val = *t;
    d->handle = (void *) t->ptr;
    d->found = 1;
  }
}

static void *json_next(const char *s, int len, void *handle, const char *path,
                       struct json_token *key, struct json_token *val, int *i) {
  struct json_token tmpval, *v = val == NULL ? &tmpval : val;
  struct json_token tmpkey, *k = key == NULL ? &tmpkey : key;
  int tmpidx, *pidx = i == NULL ? &tmpidx : i;
  struct next_data data = {handle, path, (int) strlen(path), 0, k, v, pidx};
  json_walk(s, len, json_next_cb, &data);
  return data.found ? data.handle : NULL;
}

void *json_next_key(const char *s, int len, void *handle, const char *path,
                    struct json_token *key, struct json_token *val) WEAK;
void *json_next_key(const char *s, int len, void *handle, const char *path,
                    struct json_token *key, struct json_token *val) {
  return json_next(s, len, handle, path, key, val, NULL);
}

void *json_next_elem(const char *s, int len, void *handle, const char *path,
                     int *idx, struct json_token *val) WEAK;
void *json_next_elem(const char *s, int len, void *handle, const char *path,
                     int *idx, struct json_token *val) {
  return json_next(s, len, handle, path, NULL, val, idx);
}

static int json_sprinter(struct json_out *out, const char *str, size_t len) {
  size_t old_len = out->u.buf.buf == NULL ? 0 : strlen(out->u.buf.buf);
  size_t new_len = len + old_len;
  char *p = (char *) realloc(out->u.buf.buf, new_len + 1);
  if (p != NULL) {
    memcpy(p + old_len, str, len);
    p[new_len] = '\0';
    out->u.buf.buf = p;
  }
  return len;
}

char *json_vasprintf(const char *fmt, va_list ap) WEAK;
char *json_vasprintf(const char *fmt, va_list ap) {
  struct json_out out;
  memset(&out, 0, sizeof(out));
  out.printer = json_sprinter;
  json_vprintf(&out, fmt, ap);
  return out.u.buf.buf;
}

char *json_asprintf(const char *fmt, ...) WEAK;
char *json_asprintf(const char *fmt, ...) {
  char *result = NULL;
  va_list ap;
  va_start(ap, fmt);
  result = json_vasprintf(fmt, ap);
  va_end(ap);
  return result;
}

/* Makro pembantu FFI dipindah ke baik_ffi.c, tempat pemakaiannya. */

