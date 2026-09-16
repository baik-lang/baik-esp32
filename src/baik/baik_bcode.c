/*
 * baik_bcode.c - Penghasil bytecode (emit_*), peta nomor baris, dan pengelolaan
 * potongan bytecode (baik_bcode_part_*). Pemuatan berkas skrip TIDAK di sini:
 * lihat baik_load() di baik_builtin.c dan baik_exec_file() di baik_exec.c.
 *
 * Bagian dari interpreter bahasa BAIK. Deklarasi bersama ada di
 * src/baik/baik_internal.h; antarmuka publik ada di src/baik.h.
 */
#include "baik_internal.h"

static void add_lineno_map_item(struct pstate *pstate) {
  if (pstate->last_emitted_line_no < pstate->line_no) {
    int offset = pstate->cur_idx - pstate->start_bcode_idx;
    size_t offset_llen = BAIK_EM_varint_llen(offset);
    size_t lineno_llen = BAIK_EM_varint_llen(pstate->line_no);
    mbuf_resize(&pstate->offset_lineno_map,
                pstate->offset_lineno_map.size + offset_llen + lineno_llen);
    BAIK_EM_varint_encode(offset, (uint8_t *) pstate->offset_lineno_map.buf +
                                 pstate->offset_lineno_map.len,
                     offset_llen);
    pstate->offset_lineno_map.len += offset_llen;
    BAIK_EM_varint_encode(pstate->line_no,
                     (uint8_t *) pstate->offset_lineno_map.buf +
                         pstate->offset_lineno_map.len,
                     lineno_llen);
    pstate->offset_lineno_map.len += lineno_llen;
    pstate->last_emitted_line_no = pstate->line_no;
  }
}

BAIK_PRIVATE void emit_byte(struct pstate *pstate, uint8_t byte) {
  add_lineno_map_item(pstate);
  mbuf_insert(&pstate->baik->bcode_gen, pstate->cur_idx, &byte, sizeof(byte));
  pstate->cur_idx += sizeof(byte);
}

BAIK_PRIVATE void emit_int(struct pstate *pstate, int64_t n) {
  struct mbuf *b = &pstate->baik->bcode_gen;
  size_t llen = BAIK_EM_varint_llen(n);
  add_lineno_map_item(pstate);
  mbuf_insert(b, pstate->cur_idx, NULL, llen);
  BAIK_EM_varint_encode(n, (uint8_t *) b->buf + pstate->cur_idx, llen);
  pstate->cur_idx += llen;
}

BAIK_PRIVATE void emit_str(struct pstate *pstate, const char *ptr, size_t len) {
  struct mbuf *b = &pstate->baik->bcode_gen;
  size_t llen = BAIK_EM_varint_llen(len);
  add_lineno_map_item(pstate);
  mbuf_insert(b, pstate->cur_idx, NULL, llen + len);
  BAIK_EM_varint_encode(len, (uint8_t *) b->buf + pstate->cur_idx, llen);
  memcpy(b->buf + pstate->cur_idx + llen, ptr, len);
  pstate->cur_idx += llen + len;
}

BAIK_PRIVATE int baik_bcode_insert_offset(struct pstate *p, struct baik *baik,
                                        size_t offset, size_t v) {
  int llen = (int) BAIK_EM_varint_llen(v);
  int diff = llen - BAIK_INIT_OFFSET_SIZE;
  assert(offset < baik->bcode_gen.len);
  if (diff > 0) {
    mbuf_resize(&baik->bcode_gen, baik->bcode_gen.size + diff);
  }
 
  memmove(baik->bcode_gen.buf + offset + llen,
          baik->bcode_gen.buf + offset + BAIK_INIT_OFFSET_SIZE,
          baik->bcode_gen.len - offset - BAIK_INIT_OFFSET_SIZE);
  baik->bcode_gen.len += diff;
  BAIK_EM_varint_encode(v, (uint8_t *) baik->bcode_gen.buf + offset, llen);

  if (p->cur_idx >= (int) offset) {
    p->cur_idx += diff;
  }
  return diff;
}

BAIK_PRIVATE void baik_bcode_part_add(struct baik *baik,
                                    const struct baik_bcode_part *bp) {
  mbuf_append(&baik->bcode_parts, bp, sizeof(*bp));
}

BAIK_PRIVATE struct baik_bcode_part *baik_bcode_part_get(struct baik *baik,
                                                      int num) {
  assert(num < baik_bcode_parts_cnt(baik));
  return (struct baik_bcode_part *) (baik->bcode_parts.buf +
                                    num * sizeof(struct baik_bcode_part));
}

BAIK_PRIVATE struct baik_bcode_part *baik_bcode_part_get_by_offset(struct baik *baik,
                                                                size_t offset) {
  int i;
  int parts_cnt = baik_bcode_parts_cnt(baik);
  struct baik_bcode_part *bp = NULL;

  if (offset >= baik->bcode_len) {
    return NULL;
  }

  for (i = 0; i < parts_cnt; i++) {
    bp = baik_bcode_part_get(baik, i);
    if (offset < bp->start_idx + bp->data.len) {
      break;
    }
  }

  assert(i < parts_cnt);

  return bp;
}

BAIK_PRIVATE int baik_bcode_parts_cnt(struct baik *baik) {
  return baik->bcode_parts.len / sizeof(struct baik_bcode_part);
}

BAIK_PRIVATE void baik_bcode_commit(struct baik *baik) {
  struct baik_bcode_part bp;
  memset(&bp, 0, sizeof(bp));
  mbuf_trim(&baik->bcode_gen);

  bp.data.p = baik->bcode_gen.buf;
  bp.data.len = baik->bcode_gen.len;
  mbuf_init(&baik->bcode_gen, 0);

  bp.start_idx = baik->bcode_len;
  bp.exec_res = BAIK_ERRS_CNT;

  baik_bcode_part_add(baik, &bp);
  baik->bcode_len += bp.data.len;
}

