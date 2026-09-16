/*
 * baik_json.c - Serialisasi dan penguraian JSON pada tingkat bahasa BAIK.
 *
 * Bagian dari interpreter bahasa BAIK. Deklarasi bersama ada di
 * src/baik/baik_internal.h; antarmuka publik ada di src/baik.h.
 */
#include "baik_internal.h"


#define BUF_LEFT(size, used) (((size_t)(used) < (size)) ? ((size) - (used)) : 0)

static int should_skip_for_json(enum baik_type type) {
  int ret;
  switch (type) {
    case BAIK_TYPE_NULL:
    case BAIK_TYPE_BOOLEAN:
    case BAIK_TYPE_NUMBER:
    case BAIK_TYPE_STRING:
    case BAIK_TYPE_OBJECT_GENERIC:
    case BAIK_TYPE_OBJECT_ARRAY:
      ret = 0;
      break;
    default:
      ret = 1;
      break;
  }
  return ret;
}

/*
 * Literal JSON untuk nilai boolean dan null.
 *
 * PEMISAHAN DUA JALUR: baik_to_string() (baik_conversion.c) mengembalikan
 * kata Indonesia "benar"/"salah"/"kosong" karena itulah yang harus dilihat
 * MANUSIA (dipakai tulis() lewat baik_jprintf(), dan konversi ke huruf).
 * Serialisasi JSON tidak boleh memakai kata itu: hasilnya bukan JSON yang
 * sah sehingga JSON.parse() menolaknya dan pulang-pergi
 * JSON.parse(JSON.stringify(x)) gagal.
 *
 * Mengembalikan NULL bila v bukan boolean/null, artinya pemanggil boleh
 * memakai jalur baik_to_string() yang biasa (mis. untuk angka).
 */
static const char *json_literal_of(struct baik *baik, baik_val_t v) {
  const char *ret = NULL;
  if (baik_is_null(v)) {
    ret = "null";
  } else if (baik_is_boolean(v)) {
    ret = baik_get_bool(baik, v) ? "true" : "false";
  }
  return ret;
}

static const char *hex_digits = "0123456789abcdef";
static char *append_hex(char *buf, char *limit, uint8_t c) {
  if (buf < limit) *buf++ = 'u';
  if (buf < limit) *buf++ = '0';
  if (buf < limit) *buf++ = '0';
  if (buf < limit) *buf++ = hex_digits[(int) ((c >> 4) % 0xf)];
  if (buf < limit) *buf++ = hex_digits[(int) (c & 0xf)];
  return buf;
}

static int snquote(char *buf, size_t size, const char *s, size_t len) {
  char *limit = buf + size;
  const char *end;
 
  const char *specials = "btnvfr";
  size_t i = 0;

  i++;
  if (buf < limit) *buf++ = '"';

  for (end = s + len; s < end; s++) {
    if (*s == '"' || *s == '\\') {
      i++;
      if (buf < limit) *buf++ = '\\';
    } else if (*s >= '\b' && *s <= '\r') {
      i += 2;
      if (buf < limit) *buf++ = '\\';
      if (buf < limit) *buf++ = specials[*s - '\b'];
      continue;
    } else if ((unsigned char) *s < '\b' || (*s > '\r' && *s < ' ')) {
      i += 6;
      if (buf < limit) *buf++ = '\\';
      buf = append_hex(buf, limit, (uint8_t) *s);
      continue;
    }
    i++;
    if (buf < limit) *buf++ = *s;
  }

  i++;
  if (buf < limit) *buf++ = '"';

  if (buf < limit) {
    *buf = '\0';
  } else if (size != 0) {
   
    *(buf - 1) = '\0';
  }
  return i;
}

BAIK_PRIVATE baik_err_t to_json_or_debug(struct baik *baik, baik_val_t v, char *buf,
                                       size_t size, size_t *res_len,
                                       uint8_t is_debug) {
  baik_val_t el;
  char *vp;
  baik_err_t rcode = BAIK_OK;
  size_t len = 0;

  if (size > 0) *buf = '\0';

  if (!is_debug && should_skip_for_json(baik_get_type(v))) {
    goto clean;
  }

  for (vp = baik->json_visited_stack.buf;
       vp < baik->json_visited_stack.buf + baik->json_visited_stack.len;
       vp += sizeof(baik_val_t)) {
    if (*(baik_val_t *) vp == v) {
      strncpy(buf, "[Circular]", size);
      len = 10;
      goto clean;
    }
  }

  switch (baik_get_type(v)) {
    case BAIK_TYPE_NULL:
    case BAIK_TYPE_BOOLEAN:
    case BAIK_TYPE_NUMBER:
    case BAIK_TYPE_UNDEFINED:
    case BAIK_TYPE_FOREIGN:
      {
        char *p = NULL;
        int need_free = 0;
        const char *lit = is_debug ? NULL : json_literal_of(baik, v);

        if (lit != NULL) {
          /* Jalur JSON: benar/salah/kosong ditulis sebagai true/false/null. */
          len = strlen(lit);
          c_snprintf(buf, size, "%s", lit);
        } else {
          /* Jalur tampilan-untuk-manusia (is_debug) dan seluruh angka. */
          rcode = baik_to_string(baik, &v, &p, &len, &need_free);
          c_snprintf(buf, size, "%.*s", (int) len, p);
          if (need_free) {
            free(p);
          }
        }
      }
      goto clean;

    case BAIK_TYPE_STRING: {
      size_t n;
      const char *str = baik_get_string(baik, &v, &n);
      len = snquote(buf, size, str, n);
      goto clean;
    }

    case BAIK_TYPE_OBJECT_FUNCTION:
    case BAIK_TYPE_OBJECT_GENERIC: {
      char *b = buf;
      struct baik_property *prop = NULL;
      struct baik_object *o = NULL;

      mbuf_append(&baik->json_visited_stack, (char *) &v, sizeof(v));
      b += c_snprintf(b, BUF_LEFT(size, b - buf), "{");
      o = get_object_struct(v);
      for (prop = o->properties; prop != NULL; prop = prop->next) {
        size_t n;
        const char *s;
        if (!is_debug && should_skip_for_json(baik_get_type(prop->value))) {
          continue;
        }
        if (b - buf != 1) {
          b += c_snprintf(b, BUF_LEFT(size, b - buf), ",");
        }
        s = baik_get_string(baik, &prop->name, &n);
        b += c_snprintf(b, BUF_LEFT(size, b - buf), "\"%.*s\":", (int) n, s);
        {
          size_t tmp = 0;
          rcode = to_json_or_debug(baik, prop->value, b, BUF_LEFT(size, b - buf),
                                   &tmp, is_debug);
          if (rcode != BAIK_OK) {
            goto clean_iter;
          }
          b += tmp;
        }
      }

      b += c_snprintf(b, BUF_LEFT(size, b - buf), "}");
      baik->json_visited_stack.len -= sizeof(v);

    clean_iter:
      len = b - buf;
      goto clean;
    }
    case BAIK_TYPE_OBJECT_ARRAY: {
      int has;
      char *b = buf;
      size_t i, alen = baik_array_length(baik, v);
      mbuf_append(&baik->json_visited_stack, (char *) &v, sizeof(v));
      b += c_snprintf(b, BUF_LEFT(size, b - buf), "[");
      /* Lubang array dan elemen yang tidak bisa di-JSON-kan (takterdefinisi,
       * fungsi, foreign) menjadi "null" pada jalur JSON -- sama seperti
       * JSON.stringify([undefined]) di JavaScript -- dan tetap "kosong" pada
       * jalur tampilan-untuk-manusia. */
      for (i = 0; i < alen; i++) {
        const char *hole = is_debug ? "kosong" : "null";
        el = baik_array_get2(baik, v, i, &has);
        if (has) {
          size_t tmp = 0;
          if (!is_debug && should_skip_for_json(baik_get_type(el))) {
            b += c_snprintf(b, BUF_LEFT(size, b - buf), "%s", hole);
          } else {
            rcode = to_json_or_debug(baik, el, b, BUF_LEFT(size, b - buf), &tmp,
                                     is_debug);
            if (rcode != BAIK_OK) {
              goto clean;
            }
          }
          b += tmp;
        } else {
          b += c_snprintf(b, BUF_LEFT(size, b - buf), "%s", hole);
        }
        if (i != alen - 1) {
          b += c_snprintf(b, BUF_LEFT(size, b - buf), ",");
        }
      }
      b += c_snprintf(b, BUF_LEFT(size, b - buf), "]");
      baik->json_visited_stack.len -= sizeof(v);
      len = b - buf;
      goto clean;
    }

    case BAIK_TYPES_CNT:
      abort();
  }

  abort();

  len = 0;
  goto clean;

clean:
  if (rcode != BAIK_OK) {
    len = 0;
  }
  if (res_len != NULL) {
    *res_len = len;
  }
  return rcode;
}

BAIK_PRIVATE baik_err_t baik_json_stringify(struct baik *baik, baik_val_t v,
                                         char *buf, size_t size, char **res) {
  baik_err_t rcode = BAIK_OK;
  char *p = buf;
  size_t len;

  to_json_or_debug(baik, v, buf, size, &len, 0);

  if (len >= size) {
   
    p = (char *) malloc(len + 1);
    rcode = baik_json_stringify(baik, v, p, len + 1, res);
    assert(*res == p);
    goto clean;
  } else {
    *res = p;
    goto clean;
  }

clean:
 
  if (rcode != BAIK_OK && p != buf) {
    free(p);
  }
  return rcode;
}

struct json_parse_frame {
  baik_val_t val;
  struct json_parse_frame *up;
};

struct json_parse_ctx {
  struct baik *baik;
  baik_val_t result;
  struct json_parse_frame *frame;
  enum baik_err rcode;
};

static struct json_parse_frame *alloc_json_frame(struct json_parse_ctx *ctx,
                                                 baik_val_t v) {
  struct json_parse_frame *frame =
      (struct json_parse_frame *) calloc(sizeof(struct json_parse_frame), 1);
  frame->val = v;
  baik_own(ctx->baik, &frame->val);
  return frame;
}

static struct json_parse_frame *free_json_frame(
    struct json_parse_ctx *ctx, struct json_parse_frame *frame) {
  struct json_parse_frame *up = frame->up;
  baik_disown(ctx->baik, &frame->val);
  free(frame);
  return up;
}

/* strtod() atas potongan teks yang TIDAK diakhiri NUL.
 *
 * Token dari pengurai JSON menunjuk LANGSUNG ke dalam penyangga sumber dan
 * panjangnya hanya ada di token->len - tidak ada NUL di ujungnya. Memanggil
 * strtod(token->ptr, NULL) membuatnya terus membaca melewati ujung token.
 *
 * Di dalam objek atau untaian hal itu tidak terlihat, karena karakter
 * berikutnya pasti ',' '}' atau ']' yang menghentikan strtod. Tetapi untuk
 * angka telanjang seperti JSON.parse("42"), token berakhir tepat di ujung
 * penyangga sehingga strtod membaca memori tak terinisialisasi di belakangnya
 * dan sesekali menghasilkan digit tambahan (42 terbaca 422, -7 terbaca -73).
 * Nondeterministik, sekitar satu dari sepuluh kali - dan pada mikrokontroler
 * ini adalah pembacaan di luar batas yang sesungguhnya.
 *
 * Salin dulu ke penyangga berbatas, akhiri NUL, baru diurai.
 */
static double baik_strtod_n(const char *ptr, int len) {
  char buf[64];
  size_t n;

  if (ptr == NULL || len <= 0) return 0.0;

  n = (size_t) len;
  if (n >= sizeof(buf)) n = sizeof(buf) - 1;  /* angka JSON tidak sepanjang ini */
  memcpy(buf, ptr, n);
  buf[n] = '\0';

  return strtod(buf, NULL);
}

static void frozen_cb(void *data, const char *name, size_t name_len,
                      const char *path, const struct json_token *token) {
  struct json_parse_ctx *ctx = (struct json_parse_ctx *) data;
  baik_val_t v = BAIK_UNDEFINED;

  (void) path;

  baik_own(ctx->baik, &v);

  switch (token->type) {
    case JSON_TYPE_STRING: {
      char *dst;
      if (token->len > 0 && (dst = malloc(token->len)) != NULL) {
        int len = json_unescape(token->ptr, token->len, dst, token->len);
        if (len < 0) {
          baik_prepend_errorf(ctx->baik, BAIK_TYPE_ERROR, "GALAT : kesalahan JSON string");
          break;
        }
        v = baik_mk_string(ctx->baik, dst, len, 1);
        free(dst);
      } else {
       
        v = baik_mk_string(ctx->baik, "", 0, 1);
      }
      break;
    }
    case JSON_TYPE_NUMBER:
      v = baik_mk_number(ctx->baik, baik_strtod_n(token->ptr, token->len));
      break;
    case JSON_TYPE_TRUE:
      v = baik_mk_boolean(ctx->baik, 1);
      break;
    case JSON_TYPE_FALSE:
      v = baik_mk_boolean(ctx->baik, 0);
      break;
    case JSON_TYPE_NULL:
      v = BAIK_NULL;
      break;
    case JSON_TYPE_OBJECT_START:
      v = baik_mk_object(ctx->baik);
      break;
    case JSON_TYPE_ARRAY_START:
      v = baik_mk_array(ctx->baik);
      break;

    case JSON_TYPE_OBJECT_END:
    case JSON_TYPE_ARRAY_END: {
     
      ctx->frame = free_json_frame(ctx, ctx->frame);
    } break;

    default:
      LOG(LL_ERROR, ("Wrong token type %d\n", token->type));
      break;
  }

  if (!baik_is_undefined(v)) {
    if (name != NULL && name_len != 0) {
     
      if (baik_is_object(ctx->frame->val)) {
        baik_set(ctx->baik, ctx->frame->val, name, name_len, v);
      } else if (baik_is_array(ctx->frame->val)) {
       
        int idx = (int) baik_strtod_n(name, (int) name_len);
        baik_array_set(ctx->baik, ctx->frame->val, idx, v);
      } else {
        LOG(LL_ERROR, ("Current value is neither object nor array\n"));
      }

    } else {
      assert(ctx->frame == NULL);
      ctx->result = v;
    }

    if (token->type == JSON_TYPE_OBJECT_START ||
        token->type == JSON_TYPE_ARRAY_START) {
     
      struct json_parse_frame *new_frame = alloc_json_frame(ctx, v);
      new_frame->up = ctx->frame;
      ctx->frame = new_frame;
    }
  }

  baik_disown(ctx->baik, &v);
}

BAIK_PRIVATE baik_err_t
baik_json_parse(struct baik *baik, const char *str, size_t len, baik_val_t *res) {
  struct json_parse_ctx *ctx =
      (struct json_parse_ctx *) calloc(sizeof(struct json_parse_ctx), 1);
  int json_res;
  enum baik_err rcode = BAIK_OK;

  ctx->baik = baik;
  ctx->result = BAIK_UNDEFINED;
  ctx->frame = NULL;
  ctx->rcode = BAIK_OK;

  baik_own(baik, &ctx->result);

  {
   
    char *stmp = malloc(len);
    memcpy(stmp, str, len);
    json_res = json_walk(stmp, len, frozen_cb, ctx);
    free(stmp);
    stmp = NULL;
    str = NULL;
  }

  if (ctx->rcode != BAIK_OK) {
    rcode = ctx->rcode;
    baik_prepend_errorf(baik, rcode, "GALAT : kesalahan JSON string");
  } else if (json_res < 0) {
   
    rcode = BAIK_TYPE_ERROR;
    baik_prepend_errorf(baik, rcode, "GALAT : kesalahan JSON string");
  } else {
    *res = ctx->result;
    assert(ctx->frame == NULL);
  }

  if (rcode != BAIK_OK) {
    while (ctx->frame != NULL) {
      ctx->frame = free_json_frame(ctx, ctx->frame);
    }
  }

  baik_disown(baik, &ctx->result);
  free(ctx);

  return rcode;
}

BAIK_PRIVATE void baik_op_json_stringify(struct baik *baik) {
  baik_val_t ret = BAIK_UNDEFINED;
  baik_val_t val = baik_arg(baik, 0);

  if (baik_nargs(baik) < 1) {
    baik_prepend_errorf(baik, BAIK_TYPE_ERROR, "GALAT : kesalahan nilai stringify");
  } else {
    char *p = NULL;
    if (baik_json_stringify(baik, val, NULL, 0, &p) == BAIK_OK) {
      ret = baik_mk_string(baik, p, ~0, 1);
      free(p);
    }
  }

  baik_return(baik, ret);
}

BAIK_PRIVATE void baik_op_json_parse(struct baik *baik) {
  baik_val_t ret = BAIK_UNDEFINED;
  baik_val_t arg0 = baik_arg(baik, 0);

  if (baik_is_string(arg0)) {
    size_t len;
    const char *str = baik_get_string(baik, &arg0, &len);
    baik_json_parse(baik, str, len, &ret);
  } else {
    baik_prepend_errorf(baik, BAIK_TYPE_ERROR, "GALAT : string argumen dibutuhkan");
  }

  baik_return(baik, ret);
}

