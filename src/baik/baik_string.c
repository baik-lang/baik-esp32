/*
 * baik_string.c - Tipe huruf (string): pembuatan, pembacaan, perbandingan,
 * slice, indexOf, charCodeAt, mkstr, serta unescape dan penanaman string
 * (embed_string) ke dalam mbuf.
 *
 * Bagian dari interpreter bahasa BAIK. Deklarasi bersama ada di
 * src/baik/baik_internal.h; antarmuka publik ada di src/baik.h.
 */
#include "baik_internal.h"

static int chartorune(Rune *rune, const char *str) {
  *rune = *(unsigned char *) str;
  return 1;
}
static int runetochar(char *str, Rune *rune) {
  str[0] = (char) *rune;
  return 1;
}

#ifndef BAIK_STRING_BUF_RESERVE
#define BAIK_STRING_BUF_RESERVE 100
#endif

BAIK_PRIVATE size_t unescape(const char *s, size_t len, char *to);
BAIK_PRIVATE void embed_string(struct mbuf *m, size_t offset, const char *p,
                              size_t len, uint8_t flags);

#define GET_VAL_NAN_PAYLOAD(v) ((char *) &(v))

int baik_is_string(baik_val_t v) {
  uint64_t t = v & BAIK_TAG_MASK;
  return t == BAIK_TAG_STRING_I || t == BAIK_TAG_STRING_F ||
         t == BAIK_TAG_STRING_O || t == BAIK_TAG_STRING_5 ||
         t == BAIK_TAG_STRING_D;
}

baik_val_t baik_mk_string(struct baik *baik, const char *p, size_t len, int copy) {
  struct mbuf *m;
  baik_val_t offset, tag = BAIK_TAG_STRING_F;
  if (len == 0) {
   
    copy = 1;
  }
  m = copy ? &baik->owned_strings : &baik->foreign_strings;
  offset = m->len;

  if (len == ~((size_t) 0)) len = strlen(p);

  if (copy) {
   
    if (len <= 4) {
      char *s = GET_VAL_NAN_PAYLOAD(offset) + 1;
      offset = 0;
      if (p != 0) {
        memcpy(s, p, len);
      }
      s[-1] = len;
      tag = BAIK_TAG_STRING_I;
    } else if (len == 5) {
      char *s = GET_VAL_NAN_PAYLOAD(offset);
      offset = 0;
      if (p != 0) {
        memcpy(s, p, len);
      }
      tag = BAIK_TAG_STRING_5; 
    } else {
      if (gc_strings_is_gc_needed(baik)) {
        baik->need_gc = 1;
      }

      if ((m->len + len) > m->size) {
        char *prev_buf = m->buf;
        mbuf_resize(m, m->len + len + BAIK_STRING_BUF_RESERVE);

       
        if (p >= prev_buf && p < (prev_buf + m->len)) {
          p += (m->buf - prev_buf);
        }
      }

      embed_string(m, m->len, p, len, EMBSTR_ZERO_TERM);
      tag = BAIK_TAG_STRING_O;
    }
  } else {
   
    if (sizeof(void *) <= 4 && len <= (1 << 15)) {
     
      offset = (uint64_t) len << 32 | (uint64_t)(uintptr_t) p;
    } else {
     
      size_t pos = m->len;
      size_t llen = BAIK_EM_varint_llen(len);

      mbuf_insert(m, pos, NULL, llen + sizeof(p));

      BAIK_EM_varint_encode(len, (uint8_t *) (m->buf + pos), llen);
      memcpy(m->buf + pos + llen, &p, sizeof(p));
    }
    tag = BAIK_TAG_STRING_F;
  }
  return (offset & ~BAIK_TAG_MASK) | tag;
}


const char *baik_get_string(struct baik *baik, baik_val_t *v, size_t *sizep) {
  uint64_t tag = v[0] & BAIK_TAG_MASK;
  const char *p = NULL;
  size_t size = 0, llen;

  if (!baik_is_string(*v)) {
    goto clean;
  }

  if (tag == BAIK_TAG_STRING_I) {
    p = GET_VAL_NAN_PAYLOAD(*v) + 1;
    size = p[-1];
  } else if (tag == BAIK_TAG_STRING_5) {
    p = GET_VAL_NAN_PAYLOAD(*v);
    size = 5;
  } else if (tag == BAIK_TAG_STRING_O) {
    size_t offset = (size_t) gc_string_baik_val_to_offset(*v);
    char *s = baik->owned_strings.buf + offset;
    uint64_t v = 0;
    if (offset < baik->owned_strings.len &&
        BAIK_EM_varint_decode((uint8_t *) s, baik->owned_strings.len - offset, &v,
                         &llen)) {
      size = v;
      p = s + llen;
    } else {
      goto clean;
    }
  } else if (tag == BAIK_TAG_STRING_F) {
    uint16_t len = (*v >> 32) & 0xFFFF;
    if (sizeof(void *) <= 4 && len != 0) {
      size = (size_t) len;
      p = (const char *) (uintptr_t) *v;
    } else {
      size_t offset = (size_t) gc_string_baik_val_to_offset(*v);
      char *s = baik->foreign_strings.buf + offset;
      uint64_t v = 0;
      if (offset < baik->foreign_strings.len &&
          BAIK_EM_varint_decode((uint8_t *) s, baik->foreign_strings.len - offset, &v,
                           &llen)) {
        size = v;
        memcpy((char **) &p, s + llen, sizeof(p));
      } else {
        goto clean;
      }
    }
  } else {
    assert(0);
  }

clean:
  if (sizep != NULL) {
    *sizep = size;
  }
  return p;
}

const char *baik_get_cstring(struct baik *baik, baik_val_t *value) {
  size_t size;
  const char *s = baik_get_string(baik, value, &size);
  if (s == NULL) return NULL;
  if (s[size] != 0 || strlen(s) != size) {
    return NULL;
  }
  return s;
}

int baik_strcmp(struct baik *baik, baik_val_t *a, const char *b, size_t len) {
  size_t n;
  const char *s;
  if (len == (size_t) ~0) len = strlen(b);
  s = baik_get_string(baik, a, &n);
  if (n != len) {
    return n - len;
  }
  return strncmp(s, b, len);
}

BAIK_PRIVATE unsigned long cstr_to_ulong(const char *s, size_t len, int *ok) {
  char *e;
  unsigned long res = strtoul(s, &e, 10);
  *ok = (e == s + len) && len != 0;
  return res;
}

BAIK_PRIVATE baik_err_t
str_to_ulong(struct baik *baik, baik_val_t v, int *ok, unsigned long *res) {
  enum baik_err ret = BAIK_OK;
  size_t len = 0;
  const char *p = baik_get_string(baik, &v, &len);
  *res = cstr_to_ulong(p, len, ok);

  return ret;
}

BAIK_PRIVATE int s_cmp(struct baik *baik, baik_val_t a, baik_val_t b) {
  size_t a_len, b_len;
  const char *a_ptr, *b_ptr;

  a_ptr = baik_get_string(baik, &a, &a_len);
  b_ptr = baik_get_string(baik, &b, &b_len);

  if (a_len == b_len) {
    return memcmp(a_ptr, b_ptr, a_len);
  }
  if (a_len > b_len) {
    return 1;
  } else if (a_len < b_len) {
    return -1;
  } else {
    return 0;
  }
}

BAIK_PRIVATE baik_val_t s_concat(struct baik *baik, baik_val_t a, baik_val_t b) {
  size_t a_len, b_len, res_len;
  const char *a_ptr, *b_ptr, *res_ptr;
  baik_val_t res;

  a_ptr = baik_get_string(baik, &a, &a_len);
  b_ptr = baik_get_string(baik, &b, &b_len);
  res = baik_mk_string(baik, NULL, a_len + b_len, 1);
  a_ptr = baik_get_string(baik, &a, &a_len);
  b_ptr = baik_get_string(baik, &b, &b_len);
  res_ptr = baik_get_string(baik, &res, &res_len);
  memcpy((char *) res_ptr, a_ptr, a_len);
  memcpy((char *) res_ptr + a_len, b_ptr, b_len);

  return res;
}

BAIK_PRIVATE void baik_string_slice(struct baik *baik) {
  int nargs = baik_nargs(baik);
  baik_val_t ret = baik_mk_number(baik, 0);
  baik_val_t beginSlice_v = BAIK_UNDEFINED;
  baik_val_t endSlice_v = BAIK_UNDEFINED;
  int beginSlice = 0;
  int endSlice = 0;
  size_t size;
  const char *s = NULL;

  if (!baik_check_arg(baik, -1, "this", BAIK_TYPE_STRING, NULL)) {
    goto clean;
  }
  s = baik_get_string(baik, &baik->vals.this_obj, &size);

  if (!baik_check_arg(baik, 0, "beginSlice", BAIK_TYPE_NUMBER, &beginSlice_v)) {
    goto clean;
  }
  beginSlice = baik_normalize_idx(baik_get_int(baik, beginSlice_v), size);

  if (nargs >= 2) {
   
   
    if (!baik_check_arg(baik, 1, "endSlice", BAIK_TYPE_NUMBER, &endSlice_v)) {
      goto clean;
    }
    endSlice = baik_normalize_idx(baik_get_int(baik, endSlice_v), size);
  } else {
   
    endSlice = size;
  }

  if (endSlice < beginSlice) {
    endSlice = beginSlice;
  }

  ret = baik_mk_string(baik, s + beginSlice, endSlice - beginSlice, 1);

clean:
  baik_return(baik, ret);
}

BAIK_PRIVATE void baik_string_index_of(struct baik *baik) {
  baik_val_t ret = baik_mk_number(baik, -1);
  baik_val_t substr_v = BAIK_UNDEFINED;
  baik_val_t idx_v = BAIK_UNDEFINED;
  int idx = 0;
  const char *str = NULL, *substr = NULL;
  size_t str_len = 0, substr_len = 0;

  if (!baik_check_arg(baik, -1, "this", BAIK_TYPE_STRING, NULL)) {
    goto clean;
  }
  str = baik_get_string(baik, &baik->vals.this_obj, &str_len);

  if (!baik_check_arg(baik, 0, "searchValue", BAIK_TYPE_STRING, &substr_v)) {
    goto clean;
  }
  substr = baik_get_string(baik, &substr_v, &substr_len);
  if (baik_nargs(baik) > 1) {
    if (!baik_check_arg(baik, 1, "fromIndex", BAIK_TYPE_NUMBER, &idx_v)) {
      goto clean;
    }
    idx = baik_get_int(baik, idx_v);
    if (idx < 0) idx = 0;
    if ((size_t) idx > str_len) idx = str_len;
  }
  {
    const char *substr_p;
    struct baik_generic_str mgstr, mgsubstr;
    mgstr.p = str + idx;
    mgstr.len = str_len - idx;
    mgsubstr.p = substr;
    mgsubstr.len = substr_len;
    substr_p = baik_generic_strstr(mgstr, mgsubstr);
    if (substr_p != NULL) {
      ret = baik_mk_number(baik, (int) (substr_p - str));
    }
  }

clean:
  baik_return(baik, ret);
}

BAIK_PRIVATE void baik_string_char_code_at(struct baik *baik) {
  baik_val_t ret = BAIK_UNDEFINED;
  baik_val_t idx_v = BAIK_UNDEFINED;
  int idx = 0;
  size_t size;
  const char *s = NULL;

  if (!baik_check_arg(baik, -1, "this", BAIK_TYPE_STRING, NULL)) {
    goto clean;
  }
  s = baik_get_string(baik, &baik->vals.this_obj, &size);

  if (!baik_check_arg(baik, 0, "index", BAIK_TYPE_NUMBER, &idx_v)) {
    goto clean;
  }
  idx = baik_normalize_idx(baik_get_int(baik, idx_v), size);
  if (idx >= 0 && idx < (int) size) {
    ret = baik_mk_number(baik, ((unsigned char *) s)[idx]);
  }

clean:
  baik_return(baik, ret);
}

BAIK_PRIVATE void baik_mkstr(struct baik *baik) {
  int nargs = baik_nargs(baik);
  baik_val_t ret = BAIK_UNDEFINED;

  char *ptr = NULL;
  int offset = 0;
  int len = 0;
  int copy = 0;

  baik_val_t ptr_v = BAIK_UNDEFINED;
  baik_val_t offset_v = BAIK_UNDEFINED;
  baik_val_t len_v = BAIK_UNDEFINED;
  baik_val_t copy_v = BAIK_UNDEFINED;

  if (nargs == 2) {
    ptr_v = baik_arg(baik, 0);
    len_v = baik_arg(baik, 1);
  } else if (nargs == 3) {
    ptr_v = baik_arg(baik, 0);
    offset_v = baik_arg(baik, 1);
    len_v = baik_arg(baik, 2);
  } else if (nargs == 4) {
    ptr_v = baik_arg(baik, 0);
    offset_v = baik_arg(baik, 1);
    len_v = baik_arg(baik, 2);
    copy_v = baik_arg(baik, 3);
  } else {
    baik_prepend_errorf(baik, BAIK_TYPE_ERROR,
                       "mkstr membutuhkan 2, 3 atau 4 argument: (ptr, len), (ptr, "
                       "offset, len) atau (ptr, offset, len, copy)");
    goto clean;
  }

  if (!baik_is_foreign(ptr_v)) {
    baik_prepend_errorf(baik, BAIK_TYPE_ERROR, "GALAT : pointer yang tidak dikenali ptr");
    goto clean;
  }

  if (offset_v != BAIK_UNDEFINED && !baik_is_number(offset_v)) {
    baik_prepend_errorf(baik, BAIK_TYPE_ERROR, "GALAT : offset harus angka");
    goto clean;
  }

  if (!baik_is_number(len_v)) {
    baik_prepend_errorf(baik, BAIK_TYPE_ERROR, "GALAT : len harus angka");
    goto clean;
  }

  copy = baik_is_truthy(baik, copy_v);
  ptr = (char *) baik_get_ptr(baik, ptr_v);
  if (offset_v != BAIK_UNDEFINED) {
    offset = baik_get_int(baik, offset_v);
  }
  len = baik_get_int(baik, len_v);

  ret = baik_mk_string(baik, ptr + offset, len, copy);

clean:
  baik_return(baik, ret);
}

enum unescape_error {
  SLRE_INVALID_HEX_DIGIT,
  SLRE_INVALID_ESC_CHAR,
  SLRE_UNTERM_ESC_SEQ,
};

static int hex(int c) {
  if (c >= '0' && c <= '9') return c - '0';
  if (c >= 'a' && c <= 'f') return c - 'a' + 10;
  if (c >= 'A' && c <= 'F') return c - 'A' + 10;
  return -SLRE_INVALID_HEX_DIGIT;
}

static int nextesc(const char **p) {
  const unsigned char *s = (unsigned char *) (*p)++;
  switch (*s) {
    case 0:
      return -SLRE_UNTERM_ESC_SEQ;
    case 'c':
      ++*p;
      return *s & 31;
    case 'b':
      return '\b';
    case 't':
      return '\t';
    case 'n':
      return '\n';
    case 'v':
      return '\v';
    case 'f':
      return '\f';
    case 'r':
      return '\r';
    case '\\':
      return '\\';
    case 'u':
      if (isxdigit(s[1]) && isxdigit(s[2]) && isxdigit(s[3]) &&
          isxdigit(s[4])) {
        (*p) += 4;
        return hex(s[1]) << 12 | hex(s[2]) << 8 | hex(s[3]) << 4 | hex(s[4]);
      }
      return -SLRE_INVALID_HEX_DIGIT;
    case 'x':
      if (isxdigit(s[1]) && isxdigit(s[2])) {
        (*p) += 2;
        return (hex(s[1]) << 4) | hex(s[2]);
      }
      return -SLRE_INVALID_HEX_DIGIT;
    default:
      return -SLRE_INVALID_ESC_CHAR;
  }
}

BAIK_PRIVATE size_t unescape(const char *s, size_t len, char *to) {
  const char *end = s + len;
  size_t n = 0;
  char tmp[4];
  Rune r;

  while (s < end) {
    s += chartorune(&r, s);
    if (r == '\\' && s < end) {
      switch (*s) {
        case '"':
          s++, r = '"';
          break;
        case '\'':
          s++, r = '\'';
          break;
        case '\n':
          s++, r = '\n';
          break;
        default: {
          const char *tmp_s = s;
          int i = nextesc(&s);
          switch (i) {
            case -SLRE_INVALID_ESC_CHAR:
              r = '\\';
              s = tmp_s;
              n += runetochar(to == NULL ? tmp : to + n, &r);
              s += chartorune(&r, s);
              break;
            case -SLRE_INVALID_HEX_DIGIT:
            default:
              r = i;
          }
        }
      }
    }
    n += runetochar(to == NULL ? tmp : to + n, &r);
  }

  return n;
}

BAIK_PRIVATE void embed_string(struct mbuf *m, size_t offset, const char *p,
                              size_t len, uint8_t flags) {
  char *old_base = m->buf;
  uint8_t p_backed_by_mbuf = p >= old_base && p < old_base + m->len;
  size_t n = (flags & EMBSTR_UNESCAPE) ? unescape(p, len, NULL) : len;
  size_t k = BAIK_EM_varint_llen(n);
  size_t tot_len = k + n + !!(flags & EMBSTR_ZERO_TERM);

  mbuf_insert(m, offset, NULL, tot_len);

  if (p_backed_by_mbuf) {
    p += m->buf - old_base;
  }

  BAIK_EM_varint_encode(n, (unsigned char *) m->buf + offset, k);

  if (p != 0) {
    if (flags & EMBSTR_UNESCAPE) {
      unescape(p, len, m->buf + offset + k);
    } else {
      memcpy(m->buf + offset + k, p, len);
    }
  }

  if (flags & EMBSTR_ZERO_TERM) {
    m->buf[offset + tot_len - 1] = '\0';
  }
}

#include <stdlib.h>
#include <string.h>

