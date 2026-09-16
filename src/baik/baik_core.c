/*
 * baik_core.c - Inti interpreter: daur hidup (baik_create/baik_destroy), galat,
 * tumpukan nilai, bingkai panggilan, lingkup, serta pengakses memori mentah
 * baik_mem_* yang dipakai binding tingkat skrip.
 *
 * Bagian dari interpreter bahasa BAIK. Deklarasi bersama ada di
 * src/baik/baik_internal.h; antarmuka publik ada di src/baik.h.
 */
#include "baik_internal.h"

void baik_destroy(struct baik *baik) {
  {
    int parts_cnt = baik_bcode_parts_cnt(baik);
    int i;
    for (i = 0; i < parts_cnt; i++) {
      struct baik_bcode_part *bp = baik_bcode_part_get(baik, i);
      if (!bp->in_rom) {
        free((void *) bp->data.p);
      }
    }
  }

  mbuf_free(&baik->bcode_gen);
  mbuf_free(&baik->bcode_parts);
  mbuf_free(&baik->stack);
  mbuf_free(&baik->call_stack);
  mbuf_free(&baik->arg_stack);
  mbuf_free(&baik->owned_strings);
  mbuf_free(&baik->foreign_strings);
  mbuf_free(&baik->owned_values);
  mbuf_free(&baik->scopes);
  mbuf_free(&baik->loop_addresses);
  mbuf_free(&baik->json_visited_stack);
  free(baik->error_msg);
  free(baik->stack_trace);
  //baik_ffi_args_free_list(baik);
  gc_arena_destroy(baik, &baik->object_arena);
  gc_arena_destroy(baik, &baik->property_arena);
  gc_arena_destroy(baik, &baik->ffi_sig_arena);
  free(baik);
}

struct baik *baik_create(void) {
  baik_val_t global_object;
  struct baik *baik = calloc(1, sizeof(*baik));
  mbuf_init(&baik->stack, 0);
  mbuf_init(&baik->call_stack, 0);
  mbuf_init(&baik->arg_stack, 0);
  mbuf_init(&baik->owned_strings, 0);
  mbuf_init(&baik->foreign_strings, 0);
  mbuf_init(&baik->bcode_gen, 0);
  mbuf_init(&baik->bcode_parts, 0);
  mbuf_init(&baik->owned_values, 0);
  mbuf_init(&baik->scopes, 0);
  mbuf_init(&baik->loop_addresses, 0);
  mbuf_init(&baik->json_visited_stack, 0);

  baik->bcode_len = 0;
 
  {
    char z = 0;
    mbuf_append(&baik->owned_strings, &z, 1);
  }

  gc_arena_init(&baik->object_arena, sizeof(struct baik_object),
                BAIK_OBJECT_ARENA_SIZE, BAIK_OBJECT_ARENA_INC_SIZE);
  gc_arena_init(&baik->property_arena, sizeof(struct baik_property),
                BAIK_PROPERTY_ARENA_SIZE, BAIK_PROPERTY_ARENA_INC_SIZE);
  // gc_arena_init(&baik->ffi_sig_arena, sizeof(struct baik_ffi_sig),
  //               BAIK_FUNC_FFI_ARENA_SIZE, BAIK_FUNC_FFI_ARENA_INC_SIZE);
  // baik->ffi_sig_arena.destructor = baik_ffi_sig_destructor;

  global_object = baik_mk_object(baik);
  baik_init_builtin(baik, global_object);
  // baik_set_ffi_resolver(baik, dlsym);
  push_baik_val(&baik->scopes, global_object);
  baik->vals.this_obj = BAIK_UNDEFINED;
  baik->vals.dataview_proto = BAIK_UNDEFINED;

  return baik;
}

baik_err_t baik_set_errorf(struct baik *baik, baik_err_t err, const char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  free(baik->error_msg);
  baik->error_msg = NULL;
  baik->error = err;
  if (fmt != NULL) {
    baik_generic_avprintf(&baik->error_msg, 0, fmt, ap);
  }
  va_end(ap);
  return err;
}

baik_err_t baik_prepend_errorf(struct baik *baik, baik_err_t err, const char *fmt,
                             ...) {
  char *old_error_msg = baik->error_msg;
  char *new_error_msg = NULL;
  va_list ap;
  va_start(ap, fmt);

  assert(err != BAIK_OK);

  baik->error_msg = NULL;
 
  if (baik->error == BAIK_OK) {
    baik->error = err;
  }
  baik_generic_avprintf(&new_error_msg, 0, fmt, ap);
  va_end(ap);

  if (old_error_msg != NULL) {
    baik_generic_asprintf(&baik->error_msg, 0, "%s: %s", new_error_msg, old_error_msg);
    free(new_error_msg);
    free(old_error_msg);
  } else {
    baik->error_msg = new_error_msg;
  }
  return err;
}

void baik_print_error(struct baik *baik, FILE *fp, const char *msg,
                     int print_stack_trace) {
  if (print_stack_trace && baik->stack_trace != NULL) {
    fprintf(fp, "%s", baik->stack_trace);
  }

  if (msg == NULL) {
    msg = "BAIK galat";
  }

  fprintf(fp, "%s: %s\n", msg, baik_strerror(baik, baik->error));
}

BAIK_PRIVATE void baik_die(struct baik *baik) {
  baik_val_t msg_v = BAIK_UNDEFINED;
  const char *msg = NULL;
  size_t msg_len = 0;

  if (!baik_check_arg(baik, 0, "msg", BAIK_TYPE_STRING, &msg_v)) {
    goto clean;
  }

  msg = baik_get_string(baik, &msg_v, &msg_len);

  baik_prepend_errorf(baik, BAIK_TYPE_ERROR, "%.*s", (int) msg_len, msg);

clean:
  baik_return(baik, BAIK_UNDEFINED);
}

const char *baik_strerror(struct baik *baik, enum baik_err err) {
  const char *err_names[] = {
      "NO_ERROR",        "SYNTAX_ERROR",    "REFERENCE_ERROR",
      "TYPE_ERROR",      "OUT_OF_MEMORY",   "INTERNAL_ERROR",
      "NOT_IMPLEMENTED", "FILE_OPEN_ERROR", "BAD_ARGUMENTS"};
  return baik->error_msg == NULL || baik->error_msg[0] == '\0' ? err_names[err]
                                                             : baik->error_msg;
}

BAIK_PRIVATE size_t baik_get_func_addr(baik_val_t v) {
  return v & ~BAIK_TAG_MASK;
}

BAIK_PRIVATE enum baik_type baik_get_type(baik_val_t v) {
  int tag;
  if (baik_is_number(v)) {
    return BAIK_TYPE_NUMBER;
  }
  tag = (v & BAIK_TAG_MASK) >> 48;
  switch (tag) {
    case BAIK_TAG_FOREIGN >> 48:
      return BAIK_TYPE_FOREIGN;
    case BAIK_TAG_UNDEFINED >> 48:
      return BAIK_TYPE_UNDEFINED;
    case BAIK_TAG_OBJECT >> 48:
      return BAIK_TYPE_OBJECT_GENERIC;
    case BAIK_TAG_ARRAY >> 48:
      return BAIK_TYPE_OBJECT_ARRAY;
    case BAIK_TAG_FUNCTION >> 48:
      return BAIK_TYPE_OBJECT_FUNCTION;
    case BAIK_TAG_STRING_I >> 48:
    case BAIK_TAG_STRING_O >> 48:
    case BAIK_TAG_STRING_F >> 48:
    case BAIK_TAG_STRING_D >> 48:
    case BAIK_TAG_STRING_5 >> 48:
      return BAIK_TYPE_STRING;
    case BAIK_TAG_BOOLEAN >> 48:
      return BAIK_TYPE_BOOLEAN;
    case BAIK_TAG_NULL >> 48:
      return BAIK_TYPE_NULL;
    default:
      abort();
      return BAIK_TYPE_UNDEFINED;
  }
}

baik_val_t baik_get_global(struct baik *baik) {
  return *vptr(&baik->scopes, 0);
}

static void baik_append_stack_trace_line(struct baik *baik, size_t offset) {
  if (offset != BAIK_BCODE_OFFSET_EXIT) {
    const char *filename = baik_get_bcode_filename_by_offset(baik, offset);
    int line_no = baik_get_lineno_by_offset(baik, offset);
    char *new_line = NULL;
    const char *fmt = "  at %s:%d\n";
    if (filename == NULL) {
      fprintf(stderr,
              "ERROR during stack trace generation: wrong bcode offset %d\n",
              (int) offset);
      filename = "<unknown-filename>";
    }
    baik_generic_asprintf(&new_line, 0, fmt, filename, line_no);

    if (baik->stack_trace != NULL) {
      char *old = baik->stack_trace;
      baik_generic_asprintf(&baik->stack_trace, 0, "%s%s", baik->stack_trace, new_line);
      free(old);
      free(new_line);
    } else {
      baik->stack_trace = new_line;
    }
  }
}

BAIK_PRIVATE void baik_gen_stack_trace(struct baik *baik, size_t offset) {
  baik_append_stack_trace_line(baik, offset);
  while (baik->call_stack.len >=
         sizeof(baik_val_t) * CALL_STACK_FRAME_ITEMS_CNT) {
    int i;

    offset = baik_get_int(
        baik, *vptr(&baik->call_stack, -1 - CALL_STACK_FRAME_ITEM_RETURN_ADDR));

    for (i = 0; i < CALL_STACK_FRAME_ITEMS_CNT; i++) {
      baik_pop_val(&baik->call_stack);
    }

    baik_append_stack_trace_line(baik, offset);
  }
}

void baik_own(struct baik *baik, baik_val_t *v) {
  mbuf_append(&baik->owned_values, &v, sizeof(v));
}

int baik_disown(struct baik *baik, baik_val_t *v) {
  baik_val_t **vp = (baik_val_t **) (baik->owned_values.buf +
                                   baik->owned_values.len - sizeof(v));

  for (; (char *) vp >= baik->owned_values.buf; vp--) {
    if (*vp == v) {
      *vp = *(baik_val_t **) (baik->owned_values.buf + baik->owned_values.len -
                             sizeof(v));
      baik->owned_values.len -= sizeof(v);
      return 1;
    }
  }

  return 0;
}


BAIK_PRIVATE int baik_getretvalpos(struct baik *baik) {
  int pos;
  baik_val_t *ppos = vptr(&baik->call_stack, -1);
  
  assert(ppos != NULL && baik_is_number(*ppos));
  pos = baik_get_int(baik, *ppos) - 1;
  assert(pos < (int) baik_stack_size(&baik->stack));
  return pos;
}

int baik_nargs(struct baik *baik) {
  int top = baik_stack_size(&baik->stack);
  int pos = baik_getretvalpos(baik) + 1;
  
  return pos > 0 && pos < top ? top - pos : 0;
}

baik_val_t baik_arg(struct baik *baik, int arg_index) {
  baik_val_t res = BAIK_UNDEFINED;
  int top = baik_stack_size(&baik->stack);
  int pos = baik_getretvalpos(baik) + 1;
  
  if (pos > 0 && pos + arg_index < top) {
    res = *vptr(&baik->stack, pos + arg_index);
  }

  return res;
}

void baik_return(struct baik *baik, baik_val_t v) {
  int pos = baik_getretvalpos(baik);
  
  baik->stack.len = sizeof(baik_val_t) * pos;
  baik_push(baik, v);
}

BAIK_PRIVATE baik_val_t vtop(struct mbuf *m) {
  size_t size = baik_stack_size(m);
  return size > 0 ? *vptr(m, size - 1) : BAIK_UNDEFINED;
}

BAIK_PRIVATE size_t baik_stack_size(const struct mbuf *m) {
  return m->len / sizeof(baik_val_t);
}

BAIK_PRIVATE baik_val_t *vptr(struct mbuf *m, int idx) {
  int size = baik_stack_size(m);
  if (idx < 0) idx = size + idx;
  return idx >= 0 && idx < size ? &((baik_val_t *) m->buf)[idx] : NULL;
}

BAIK_PRIVATE baik_val_t baik_pop(struct baik *baik) {
  if (baik->stack.len == 0) {
    baik_set_errorf(baik, BAIK_INTERNAL_ERROR, "stack underflow");
    return BAIK_UNDEFINED;
  } else {
    return baik_pop_val(&baik->stack);
  }
}

BAIK_PRIVATE void push_baik_val(struct mbuf *m, baik_val_t v) {
  mbuf_append(m, &v, sizeof(v));
}

BAIK_PRIVATE baik_val_t baik_pop_val(struct mbuf *m) {
  baik_val_t v = BAIK_UNDEFINED;
  assert(m->len >= sizeof(v));
  if (m->len >= sizeof(v)) {
    memcpy(&v, m->buf + m->len - sizeof(v), sizeof(v));
    m->len -= sizeof(v);
  }
  return v;
}

BAIK_PRIVATE void baik_push(struct baik *baik, baik_val_t v) {
  push_baik_val(&baik->stack, v);
}

void baik_set_generate_jsc(struct baik *baik, int generate_jsc) {
  baik->generate_jsc = generate_jsc;
}

void *baik_mem_to_ptr(unsigned val) {
  return (void *) (uintptr_t) val;
}

void *baik_mem_get_ptr(void *base, int offset) {
  return (char *) base + offset;
}

void baik_mem_set_ptr(void *ptr, void *val) {
  *(void **) ptr = val;
}

double baik_mem_get_dbl(void *ptr) {
  double v;
  memcpy(&v, ptr, sizeof(v));
  return v;
}

void baik_mem_set_dbl(void *ptr, double val) {
  memcpy(ptr, &val, sizeof(val));
}

double baik_mem_get_uint(void *ptr, int size, int bigendian) {
  uint8_t *p = (uint8_t *) ptr;
  int i, inc = bigendian ? 1 : -1;
  unsigned int res = 0;
  p += bigendian ? 0 : size - 1;
  for (i = 0; i < size; i++, p += inc) {
    res <<= 8;
    res |= *p;
  }
  return res;
}

double baik_mem_get_int(void *ptr, int size, int bigendian) {
  uint8_t *p = (uint8_t *) ptr;
  int i, inc = bigendian ? 1 : -1;
  int res = 0;
  p += bigendian ? 0 : size - 1;

  for (i = 0; i < size; i++, p += inc) {
    res <<= 8;
    res |= *p;
  }

  {
    int extra = sizeof(res) - size;
    for (i = 0; i < extra; i++) res <<= 8;
    for (i = 0; i < extra; i++) res >>= 8;
  }

  return res;
}

void baik_mem_set_uint(void *ptr, unsigned int val, int size, int bigendian) {
  uint8_t *p = (uint8_t *) ptr + (bigendian ? size - 1 : 0);
  int i, inc = bigendian ? -1 : 1;
  for (i = 0; i < size; i++, p += inc) {
    *p = val & 0xff;
    val >>= 8;
  }
}

void baik_mem_set_int(void *ptr, int val, int size, int bigendian) {
  baik_mem_set_uint(ptr, val, size, bigendian);
}

#if BAIK_GENERATE_INAC && defined(BAIK_EM_MMAP)
#include <sys/mman.h>
#endif

BAIK_PRIVATE void call_stack_push_frame(struct baik *baik, size_t offset,
                                  baik_val_t retval_stack_idx) {
 
  baik_val_t this_obj = baik_pop_val(&baik->arg_stack);
  push_baik_val(&baik->call_stack, baik->vals.this_obj);
  baik->vals.this_obj = this_obj;
  push_baik_val(&baik->call_stack, baik_mk_number(baik, (double) offset));
  push_baik_val(&baik->call_stack,
               baik_mk_number(baik, (double) baik_stack_size(&baik->scopes)));
  push_baik_val(
      &baik->call_stack,
      baik_mk_number(baik, (double) baik_stack_size(&baik->loop_addresses)));
  push_baik_val(&baik->call_stack, retval_stack_idx);
}


BAIK_PRIVATE size_t call_stack_restore_frame(struct baik *baik) {
  size_t retval_stack_idx, return_address, scope_index, loop_addr_index;
  assert(baik_stack_size(&baik->call_stack) >= CALL_STACK_FRAME_ITEMS_CNT);

  retval_stack_idx = baik_get_int(baik, baik_pop_val(&baik->call_stack));
  loop_addr_index = baik_get_int(baik, baik_pop_val(&baik->call_stack));
  scope_index = baik_get_int(baik, baik_pop_val(&baik->call_stack));
  return_address = baik_get_int(baik, baik_pop_val(&baik->call_stack));
  baik->vals.this_obj = baik_pop_val(&baik->call_stack);

  while (baik_stack_size(&baik->scopes) > scope_index) {
    baik_pop_val(&baik->scopes);
  }

  while (baik_stack_size(&baik->loop_addresses) > loop_addr_index) {
    baik_pop_val(&baik->loop_addresses);
  }

  baik->stack.len = retval_stack_idx * sizeof(baik_val_t);

  return return_address;
}

BAIK_PRIVATE baik_val_t baik_find_scope(struct baik *baik, baik_val_t key) {
  size_t num_scopes = baik_stack_size(&baik->scopes);
  while (num_scopes > 0) {
    baik_val_t scope = *vptr(&baik->scopes, num_scopes - 1);
    num_scopes--;
    if (baik_get_own_property_v(baik, scope, key) != NULL) return scope;
  }
  baik_set_errorf(baik, BAIK_REFERENCE_ERROR, "[%s] tidak terdefinisikan",
                 baik_get_cstring(baik, &key));
  return BAIK_UNDEFINED;
}

baik_val_t baik_get_this(struct baik *baik) {
  return baik->vals.this_obj;
}

