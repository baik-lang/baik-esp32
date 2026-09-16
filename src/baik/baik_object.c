/*
 * baik_object.c - Tipe objek: properti, pencarian, penyetelan, penghapusan, iterasi, prototipe.
 *
 * Bagian dari interpreter bahasa BAIK. Deklarasi bersama ada di
 * src/baik/baik_internal.h; antarmuka publik ada di src/baik.h.
 */
#include "baik_internal.h"

BAIK_PRIVATE baik_val_t baik_object_to_value(struct baik_object *o) {
  if (o == NULL) {
    return BAIK_NULL;
  } else {
    return baik_legit_pointer_to_value(o) | BAIK_TAG_OBJECT;
  }
}

BAIK_PRIVATE struct baik_object *get_object_struct(baik_val_t v) {
  struct baik_object *ret = NULL;
  if (baik_is_null(v)) {
    ret = NULL;
  } else {
    assert(baik_is_object(v));
    ret = (struct baik_object *) get_ptr(v);
  }
  return ret;
}

baik_val_t baik_mk_object(struct baik *baik) {
  struct baik_object *o = new_object(baik);
  if (o == NULL) {
    return BAIK_NULL;
  }
  (void) baik;
  o->properties = NULL;
  return baik_object_to_value(o);
}

int baik_is_object(baik_val_t v) {
  return (v & BAIK_TAG_MASK) == BAIK_TAG_OBJECT ||
         (v & BAIK_TAG_MASK) == BAIK_TAG_ARRAY;
}

BAIK_PRIVATE struct baik_property *baik_get_own_property(struct baik *baik,
                                                      baik_val_t obj,
                                                      const char *name,
                                                      size_t len) {
  struct baik_property *p;
  struct baik_object *o;

  if (!baik_is_object(obj)) {
    return NULL;
  }

  o = get_object_struct(obj);

  if (len <= 5) {
    baik_val_t ss = baik_mk_string(baik, name, len, 1);
    for (p = o->properties; p != NULL; p = p->next) {
      if (p->name == ss) return p;
    }
  } else {
    for (p = o->properties; p != NULL; p = p->next) {
      if (baik_strcmp(baik, &p->name, name, len) == 0) return p;
    }
    return p;
  }

  return NULL;
}

BAIK_PRIVATE struct baik_property *baik_get_own_property_v(struct baik *baik,
                                                        baik_val_t obj,
                                                        baik_val_t key) {
  size_t n;
  char *s = NULL;
  int need_free = 0;
  struct baik_property *p = NULL;
  baik_err_t err = baik_to_string(baik, &key, &s, &n, &need_free);
  if (err == BAIK_OK) {
    p = baik_get_own_property(baik, obj, s, n);
  }
  if (need_free) free(s);
  return p;
}

BAIK_PRIVATE struct baik_property *baik_mk_property(struct baik *baik,
                                                 baik_val_t name,
                                                 baik_val_t value) {
  struct baik_property *p = new_property(baik);
  p->next = NULL;
  p->name = name;
  p->value = value;
  return p;
}

baik_val_t baik_get(struct baik *baik, baik_val_t obj, const char *name,
                  size_t name_len) {
  struct baik_property *p;

  if (name_len == (size_t) ~0) {
    name_len = strlen(name);
  }

  p = baik_get_own_property(baik, obj, name, name_len);
  if (p == NULL) {
    return BAIK_UNDEFINED;
  } else {
    return p->value;
  }
}

baik_val_t baik_get_v(struct baik *baik, baik_val_t obj, baik_val_t name) {
  size_t n;
  char *s = NULL;
  int need_free = 0;
  baik_val_t ret = BAIK_UNDEFINED;

  baik_err_t err = baik_to_string(baik, &name, &s, &n, &need_free);

  if (err == BAIK_OK) {
   
    ret = baik_get(baik, obj, s, n);
  }

  if (need_free) {
    free(s);
    s = NULL;
  }
  return ret;
}

baik_val_t baik_get_v_proto(struct baik *baik, baik_val_t obj, baik_val_t key) {
  struct baik_property *p;
  baik_val_t pn = baik_mk_string(baik, BAIK_PROTO_PROP_NAME, ~0, 1);
  if ((p = baik_get_own_property_v(baik, obj, key)) != NULL) return p->value;
  if ((p = baik_get_own_property_v(baik, obj, pn)) == NULL) return BAIK_UNDEFINED;
  return baik_get_v_proto(baik, p->value, key);
}

baik_err_t baik_set(struct baik *baik, baik_val_t obj, const char *name,
                  size_t name_len, baik_val_t val) {
  return baik_set_internal(baik, obj, BAIK_UNDEFINED, (char *) name, name_len,
                          val);
}

baik_err_t baik_set_v(struct baik *baik, baik_val_t obj, baik_val_t name,
                    baik_val_t val) {
  return baik_set_internal(baik, obj, name, NULL, 0, val);
}

BAIK_PRIVATE baik_err_t baik_set_internal(struct baik *baik, baik_val_t obj,
                                       baik_val_t name_v, char *name,
                                       size_t name_len, baik_val_t val) {
  baik_err_t rcode = BAIK_OK;
  struct baik_property *p;
  int need_free = 0;

  if (name == NULL) {
    rcode = baik_to_string(baik, &name_v, &name, &name_len, &need_free);
    if (rcode != BAIK_OK) {
      goto clean;
    }
  } else {
    name_v = BAIK_UNDEFINED;
  }

  p = baik_get_own_property(baik, obj, name, name_len);

  if (p == NULL) {
    struct baik_object *o;
    if (!baik_is_object(obj)) {
      return BAIK_REFERENCE_ERROR;
    }

    if (!baik_is_string(name_v)) {
      name_v = baik_mk_string(baik, name, name_len, 1);
    }

    p = baik_mk_property(baik, name_v, val);

    o = get_object_struct(obj);
    p->next = o->properties;
    o->properties = p;
  }

  p->value = val;

clean:
  if (need_free) {
    free(name);
    name = NULL;
  }
  return rcode;
}

BAIK_PRIVATE void baik_destroy_property(struct baik_property **p) {
  *p = NULL;
}


int baik_del(struct baik *baik, baik_val_t obj, const char *name, size_t len) {
  struct baik_property *prop, *prev;

  if (!baik_is_object(obj)) {
    return -1;
  }
  if (len == (size_t) ~0) {
    len = strlen(name);
  }
  for (prev = NULL, prop = get_object_struct(obj)->properties; prop != NULL;
       prev = prop, prop = prop->next) {
    size_t n;
    const char *s = baik_get_string(baik, &prop->name, &n);
    if (n == len && strncmp(s, name, len) == 0) {
      if (prev) {
        prev->next = prop->next;
      } else {
        get_object_struct(obj)->properties = prop->next;
      }
      baik_destroy_property(&prop);
      return 0;
    }
  }
  return -1;
}

baik_val_t baik_next(struct baik *baik, baik_val_t obj, baik_val_t *iterator) {
  struct baik_property *p = NULL;
  baik_val_t key = BAIK_UNDEFINED;

  if (*iterator == BAIK_UNDEFINED) {
    struct baik_object *o = get_object_struct(obj);
    p = o->properties;
  } else {
    p = ((struct baik_property *) get_ptr(*iterator))->next;
  }

  if (p == NULL) {
    *iterator = BAIK_UNDEFINED;
  } else {
    key = p->name;
    *iterator = baik_mk_foreign(baik, p);
  }

  return key;
}

BAIK_PRIVATE void baik_op_create_object(struct baik *baik) {
  baik_val_t ret = BAIK_UNDEFINED;
  baik_val_t proto_v = baik_arg(baik, 0);

  if (!baik_check_arg(baik, 0, "proto", BAIK_TYPE_OBJECT_GENERIC, &proto_v)) {
    goto clean;
  }

  ret = baik_mk_object(baik);
  baik_set(baik, ret, BAIK_PROTO_PROP_NAME, ~0, proto_v);

clean:
  baik_return(baik, ret);
}

baik_val_t baik_struct_to_obj(struct baik *baik, const void *base,
                            const struct baik_c_struct_member *defs) {
  baik_val_t obj;
  const struct baik_c_struct_member *def = defs;
  if (base == NULL || def == NULL) return BAIK_UNDEFINED;
  obj = baik_mk_object(baik);
 
  baik_own(baik, &obj);
 
  while (def->name != NULL) def++;
  for (def--; def >= defs; def--) {
    baik_val_t v = BAIK_UNDEFINED;
    const char *ptr = (const char *) base + def->offset;
    switch (def->type) {
      case BAIK_STRUCT_FIELD_TYPE_STRUCT: {
        const void *sub_base = (const void *) ptr;
        const struct baik_c_struct_member *sub_def =
            (const struct baik_c_struct_member *) def->arg;
        v = baik_struct_to_obj(baik, sub_base, sub_def);
        break;
      }
      case BAIK_STRUCT_FIELD_TYPE_STRUCT_PTR: {
        const void **sub_base = (const void **) ptr;
        const struct baik_c_struct_member *sub_def =
            (const struct baik_c_struct_member *) def->arg;
        if (*sub_base != NULL) {
          v = baik_struct_to_obj(baik, *sub_base, sub_def);
        } else {
          v = BAIK_NULL;
        }
        break;
      }
      case BAIK_STRUCT_FIELD_TYPE_INT: {
        double value = (double) (*(int *) ptr);
        v = baik_mk_number(baik, value);
        break;
      }
      case BAIK_STRUCT_FIELD_TYPE_BOOL: {
        v = baik_mk_boolean(baik, *(bool *) ptr);
        break;
      }
      case BAIK_STRUCT_FIELD_TYPE_DOUBLE: {
        v = baik_mk_number(baik, *(double *) ptr);
        break;
      }
      case BAIK_STRUCT_FIELD_TYPE_FLOAT: {
        float value = *(float *) ptr;
        v = baik_mk_number(baik, value);
        break;
      }
      case BAIK_STRUCT_FIELD_TYPE_CHAR_PTR: {
        const char *value = *(const char **) ptr;
        v = baik_mk_string(baik, value, ~0, 1);
        break;
      }
      case BAIK_STRUCT_FIELD_TYPE_VOID_PTR: {
        v = baik_mk_foreign(baik, *(void **) ptr);
        break;
      }
      case BAIK_STRUCT_FIELD_TYPE_GENERIC_STR_PTR: {
        const struct baik_generic_str *s = *(const struct baik_generic_str **) ptr;
        if (s != NULL) {
          v = baik_mk_string(baik, s->p, s->len, 1);
        } else {
          v = BAIK_NULL;
        }
        break;
      }
      case BAIK_STRUCT_FIELD_TYPE_GENERIC_STR: {
        const struct baik_generic_str *s = (const struct baik_generic_str *) ptr;
        v = baik_mk_string(baik, s->p, s->len, 1);
        break;
      }
      case BAIK_STRUCT_FIELD_TYPE_DATA: {
        const char *dptr = (const char *) ptr;
        const intptr_t dlen = (intptr_t) def->arg;
        v = baik_mk_string(baik, dptr, dlen, 1);
        break;
      }
      case BAIK_STRUCT_FIELD_TYPE_INT8: {
        double value = (double) (*(int8_t *) ptr);
        v = baik_mk_number(baik, value);
        break;
      }
      case BAIK_STRUCT_FIELD_TYPE_INT16: {
        double value = (double) (*(int16_t *) ptr);
        v = baik_mk_number(baik, value);
        break;
      }
      case BAIK_STRUCT_FIELD_TYPE_UINT8: {
        double value = (double) (*(uint8_t *) ptr);
        v = baik_mk_number(baik, value);
        break;
      }
      case BAIK_STRUCT_FIELD_TYPE_UINT16: {
        double value = (double) (*(uint16_t *) ptr);
        v = baik_mk_number(baik, value);
        break;
      }
      case BAIK_STRUCT_FIELD_TYPE_CUSTOM: {
        baik_val_t (*fptr)(struct baik *, const void *) =
            (baik_val_t (*) (struct baik *, const void *)) def->arg;
        v = fptr(baik, ptr);
      }
      default: { break; }
    }
    baik_set(baik, obj, def->name, ~0, v);
  }
  baik_disown(baik, &obj);
  return obj;
}

