/*
 * baik_primitive.c - Tipe primitif: angka, boolean, kosong, takterdefinisi, pointer asing, fungsi.
 *
 * Bagian dari interpreter bahasa BAIK. Deklarasi bersama ada di
 * src/baik/baik_internal.h; antarmuka publik ada di src/baik.h.
 */
#include "baik_internal.h"

baik_val_t baik_mk_null(void) {
  return BAIK_NULL;
}

int baik_is_null(baik_val_t v) {
  return v == BAIK_NULL;
}

baik_val_t baik_mk_undefined(void) {
  return BAIK_UNDEFINED;
}

int baik_is_undefined(baik_val_t v) {
  return v == BAIK_UNDEFINED;
}

baik_val_t baik_mk_number(struct baik *baik, double v) {
  baik_val_t res;
  (void) baik;
 
  if (isnan(v)) {
    res = BAIK_TAG_NAN;
  } else {
    union {
      double d;
      baik_val_t r;
    } u;
    u.d = v;
    res = u.r;
  }
  return res;
}

static double get_double(baik_val_t v) {
  union {
    double d;
    baik_val_t v;
  } u;
  u.v = v;
 
  return u.d;
}

double baik_get_double(struct baik *baik, baik_val_t v) {
  (void) baik;
  return get_double(v);
}

int baik_get_int(struct baik *baik, baik_val_t v) {
  (void) baik;
 
  return (int) (unsigned int) get_double(v);
}

int32_t baik_get_int32(struct baik *baik, baik_val_t v) {
  (void) baik;
  return (int32_t) get_double(v);
}

int baik_is_number(baik_val_t v) {
  return v == BAIK_TAG_NAN || !isnan(get_double(v));
}

baik_val_t baik_mk_boolean(struct baik *baik, int v) {
  (void) baik;
  return (v ? 1 : 0) | BAIK_TAG_BOOLEAN;
}

int baik_get_bool(struct baik *baik, baik_val_t v) {
  (void) baik;
  if (baik_is_boolean(v)) {
    return v & 1;
  } else {
    return 0;
  }
}

int baik_is_boolean(baik_val_t v) {
  return (v & BAIK_TAG_MASK) == BAIK_TAG_BOOLEAN;
}

#define BAIK_IS_POINTER_LEGIT(n) \
  (((n) &BAIK_TAG_MASK) == 0 || ((n) &BAIK_TAG_MASK) == (~0 & BAIK_TAG_MASK))

BAIK_PRIVATE baik_val_t baik_pointer_to_value(struct baik *baik, void *p) {
  uint64_t n = ((uint64_t)(uintptr_t) p);

  if (!BAIK_IS_POINTER_LEGIT(n)) {
    baik_prepend_errorf(baik, BAIK_TYPE_ERROR, "GALAT : kesalahan nilai pointer: %p", p);
  }
  return n & ~BAIK_TAG_MASK;
}

BAIK_PRIVATE baik_val_t baik_legit_pointer_to_value(void *p) {
  uint64_t n = ((uint64_t)(uintptr_t) p);

  assert(BAIK_IS_POINTER_LEGIT(n));
  return n & ~BAIK_TAG_MASK;
}

BAIK_PRIVATE void *get_ptr(baik_val_t v) {
  return (void *) (uintptr_t)(v & 0xFFFFFFFFFFFFUL);
}

void *baik_get_ptr(struct baik *baik, baik_val_t v) {
  (void) baik;
  if (!baik_is_foreign(v)) {
    return NULL;
  }
  return get_ptr(v);
}

baik_val_t baik_mk_foreign(struct baik *baik, void *p) {
  (void) baik;
  return baik_pointer_to_value(baik, p) | BAIK_TAG_FOREIGN;
}

baik_val_t baik_mk_foreign_func(struct baik *baik, baik_func_ptr_t fn) {
  union {
    baik_func_ptr_t fn;
    void *p;
  } u;
  u.fn = fn;
  (void) baik;
  return baik_pointer_to_value(baik, u.p) | BAIK_TAG_FOREIGN;
}

int baik_is_foreign(baik_val_t v) {
  return (v & BAIK_TAG_MASK) == BAIK_TAG_FOREIGN;
}

baik_val_t baik_mk_function(struct baik *baik, size_t off) {
  (void) baik;
  return (baik_val_t) off | BAIK_TAG_FUNCTION;
}

int baik_is_function(baik_val_t v) {
  return (v & BAIK_TAG_MASK) == BAIK_TAG_FUNCTION;
}

BAIK_PRIVATE void baik_op_isnan(struct baik *baik) {
  baik_val_t ret = BAIK_UNDEFINED;
  baik_val_t val = baik_arg(baik, 0);

  ret = baik_mk_boolean(baik, val == BAIK_TAG_NAN);

  baik_return(baik, ret);
}

/* typedef Rune dipindah ke baik_internal.h (dipakai baik_string.c). */
