/*
 * baik_gc.c - Pemungut sampah: arena, penandaan, penyapuan, pemadatan string.
 *
 * Bagian dari interpreter bahasa BAIK. Deklarasi bersama ada di
 * src/baik/baik_internal.h; antarmuka publik ada di src/baik.h.
 */
#include "baik_internal.h"


#include <stdio.h>

#define MARK(p) (((struct gc_cell *) (p))->head.word |= 1)
#define UNMARK(p) (((struct gc_cell *) (p))->head.word &= ~1)
#define MARKED(p) (((struct gc_cell *) (p))->head.word & 1)
#define MARK_FREE(p) (((struct gc_cell *) (p))->head.word |= 2)
#define UNMARK_FREE(p) (((struct gc_cell *) (p))->head.word &= ~2)
#define MARKED_FREE(p) (((struct gc_cell *) (p))->head.word & 2)
#define GC_ARENA_CELLS_RESERVE 2

static struct gc_block *gc_new_block(struct gc_arena *a, size_t size);
static void gc_free_block(struct gc_block *b);
static void gc_mark_mbuf_pt(struct baik *baik, const struct mbuf *mbuf);

BAIK_PRIVATE struct baik_object *new_object(struct baik *baik) {
  return (struct baik_object *) gc_alloc_cell(baik, &baik->object_arena);
}

BAIK_PRIVATE struct baik_property *new_property(struct baik *baik) {
  return (struct baik_property *) gc_alloc_cell(baik, &baik->property_arena);
}

BAIK_PRIVATE struct baik_ffi_sig *new_ffi_sig(struct baik *baik) {
  return (struct baik_ffi_sig *) gc_alloc_cell(baik, &baik->ffi_sig_arena);
}

BAIK_PRIVATE void gc_arena_init(struct gc_arena *a, size_t cell_size,
                               size_t initial_size, size_t size_increment) {
  assert(cell_size >= sizeof(uintptr_t));

  memset(a, 0, sizeof(*a));
  a->cell_size = cell_size;
  a->size_increment = size_increment;
  a->blocks = gc_new_block(a, initial_size);
}

BAIK_PRIVATE void gc_arena_destroy(struct baik *baik, struct gc_arena *a) {
  struct gc_block *b;

  if (a->blocks != NULL) {
    gc_sweep(baik, a, 0);
    for (b = a->blocks; b != NULL;) {
      struct gc_block *tmp;
      tmp = b;
      b = b->next;
      gc_free_block(tmp);
    }
  }
}

static void gc_free_block(struct gc_block *b) {
  free(b->base);
  free(b);
}

static struct gc_block *gc_new_block(struct gc_arena *a, size_t size) {
  struct gc_cell *cur;
  struct gc_block *b;

  b = (struct gc_block *) calloc(1, sizeof(*b));
  if (b == NULL) abort();

  b->size = size;
  b->base = (struct gc_cell *) calloc(a->cell_size, b->size);
  if (b->base == NULL) abort();

  for (cur = GC_CELL_OP(a, b->base, +, 0);
       cur < GC_CELL_OP(a, b->base, +, b->size);
       cur = GC_CELL_OP(a, cur, +, 1)) {
    cur->head.link = a->free;
    a->free = cur;
  }

  return b;
}


static int gc_arena_is_gc_needed(struct gc_arena *a) {
  struct gc_cell *r = a->free;
  int i;

  for (i = 0; i <= GC_ARENA_CELLS_RESERVE; i++, r = r->head.link) {
    if (r == NULL) {
      return 1;
    }
  }

  return 0;
}

BAIK_PRIVATE int gc_strings_is_gc_needed(struct baik *baik) {
  struct mbuf *m = &baik->owned_strings;
  return (double) m->len / (double) m->size > 0.9;
}

BAIK_PRIVATE void *gc_alloc_cell(struct baik *baik, struct gc_arena *a) {
  struct gc_cell *r;

  if (a->free == NULL) {
    struct gc_block *b = gc_new_block(a, a->size_increment);
    b->next = a->blocks;
    a->blocks = b;
  }
  r = a->free;

  UNMARK(r);

  a->free = r->head.link;

#if BAIK_MEMORY_STATS
  a->allocations++;
  a->alive++;
#endif

  if (gc_arena_is_gc_needed(a)) {
    baik->need_gc = 1;
  }

  memset(r, 0, a->cell_size);
  return (void *) r;
}


void gc_sweep(struct baik *baik, struct gc_arena *a, size_t start) {
  struct gc_block *b;
  struct gc_cell *cur;
  struct gc_block **prevp = &a->blocks;
#if BAIK_MEMORY_STATS
  a->alive = 0;
#endif

  {
    struct gc_cell *next;
    for (cur = a->free; cur != NULL; cur = next) {
      next = cur->head.link;
      MARK_FREE(cur);
    }
  }

  a->free = NULL;

  for (b = a->blocks; b != NULL;) {
    size_t freed_in_block = 0;
   
    struct gc_cell *prev_free = a->free;

    for (cur = GC_CELL_OP(a, b->base, +, start);
         cur < GC_CELL_OP(a, b->base, +, b->size);
         cur = GC_CELL_OP(a, cur, +, 1)) {
      if (MARKED(cur)) {
        UNMARK(cur);
#if BAIK_MEMORY_STATS
        a->alive++;
#endif
      } else {
        if (MARKED_FREE(cur)) {
         
          UNMARK_FREE(cur);
        } else {
         
          if (a->destructor != NULL) {
            a->destructor(baik, cur);
          }
          memset(cur, 0, a->cell_size);
        }
        cur->head.link = a->free;
        a->free = cur;
        freed_in_block++;
#if BAIK_MEMORY_STATS
        a->garbage++;
#endif
      }
    }

    if (b->next != NULL && freed_in_block == b->size) {
      *prevp = b->next;
      gc_free_block(b);
      b = *prevp;
      a->free = prev_free;
    } else {
      prevp = &b->next;
      b = b->next;
    }
  }
}


// static void gc_mark_ffi_sig(struct baik *baik, baik_val_t *v) {
//   struct baik_ffi_sig *psig;

//   assert(baik_is_ffi_sig(*v));

//   psig = baik_get_ffi_sig_struct(*v);

//   if (!gc_check_val(baik, *v)) {
//     abort();
//   }

//   if (MARKED(psig)) return;

//   MARK(psig);
// }


static void gc_mark_object(struct baik *baik, baik_val_t *v) {
  struct baik_object *obj_base;
  struct baik_property *prop;
  struct baik_property *next;

  assert(baik_is_object(*v));

  obj_base = get_object_struct(*v);

  if (!gc_check_val(baik, *v)) {
    abort();
  }

  if (MARKED(obj_base)) return;

  for ((prop = obj_base->properties), MARK(obj_base); prop != NULL;
       prop = next) {
    if (!gc_check_ptr(&baik->property_arena, prop)) {
      abort();
    }

    gc_mark(baik, &prop->name);
    gc_mark(baik, &prop->value);

    next = prop->next;
    MARK(prop);
  }

}


static void gc_mark_string(struct baik *baik, baik_val_t *v) {
  baik_val_t h, tmp = 0;
  char *s;
  assert((*v & BAIK_TAG_MASK) == BAIK_TAG_STRING_O);

  s = baik->owned_strings.buf + gc_string_baik_val_to_offset(*v);
  assert(s < baik->owned_strings.buf + baik->owned_strings.len);
  if (s[-1] == '\0') {
    memcpy(&tmp, s, sizeof(tmp) - 2);
    tmp |= BAIK_TAG_STRING_C;
  } else {
    memcpy(&tmp, s, sizeof(tmp) - 2);
    tmp |= BAIK_TAG_FOREIGN;
  }

  h = (baik_val_t)(uintptr_t) v;
  s[-1] = 1;
  memcpy(s, &h, sizeof(h) - 2);
  memcpy(v, &tmp, sizeof(tmp));
}

BAIK_PRIVATE void gc_mark(struct baik *baik, baik_val_t *v) {
  if (baik_is_object(*v)) {
    gc_mark_object(baik, v);
  }
  // if (baik_is_ffi_sig(*v)) {
  //   gc_mark_ffi_sig(baik, v);
  // }
  if ((*v & BAIK_TAG_MASK) == BAIK_TAG_STRING_O) {
    gc_mark_string(baik, v);
  }
}

BAIK_PRIVATE uint64_t gc_string_baik_val_to_offset(baik_val_t v) {
  return (((uint64_t)(uintptr_t) get_ptr(v)) & ~BAIK_TAG_MASK);
}

BAIK_PRIVATE baik_val_t gc_string_val_from_offset(uint64_t s) {
  return s | BAIK_TAG_STRING_O;
}

void gc_compact_strings(struct baik *baik) {
  char *p = baik->owned_strings.buf + 1;
  uint64_t h, next, head = 1;
  int len, llen;

  while (p < baik->owned_strings.buf + baik->owned_strings.len) {
    if (p[-1] == '\1') {
     
      h = 0;
      memcpy(&h, p, sizeof(h) - 2);

      for (; (h & BAIK_TAG_MASK) != BAIK_TAG_STRING_C; h = next) {
        h &= ~BAIK_TAG_MASK;
        memcpy(&next, (char *) (uintptr_t) h, sizeof(h));

        *(baik_val_t *) (uintptr_t) h = gc_string_val_from_offset(head);
      }
      h &= ~BAIK_TAG_MASK;

      len = BAIK_EM_varint_decode_unsafe((unsigned char *) &h, &llen);
      len += llen + 1;

      memcpy(p, &h, sizeof(h) - 2);

      memmove(baik->owned_strings.buf + head, p, len);
      baik->owned_strings.buf[head - 1] = 0x0;
      p += len;
      head += len;
    } else {
      len = BAIK_EM_varint_decode_unsafe((unsigned char *) p, &llen);
      len += llen + 1;

      p += len;
    }
  }

  baik->owned_strings.len = head;
}

BAIK_PRIVATE int maybe_gc(struct baik *baik) {
  if (!baik->inhibit_gc) {
    baik_gc(baik, 0);
    return 1;
  }
  return 0;
}


static void gc_mark_val_array(struct baik *baik, baik_val_t *vals, size_t len) {
  baik_val_t *vp;
  for (vp = vals; vp < vals + len; vp++) {
    gc_mark(baik, vp);
  }
}


static void gc_mark_mbuf_pt(struct baik *baik, const struct mbuf *mbuf) {
  baik_val_t **vp;
  for (vp = (baik_val_t **) mbuf->buf; (char *) vp < mbuf->buf + mbuf->len;
       vp++) {
    gc_mark(baik, *vp);
  }
}


static void gc_mark_mbuf_val(struct baik *baik, const struct mbuf *mbuf) {
  gc_mark_val_array(baik, (baik_val_t *) mbuf->buf,
                    mbuf->len / sizeof(baik_val_t));
}

// static void gc_mark_ffi_cbargs_list(struct baik *baik, ffi_cb_args_t *cbargs) {
//   for (; cbargs != NULL; cbargs = cbargs->next) {
//     gc_mark(baik, &cbargs->func);
//     gc_mark(baik, &cbargs->userdata);
//   }
// }


void baik_gc(struct baik *baik, int full) {
  gc_mark_val_array(baik, (baik_val_t *) &baik->vals,
                    sizeof(baik->vals) / sizeof(baik_val_t));

  gc_mark_mbuf_pt(baik, &baik->owned_values);
  gc_mark_mbuf_val(baik, &baik->scopes);
  gc_mark_mbuf_val(baik, &baik->stack);
  gc_mark_mbuf_val(baik, &baik->call_stack);
  //gc_mark_ffi_cbargs_list(baik, baik->ffi_cb_args);
  gc_compact_strings(baik);
  gc_sweep(baik, &baik->object_arena, 0);
  gc_sweep(baik, &baik->property_arena, 0);
  gc_sweep(baik, &baik->ffi_sig_arena, 0);

  if (full) {
    size_t trimmed_size = baik->owned_strings.len + _BAIK_STRING_BUF_RESERVE;
    if (trimmed_size < baik->owned_strings.size) {
      mbuf_resize(&baik->owned_strings, trimmed_size);
    }
  }
}

BAIK_PRIVATE int gc_check_val(struct baik *baik, baik_val_t v) {
  if (baik_is_object(v)) {
    return gc_check_ptr(&baik->object_arena, get_object_struct(v));
  }
  // if (baik_is_ffi_sig(v)) {
  //   return gc_check_ptr(&baik->ffi_sig_arena, baik_get_ffi_sig_struct(v));
  // }
  return 1;
}

BAIK_PRIVATE int gc_check_ptr(const struct gc_arena *a, const void *ptr) {
  const struct gc_cell *p = (const struct gc_cell *) ptr;
  struct gc_block *b;
  for (b = a->blocks; b != NULL; b = b->next) {
    if (p >= b->base && p < GC_CELL_OP(a, b->base, +, b->size)) {
      return 1;
    }
  }
  return 0;
}
