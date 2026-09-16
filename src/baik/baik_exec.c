/*
 * baik_exec.c - Mesin virtual: operator aritmetika dan logika, evaluasi
 * ekspresi, eksekusi bytecode, serta baik_exec/baik_call/baik_apply.
 * CATATAN: di ekor berkas masih tertinggal sisa FFI (RTLD_DEFAULT,
 * struct cbdata, dan parse_cval_type yang dikomentari) yang semestinya
 * berada di baik_ffi.c.
 *
 * Bagian dari interpreter bahasa BAIK. Deklarasi bersama ada di
 * src/baik/baik_internal.h; antarmuka publik ada di src/baik.h.
 */
#include "baik_internal.h"

static double do_arith_op(double da, double db, int op, bool *resnan) {
  *resnan = false;

  if (isnan(da) || isnan(db)) {
    *resnan = true;
    return 0;
  }
 
  switch (op) {
    case TOK_MINUS:   return da - db;
    case TOK_PLUS:    return da + db;
    case TOK_MUL:     return da * db;
    case TOK_DIV:
      if (db != 0) {
        return da / db;
      } else {
       
        *resnan = true;
        return 0;
      }
    case TOK_REM:
     
      db = (int) db;
      if (db != 0) {
        bool neg = false;
        if (da < 0) {
          neg = true;
          da = -da;
        }
        if (db < 0) {
          db = -db;
        }
        da = (double) ((int64_t) da % (int64_t) db);
        if (neg) {
          da = -da;
        }
        return da;
      } else {
        *resnan = true;
        return 0;
      }
    case TOK_AND:     return (double) ((int64_t) da & (int64_t) db);
    case TOK_OR:      return (double) ((int64_t) da | (int64_t) db);
    case TOK_XOR:     return (double) ((int64_t) da ^ (int64_t) db);
    case TOK_LSHIFT:  return (double) ((int64_t) da << (int64_t) db);
    case TOK_RSHIFT:  return (double) ((int64_t) da >> (int64_t) db);
    case TOK_URSHIFT: return (double) ((uint32_t) da >> (uint32_t) db);
  }
 
  *resnan = true;
  return 0;
}

static void set_no_autoconversion_error(struct baik *baik) {
  baik_prepend_errorf(baik, BAIK_TYPE_ERROR,
                     "galat : konversi tipe implisit dilarang");
}

static baik_val_t do_op(struct baik *baik, baik_val_t a, baik_val_t b, int op) {
  baik_val_t ret = BAIK_UNDEFINED;
  bool resnan = false;
  if ((baik_is_foreign(a) || baik_is_number(a)) &&
      (baik_is_foreign(b) || baik_is_number(b))) {
    int is_result_ptr = 0;
    double da, db, result;

    if (baik_is_foreign(a) && baik_is_foreign(b)) {
      if (op != TOK_MINUS) {
        baik_prepend_errorf(baik, BAIK_TYPE_ERROR, "GALAT : operan tidak valid");
      }
    } else if (baik_is_foreign(a) || baik_is_foreign(b)) {
      if (op != TOK_MINUS && op != TOK_PLUS) {
        baik_prepend_errorf(baik, BAIK_TYPE_ERROR, "GALAT : operan tidak valid");
      }
      is_result_ptr = 1;
    }
    da = baik_is_number(a) ? baik_get_double(baik, a)
                          : (double) (uintptr_t) baik_get_ptr(baik, a);
    db = baik_is_number(b) ? baik_get_double(baik, b)
                          : (double) (uintptr_t) baik_get_ptr(baik, b);
    result = do_arith_op(da, db, op, &resnan);
    if (resnan) {
      ret = BAIK_TAG_NAN;
    } else {
     
      ret = is_result_ptr ? baik_mk_foreign(baik, (void *) (uintptr_t) result)
                          : baik_mk_number(baik, result);
    }
  } else if (baik_is_string(a) && baik_is_string(b) && (op == TOK_PLUS)) {
    ret = s_concat(baik, a, b);
  } else {
    set_no_autoconversion_error(baik);
  }
  return ret;
}

static void op_assign(struct baik *baik, int op) {
  baik_val_t val = baik_pop(baik);
  baik_val_t obj = baik_pop(baik);
  baik_val_t key = baik_pop(baik);
  if (baik_is_object(obj) && baik_is_string(key)) {
    baik_val_t v = baik_get_v(baik, obj, key);
    baik_set_v(baik, obj, key, do_op(baik, v, val, op));
    baik_push(baik, v);
  } else {
    baik_set_errorf(baik, BAIK_TYPE_ERROR, "invalid operand");
  }
}

static int check_equal(struct baik *baik, baik_val_t a, baik_val_t b) {
  int ret = 0;
  if (a == BAIK_TAG_NAN && b == BAIK_TAG_NAN) {
    ret = 0;
  } else if (a == b) {
    ret = 1;
  } else if (baik_is_number(a) && baik_is_number(b)) {
    ret = 0;
  } else if (baik_is_string(a) && baik_is_string(b)) {
    ret = s_cmp(baik, a, b) == 0;
  } else if (baik_is_foreign(a) && b == BAIK_NULL) {
    ret = baik_get_ptr(baik, a) == NULL;
  } else if (a == BAIK_NULL && baik_is_foreign(b)) {
    ret = baik_get_ptr(baik, b) == NULL;
  } else {
    ret = 0;
  }
  return ret;
}

static void exec_expr(struct baik *baik, int op) {
  switch (op) {
    case TOK_DOT:
      break;
    case TOK_MINUS:
    case TOK_PLUS:
    case TOK_MUL:
    case TOK_DIV:
    case TOK_REM:
    case TOK_XOR:
    case TOK_AND:
    case TOK_OR:
    case TOK_LSHIFT:
    case TOK_RSHIFT:
    case TOK_URSHIFT: {
      baik_val_t b = baik_pop(baik);
      baik_val_t a = baik_pop(baik);
      baik_push(baik, do_op(baik, a, b, op));
      break;
    }
    case TOK_UNARY_MINUS: {
      double a = baik_get_double(baik, baik_pop(baik));
      baik_push(baik, baik_mk_number(baik, -a));
      break;
    }
    case TOK_NOT: {
      baik_val_t val = baik_pop(baik);
      baik_push(baik, baik_mk_boolean(baik, !baik_is_truthy(baik, val)));
      break;
    }
    case TOK_TILDA: {
      double a = baik_get_double(baik, baik_pop(baik));
      baik_push(baik, baik_mk_number(baik, (double) (~(int64_t) a)));
      break;
    }
    case TOK_UNARY_PLUS:
      break;
    case TOK_EQ:
      baik_set_errorf(baik, BAIK_NOT_IMPLEMENTED_ERROR, "Use ===, not ==");
      break;
    case TOK_NE:
      baik_set_errorf(baik, BAIK_NOT_IMPLEMENTED_ERROR, "Use !==, not !=");
      break;
    case TOK_EQ_EQ: {
      baik_val_t a = baik_pop(baik);
      baik_val_t b = baik_pop(baik);
      baik_push(baik, baik_mk_boolean(baik, check_equal(baik, a, b)));
      break;
    }
    case TOK_NE_NE: {
      baik_val_t a = baik_pop(baik);
      baik_val_t b = baik_pop(baik);
      baik_push(baik, baik_mk_boolean(baik, !check_equal(baik, a, b)));
      break;
    }
    case TOK_LT: {
      double b = baik_get_double(baik, baik_pop(baik));
      double a = baik_get_double(baik, baik_pop(baik));
      baik_push(baik, baik_mk_boolean(baik, a < b));
      break;
    }
    case TOK_GT: {
      double b = baik_get_double(baik, baik_pop(baik));
      double a = baik_get_double(baik, baik_pop(baik));
      baik_push(baik, baik_mk_boolean(baik, a > b));
      break;
    }
    case TOK_LE: {
      double b = baik_get_double(baik, baik_pop(baik));
      double a = baik_get_double(baik, baik_pop(baik));
      baik_push(baik, baik_mk_boolean(baik, a <= b));
      break;
    }
    case TOK_GE: {
      double b = baik_get_double(baik, baik_pop(baik));
      double a = baik_get_double(baik, baik_pop(baik));
      baik_push(baik, baik_mk_boolean(baik, a >= b));
      break;
    }
    case TOK_ASSIGN: {
      baik_val_t val = baik_pop(baik);
      baik_val_t obj = baik_pop(baik);
      baik_val_t key = baik_pop(baik);
      if (baik_is_object(obj)) {
        baik_set_v(baik, obj, key, val);
      } else if (baik_is_foreign(obj)) {
       

        int ikey = baik_get_int(baik, key);
        int ival = baik_get_int(baik, val);

        if (!baik_is_number(key)) {
          baik_prepend_errorf(baik, BAIK_TYPE_ERROR, "GALAT : index harus angka");
          val = BAIK_UNDEFINED;
        } else if (!baik_is_number(val) || ival < 0 || ival > 0xff) {
          baik_prepend_errorf(baik, BAIK_TYPE_ERROR,
                             "GALAT : hanya angka 0 .. 255 yang bisa digunakan");
          val = BAIK_UNDEFINED;
        } else {
          uint8_t *ptr = (uint8_t *) baik_get_ptr(baik, obj);
          *(ptr + ikey) = (uint8_t) ival;
        }
      } else {
        baik_prepend_errorf(baik, BAIK_TYPE_ERROR, "GALAT : tipe objek tidak didukung");
      }
      baik_push(baik, val);
      break;
    }
    case TOK_POSTFIX_PLUS: {
      baik_val_t obj = baik_pop(baik);
      baik_val_t key = baik_pop(baik);
      if (baik_is_object(obj) && baik_is_string(key)) {
        baik_val_t v = baik_get_v(baik, obj, key);
        baik_val_t v1 = do_op(baik, v, baik_mk_number(baik, 1), TOK_PLUS);
        baik_set_v(baik, obj, key, v1);
        baik_push(baik, v);
      } else {
        baik_set_errorf(baik, BAIK_TYPE_ERROR, "invalid operand for ++");
      }
      break;
    }
    case TOK_POSTFIX_MINUS: {
      baik_val_t obj = baik_pop(baik);
      baik_val_t key = baik_pop(baik);
      if (baik_is_object(obj) && baik_is_string(key)) {
        baik_val_t v = baik_get_v(baik, obj, key);
        baik_val_t v1 = do_op(baik, v, baik_mk_number(baik, 1), TOK_MINUS);
        baik_set_v(baik, obj, key, v1);
        baik_push(baik, v);
      } else {
        baik_set_errorf(baik, BAIK_TYPE_ERROR, "invalid operand for --");
      }
      break;
    }
    case TOK_MINUS_MINUS: {
      baik_val_t obj = baik_pop(baik);
      baik_val_t key = baik_pop(baik);
      if (baik_is_object(obj) && baik_is_string(key)) {
        baik_val_t v = baik_get_v(baik, obj, key);
        v = do_op(baik, v, baik_mk_number(baik, 1), TOK_MINUS);
        baik_set_v(baik, obj, key, v);
        baik_push(baik, v);
      } else {
        baik_set_errorf(baik, BAIK_TYPE_ERROR, "invalid operand for --");
      }
      break;
    }
    case TOK_PLUS_PLUS: {
      baik_val_t obj = baik_pop(baik);
      baik_val_t key = baik_pop(baik);
      if (baik_is_object(obj) && baik_is_string(key)) {
        baik_val_t v = baik_get_v(baik, obj, key);
        v = do_op(baik, v, baik_mk_number(baik, 1), TOK_PLUS);
        baik_set_v(baik, obj, key, v);
        baik_push(baik, v);
      } else {
        baik_set_errorf(baik, BAIK_TYPE_ERROR, "invalid operand for ++");
      }
      break;
    }
   

   
    case TOK_MINUS_ASSIGN:    op_assign(baik, TOK_MINUS);    break;
    case TOK_PLUS_ASSIGN:     op_assign(baik, TOK_PLUS);     break;
    case TOK_MUL_ASSIGN:      op_assign(baik, TOK_MUL);      break;
    case TOK_DIV_ASSIGN:      op_assign(baik, TOK_DIV);      break;
    case TOK_REM_ASSIGN:      op_assign(baik, TOK_REM);      break;
    case TOK_AND_ASSIGN:      op_assign(baik, TOK_AND);      break;
    case TOK_OR_ASSIGN:       op_assign(baik, TOK_OR);       break;
    case TOK_XOR_ASSIGN:      op_assign(baik, TOK_XOR);      break;
    case TOK_LSHIFT_ASSIGN:   op_assign(baik, TOK_LSHIFT);   break;
    case TOK_RSHIFT_ASSIGN:   op_assign(baik, TOK_RSHIFT);   break;
    case TOK_URSHIFT_ASSIGN:  op_assign(baik, TOK_URSHIFT);  break;
    case TOK_COMMA: break;
   
    case TOK_KEYWORD_TIPE:
      baik_push(baik, baik_mk_string(baik, baik_typeof(baik_pop(baik)), ~0, 1));
      break;
    default:
      LOG(LL_ERROR, ("Unknown expr: %d", op));
      break;
  }
}

static int getprop_builtin_string(struct baik *baik, baik_val_t val,
                                  const char *name, size_t name_len,
                                  baik_val_t *res) {
  int isnum = 0;
  int idx = cstr_to_ulong(name, name_len, &isnum);

  if (strcmp(name, "panjang") == 0) {
    size_t val_len;
    baik_get_string(baik, &val, &val_len);
    *res = baik_mk_number(baik, (double) val_len);
    return 1;
  } else if (strcmp(name, "at") == 0 || strcmp(name, "charCodeAt") == 0) {
    *res = baik_mk_foreign_func(baik, (baik_func_ptr_t) baik_string_char_code_at);
    return 1;
  } else if (strcmp(name, "indexOf") == 0) {
    *res = baik_mk_foreign_func(baik, (baik_func_ptr_t) baik_string_index_of);
    return 1;
  } else if (strcmp(name, "slice") == 0) {
    *res = baik_mk_foreign_func(baik, (baik_func_ptr_t) baik_string_slice);
    return 1;
  } else if (isnum) {
   
    size_t val_len;
    const char *str = baik_get_string(baik, &val, &val_len);
    if (idx >= 0 && idx < (int) val_len) {
      *res = baik_mk_string(baik, str + idx, 1, 1);
    } else {
      *res = BAIK_UNDEFINED;
    }
    return 1;
  }
  return 0;
}

static int getprop_builtin_array(struct baik *baik, baik_val_t val,
                                 const char *name, size_t name_len,
                                 baik_val_t *res) {
  if (strcmp(name, "splice") == 0) {
    *res = baik_mk_foreign_func(baik, (baik_func_ptr_t) baik_array_splice);
    return 1;
  } else if (strcmp(name, "push") == 0) {
    *res = baik_mk_foreign_func(baik, (baik_func_ptr_t) baik_array_push_internal);
    return 1;
  } else if (strcmp(name, "panjang") == 0) {
    *res = baik_mk_number(baik, baik_array_length(baik, val));
    return 1;
  }

  (void) name_len;
  return 0;
}

static int getprop_builtin_foreign(struct baik *baik, baik_val_t val,
                                   const char *name, size_t name_len,
                                   baik_val_t *res) {
  int isnum = 0;
  int idx = cstr_to_ulong(name, name_len, &isnum);

  if (!isnum) {
    baik_prepend_errorf(baik, BAIK_TYPE_ERROR, "GALAT : index harus angka");
  } else {
    uint8_t *ptr = (uint8_t *) baik_get_ptr(baik, val);
    *res = baik_mk_number(baik, *(ptr + idx));
  }
  return 1;
}

static void baik_apply_(struct baik *baik) {
  baik_val_t res = BAIK_UNDEFINED, *args = NULL;
  baik_val_t func = baik->vals.this_obj, v = baik_arg(baik, 1);
  int i, nargs = 0;
  if (baik_is_array(v)) {
    nargs = baik_array_length(baik, v);
    args = calloc(nargs, sizeof(args[0]));
    for (i = 0; i < nargs; i++) args[i] = baik_array_get(baik, v, i);
  }
  baik_apply(baik, &res, func, baik_arg(baik, 0), nargs, args);
  free(args);
  baik_return(baik, res);
}

static int getprop_builtin(struct baik *baik, baik_val_t val, baik_val_t name,
                           baik_val_t *res) {
  size_t n;
  char *s = NULL;
  int need_free = 0;
  int handled = 0;

  baik_err_t err = baik_to_string(baik, &name, &s, &n, &need_free);

  if (err == BAIK_OK) {
    if (baik_is_string(val)) {
      handled = getprop_builtin_string(baik, val, s, n, res);
    } else if (s != NULL && n == 5 && strncmp(s, "apply", n) == 0) {
      *res = baik_mk_foreign_func(baik, (baik_func_ptr_t) baik_apply_);
      handled = 1;
    } else if (baik_is_array(val)) {
      handled = getprop_builtin_array(baik, val, s, n, res);
    } else if (baik_is_foreign(val)) {
      handled = getprop_builtin_foreign(baik, val, s, n, res);
    }
  }

  if (need_free) {
    free(s);
    s = NULL;
  }

  return handled;
}

BAIK_PRIVATE baik_err_t baik_execute(struct baik *baik, size_t off, baik_val_t *res) {
  size_t i;
  uint8_t prev_opcode = OP_MAX;
  uint8_t opcode = OP_MAX;

 
  int stack_len = baik->stack.len;
  int call_stack_len = baik->call_stack.len;
  int arg_stack_len = baik->arg_stack.len;
  int scopes_len = baik->scopes.len;
  int loop_addresses_len = baik->loop_addresses.len;
  size_t start_off = off;
  const uint8_t *code;

  struct baik_bcode_part bp = *baik_bcode_part_get_by_offset(baik, off);

  baik_set_errorf(baik, BAIK_OK, NULL);
  free(baik->stack_trace);
  baik->stack_trace = NULL;

  off -= bp.start_idx;

  for (i = off; i < bp.data.len; i++) {
    baik->cur_bcode_offset = i;

    if (baik->need_gc) {
      if (maybe_gc(baik)) {
        baik->need_gc = 0;
      }
    }
#if BAIK_AGGRESSIVE_GC
    maybe_gc(baik);
#endif

    code = (const uint8_t *) bp.data.p;
    baik_disasm_single(code, i);
    prev_opcode = opcode;
    opcode = code[i];
    switch (opcode) {
      case OP_BCODE_HEADER: {
        baik_header_item_t bcode_offset;
        memcpy(&bcode_offset,
               code + i + 1 +
                   sizeof(baik_header_item_t) * BAIK_HDR_ITEM_BCODE_OFFSET,
               sizeof(bcode_offset));
        i += bcode_offset;
      } break;
      case OP_PUSH_NULL:
        baik_push(baik, baik_mk_null());
        break;
      case OP_PUSH_UNDEF:
        baik_push(baik, baik_mk_undefined());
        break;
      case OP_PUSH_FALSE:
        baik_push(baik, baik_mk_boolean(baik, 0));
        break;
      case OP_PUSH_TRUE:
        baik_push(baik, baik_mk_boolean(baik, 1));
        break;
      case OP_PUSH_OBJ:
        baik_push(baik, baik_mk_object(baik));
        break;
      case OP_PUSH_ARRAY:
        baik_push(baik, baik_mk_array(baik));
        break;
      case OP_PUSH_FUNC: {
        int llen, n = BAIK_EM_varint_decode_unsafe(&code[i + 1], &llen);
        baik_push(baik, baik_mk_function(baik, bp.start_idx + i - n));
        i += llen;
        break;
      }
      case OP_PUSH_THIS:
        baik_push(baik, baik->vals.this_obj);
        break;
      case OP_JMP: {
        int llen, n = BAIK_EM_varint_decode_unsafe(&code[i + 1], &llen);
        i += n + llen;
        break;
      }
      case OP_JMP_FALSE: {
        int llen, n = BAIK_EM_varint_decode_unsafe(&code[i + 1], &llen);
        i += llen;
        if (!baik_is_truthy(baik, baik_pop(baik))) {
          baik_push(baik, BAIK_UNDEFINED);
          i += n;
        }
        break;
      }
     
      case OP_JMP_NEUTRAL_TRUE: {
        int llen, n = BAIK_EM_varint_decode_unsafe(&code[i + 1], &llen);
        i += llen;
        if (baik_is_truthy(baik, vtop(&baik->stack))) {
          i += n;
        }
        break;
      }
      case OP_JMP_NEUTRAL_FALSE: {
        int llen, n = BAIK_EM_varint_decode_unsafe(&code[i + 1], &llen);
        i += llen;
        if (!baik_is_truthy(baik, vtop(&baik->stack))) {
          i += n;
        }
        break;
      }
      case OP_FIND_SCOPE: {
        baik_val_t key = vtop(&baik->stack);
        baik_push(baik, baik_find_scope(baik, key));
        break;
      }
      case OP_CREATE: {
        baik_val_t obj = baik_pop(baik);
        baik_val_t key = baik_pop(baik);
        if (baik_get_own_property_v(baik, obj, key) == NULL) {
          baik_set_v(baik, obj, key, BAIK_UNDEFINED);
        }
        break;
      }
      case OP_APPEND: {
        baik_val_t val = baik_pop(baik);
        baik_val_t arr = baik_pop(baik);
        baik_err_t err = baik_array_push(baik, arr, val);
        if (err != BAIK_OK) {
          baik_set_errorf(baik, BAIK_TYPE_ERROR, "append to non-array");
        }
        break;
      }
      case OP_GET: {
        baik_val_t obj = baik_pop(baik);
        baik_val_t key = baik_pop(baik);
        baik_val_t val = BAIK_UNDEFINED;

        if (!getprop_builtin(baik, obj, key, &val)) {
          if (baik_is_object(obj)) {
            val = baik_get_v_proto(baik, obj, key);
          } else {
            baik_prepend_errorf(baik, BAIK_TYPE_ERROR, "GALAT : tipe galat");
          }
        }

        baik_push(baik, val);
        if (prev_opcode != OP_FIND_SCOPE) {
         
          baik->vals.last_getprop_obj = obj;
        } else {
         
          baik->vals.last_getprop_obj = BAIK_UNDEFINED;
        }
        break;
      }
      case OP_DEL_SCOPE:
        if (baik->scopes.len <= 1) {
          baik_set_errorf(baik, BAIK_INTERNAL_ERROR, "scopes underflow");
        } else {
          baik_pop_val(&baik->scopes);
        }
        break;
      case OP_NEW_SCOPE:
        push_baik_val(&baik->scopes, baik_mk_object(baik));
        break;
      case OP_PUSH_SCOPE:
        assert(baik_stack_size(&baik->scopes) > 0);
        baik_push(baik, vtop(&baik->scopes));
        break;
      case OP_PUSH_STR: {
        int llen, n = BAIK_EM_varint_decode_unsafe(&code[i + 1], &llen);
        baik_push(baik, baik_mk_string(baik, (char *) code + i + 1 + llen, n, 1));
        i += llen + n;
        break;
      }
      case OP_PUSH_INT: {
        int llen;
        int64_t n = BAIK_EM_varint_decode_unsafe(&code[i + 1], &llen);
        baik_push(baik, baik_mk_number(baik, (double) n));
        i += llen;
        break;
      }
      case OP_PUSH_DBL: {
        int llen, n = BAIK_EM_varint_decode_unsafe(&code[i + 1], &llen);
        baik_push(baik, baik_mk_number(
                          baik, strtod((char *) code + i + 1 + llen, NULL)));
        i += llen + n;
        break;
      }
      case OP_FOR_IN_NEXT: {
       
        baik_val_t *iterator = vptr(&baik->stack, -1);
        baik_val_t obj = *vptr(&baik->stack, -2);
        if (baik_is_object(obj)) {
          baik_val_t var_name = *vptr(&baik->stack, -3);
          baik_val_t key = baik_next(baik, obj, iterator);
          if (key != BAIK_UNDEFINED) {
            baik_val_t scope = baik_find_scope(baik, var_name);
            baik_set_v(baik, scope, var_name, key);
          }
        } else {
          baik_set_errorf(baik, BAIK_TYPE_ERROR,
                         "can't iterate over non-object value");
        }
        break;
      }
      case OP_RETURN: {
       
        size_t off_ret = call_stack_restore_frame(baik);
        if (off_ret != BAIK_BCODE_OFFSET_EXIT) {
          bp = *baik_bcode_part_get_by_offset(baik, off_ret);
          code = (const uint8_t *) bp.data.p;
          i = off_ret - bp.start_idx;
          LOG(LL_VERBOSE_DEBUG, ("RETURNING TO %d", (int) off_ret + 1));
        } else {
          goto clean;
        }
        
        break;
      }
      case OP_ARGS: {
       
        if (prev_opcode != OP_GET) {
          baik->vals.last_getprop_obj = BAIK_UNDEFINED;
        }

       
        push_baik_val(&baik->arg_stack, baik->vals.last_getprop_obj);
       
        push_baik_val(&baik->arg_stack,
                     baik_mk_number(baik, (double) baik_stack_size(&baik->stack)));
        break;
      }
      case OP_CALL: {
        
        
        int func_pos;
        baik_val_t *func;
        baik_val_t retval_stack_idx = vtop(&baik->arg_stack);
        func_pos = baik_get_int(baik, retval_stack_idx) - 1;
        func = vptr(&baik->stack, func_pos);

       
        baik_pop_val(&baik->arg_stack);

        if (baik_is_function(*func)) {
          size_t off_call;
          call_stack_push_frame(baik, bp.start_idx + i, retval_stack_idx);

         
          off_call = baik_get_func_addr(*func) - 1;
          bp = *baik_bcode_part_get_by_offset(baik, off_call);
          code = (const uint8_t *) bp.data.p;
          i = off_call - bp.start_idx;

          *func = BAIK_UNDEFINED;  
          
        } else if (baik_is_string(*func) ){//|| baik_is_ffi_sig(*func)) {
         

          call_stack_push_frame(baik, bp.start_idx + i, retval_stack_idx);

         
          //baik_ffi_call2(baik);

          call_stack_restore_frame(baik);
        } else if (baik_is_foreign(*func)) {
         

          call_stack_push_frame(baik, bp.start_idx + i, retval_stack_idx);

         
          ((void (*) (struct baik *)) baik_get_ptr(baik, *func))(baik);

          call_stack_restore_frame(baik);
        } else {
          baik_set_errorf(baik, BAIK_TYPE_ERROR, "calling non-callable");
        }
        break;
      }
      case OP_SET_ARG: {
        int llen1, llen2, n,
            arg_no = BAIK_EM_varint_decode_unsafe(&code[i + 1], &llen1);
        baik_val_t obj, key, v;
        n = BAIK_EM_varint_decode_unsafe(&code[i + llen1 + 1], &llen2);
        key = baik_mk_string(baik, (char *) code + i + 1 + llen1 + llen2, n, 1);
        obj = vtop(&baik->scopes);
        v = baik_arg(baik, arg_no);
        baik_set_v(baik, obj, key, v);
        i += llen1 + llen2 + n;
        break;
      }
      case OP_SETRETVAL: {
        if (baik_stack_size(&baik->call_stack) < CALL_STACK_FRAME_ITEMS_CNT) {
          baik_set_errorf(baik, BAIK_INTERNAL_ERROR, "cannot return");
        } else {
          size_t retval_pos = baik_get_int(
              baik, *vptr(&baik->call_stack,
                         -1 - CALL_STACK_FRAME_ITEM_RETVAL_STACK_IDX));
          *vptr(&baik->stack, retval_pos - 1) = baik_pop(baik);
        }
        
        
        break;
      }
      case OP_EXPR: {
        int op = code[i + 1];
        exec_expr(baik, op);
        i++;
        break;
      }
      case OP_DROP: {
        baik_pop(baik);
        break;
      }
      case OP_DUP: {
        baik_push(baik, vtop(&baik->stack));
        break;
      }
      case OP_SWAP: {
        baik_val_t a = baik_pop(baik);
        baik_val_t b = baik_pop(baik);
        baik_push(baik, a);
        baik_push(baik, b);
        break;
      }
      case OP_LOOP: {
        int l1, l2, off = BAIK_EM_varint_decode_unsafe(&code[i + 1], &l1);
       
        push_baik_val(&baik->loop_addresses,
                     baik_mk_number(baik, (double) baik_stack_size(&baik->scopes)));

       
        push_baik_val(
            &baik->loop_addresses,
            baik_mk_number(baik, (double) (i + 1 + l1 + off)));
        off = BAIK_EM_varint_decode_unsafe(&code[i + 1 + l1], &l2);

       
        push_baik_val(
            &baik->loop_addresses,
            baik_mk_number(baik, (double) (i + 1 + l1 + l2 + off)));
        i += l1 + l2;
        break;
      }
      case OP_CONTINUE: {
        if (baik_stack_size(&baik->loop_addresses) >= 3) {
          size_t scopes_len = baik_get_int(baik, *vptr(&baik->loop_addresses, -3));
          assert(baik_stack_size(&baik->scopes) >= scopes_len);
          baik->scopes.len = scopes_len * sizeof(baik_val_t);

         
          i = baik_get_int(baik, vtop(&baik->loop_addresses)) - 1;
        } else {
          baik_set_errorf(baik, BAIK_SYNTAX_ERROR, "misplaced 'continue'");
        }
      } break;
      case OP_BREAK: {
        if (baik_stack_size(&baik->loop_addresses) >= 3) {
          size_t scopes_len;
         
          baik_pop_val(&baik->loop_addresses);

         
          i = baik_get_int(baik, baik_pop_val(&baik->loop_addresses)) - 1;

         
          scopes_len = baik_get_int(baik, baik_pop_val(&baik->loop_addresses));
          assert(baik_stack_size(&baik->scopes) >= scopes_len);
          baik->scopes.len = scopes_len * sizeof(baik_val_t);

          LOG(LL_VERBOSE_DEBUG, ("BREAKING TO %d", (int) i + 1));
        } else {
          baik_set_errorf(baik, BAIK_SYNTAX_ERROR, "misplaced 'break'");
        }
      } break;
      case OP_NOP:
        break;
      case OP_EXIT:
        i = bp.data.len;
        break;
      default:
#if BAIK_ENABLE_DEBUG
        baik_dump(baik, 1);
#endif
        baik_set_errorf(baik, BAIK_INTERNAL_ERROR, "Unknown opcode: %d, off %d+%d",
                       (int) opcode, (int) bp.start_idx, (int) i);
        i = bp.data.len;
        break;
    }
    if (baik->error != BAIK_OK) {
      baik_gen_stack_trace(baik, bp.start_idx + i - 1);

      baik->stack.len = stack_len;
      baik->call_stack.len = call_stack_len;
      baik->arg_stack.len = arg_stack_len;
      baik->scopes.len = scopes_len;
      baik->loop_addresses.len = loop_addresses_len;

      baik_push(baik, BAIK_UNDEFINED);
      break;
    }
  }

clean:
 
  baik_bcode_part_get_by_offset(baik, start_off)->exec_res = baik->error;

  *res = baik_pop(baik);
  return baik->error;
}

BAIK_PRIVATE baik_err_t baik_exec_internal(struct baik *baik, const char *path,
                                        const char *src, int generate_jsc,
                                        baik_val_t *res) {
  size_t off = baik->bcode_len;
  baik_val_t r = BAIK_UNDEFINED;
  baik->error = baik_parse(path, src, baik);
  if (BAIK_EM_log_level >= LL_VERBOSE_DEBUG) baik_dump(baik, 1);
  if (generate_jsc == -1) generate_jsc = baik->generate_jsc;
  if (baik->error == BAIK_OK) {
#if BAIK_GENERATE_INAC && defined(BAIK_EM_MMAP)
    if (generate_jsc && path != NULL) {
      const char *jsext = ".ina";
      int basename_len = (int) strlen(path) - strlen(jsext);
      if (basename_len > 0 && strcmp(path + basename_len, jsext) == 0) {
       
        int rewrite = 1;
        int read_mmapped = 1;

       
        const char *jscext = ".inac";
        char filename_jsc[basename_len + strlen(jscext) + 1];
        memcpy(filename_jsc, path, basename_len);
        strcpy(filename_jsc + basename_len, jscext);

       
        struct baik_bcode_part *bp =
            baik_bcode_part_get(baik, baik_bcode_parts_cnt(baik) - 1);

       
        {
          size_t size;
          char *data = BAIK_EM_mmap_file(filename_jsc, &size);
          if (data != NULL) {
            if (size == bp->data.len) {
              if (memcmp(data, bp->data.p, size) == 0) {
               
                rewrite = 0;
              }
            }
            munmap(data, size);
          }
        }

       
        if (rewrite) {
          FILE *fp = fopen(filename_jsc, "wb");
          if (fp != NULL) {
           
            fwrite(bp->data.p, bp->data.len, 1, fp);
            fclose(fp);
          } else {
            LOG(LL_WARN, ("Failed to open %s for writing", filename_jsc));
            read_mmapped = 0;
          }
        }

        if (read_mmapped) {
          free((void *) bp->data.p);
          bp->data.p = BAIK_EM_mmap_file(filename_jsc, &bp->data.len);
          bp->in_rom = 1;
        }
      }
    }
#else
    (void) generate_jsc;
#endif

    baik_execute(baik, off, &r);
  }
  if (res != NULL) *res = r;
  return baik->error;
}

baik_err_t baik_exec(struct baik *baik, const char *src, baik_val_t *res) {
  return baik_exec_internal(baik, "<stdin>", src, 0, res);
}

baik_err_t baik_exec_file(struct baik *baik, const char *path, baik_val_t *res) {
  baik_err_t error = BAIK_FILE_READ_ERROR;
  baik_val_t r = BAIK_UNDEFINED;
  size_t size;
  char *source_code = BAIK_EM_read_file(path, &size);

  if (source_code == NULL) {
    error = BAIK_FILE_READ_ERROR;
    baik_prepend_errorf(baik, error, "GALAT : gagal membaca file \"%s\"", path);
    goto clean;
  }

  r = BAIK_UNDEFINED;
  error = baik_exec_internal(baik, path, source_code, -1, &r);
  free(source_code);

clean:
  if (res != NULL) *res = r;
  return error;
}

baik_err_t baik_call(struct baik *baik, baik_val_t *res, baik_val_t func,
                   baik_val_t this_val, int nargs, ...) {
  va_list ap;
  int i;
  baik_err_t ret;
  baik_val_t *args = calloc(nargs, sizeof(baik_val_t));
  va_start(ap, nargs);
  for (i = 0; i < nargs; i++) {
    args[i] = va_arg(ap, baik_val_t);
  }
  va_end(ap);

  ret = baik_apply(baik, res, func, this_val, nargs, args);

  free(args);
  return ret;
}

baik_err_t baik_apply(struct baik *baik, baik_val_t *res, baik_val_t func,
                    baik_val_t this_val, int nargs, baik_val_t *args) {
  baik_val_t r, prev_this_val, retval_stack_idx, *resp;
  int i;

  //if (!baik_is_function(func) && !baik_is_foreign(func) &&
  //     !baik_is_ffi_sig(func)) {
  //   return baik_set_errorf(baik, BAIK_TYPE_ERROR, "calling non-callable");
  // }

  //LOG(LL_VERBOSE_DEBUG, ("applying func %d", (int) baik_get_func_addr(func)));

  prev_this_val = baik->vals.this_obj;

  baik_push(baik, func);
  resp = vptr(&baik->stack, -1);
  retval_stack_idx = baik_mk_number(baik, (double) baik_stack_size(&baik->stack));

  for (i = 0; i < nargs; i++) {
    baik_push(baik, args[i]);
  }

  push_baik_val(&baik->arg_stack, this_val);
  call_stack_push_frame(baik, BAIK_BCODE_OFFSET_EXIT, retval_stack_idx);

  // if (baik_is_foreign(func)) {
  //   ((void (*) (struct baik *)) baik_get_ptr(baik, func))(baik);
  //   if (res != NULL) *res = *resp;
  // } else if (baik_is_ffi_sig(func)) {
  //   baik_ffi_call2(baik);
  //   if (res != NULL) *res = *resp;
  // } else {
    size_t addr = baik_get_func_addr(func);
    baik_execute(baik, addr, &r);
    if (res != NULL) *res = r;
  //}

  if (baik->error != BAIK_OK) {
    call_stack_restore_frame(baik);

    
    baik_pop(baik);
  }
  baik->vals.this_obj = prev_this_val;

  return baik->error;
}

#ifndef RTLD_DEFAULT
#define RTLD_DEFAULT NULL
#endif

// static ffi_fn_t *get_cb_impl_by_signature(const baik_ffi_sig_t *sig);


struct cbdata {
  baik_val_t func;
  baik_val_t userdata;
  int8_t func_idx;
  int8_t userdata_idx;
};

// void baik_set_ffi_resolver(struct baik *baik, baik_ffi_resolver_t *dlsym) {
//   baik->dlsym = dlsym;
// }

// static baik_ffi_ctype_t parse_cval_type(struct baik *baik, const char *s,
//                                        const char *e) {
//   struct baik_generic_str ms = GENERIC_NULL_STR;
 
//   while (s < e && isspace((int) *s)) s++;
//   while (e > s && isspace((int) e[-1])) e--;
//   ms.p = s;
//   ms.len = e - s;
//   if (baik_generic_vcmp(&ms, "void") == 0) {
//     return BAIK_FFI_CTYPE_NONE;
//   } else if (baik_generic_vcmp(&ms, "userdata") == 0) {
//     return BAIK_FFI_CTYPE_USERDATA;
//   } else if (baik_generic_vcmp(&ms, "int") == 0) {
//     return BAIK_FFI_CTYPE_INT;
//   } else if (baik_generic_vcmp(&ms, "bool") == 0) {
//     return BAIK_FFI_CTYPE_BOOL;
//   } else if (baik_generic_vcmp(&ms, "double") == 0) {
//     return BAIK_FFI_CTYPE_DOUBLE;
//   } else if (baik_generic_vcmp(&ms, "float") == 0) {
//     return BAIK_FFI_CTYPE_FLOAT;
//   } else if (baik_generic_vcmp(&ms, "char*") == 0 || baik_generic_vcmp(&ms, "char *") == 0) {
//     return BAIK_FFI_CTYPE_CHAR_PTR;
//   } else if (baik_generic_vcmp(&ms, "void*") == 0 || baik_generic_vcmp(&ms, "void *") == 0) {
//     return BAIK_FFI_CTYPE_VOID_PTR;
//   } else if (baik_generic_vcmp(&ms, "struct baik_generic_str") == 0) {
//     return BAIK_FFI_CTYPE_STRUCT_GENERIC_STR;
//   } else if (baik_generic_vcmp(&ms, "struct baik_generic_str *") == 0 ||
//              baik_generic_vcmp(&ms, "struct baik_generic_str*") == 0) {
//     return BAIK_FFI_CTYPE_STRUCT_GENERIC_STR_PTR;
//   } else {
//     baik_prepend_errorf(baik, BAIK_TYPE_ERROR, "GALAT : gagal menguraikan isi tipe \"%.*s\"",
//                        (int) ms.len, ms.p);
//     return BAIK_FFI_CTYPE_INVALID;
//   }
// }
