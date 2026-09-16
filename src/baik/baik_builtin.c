/*
 * baik_builtin.c - Tabel fungsi bawaan bahasa (baik_init_builtin) beserta
 * implementasi tulis, muat, getBAIK, chr, gc, dan s2o. Nama bawaan lain yang
 * didaftarkan di sini diimplementasikan di modul lain: mkstr (baik_string.c),
 * die (baik_core.c), isNaN (baik_primitive.c), JSON (baik_json.c),
 * Object.create (baik_object.c).
 *
 * Bagian dari interpreter bahasa BAIK. Deklarasi bersama ada di
 * src/baik/baik_internal.h; antarmuka publik ada di src/baik.h.
 */
#include "baik_internal.h"

static void baik_print(struct baik *baik) {
  size_t i, num_args = baik_nargs(baik);
  for (i = 0; i < num_args; i++) {
    baik_fprintf(baik_arg(baik, i), baik, stdout);
    putchar(' ');
  }
  putchar('\n');
  baik_return(baik, BAIK_UNDEFINED);
}

static struct baik_bcode_part *baik_get_loaded_file_bcode(struct baik *baik,
                                                        const char *filename) {
  int parts_cnt = baik_bcode_parts_cnt(baik);
  int i;

  if (filename == NULL) {
    return 0;
  }

  for (i = 0; i < parts_cnt; i++) {
    struct baik_bcode_part *bp = baik_bcode_part_get(baik, i);
    const char *cur_fn = baik_get_bcode_filename(baik, bp);
    if (strcmp(filename, cur_fn) == 0) {
      return bp;
    }
  }
  return NULL;
}

static void baik_load(struct baik *baik) {
  baik_val_t res = BAIK_UNDEFINED;
  baik_val_t arg0 = baik_arg(baik, 0);
  baik_val_t arg1 = baik_arg(baik, 1);
  int custom_global = 0;

  if (baik_is_string(arg0)) {
    const char *path = baik_get_cstring(baik, &arg0);
    struct baik_bcode_part *bp = NULL;
    baik_err_t ret;

    if (baik_is_object(arg1)) {
      custom_global = 1;
      push_baik_val(&baik->scopes, arg1);
    }
    bp = baik_get_loaded_file_bcode(baik, path);
    if (bp == NULL) {
     
      ret = baik_exec_file(baik, path, &res);
    } else {
     
      if (bp->exec_res != BAIK_OK || custom_global) {
        ret = baik_execute(baik, bp->start_idx, &res);
      } else {
        ret = BAIK_OK;
      }
    }
    if (ret != BAIK_OK) {
     
      arg0 = baik_arg(baik, 0);
      path = baik_get_cstring(baik, &arg0);
      baik_prepend_errorf(baik, ret, "galat : eksekusi file \"%s\"", path);
      goto clean;
    }

  clean:
    if (custom_global) {
      baik_pop_val(&baik->scopes);
    }
  }
  baik_return(baik, res);
}

static void baik_get_baik(struct baik *baik) {
  baik_return(baik, baik_mk_foreign(baik, baik));
}

static void baik_chr(struct baik *baik) {
  baik_val_t arg0 = baik_arg(baik, 0), res = BAIK_NULL;
  int n = baik_get_int(baik, arg0);
  if (baik_is_number(arg0) && n >= 0 && n <= 255) {
    uint8_t s = n;
    res = baik_mk_string(baik, (const char *) &s, sizeof(s), 1);
  }
  baik_return(baik, res);
}

static void baik_do_gc(struct baik *baik) {
  baik_val_t arg0 = baik_arg(baik, 0);
  baik_gc(baik, baik_is_boolean(arg0) ? baik_get_bool(baik, arg0) : 0);
  baik_return(baik, arg0);
}

static void baik_s2o(struct baik *baik) {
  baik_return(baik,
             baik_struct_to_obj(baik, baik_get_ptr(baik, baik_arg(baik, 0)),
                               (const struct baik_c_struct_member *) baik_get_ptr(
                                   baik, baik_arg(baik, 1))));
}

void baik_init_builtin(struct baik *baik, baik_val_t obj) {
  baik_val_t v;

  baik_set(baik, obj, "global", ~0, obj);
  baik_set(baik, obj, "muat", ~0,
          baik_mk_foreign_func(baik, (baik_func_ptr_t) baik_load));
  baik_set(baik, obj, "tulis", ~0,
          baik_mk_foreign_func(baik, (baik_func_ptr_t) baik_print));
  // baik_set(baik, obj, "ffi", ~0,
  //         baik_mk_foreign_func(baik, (baik_func_ptr_t) baik_ffi_call));
  // baik_set(baik, obj, "ffi_cb_free", ~0,
  //         baik_mk_foreign_func(baik, (baik_func_ptr_t) baik_ffi_cb_free));
  baik_set(baik, obj, "mkstr", ~0,
          baik_mk_foreign_func(baik, (baik_func_ptr_t) baik_mkstr));
  baik_set(baik, obj, "getBAIK", ~0,
          baik_mk_foreign_func(baik, (baik_func_ptr_t) baik_get_baik));
  baik_set(baik, obj, "die", ~0,
          baik_mk_foreign_func(baik, (baik_func_ptr_t) baik_die));
  baik_set(baik, obj, "gc", ~0,
          baik_mk_foreign_func(baik, (baik_func_ptr_t) baik_do_gc));
  baik_set(baik, obj, "chr", ~0,
          baik_mk_foreign_func(baik, (baik_func_ptr_t) baik_chr));
  baik_set(baik, obj, "s2o", ~0,
          baik_mk_foreign_func(baik, (baik_func_ptr_t) baik_s2o));
  v = baik_mk_object(baik);
  baik_set(baik, v, "stringify", ~0,
          baik_mk_foreign_func(baik, (baik_func_ptr_t) baik_op_json_stringify));
  baik_set(baik, v, "parse", ~0,
          baik_mk_foreign_func(baik, (baik_func_ptr_t) baik_op_json_parse));
  baik_set(baik, obj, "JSON", ~0, v);
  v = baik_mk_object(baik);
  baik_set(baik, v, "create", ~0,
          baik_mk_foreign_func(baik, (baik_func_ptr_t) baik_op_create_object));
  baik_set(baik, obj, "Object", ~0, v);
  baik_set(baik, obj, "NaN", ~0, BAIK_TAG_NAN);
  baik_set(baik, obj, "isNaN", ~0,
          baik_mk_foreign_func(baik, (baik_func_ptr_t) baik_op_isnan));
}

