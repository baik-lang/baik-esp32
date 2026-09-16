/*
 * baik_conversion.c - Konversi antar tipe: ke string, ke boolean, uji kebenaran nilai.
 *
 * Bagian dari interpreter bahasa BAIK. Deklarasi bersama ada di
 * src/baik/baik_internal.h; antarmuka publik ada di src/baik.h.
 */
#include "baik_internal.h"

BAIK_PRIVATE baik_err_t baik_to_string(struct baik *baik, baik_val_t *v, char **p,
                                    size_t *sizep, int *need_free) {
  baik_err_t ret = BAIK_OK;

  *p = NULL;
  *sizep = 0;
  *need_free = 0;

  if (baik_is_string(*v)) {
    *p = (char *) baik_get_string(baik, v, sizep);
  } else if (baik_is_number(*v)) {
    char buf[50] = "";
    struct json_out out = JSON_OUT_BUF(buf, sizeof(buf));
    baik_jprintf(*v, baik, &out);
    *sizep = strlen(buf);
    *p = malloc(*sizep + 1);
    if (*p == NULL) {
      ret = BAIK_OUT_OF_MEMORY;
      goto clean;
    }
    memmove(*p, buf, *sizep + 1);
    *need_free = 1;
  } else if (baik_is_boolean(*v)) {
    /* Panjang dihitung dari literalnya sendiri, jangan ditulis tangan.
     * Sebelumnya angka-angka ini konstan dan tidak cocok dengan panjang
     * kata yang sebenarnya, sehingga JSON.stringify(benar) menghasilkan
     * "bena", kosong -> "koso", dan takterdefinisi -> "takterdef". */
    if (baik_get_bool(baik, *v)) {
      *p = "benar";
      *sizep = sizeof("benar") - 1;
    } else {
      *p = "salah";
      *sizep = sizeof("salah") - 1;
    }
  } else if (baik_is_undefined(*v)) {
    *p = "takterdefinisi";
    *sizep = sizeof("takterdefinisi") - 1;
  } else if (baik_is_null(*v)) {
    *p = "kosong";
    *sizep = sizeof("kosong") - 1;
  } else if (baik_is_object(*v)) {
    ret = BAIK_TYPE_ERROR;
    baik_set_errorf(baik, ret,
                   "conversion from object to string is not supported");
  } else if (baik_is_foreign(*v)) {
    *p = "TODO_foreign";
    *sizep = 12;
  } else {
    ret = BAIK_TYPE_ERROR;
    baik_set_errorf(baik, ret, "unknown type to convert to string");
  }

clean:
  return ret;
}

BAIK_PRIVATE baik_val_t baik_to_boolean_v(struct baik *baik, baik_val_t v) {
  size_t len;
  int is_truthy;

  is_truthy =
      ((baik_is_boolean(v) && baik_get_bool(baik, v)) ||
       (baik_is_number(v) && baik_get_double(baik, v) != 0.0) ||
       (baik_is_string(v) && baik_get_string(baik, &v, &len) && len > 0) ||
       (baik_is_function(v)) || (baik_is_foreign(v)) || (baik_is_object(v))) &&
      v != BAIK_TAG_NAN;

  return baik_mk_boolean(baik, is_truthy);
}

BAIK_PRIVATE int baik_is_truthy(struct baik *baik, baik_val_t v) {
  return baik_get_bool(baik, baik_to_boolean_v(baik, v));
}

/* Ukuran arena dipindah ke baik_internal.h (dipakai baik_core.c). */

