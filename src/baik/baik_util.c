/*
 * baik_util.c - Utilitas: typeof, pencetakan nilai, disassembler bytecode, pemetaan nomor baris.
 *
 * Bagian dari interpreter bahasa BAIK. Deklarasi bersama ada di
 * src/baik/baik_internal.h; antarmuka publik ada di src/baik.h.
 */
#include "baik_internal.h"

const char *baik_typeof(baik_val_t v) {
  return baik_stringify_type(baik_get_type(v));
}

BAIK_PRIVATE const char *baik_stringify_type(enum baik_type t) {
  switch (t) {
    case BAIK_TYPE_NUMBER:
      return "angka";
    case BAIK_TYPE_BOOLEAN:
      return "boolean";
    case BAIK_TYPE_STRING:
      return "huruf";
    case BAIK_TYPE_OBJECT_ARRAY:
      return "untaian";
    case BAIK_TYPE_OBJECT_GENERIC:
      return "objek";
    case BAIK_TYPE_FOREIGN:
      return "foreign_ptr";
    case BAIK_TYPE_OBJECT_FUNCTION:
      return "fungsi";
    case BAIK_TYPE_NULL:
      return "kosong";
    case BAIK_TYPE_UNDEFINED:
      return "takterdefinisi";
    default:
      return "???";
  }
}

void baik_jprintf(baik_val_t v, struct baik *baik, struct json_out *out) {
  if (baik_is_number(v)) {
    double iv, d = baik_get_double(baik, v);
    if (modf(d, &iv) == 0) {
      json_printf(out, "%" INT64_FMT, (int64_t) d);
    } else {
      json_printf(out, "%f", baik_get_double(baik, v));
    }
  } else if (baik_is_boolean(v)) {
    json_printf(out, "%s", baik_get_bool(baik, v) ? "benar" : "salah");
  } else if (baik_is_string(v)) {
    size_t i, size;
    const char *s = baik_get_string(baik, &v, &size);
    for (i = 0; i < size; i++) {
      int ch = ((unsigned char *) s)[i];
      if (isprint(ch)) {
        json_printf(out, "%c", ch);
      } else {
        json_printf(out, "%s%02x", "\\x", ch);
      }
    }
  } else if (baik_is_array(v)) {
    json_printf(out, "%s", "<untaian>");
  } else if (baik_is_object(v)) {
    json_printf(out, "%s", "<objek>");
  } else if (baik_is_foreign(v)) {
    json_printf(out, "%s%lx%s", "<foreign_ptr@",
                (unsigned long) (uintptr_t) baik_get_ptr(baik, v), ">");
  } else if (baik_is_function(v)) {
    json_printf(out, "%s%d%s", "<fungsi@", (int) baik_get_func_addr(v), ">");
  } else if (baik_is_null(v)) {
    json_printf(out, "%s", "kosong");
  } else if (baik_is_undefined(v)) {
    json_printf(out, "%s", "takterdefinisi");
  } else {
    json_printf(out, "%s%" INT64_FMT "%s", "<???", (int64_t) v, ">");
  }
}

void baik_sprintf(baik_val_t v, struct baik *baik, char *buf, size_t n) {
  struct json_out out = JSON_OUT_BUF(buf, n);
  baik_jprintf(v, baik, &out);
}

void baik_fprintf(baik_val_t v, struct baik *baik, FILE *fp) {
  struct json_out out = JSON_OUT_FILE(fp);
  baik_jprintf(v, baik, &out);
}

#if BAIK_ENABLE_DEBUG

BAIK_PRIVATE const char *opcodetostr(uint8_t opcode) {
  static const char *names[] = {
      "NOP", "DROP", "DUP", "SWAP", "JMP", "JMP_TRUE", "JMP_NEUTRAL_TRUE",
      "JMP_FALSE", "JMP_NEUTRAL_FALSE", "FIND_SCOPE", "PUSH_SCOPE", "PUSH_STR",
      "PUSH_TRUE", "PUSH_FALSE", "PUSH_INT", "PUSH_DBL", "PUSH_NULL",
      "PUSH_UNDEF", "PUSH_OBJ", "PUSH_ARRAY", "PUSH_FUNC", "PUSH_THIS", "GET",
      "CREATE", "EXPR", "APPEND", "SET_ARG", "NEW_SCOPE", "DEL_SCOPE", "CALL",
      "RETURN", "LOOP", "BREAK", "CONTINUE", "SETRETVAL", "EXIT", "BCODE_HDR",
      "ARGS", "FOR_IN_NEXT",
  };
  const char *name = "???";
  assert(ARRAY_SIZE(names) == OP_MAX);
  if (opcode < ARRAY_SIZE(names)) name = names[opcode];
  return name;
}

BAIK_PRIVATE size_t baik_disasm_single(const uint8_t *code, size_t i) {
  char buf[40];
  size_t start_i = i;
  size_t llen;
  uint64_t n;

  snprintf(buf, sizeof(buf), "\t%-3u %-8s", (unsigned) i, opcodetostr(code[i]));

  switch (code[i]) {
    case OP_PUSH_FUNC: {
      BAIK_EM_varint_decode(&code[i + 1], ~0, &n, &llen);
      LOG(LL_VERBOSE_DEBUG, ("%s %04u", buf, (unsigned) (i - n)));
      i += llen;
      break;
    }
    case OP_PUSH_INT: {
      BAIK_EM_varint_decode(&code[i + 1], ~0, &n, &llen);
      LOG(LL_VERBOSE_DEBUG, ("%s\t%lu", buf, (unsigned long) n));
      i += llen;
      break;
    }
    case OP_SET_ARG: {
      size_t llen2;
      uint64_t arg_no;
      BAIK_EM_varint_decode(&code[i + 1], ~0, &arg_no, &llen);
      BAIK_EM_varint_decode(&code[i + llen + 1], ~0, &n, &llen2);
      LOG(LL_VERBOSE_DEBUG, ("%s\t[%.*s] %u", buf, (int) n,
                             code + i + 1 + llen + llen2, (unsigned) arg_no));
      i += llen + llen2 + n;
      break;
    }
    case OP_PUSH_STR:
    case OP_PUSH_DBL: {
      BAIK_EM_varint_decode(&code[i + 1], ~0, &n, &llen);
      LOG(LL_VERBOSE_DEBUG, ("%s\t[%.*s]", buf, (int) n, code + i + 1 + llen));
      i += llen + n;
      break;
    }
    case OP_JMP:
    case OP_JMP_TRUE:
    case OP_JMP_NEUTRAL_TRUE:
    case OP_JMP_FALSE:
    case OP_JMP_NEUTRAL_FALSE: {
      BAIK_EM_varint_decode(&code[i + 1], ~0, &n, &llen);
      LOG(LL_VERBOSE_DEBUG,
          ("%s\t%u", buf,
           (unsigned) (i + n + llen +
                       1)));
      i += llen;
      break;
    }
    case OP_LOOP: {
      size_t l1, l2;
      uint64_t n1, n2;
      BAIK_EM_varint_decode(&code[i + 1], ~0, &n1, &l1);
      BAIK_EM_varint_decode(&code[i + l1 + 1], ~0, &n2, &l2);
      LOG(LL_VERBOSE_DEBUG,
          ("%s\tB:%lu C:%lu (%d)", buf,
           (unsigned long) (i + 1 + l1 + n1),
           (unsigned long) (i + 1 + l1 + l2 + n2), (int) i));
      i += l1 + l2;
      break;
    }
    case OP_EXPR: {
      int op = code[i + 1];
      const char *name = "???";
     
      switch (op) {
        case TOK_DOT:       name = "."; break;
        case TOK_MINUS:     name = "-"; break;
        case TOK_PLUS:      name = "+"; break;
        case TOK_MUL:       name = "*"; break;
        case TOK_DIV:       name = "/"; break;
        case TOK_REM:       name = "%"; break;
        case TOK_XOR:       name = "^"; break;
        case TOK_AND:       name = "&"; break;
        case TOK_OR:        name = "|"; break;
        case TOK_LSHIFT:    name = "<<"; break;
        case TOK_RSHIFT:    name = ">>"; break;
        case TOK_URSHIFT:   name = ">>>"; break;
        case TOK_UNARY_MINUS:   name = "- (unary)"; break;
        case TOK_UNARY_PLUS:    name = "+ (unary)"; break;
        case TOK_NOT:       name = "!"; break;
        case TOK_TILDA:     name = "~"; break;
        case TOK_EQ:        name = "=="; break;
        case TOK_NE:        name = "!="; break;
        case TOK_EQ_EQ:     name = "==="; break;
        case TOK_NE_NE:     name = "!=="; break;
        case TOK_LT:        name = "<"; break;
        case TOK_GT:        name = ">"; break;
        case TOK_LE:        name = "<="; break;
        case TOK_GE:        name = ">="; break;
        case TOK_ASSIGN:    name = "="; break;
        case TOK_POSTFIX_PLUS:  name = "++ (postfix)"; break;
        case TOK_POSTFIX_MINUS: name = "-- (postfix)"; break;
        case TOK_MINUS_MINUS:   name = "--"; break;
        case TOK_PLUS_PLUS:     name = "++"; break;
        case TOK_LOGICAL_AND:   name = "&&"; break;
        case TOK_LOGICAL_OR:    name = "||"; break;
        case TOK_KEYWORD_TIPE:  name = "tipe"; break;
        case TOK_PLUS_ASSIGN:     name = "+="; break;
        case TOK_MINUS_ASSIGN:    name = "-="; break;
        case TOK_MUL_ASSIGN:      name = "*="; break;
        case TOK_DIV_ASSIGN:      name = "/="; break;
        case TOK_REM_ASSIGN:      name = "%="; break;
        case TOK_XOR_ASSIGN:      name = "^="; break;
        case TOK_AND_ASSIGN:      name = "&="; break;
        case TOK_OR_ASSIGN:       name = "|="; break;
        case TOK_LSHIFT_ASSIGN:   name = "<<="; break;
        case TOK_RSHIFT_ASSIGN:   name = ">>="; break;
        case TOK_URSHIFT_ASSIGN:  name = ">>>="; break;
      }
     
      LOG(LL_VERBOSE_DEBUG, ("%s\t%s", buf, name));
      i++;
      break;
    }
    case OP_BCODE_HEADER: {
      size_t start = 0;
      baik_header_item_t map_offset = 0, total_size = 0;
      start = i;
      memcpy(&total_size, &code[i + 1], sizeof(total_size));
      memcpy(&map_offset,
             &code[i + 1 + BAIK_HDR_ITEM_MAP_OFFSET * sizeof(total_size)],
             sizeof(map_offset));
      i += sizeof(baik_header_item_t) * BAIK_HDR_ITEMS_CNT;
      LOG(LL_VERBOSE_DEBUG, ("%s\t[%s] end:%lu map_offset: %lu", buf,
                             &code[i + 1], (unsigned long) start + total_size,
                             (unsigned long) start + map_offset));
      i += strlen((char *) (code + i + 1)) + 1;
      break;
    }
    default:
      LOG(LL_VERBOSE_DEBUG, ("%s", buf));
      break;
  }
  return i - start_i;
}

void baik_disasm(const uint8_t *code, size_t len) {
  size_t i, start = 0;
  baik_header_item_t map_offset = 0, total_size = 0;

  for (i = 0; i < len; i++) {
    size_t delta = baik_disasm_single(code, i);
    if (code[i] == OP_BCODE_HEADER) {
      start = i;
      memcpy(&total_size, &code[i + 1], sizeof(total_size));
      memcpy(&map_offset,
             &code[i + 1 + BAIK_HDR_ITEM_MAP_OFFSET * sizeof(total_size)],
             sizeof(map_offset));
    }

    i += delta;

    if (map_offset > 0 && i == start + map_offset) {
      i = start + total_size - 1;
      continue;
    }
  }
}

static void baik_dump_obj_stack(const char *name, const struct mbuf *m,
                               struct baik *baik) {
  char buf[50];
  size_t i, n;
  n = baik_stack_size(m);
  LOG(LL_VERBOSE_DEBUG, ("%12s (%d elems): ", name, (int) n));
  for (i = 0; i < n; i++) {
    baik_sprintf(((baik_val_t *) m->buf)[i], baik, buf, sizeof(buf));
    LOG(LL_VERBOSE_DEBUG, ("%34s", buf));
  }
}

void baik_dump(struct baik *baik, int do_disasm) {
  LOG(LL_VERBOSE_DEBUG, ("------- BAIK VM DUMP BEGIN"));
  baik_dump_obj_stack("DATA_STACK", &baik->stack, baik);
  baik_dump_obj_stack("CALL_STACK", &baik->call_stack, baik);
  baik_dump_obj_stack("SCOPES", &baik->scopes, baik);
  baik_dump_obj_stack("LOOP_OFFSETS", &baik->loop_addresses, baik);
  baik_dump_obj_stack("ARG_STACK", &baik->arg_stack, baik);
  if (do_disasm) {
    int parts_cnt = baik_bcode_parts_cnt(baik);
    int i;
    LOG(LL_VERBOSE_DEBUG, ("%23s", "CODE:"));
    for (i = 0; i < parts_cnt; i++) {
      struct baik_bcode_part *bp = baik_bcode_part_get(baik, i);
      baik_disasm((uint8_t *) bp->data.p, bp->data.len);
    }
  }
  LOG(LL_VERBOSE_DEBUG, ("------- BAIK VM DUMP END"));
}

BAIK_PRIVATE int baik_check_arg(struct baik *baik, int arg_num,
                              const char *arg_name, enum baik_type expected_type,
                              baik_val_t *parg) {
  baik_val_t arg = BAIK_UNDEFINED;
  enum baik_type actual_type;

  if (arg_num >= 0) {
    int nargs = baik_nargs(baik);
    if (nargs < arg_num + 1) {
      baik_prepend_errorf(baik, BAIK_TYPE_ERROR, "GALAT : argumen salah %s", arg_name);
      return 0;
    }

    arg = baik_arg(baik, arg_num);
  } else {
   
    arg = baik->vals.this_obj;
  }

  actual_type = baik_get_type(arg);
  if (actual_type != expected_type) {
    baik_prepend_errorf(baik, BAIK_TYPE_ERROR, "GALAT : %s should be a %s, %s given",
                       arg_name, baik_stringify_type(expected_type),
                       baik_stringify_type(actual_type));
    return 0;
  }

  if (parg != NULL) {
    *parg = arg;
  }

  return 1;
}

BAIK_PRIVATE int baik_normalize_idx(int idx, int size) {
  if (idx < 0) {
    idx = size + idx;
    if (idx < 0) {
      idx = 0;
    }
  }
  if (idx > size) {
    idx = size;
  }
  return idx;
}

BAIK_PRIVATE const char *baik_get_bcode_filename(struct baik *baik,
                                               struct baik_bcode_part *bp) {
  (void) baik;
  return bp->data.p + 1 +
         sizeof(baik_header_item_t) * BAIK_HDR_ITEMS_CNT;
}

const char *baik_get_bcode_filename_by_offset(struct baik *baik, int offset) {
  const char *ret = NULL;
  struct baik_bcode_part *bp = baik_bcode_part_get_by_offset(baik, offset);
  if (bp != NULL) {
    ret = baik_get_bcode_filename(baik, bp);
  }
  return ret;
}

int baik_get_lineno_by_offset(struct baik *baik, int offset) {
  size_t llen;
  uint64_t map_len;
  int prev_line_no, ret = 1;
  struct baik_bcode_part *bp = baik_bcode_part_get_by_offset(baik, offset);
  uint8_t *p, *pe;
  if (bp != NULL) {
    baik_header_item_t map_offset, bcode_offset;
    memcpy(&map_offset, bp->data.p + 1 +
                            sizeof(baik_header_item_t) * BAIK_HDR_ITEM_MAP_OFFSET,
           sizeof(map_offset));

    memcpy(&bcode_offset,
           bp->data.p + 1 +
               sizeof(baik_header_item_t) * BAIK_HDR_ITEM_BCODE_OFFSET,
           sizeof(bcode_offset));

    offset -= (1 + bcode_offset) + bp->start_idx;

   
    p = (uint8_t *) bp->data.p + 1 + map_offset;

    BAIK_EM_varint_decode(p, ~0, &map_len, &llen);
    p += llen;
    pe = p + map_len;

    prev_line_no = 1;
    while (p < pe) {
      uint64_t cur_offset, line_no;
      BAIK_EM_varint_decode(p, ~0, &cur_offset, &llen);
      p += llen;
      BAIK_EM_varint_decode(p, ~0, &line_no, &llen);
      p += llen;

      if (cur_offset >= (uint64_t) offset) {
        ret = prev_line_no;
        break;
      }
      prev_line_no = line_no;
    }
  }
  return ret;
}

int baik_get_offset_by_call_frame_num(struct baik *baik, int cf_num) {
  int ret = -1;
  if (cf_num == 0) {
   
    ret = baik->cur_bcode_offset;
  } else if (cf_num > 0 &&
             baik->call_stack.len >=
                 sizeof(baik_val_t) * CALL_STACK_FRAME_ITEMS_CNT * cf_num) {
   
    int pos = CALL_STACK_FRAME_ITEM_RETURN_ADDR +
              CALL_STACK_FRAME_ITEMS_CNT * (cf_num - 1);
    baik_val_t val = *vptr(&baik->call_stack, -1 - pos);
    ret = baik_get_int(baik, val);
  }
  return ret;
}

#endif /* BAIK_ENABLE_DEBUG - dibuka di dekat baik_disasm_single() */
