/*
 * baik_array.c - Tipe untaian (array): pembuatan, akses, panjang, push, splice.
 *
 * Bagian dari interpreter bahasa BAIK. Deklarasi bersama ada di
 * src/baik/baik_internal.h; antarmuka publik ada di src/baik.h.
 */
#include "baik_internal.h"


#include <stdio.h>

#define SPLICE_NEW_ITEM_IDX 2

static int v_sprintf_s(char *buf, size_t size, const char *fmt, ...) {
  size_t n;
  va_list ap;
  va_start(ap, fmt);
  n = c_vsnprintf(buf, size, fmt, ap);
  if (n > size) {
    return size;
  }
  return n;
}

baik_val_t baik_mk_array(struct baik *baik) {
  baik_val_t ret = baik_mk_object(baik);
  ret &= ~BAIK_TAG_MASK;
  ret |= BAIK_TAG_ARRAY;
  return ret;
}

int baik_is_array(baik_val_t v) {
  return (v & BAIK_TAG_MASK) == BAIK_TAG_ARRAY;
}

baik_val_t baik_array_get(struct baik *baik, baik_val_t arr, unsigned long index) {
  return baik_array_get2(baik, arr, index, NULL);
}

baik_val_t baik_array_get2(struct baik *baik, baik_val_t arr, unsigned long index,
                         int *has) {
  baik_val_t res = BAIK_UNDEFINED;

  if (has != NULL) {
    *has = 0;
  }

  if (baik_is_object(arr)) {
    struct baik_property *p;
    char buf[20];
    int n = v_sprintf_s(buf, sizeof(buf), "%lu", index);
    p = baik_get_own_property(baik, arr, buf, n);
    if (p != NULL) {
      if (has != NULL) {
        *has = 1;
      }
      res = p->value;
    }
  }

  return res;
}

unsigned long baik_array_length(struct baik *baik, baik_val_t v) {
  struct baik_property *p;
  unsigned long len = 0;

  if (!baik_is_object(v)) {
    len = 0;
    goto clean;
  }

  for (p = get_object_struct(v)->properties; p != NULL; p = p->next) {
    int ok = 0;
    unsigned long n = 0;
    str_to_ulong(baik, p->name, &ok, &n);
    if (ok && n >= len && n < 0xffffffff) {
      len = n + 1;
    }
  }

clean:
  return len;
}

baik_err_t baik_array_set(struct baik *baik, baik_val_t arr, unsigned long index,
                        baik_val_t v) {
  baik_err_t ret = BAIK_OK;

  if (baik_is_object(arr)) {
    char buf[20];
    int n = v_sprintf_s(buf, sizeof(buf), "%lu", index);
    ret = baik_set(baik, arr, buf, n, v);
  } else {
    ret = BAIK_TYPE_ERROR;
  }

  return ret;
}

void baik_array_del(struct baik *baik, baik_val_t arr, unsigned long index) {
  char buf[20];
  int n = v_sprintf_s(buf, sizeof(buf), "%lu", index);
  baik_del(baik, arr, buf, n);
}

baik_err_t baik_array_push(struct baik *baik, baik_val_t arr, baik_val_t v) {
  return baik_array_set(baik, arr, baik_array_length(baik, arr), v);
}

BAIK_PRIVATE void baik_array_push_internal(struct baik *baik) {
  baik_err_t rcode = BAIK_OK;
  baik_val_t ret = BAIK_UNDEFINED;
  int nargs = baik_nargs(baik);
  int i;

  if (!baik_check_arg(baik, -1, "this", BAIK_TYPE_OBJECT_ARRAY, NULL)) {
    goto clean;
  }

  for (i = 0; i < nargs; i++) {
    rcode = baik_array_push(baik, baik->vals.this_obj, baik_arg(baik, i));
    if (rcode != BAIK_OK) {
      baik_prepend_errorf(baik, rcode, "");
      goto clean;
    }
  }

  ret = baik_mk_number(baik, baik_array_length(baik, baik->vals.this_obj));

clean:
  baik_return(baik, ret);
  return;
}

static void move_item(struct baik *baik, baik_val_t arr, unsigned long from,
                      unsigned long to) {
  baik_val_t cur = baik_array_get(baik, arr, from);
  baik_array_set(baik, arr, to, cur);
  baik_array_del(baik, arr, from);
}

BAIK_PRIVATE void baik_array_splice(struct baik *baik) {
  int nargs = baik_nargs(baik);
  baik_err_t rcode = BAIK_OK;
  baik_val_t ret = baik_mk_array(baik);
  baik_val_t start_v = BAIK_UNDEFINED;
  baik_val_t deleteCount_v = BAIK_UNDEFINED;
  int start = 0;
  int arr_len;
  int delete_cnt = 0;
  int new_items_cnt = 0;
  int delta = 0;
  int i;

  if (!baik_check_arg(baik, -1, "this", BAIK_TYPE_OBJECT_ARRAY, NULL)) {
    goto clean;
  }

  arr_len = baik_array_length(baik, baik->vals.this_obj);

  if (!baik_check_arg(baik, 0, "start", BAIK_TYPE_NUMBER, &start_v)) {
    goto clean;
  }

  start = baik_normalize_idx(baik_get_int(baik, start_v), arr_len);

  if (nargs >= SPLICE_NEW_ITEM_IDX) {
    if (!baik_check_arg(baik, 1, "deleteCount", BAIK_TYPE_NUMBER,
                       &deleteCount_v)) {
      goto clean;
    }
    delete_cnt = baik_get_int(baik, deleteCount_v);
    new_items_cnt = nargs - SPLICE_NEW_ITEM_IDX;
  } else {
   
    delete_cnt = arr_len - start;
  }
  if (delete_cnt > arr_len - start) {
    delete_cnt = arr_len - start;
  } else if (delete_cnt < 0) {
    delete_cnt = 0;
  }

  delta = new_items_cnt - delete_cnt;

  for (i = 0; i < delete_cnt; i++) {
    baik_val_t cur = baik_array_get(baik, baik->vals.this_obj, start + i);
    rcode = baik_array_push(baik, ret, cur);
    if (rcode != BAIK_OK) {
      baik_prepend_errorf(baik, rcode, "");
      goto clean;
    }
  }

  if (delta < 0) {
    for (i = start; i < arr_len; i++) {
      if (i >= start - delta) {
        move_item(baik, baik->vals.this_obj, i, i + delta);
      } else {
        baik_array_del(baik, baik->vals.this_obj, i);
      }
    }
  } else if (delta > 0) {
    for (i = arr_len - 1; i >= start; i--) {
      move_item(baik, baik->vals.this_obj, i, i + delta);
    }
  }

  for (i = 0; i < nargs - SPLICE_NEW_ITEM_IDX; i++) {
    baik_array_set(baik, baik->vals.this_obj, start + i,
                  baik_arg(baik, SPLICE_NEW_ITEM_IDX + i));
  }

clean:
  baik_return(baik, ret);
}

