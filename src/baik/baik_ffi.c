/*
 * baik_ffi.c - Antarmuka fungsi asing (FFI). Sebagian besar dinonaktifkan pada build ini.
 *
 * Bagian dari interpreter bahasa BAIK. Deklarasi bersama ada di
 * src/baik/baik_internal.h; antarmuka publik ada di src/baik.h.
 */
#include "baik_internal.h"

/* Makro pembantu untuk membongkar struct ffi_arg.
 *
 * Ketika interpreter masih satu berkas, makro ini berada tepat sebelum
 * pemakaiannya. Saat dipecah, ia terbawa ke modul tetangga sehingga berkas ini
 * gagal ditaut - tetapi hanya pada build yang TIDAK memakai --gc-sections,
 * karena kode FFI di build ESP32 memang dibuang penaut. */
#define IS_W(arg) ((arg).ctype == FFI_CTYPE_WORD)
#define IS_D(arg) ((arg).ctype == FFI_CTYPE_DOUBLE)
#define IS_F(arg) ((arg).ctype == FFI_CTYPE_FLOAT)

#define W(arg) ((ffi_word_t)(arg).v.i)
#define D(arg) ((arg).v.d)
#define F(arg) ((arg).v.f)

/* Satu-satunya definisi baik_dlsym_file (dideklarasikan extern di
 * baik_internal.h). Saat ini hanya dipakai oleh kode FFI yang dinonaktifkan,
 * tetapi disediakan di sini supaya kode itu langsung menaut bila diaktifkan. */
const char *baik_dlsym_file;

void ffi_set_word(struct ffi_arg *arg, ffi_word_t v) {
  arg->ctype = FFI_CTYPE_WORD;
  arg->v.i = v;
}

void ffi_set_bool(struct ffi_arg *arg, bool v) {
  arg->ctype = FFI_CTYPE_BOOL;
  arg->v.i = v;
}

void ffi_set_ptr(struct ffi_arg *arg, void *v) {
  ffi_set_word(arg, (ffi_word_t) v);
}

void ffi_set_double(struct ffi_arg *arg, double v) {
  arg->ctype = FFI_CTYPE_DOUBLE;
  arg->v.d = v;
}

void ffi_set_float(struct ffi_arg *arg, float v) {
  arg->ctype = FFI_CTYPE_FLOAT;
  arg->v.f = v;
}

typedef ffi_word_t (*w4w_t)(ffi_word_t, ffi_word_t, ffi_word_t, ffi_word_t);
typedef ffi_word_t (*w5w_t)(ffi_word_t, ffi_word_t, ffi_word_t, ffi_word_t,
                            ffi_word_t);
typedef ffi_word_t (*w6w_t)(ffi_word_t, ffi_word_t, ffi_word_t, ffi_word_t,
                            ffi_word_t, ffi_word_t);
typedef ffi_word_t (*wdw_t)(double, ffi_word_t);
typedef ffi_word_t (*wwd_t)(ffi_word_t, double);
typedef ffi_word_t (*wdd_t)(double, double);
typedef ffi_word_t (*wwwd_t)(ffi_word_t, ffi_word_t, double);
typedef ffi_word_t (*wwdw_t)(ffi_word_t, double, ffi_word_t);
typedef ffi_word_t (*wwdd_t)(ffi_word_t, double, double);
typedef ffi_word_t (*wdww_t)(double, ffi_word_t, ffi_word_t);
typedef ffi_word_t (*wdwd_t)(double, ffi_word_t, double);
typedef ffi_word_t (*wddw_t)(double, double, ffi_word_t);
typedef ffi_word_t (*wddd_t)(double, double, double);
typedef ffi_word_t (*wfw_t)(float, ffi_word_t);
typedef ffi_word_t (*wwf_t)(ffi_word_t, float);
typedef ffi_word_t (*wff_t)(float, float);
typedef ffi_word_t (*wwwf_t)(ffi_word_t, ffi_word_t, float);
typedef ffi_word_t (*wwfw_t)(ffi_word_t, float, ffi_word_t);
typedef ffi_word_t (*wwff_t)(ffi_word_t, float, float);
typedef ffi_word_t (*wfww_t)(float, ffi_word_t, ffi_word_t);
typedef ffi_word_t (*wfwf_t)(float, ffi_word_t, float);
typedef ffi_word_t (*wffw_t)(float, float, ffi_word_t);
typedef ffi_word_t (*wfff_t)(float, float, float);

typedef bool (*b4w_t)(ffi_word_t, ffi_word_t, ffi_word_t, ffi_word_t);
typedef bool (*b5w_t)(ffi_word_t, ffi_word_t, ffi_word_t, ffi_word_t,
                      ffi_word_t);
typedef bool (*b6w_t)(ffi_word_t, ffi_word_t, ffi_word_t, ffi_word_t,
                      ffi_word_t, ffi_word_t);
typedef bool (*bdw_t)(double, ffi_word_t);
typedef bool (*bwd_t)(ffi_word_t, double);
typedef bool (*bdd_t)(double, double);
typedef bool (*bwwd_t)(ffi_word_t, ffi_word_t, double);
typedef bool (*bwdw_t)(ffi_word_t, double, ffi_word_t);
typedef bool (*bwdd_t)(ffi_word_t, double, double);
typedef bool (*bdww_t)(double, ffi_word_t, ffi_word_t);
typedef bool (*bdwd_t)(double, ffi_word_t, double);
typedef bool (*bddw_t)(double, double, ffi_word_t);
typedef bool (*bddd_t)(double, double, double);
typedef bool (*bfw_t)(float, ffi_word_t);
typedef bool (*bwf_t)(ffi_word_t, float);
typedef bool (*bff_t)(float, float);
typedef bool (*bwwf_t)(ffi_word_t, ffi_word_t, float);
typedef bool (*bwfw_t)(ffi_word_t, float, ffi_word_t);
typedef bool (*bwff_t)(ffi_word_t, float, float);
typedef bool (*bfww_t)(float, ffi_word_t, ffi_word_t);
typedef bool (*bfwf_t)(float, ffi_word_t, float);
typedef bool (*bffw_t)(float, float, ffi_word_t);
typedef bool (*bfff_t)(float, float, float);

typedef double (*d4w_t)(ffi_word_t, ffi_word_t, ffi_word_t, ffi_word_t);
typedef double (*d5w_t)(ffi_word_t, ffi_word_t, ffi_word_t, ffi_word_t,
                        ffi_word_t);
typedef double (*d6w_t)(ffi_word_t, ffi_word_t, ffi_word_t, ffi_word_t,
                        ffi_word_t, ffi_word_t);
typedef double (*ddw_t)(double, ffi_word_t);
typedef double (*dwd_t)(ffi_word_t, double);
typedef double (*ddd_t)(double, double);
typedef double (*dwwd_t)(ffi_word_t, ffi_word_t, double);
typedef double (*dwdw_t)(ffi_word_t, double, ffi_word_t);
typedef double (*dwdd_t)(ffi_word_t, double, double);
typedef double (*ddww_t)(double, ffi_word_t, ffi_word_t);
typedef double (*ddwd_t)(double, ffi_word_t, double);
typedef double (*dddw_t)(double, double, ffi_word_t);
typedef double (*dddd_t)(double, double, double);

typedef float (*f4w_t)(ffi_word_t, ffi_word_t, ffi_word_t, ffi_word_t);
typedef float (*f5w_t)(ffi_word_t, ffi_word_t, ffi_word_t, ffi_word_t,
                       ffi_word_t);
typedef float (*f6w_t)(ffi_word_t, ffi_word_t, ffi_word_t, ffi_word_t,
                       ffi_word_t, ffi_word_t);
typedef float (*ffw_t)(float, ffi_word_t);
typedef float (*fwf_t)(ffi_word_t, float);
typedef float (*fff_t)(float, float);
typedef float (*fwwf_t)(ffi_word_t, ffi_word_t, float);
typedef float (*fwfw_t)(ffi_word_t, float, ffi_word_t);
typedef float (*fwff_t)(ffi_word_t, float, float);
typedef float (*ffww_t)(float, ffi_word_t, ffi_word_t);
typedef float (*ffwf_t)(float, ffi_word_t, float);
typedef float (*fffw_t)(float, float, ffi_word_t);
typedef float (*ffff_t)(float, float, float);

int ffi_call(ffi_fn_t *func, int nargs, struct ffi_arg *res,
             struct ffi_arg *args) {
  int i, doubles = 0, floats = 0;

  if (nargs > 6) return -1;
  for (i = 0; i < nargs; i++) {
    doubles += (IS_D(args[i]));
    floats += (IS_F(args[i]));
  }

  if (doubles > 0 && floats > 0) {
    return -1;
  }

  switch (res->ctype) {
    case FFI_CTYPE_WORD: {
      ffi_word_t r;
      if (doubles == 0) {
        if (floats == 0) {
         
          if (nargs <= 4) {
            w4w_t f = (w4w_t) func;
            r = f(W(args[0]), W(args[1]), W(args[2]), W(args[3]));
          } else if (nargs == 5) {
            w5w_t f = (w5w_t) func;
            r = f(W(args[0]), W(args[1]), W(args[2]), W(args[3]), W(args[4]));
          } else if (nargs == 6) {
            w6w_t f = (w6w_t) func;
            r = f(W(args[0]), W(args[1]), W(args[2]), W(args[3]), W(args[4]),
                  W(args[5]));
          } else {
            abort();
          }
        } else {
         
          switch (nargs) {
            case 0:
            case 1:
            case 2:
              if (IS_F(args[0]) && IS_F(args[1])) {
                wff_t f = (wff_t) func;
                r = f(F(args[0]), F(args[1]));
              } else if (IS_F(args[0])) {
                wfw_t f = (wfw_t) func;
                r = f(F(args[0]), W(args[1]));
              } else {
                wwf_t f = (wwf_t) func;
                r = f(W(args[0]), F(args[1]));
              }
              break;

            case 3:
              if (IS_W(args[0]) && IS_W(args[1]) && IS_F(args[2])) {
                wwwf_t f = (wwwf_t) func;
                r = f(W(args[0]), W(args[1]), F(args[2]));
              } else if (IS_W(args[0]) && IS_F(args[1]) && IS_W(args[2])) {
                wwfw_t f = (wwfw_t) func;
                r = f(W(args[0]), F(args[1]), W(args[2]));
              } else if (IS_W(args[0]) && IS_F(args[1]) && IS_F(args[2])) {
                wwff_t f = (wwff_t) func;
                r = f(W(args[0]), F(args[1]), F(args[2]));
              } else if (IS_F(args[0]) && IS_W(args[1]) && IS_W(args[2])) {
                wfww_t f = (wfww_t) func;
                r = f(F(args[0]), W(args[1]), W(args[2]));
              } else if (IS_F(args[0]) && IS_W(args[1]) && IS_F(args[2])) {
                wfwf_t f = (wfwf_t) func;
                r = f(F(args[0]), W(args[1]), F(args[2]));
              } else if (IS_F(args[0]) && IS_F(args[1]) && IS_W(args[2])) {
                wffw_t f = (wffw_t) func;
                r = f(F(args[0]), F(args[1]), W(args[2]));
              } else if (IS_F(args[0]) && IS_F(args[1]) && IS_F(args[2])) {
                wfff_t f = (wfff_t) func;
                r = f(F(args[0]), F(args[1]), F(args[2]));
              } else {
                
                abort();
              }
              break;
            default:
              return -1;
          }
        }
      } else {
        switch (nargs) {
          case 0:
          case 1:
          case 2:
            if (IS_D(args[0]) && IS_D(args[1])) {
              wdd_t f = (wdd_t) func;
              r = f(D(args[0]), D(args[1]));
            } else if (IS_D(args[0])) {
              wdw_t f = (wdw_t) func;
              r = f(D(args[0]), W(args[1]));
            } else {
              wwd_t f = (wwd_t) func;
              r = f(W(args[0]), D(args[1]));
            }
            break;

          case 3:
            if (IS_W(args[0]) && IS_W(args[1]) && IS_D(args[2])) {
              wwwd_t f = (wwwd_t) func;
              r = f(W(args[0]), W(args[1]), D(args[2]));
            } else if (IS_W(args[0]) && IS_D(args[1]) && IS_W(args[2])) {
              wwdw_t f = (wwdw_t) func;
              r = f(W(args[0]), D(args[1]), W(args[2]));
            } else if (IS_W(args[0]) && IS_D(args[1]) && IS_D(args[2])) {
              wwdd_t f = (wwdd_t) func;
              r = f(W(args[0]), D(args[1]), D(args[2]));
            } else if (IS_D(args[0]) && IS_W(args[1]) && IS_W(args[2])) {
              wdww_t f = (wdww_t) func;
              r = f(D(args[0]), W(args[1]), W(args[2]));
            } else if (IS_D(args[0]) && IS_W(args[1]) && IS_D(args[2])) {
              wdwd_t f = (wdwd_t) func;
              r = f(D(args[0]), W(args[1]), D(args[2]));
            } else if (IS_D(args[0]) && IS_D(args[1]) && IS_W(args[2])) {
              wddw_t f = (wddw_t) func;
              r = f(D(args[0]), D(args[1]), W(args[2]));
            } else if (IS_D(args[0]) && IS_D(args[1]) && IS_D(args[2])) {
              wddd_t f = (wddd_t) func;
              r = f(D(args[0]), D(args[1]), D(args[2]));
            } else {
              
              abort();
            }
            break;
          default:
            return -1;
        }
      }
      res->v.i = (uint64_t) r;
    } break;              
    case FFI_CTYPE_BOOL: {
      ffi_word_t r;
      if (doubles == 0) {
        if (floats == 0) {
         
          if (nargs <= 4) {
            b4w_t f = (b4w_t) func;
            r = f(W(args[0]), W(args[1]), W(args[2]), W(args[3]));
          } else if (nargs == 5) {
            b5w_t f = (b5w_t) func;
            r = f(W(args[0]), W(args[1]), W(args[2]), W(args[3]), W(args[4]));
          } else if (nargs == 6) {
            b6w_t f = (b6w_t) func;
            r = f(W(args[0]), W(args[1]), W(args[2]), W(args[3]), W(args[4]),
                  W(args[5]));
          } else {
            abort();
          }
        } else {
         
          switch (nargs) {
            case 0:
            case 1:
            case 2:
              if (IS_F(args[0]) && IS_F(args[1])) {
                bff_t f = (bff_t) func;
                r = f(F(args[0]), F(args[1]));
              } else if (IS_F(args[0])) {
                bfw_t f = (bfw_t) func;
                r = f(F(args[0]), W(args[1]));
              } else {
                bwf_t f = (bwf_t) func;
                r = f(W(args[0]), F(args[1]));
              }
              break;

            case 3:
              if (IS_W(args[0]) && IS_W(args[1]) && IS_F(args[2])) {
                bwwf_t f = (bwwf_t) func;
                r = f(W(args[0]), W(args[1]), F(args[2]));
              } else if (IS_W(args[0]) && IS_F(args[1]) && IS_W(args[2])) {
                bwfw_t f = (bwfw_t) func;
                r = f(W(args[0]), F(args[1]), W(args[2]));
              } else if (IS_W(args[0]) && IS_F(args[1]) && IS_F(args[2])) {
                bwff_t f = (bwff_t) func;
                r = f(W(args[0]), F(args[1]), F(args[2]));
              } else if (IS_F(args[0]) && IS_W(args[1]) && IS_W(args[2])) {
                bfww_t f = (bfww_t) func;
                r = f(F(args[0]), W(args[1]), W(args[2]));
              } else if (IS_F(args[0]) && IS_W(args[1]) && IS_F(args[2])) {
                bfwf_t f = (bfwf_t) func;
                r = f(F(args[0]), W(args[1]), F(args[2]));
              } else if (IS_F(args[0]) && IS_F(args[1]) && IS_W(args[2])) {
                bffw_t f = (bffw_t) func;
                r = f(F(args[0]), F(args[1]), W(args[2]));
              } else if (IS_F(args[0]) && IS_F(args[1]) && IS_F(args[2])) {
                bfff_t f = (bfff_t) func;
                r = f(F(args[0]), F(args[1]), F(args[2]));
              } else {
                
                abort();
              }
              break;
            default:
              return -1;
          }
        }
      } else {
       
        switch (nargs) {
          case 0:
          case 1:
          case 2:
            if (IS_D(args[0]) && IS_D(args[1])) {
              bdd_t f = (bdd_t) func;
              r = f(D(args[0]), D(args[1]));
            } else if (IS_D(args[0])) {
              bdw_t f = (bdw_t) func;
              r = f(D(args[0]), W(args[1]));
            } else {
              bwd_t f = (bwd_t) func;
              r = f(W(args[0]), D(args[1]));
            }
            break;

          case 3:
            if (IS_W(args[0]) && IS_W(args[1]) && IS_D(args[2])) {
              bwwd_t f = (bwwd_t) func;
              r = f(W(args[0]), W(args[1]), D(args[2]));
            } else if (IS_W(args[0]) && IS_D(args[1]) && IS_W(args[2])) {
              bwdw_t f = (bwdw_t) func;
              r = f(W(args[0]), D(args[1]), W(args[2]));
            } else if (IS_W(args[0]) && IS_D(args[1]) && IS_D(args[2])) {
              bwdd_t f = (bwdd_t) func;
              r = f(W(args[0]), D(args[1]), D(args[2]));
            } else if (IS_D(args[0]) && IS_W(args[1]) && IS_W(args[2])) {
              bdww_t f = (bdww_t) func;
              r = f(D(args[0]), W(args[1]), W(args[2]));
            } else if (IS_D(args[0]) && IS_W(args[1]) && IS_D(args[2])) {
              bdwd_t f = (bdwd_t) func;
              r = f(D(args[0]), W(args[1]), D(args[2]));
            } else if (IS_D(args[0]) && IS_D(args[1]) && IS_W(args[2])) {
              bddw_t f = (bddw_t) func;
              r = f(D(args[0]), D(args[1]), W(args[2]));
            } else if (IS_D(args[0]) && IS_D(args[1]) && IS_D(args[2])) {
              bddd_t f = (bddd_t) func;
              r = f(D(args[0]), D(args[1]), D(args[2]));
            } else {
              
              abort();
            }
            break;
          default:
            return -1;
        }
      }
      res->v.i = (uint64_t) r;
    } break;                
    case FFI_CTYPE_DOUBLE: {
      double r;
      if (doubles == 0) {
       
        if (nargs <= 4) {
          d4w_t f = (d4w_t) func;
          r = f(W(args[0]), W(args[1]), W(args[2]), W(args[3]));
        } else if (nargs == 5) {
          d5w_t f = (d5w_t) func;
          r = f(W(args[0]), W(args[1]), W(args[2]), W(args[3]), W(args[4]));
        } else if (nargs == 6) {
          d6w_t f = (d6w_t) func;
          r = f(W(args[0]), W(args[1]), W(args[2]), W(args[3]), W(args[4]),
                W(args[5]));
        } else {
          abort();
        }
      } else {
        switch (nargs) {
          case 0:
          case 1:
          case 2:
            if (IS_D(args[0]) && IS_D(args[1])) {
              ddd_t f = (ddd_t) func;
              r = f(D(args[0]), D(args[1]));
            } else if (IS_D(args[0])) {
              ddw_t f = (ddw_t) func;
              r = f(D(args[0]), W(args[1]));
            } else {
              dwd_t f = (dwd_t) func;
              r = f(W(args[0]), D(args[1]));
            }
            break;

          case 3:
            if (IS_W(args[0]) && IS_W(args[1]) && IS_D(args[2])) {
              dwwd_t f = (dwwd_t) func;
              r = f(W(args[0]), W(args[1]), D(args[2]));
            } else if (IS_W(args[0]) && IS_D(args[1]) && IS_W(args[2])) {
              dwdw_t f = (dwdw_t) func;
              r = f(W(args[0]), D(args[1]), W(args[2]));
            } else if (IS_W(args[0]) && IS_D(args[1]) && IS_D(args[2])) {
              dwdd_t f = (dwdd_t) func;
              r = f(W(args[0]), D(args[1]), D(args[2]));
            } else if (IS_D(args[0]) && IS_W(args[1]) && IS_W(args[2])) {
              ddww_t f = (ddww_t) func;
              r = f(D(args[0]), W(args[1]), W(args[2]));
            } else if (IS_D(args[0]) && IS_W(args[1]) && IS_D(args[2])) {
              ddwd_t f = (ddwd_t) func;
              r = f(D(args[0]), W(args[1]), D(args[2]));
            } else if (IS_D(args[0]) && IS_D(args[1]) && IS_W(args[2])) {
              dddw_t f = (dddw_t) func;
              r = f(D(args[0]), D(args[1]), W(args[2]));
            } else if (IS_D(args[0]) && IS_D(args[1]) && IS_D(args[2])) {
              dddd_t f = (dddd_t) func;
              r = f(D(args[0]), D(args[1]), D(args[2]));
            } else {
              
              abort();
            }
            break;
          default:
            return -1;
        }
      }
      res->v.d = r;
    } break;               
    case FFI_CTYPE_FLOAT: {
      double r;
      if (floats == 0) {
       
        if (nargs <= 4) {
          f4w_t f = (f4w_t) func;
          r = f(W(args[0]), W(args[1]), W(args[2]), W(args[3]));
        } else if (nargs == 5) {
          f5w_t f = (f5w_t) func;
          r = f(W(args[0]), W(args[1]), W(args[2]), W(args[3]), W(args[4]));
        } else if (nargs == 6) {
          f6w_t f = (f6w_t) func;
          r = f(W(args[0]), W(args[1]), W(args[2]), W(args[3]), W(args[4]),
                W(args[5]));
        } else {
          abort();
        }
      } else {
       
        switch (nargs) {
          case 0:
          case 1:
          case 2:
            if (IS_F(args[0]) && IS_F(args[1])) {
              fff_t f = (fff_t) func;
              r = f(F(args[0]), F(args[1]));
            } else if (IS_F(args[0])) {
              ffw_t f = (ffw_t) func;
              r = f(F(args[0]), W(args[1]));
            } else {
              fwf_t f = (fwf_t) func;
              r = f(W(args[0]), F(args[1]));
            }
            break;

          case 3:
            if (IS_W(args[0]) && IS_W(args[1]) && IS_F(args[2])) {
              fwwf_t f = (fwwf_t) func;
              r = f(W(args[0]), W(args[1]), F(args[2]));
            } else if (IS_W(args[0]) && IS_F(args[1]) && IS_W(args[2])) {
              fwfw_t f = (fwfw_t) func;
              r = f(W(args[0]), F(args[1]), W(args[2]));
            } else if (IS_W(args[0]) && IS_F(args[1]) && IS_F(args[2])) {
              fwff_t f = (fwff_t) func;
              r = f(W(args[0]), F(args[1]), F(args[2]));
            } else if (IS_F(args[0]) && IS_W(args[1]) && IS_W(args[2])) {
              ffww_t f = (ffww_t) func;
              r = f(F(args[0]), W(args[1]), W(args[2]));
            } else if (IS_F(args[0]) && IS_W(args[1]) && IS_F(args[2])) {
              ffwf_t f = (ffwf_t) func;
              r = f(F(args[0]), W(args[1]), F(args[2]));
            } else if (IS_F(args[0]) && IS_F(args[1]) && IS_W(args[2])) {
              fffw_t f = (fffw_t) func;
              r = f(F(args[0]), F(args[1]), W(args[2]));
            } else if (IS_F(args[0]) && IS_F(args[1]) && IS_F(args[2])) {
              ffff_t f = (ffff_t) func;
              r = f(F(args[0]), F(args[1]), F(args[2]));
            } else {
              
              abort();
            }
            break;
          default:
            return -1;
        }
      }
      res->v.f = r;
    } break;
  }

  return 0;
}

static const char *find_paren(const char *s, const char *e) {
  for (; s < e; s++) {
    if (*s == '(') return s;
  }
  return NULL;
}

static const char *find_closing_paren(const char *s, const char *e) {
  int nesting = 1;
  while (s < e) {
    if (*s == '(') {
      nesting++;
    } else if (*s == ')') {
      if (--nesting == 0) break;
    }
    s++;
  }
  return (s < e ? s : NULL);
}

// BAIK_PRIVATE baik_err_t baik_parse_ffi_signature(struct baik *baik, const char *s,
//                                               int sig_len, baik_ffi_sig_t *sig,
//                                               enum ffi_sig_type sig_type) {
//   baik_err_t ret = BAIK_OK;
//   int vtidx = 0;
//   const char *cur, *e, *tmp_e, *tmp;
//   struct baik_generic_str rt = GENERIC_NULL_STR, fn = GENERIC_NULL_STR, args = GENERIC_NULL_STR;
//   baik_ffi_ctype_t val_type = BAIK_FFI_CTYPE_INVALID;
//   if (sig_len == ~0) {
//     sig_len = strlen(s);
//   }
//   e = s + sig_len;

//   baik_ffi_sig_init(sig);
//   for (cur = s; cur < e && isspace((int) *cur); cur++);

//   tmp_e = find_paren(cur, e);
//   if (tmp_e == NULL || tmp_e - s < 2) {
//     ret = BAIK_TYPE_ERROR;
//     baik_prepend_errorf(baik, ret, "1");
//     goto clean;
//   }
//   tmp = find_closing_paren(tmp_e + 1, e);
//   if (tmp == NULL) {
//     ret = BAIK_TYPE_ERROR;
//     baik_prepend_errorf(baik, ret, "2");
//     goto clean;
//   }

//   args.p = find_paren(tmp + 1, e);
//   if (args.p == NULL) {
   
//     fn.p = tmp_e - 1;
//     while (fn.p > cur && isspace((int) *fn.p)) fn.p--;
//     while (fn.p > cur && (isalnum((int) *fn.p) || *fn.p == '_')) {
//       fn.p--;
//       fn.len++;
//     }
//     fn.p++;
//     rt.p = cur;
//     rt.len = fn.p - rt.p;
   
//     args.p = tmp_e + 1;
//     args.len = tmp - args.p;
//   } else {
   
//     fn.p = tmp + 1;
//     fn.len = args.p - tmp;
//     rt.p = cur;
//     rt.len = tmp_e - rt.p;
//     args.p++;
//     tmp = find_closing_paren(args.p, e);
//     if (tmp == NULL) {
//       ret = BAIK_TYPE_ERROR;
//       baik_prepend_errorf(baik, ret, "3");
//       goto clean;
//     }
//     args.len = tmp - args.p;
   
//     sig->is_callback = 1;
//   }

//   val_type = parse_cval_type(baik, rt.p, rt.p + rt.len);
//   if (val_type == BAIK_FFI_CTYPE_INVALID) {
//     ret = baik->error;
//     goto clean;
//   }
//   baik_ffi_sig_set_val_type(sig, vtidx++, val_type);

//   if (!sig->is_callback) {
//     char buf[100];
//     if (baik->dlsym == NULL) {
//       ret = BAIK_TYPE_ERROR;
//       baik_prepend_errorf(baik, ret,
//                          "GALAT : resolver belum diaktifkan, panggil baik_set_ffi_resolver");
//       goto clean;
//     }

//     snprintf(buf, sizeof(buf), "%.*s", (int) fn.len, fn.p);
//     sig->fn = (ffi_fn_t *) baik->dlsym(RTLD_DEFAULT, buf);
//     if (sig->fn == NULL) {
//       ret = BAIK_TYPE_ERROR;
//       baik_prepend_errorf(baik, ret, "GALAT : dlsym('%s') gagal", buf);
//       goto clean;
//     }
//   } else {
//     tmp_e = strchr(tmp_e, ')');
//     if (tmp_e == NULL) {
//       ret = BAIK_TYPE_ERROR;
//       goto clean;
//     }
//   }

//   cur = tmp_e = args.p;

//   while (tmp_e - args.p < (ptrdiff_t) args.len) {
//     int level = 0;
//     int is_fp = 0;
//     tmp_e = cur;

//     while (*tmp_e && (level > 0 || (*tmp_e != ',' && *tmp_e != ')'))) {
//       switch (*tmp_e) {
//         case '(':
//           level++;
         
//           is_fp = 1;
//           break;
//         case ')':
//           level--;
//           break;
//       }
//       tmp_e++;
//     }

//     if (tmp_e == cur) break;

//     if (is_fp) {
     
//       if (sig->cb_sig != NULL) {
       
//         ret = BAIK_TYPE_ERROR;
//         baik_prepend_errorf(baik, ret, "GALAT : hanya satu callback yang diijinkan");
//         goto clean;
//       }

//       sig->cb_sig = calloc(sizeof(*sig->cb_sig), 1);
//       ret = baik_parse_ffi_signature(baik, cur, tmp_e - cur, sig->cb_sig,
//                                     FFI_SIG_CALLBACK);
//       if (ret != BAIK_OK) {
//         baik_ffi_sig_free(sig->cb_sig);
//         free(sig->cb_sig);
//         sig->cb_sig = NULL;
//         goto clean;
//       }
//       val_type = BAIK_FFI_CTYPE_CALLBACK;
//     } else {
     
//       val_type = parse_cval_type(baik, cur, tmp_e);
//       if (val_type == BAIK_FFI_CTYPE_INVALID) {
       
//         ret = BAIK_TYPE_ERROR;
//         goto clean;
//       }
//     }

//     if (!baik_ffi_sig_set_val_type(sig, vtidx++, val_type)) {
//       ret = BAIK_TYPE_ERROR;
//       baik_prepend_errorf(baik, ret, "GALAT : begitu banyak argumen callback");
//       goto clean;
//     }

//     if (*tmp_e == ',') {
//       cur = tmp_e + 1;
//       while (*cur == ' ') cur++;
//     } else {
     
//       break;
//     }
//   }
 

 
//   baik_ffi_sig_validate(baik, sig, sig_type);
//   if (!sig->is_valid) {
//     ret = BAIK_TYPE_ERROR;
//     goto clean;
//   }

//   if (sig->is_callback) {
//     sig->fn = get_cb_impl_by_signature(sig);
//     if (sig->fn == NULL) {
//       ret = BAIK_TYPE_ERROR;
//       baik_prepend_errorf(baik, ret,
//                          "GALAT : callback signature valid, "
//                          "tetapi tidak ada callback yang terdefinisi");
//       goto clean;
//     }
//   }

// clean:
//   if (ret != BAIK_OK) {
//     baik_prepend_errorf(baik, ret, "GALAT : ffi signature bermasalah: \"%.*s\"", sig_len, s);
//     sig->is_valid = 0;
//   }
//   return ret;
// }

union ffi_cb_data_val {
  void *p;
  uintptr_t w;
  double d;
  float f;
};

// struct ffi_cb_data {
//   union ffi_cb_data_val args[BAIK_CB_ARGS_MAX_CNT];
// };

// static union ffi_cb_data_val ffi_cb_impl_generic(void *param,
//                                                  struct ffi_cb_data *data) {
//   struct baik_ffi_cb_args *cbargs = (struct baik_ffi_cb_args *) param;
//   baik_val_t *args, res = BAIK_UNDEFINED;
//   union ffi_cb_data_val ret;
//   int i;
//   struct baik *baik = cbargs->baik;
//   baik_ffi_ctype_t return_ctype = BAIK_FFI_CTYPE_NONE;
//   baik_err_t err;

//   memset(&ret, 0, sizeof(ret));
//   baik_own(baik, &res);

//   assert(cbargs->sig.args_cnt > 0);

//   args = calloc(1, sizeof(baik_val_t) * cbargs->sig.args_cnt);
//   for (i = 0; i < cbargs->sig.args_cnt; i++) {
//     baik_ffi_ctype_t val_type =
//         cbargs->sig.val_types[i + 1];
//     switch (val_type) {
//       case BAIK_FFI_CTYPE_USERDATA:
//         args[i] = cbargs->userdata;
//         break;
//       case BAIK_FFI_CTYPE_INT:
//         args[i] = baik_mk_number(baik, (double) data->args[i].w);
//         break;
//       case BAIK_FFI_CTYPE_BOOL:
//         args[i] = baik_mk_boolean(baik, !!data->args[i].w);
//         break;
//       case BAIK_FFI_CTYPE_CHAR_PTR: {
//         const char *s = (char *) data->args[i].w;
//         if (s == NULL) s = "";
//         args[i] = baik_mk_string(baik, s, ~0, 1);
//         break;
//       }
//       case BAIK_FFI_CTYPE_VOID_PTR:
//         args[i] = baik_mk_foreign(baik, (void *) data->args[i].w);
//         break;
//       case BAIK_FFI_CTYPE_DOUBLE:
//         args[i] = baik_mk_number(baik, data->args[i].d);
//         break;
//       case BAIK_FFI_CTYPE_FLOAT:
//         args[i] = baik_mk_number(baik, data->args[i].f);
//         break;
//       case BAIK_FFI_CTYPE_STRUCT_GENERIC_STR_PTR: {
//         struct baik_generic_str *s = (struct baik_generic_str *) (void *) data->args[i].w;
//         args[i] = baik_mk_string(baik, s->p, s->len, 1);
//         break;
//       }
//       default:
       
//         LOG(LL_ERROR, ("unexpected val type for arg #%d: %d\n", i, val_type));
//         abort();
//     }
//   }

 
//   return_ctype = cbargs->sig.val_types[0];

//   LOG(LL_VERBOSE_DEBUG, ("calling BAIK callback void-void %d from C",
//                          baik_get_int(baik, cbargs->func)));
//   err = baik_apply(baik, &res, cbargs->func, BAIK_UNDEFINED, cbargs->sig.args_cnt,
//                   args);
//   cbargs = NULL;
//   if (err != BAIK_OK) {
//     baik_print_error(baik, stderr, "BAIK callback error",
//                     1);
//     goto clean;
//   }

 
//   switch (return_ctype) {
//     case BAIK_FFI_CTYPE_NONE:
//       break;
//     case BAIK_FFI_CTYPE_INT:
//       ret.w = baik_get_int(baik, res);
//       break;
//     case BAIK_FFI_CTYPE_BOOL:
//       ret.w = baik_get_bool(baik, res);
//       break;
//     case BAIK_FFI_CTYPE_VOID_PTR:
//       ret.p = baik_get_ptr(baik, res);
//       break;
//     case BAIK_FFI_CTYPE_DOUBLE:
//       ret.d = baik_get_double(baik, res);
//       break;
//     case BAIK_FFI_CTYPE_FLOAT:
//       ret.f = (float) baik_get_double(baik, res);
//       break;
//     default:
//       LOG(LL_ERROR, ("unexpected return val type %d\n", return_ctype));
//       abort();
//   }

// clean:
//   free(args);
//   baik_disown(baik, &res);
//   return ret;
// }

// static void ffi_init_cb_data_wwww(struct ffi_cb_data *data, uintptr_t w0,
//                                   uintptr_t w1, uintptr_t w2, uintptr_t w3,
//                                   uintptr_t w4, uintptr_t w5) {
//   memset(data, 0, sizeof(*data));
//   data->args[0].w = w0;
//   data->args[1].w = w1;
//   data->args[2].w = w2;
//   data->args[3].w = w3;
//   data->args[4].w = w4;
//   data->args[5].w = w5;
// }

// static uintptr_t ffi_cb_impl_wpwwwww(uintptr_t w0, uintptr_t w1, uintptr_t w2,
//                                      uintptr_t w3, uintptr_t w4, uintptr_t w5) {
//   struct ffi_cb_data data;
//   ffi_init_cb_data_wwww(&data, w0, w1, w2, w3, w4, w5);
//   return ffi_cb_impl_generic((void *) w0, &data).w;
// }

// static uintptr_t ffi_cb_impl_wwpwwww(uintptr_t w0, uintptr_t w1, uintptr_t w2,
//                                      uintptr_t w3, uintptr_t w4, uintptr_t w5) {
//   struct ffi_cb_data data;
//   ffi_init_cb_data_wwww(&data, w0, w1, w2, w3, w4, w5);
//   return ffi_cb_impl_generic((void *) w1, &data).w;
// }

// static uintptr_t ffi_cb_impl_wwwpwww(uintptr_t w0, uintptr_t w1, uintptr_t w2,
//                                      uintptr_t w3, uintptr_t w4, uintptr_t w5) {
//   struct ffi_cb_data data;
//   ffi_init_cb_data_wwww(&data, w0, w1, w2, w3, w4, w5);
//   return ffi_cb_impl_generic((void *) w2, &data).w;
// }

// static uintptr_t ffi_cb_impl_wwwwpww(uintptr_t w0, uintptr_t w1, uintptr_t w2,
//                                      uintptr_t w3, uintptr_t w4, uintptr_t w5) {
//   struct ffi_cb_data data;
//   ffi_init_cb_data_wwww(&data, w0, w1, w2, w3, w4, w5);
//   return ffi_cb_impl_generic((void *) w3, &data).w;
// }

// static uintptr_t ffi_cb_impl_wwwwwpw(uintptr_t w0, uintptr_t w1, uintptr_t w2,
//                                      uintptr_t w3, uintptr_t w4, uintptr_t w5) {
//   struct ffi_cb_data data;
//   ffi_init_cb_data_wwww(&data, w0, w1, w2, w3, w4, w5);
//   return ffi_cb_impl_generic((void *) w4, &data).w;
// }

// static uintptr_t ffi_cb_impl_wwwwwwp(uintptr_t w0, uintptr_t w1, uintptr_t w2,
//                                      uintptr_t w3, uintptr_t w4, uintptr_t w5) {
//   struct ffi_cb_data data;
//   ffi_init_cb_data_wwww(&data, w0, w1, w2, w3, w4, w5);
//   return ffi_cb_impl_generic((void *) w5, &data).w;
// }

// static uintptr_t ffi_cb_impl_wpd(uintptr_t w0, double d1) {
//   struct ffi_cb_data data;

//   memset(&data, 0, sizeof(data));
//   data.args[0].w = w0;
//   data.args[1].d = d1;

//   return ffi_cb_impl_generic((void *) w0, &data).w;
// }

// static uintptr_t ffi_cb_impl_wdp(double d0, uintptr_t w1) {
//   struct ffi_cb_data data;

//   memset(&data, 0, sizeof(data));
//   data.args[0].d = d0;
//   data.args[1].w = w1;

//   return ffi_cb_impl_generic((void *) w1, &data).w;
// }


// static struct baik_ffi_cb_args **ffi_get_matching(struct baik_ffi_cb_args **plist,
//                                                  baik_val_t func,
//                                                  baik_val_t userdata) {
//   for (; *plist != NULL; plist = &((*plist)->next)) {
//     if ((*plist)->func == func && (*plist)->userdata == userdata) {
//       break;
//     }
//   }
//   return plist;
// }

// static ffi_fn_t *get_cb_impl_by_signature(const baik_ffi_sig_t *sig) {
//   if (sig->is_valid) {
//     int i;
//     int double_cnt = 0;
//     int float_cnt = 0;
//     int userdata_idx = 0;

//     for (i = 1; i < BAIK_CB_SIGNATURE_MAX_SIZE;
//          i++) {
//       baik_ffi_ctype_t type = sig->val_types[i];
//       switch (type) {
//         case BAIK_FFI_CTYPE_DOUBLE:
//           double_cnt++;
//           break;
//         case BAIK_FFI_CTYPE_FLOAT:
//           float_cnt++;
//           break;
//         case BAIK_FFI_CTYPE_USERDATA:
//           assert(userdata_idx == 0);
//           userdata_idx = i;
//           break;
//         default:
//           break;
//       }
//     }

//     if (float_cnt > 0) {
//       return NULL;
//     }

//     assert(userdata_idx > 0);

//     if (sig->args_cnt <= BAIK_CB_ARGS_MAX_CNT) {
//       if (baik_ffi_is_regular_word_or_void(sig->val_types[0])) {
       
//         switch (double_cnt) {
//           case 0:
//             switch (userdata_idx) {
//               case 1:
//                 return (ffi_fn_t *) ffi_cb_impl_wpwwwww;
//               case 2:
//                 return (ffi_fn_t *) ffi_cb_impl_wwpwwww;
//               case 3:
//                 return (ffi_fn_t *) ffi_cb_impl_wwwpwww;
//               case 4:
//                 return (ffi_fn_t *) ffi_cb_impl_wwwwpww;
//               case 5:
//                 return (ffi_fn_t *) ffi_cb_impl_wwwwwpw;
//               case 6:
//                 return (ffi_fn_t *) ffi_cb_impl_wwwwwwp;
//               default:
//                 abort();
//             }
//             break;
//           case 1:
           
//             switch (userdata_idx) {
//               case 1:
//                 return (ffi_fn_t *) ffi_cb_impl_wpd;
//               case 2:
//                 return (ffi_fn_t *) ffi_cb_impl_wdp;
//             }
//             break;
//         }
//       }
//     } else {}
//   }

//   return NULL;
// }

// BAIK_PRIVATE baik_val_t baik_ffi_sig_to_value(struct baik_ffi_sig *psig) {
//   if (psig == NULL) {
//     return BAIK_NULL;
//   } else {
//     return baik_legit_pointer_to_value(psig) | BAIK_TAG_FUNCTION_FFI;
//   }
// }

// BAIK_PRIVATE int baik_is_ffi_sig(baik_val_t v) {
//   return (v & BAIK_TAG_MASK) == BAIK_TAG_FUNCTION_FFI;
// }

// BAIK_PRIVATE struct baik_ffi_sig *baik_get_ffi_sig_struct(baik_val_t v) {
//   struct baik_ffi_sig *ret = NULL;
//   assert(baik_is_ffi_sig(v));
//   ret = (struct baik_ffi_sig *) get_ptr(v);
//   return ret;
// }

// BAIK_PRIVATE baik_val_t baik_mk_ffi_sig(struct baik *baik) {
//   struct baik_ffi_sig *psig = new_ffi_sig(baik);
//   baik_ffi_sig_init(psig);
//   return baik_ffi_sig_to_value(psig);
// }

// BAIK_PRIVATE void baik_ffi_sig_destructor(struct baik *baik, void *psig) {
//   baik_ffi_sig_free((baik_ffi_sig_t *) psig);
//   (void) baik;
// }

// BAIK_PRIVATE void *baik_dlsym(void *handle, const char *name) {
//     void *client_hndl;
//     typedef int main_t(int, char**);
//     main_t *client_main;
//     client_hndl = dlopen(baik_dlsym_file,  RTLD_LAZY);
//     if (!client_hndl){
//         fprintf(stderr, "%s\n", dlerror());
//         exit(1);
//     }
//     client_main = (main_t*)dlsym(client_hndl, name);
//     if (!client_main){
//         fprintf(stderr, "%s\n", dlerror());
//         exit(2);
//     }
//     else{
//         return (main_t*)dlsym(client_hndl, name);
//     }
//     return NULL;
// }

// BAIK_PRIVATE baik_err_t baik_ffi_call(struct baik *baik) {
//   baik_err_t e = BAIK_OK;
//   const char *sig_str = NULL;
//   baik_val_t sig_str_v = baik_arg(baik, 0);
//   baik_val_t ret_v = BAIK_UNDEFINED;
//   struct baik_ffi_sig *psig = baik_get_ffi_sig_struct(baik_mk_ffi_sig(baik));
//   size_t sig_str_len;
  
//   sig_str = baik_get_string(baik, &sig_str_v, &sig_str_len);

//   size_t sig_str_len_dlsym_file;
//   baik_val_t sig_str_v_dlsym_file = baik_arg(baik, 1);
//   baik_dlsym_file = baik_get_string(baik, &sig_str_v_dlsym_file, &sig_str_len_dlsym_file);
//   if(baik_dlsym_file != NULL){
//     //printf("%s",baik_dlsym_file);
//     baik_set_ffi_resolver(baik, baik_dlsym);
//   }

//   e = baik_parse_ffi_signature(baik, sig_str, sig_str_len, psig, FFI_SIG_FUNC);
//   if (e != BAIK_OK) goto clean;
//   ret_v = baik_ffi_sig_to_value(psig);

// clean:
//   baik_return(baik, ret_v);
//   return e;
// }

// BAIK_PRIVATE baik_err_t baik_ffi_call2(struct baik *baik) {
//   baik_err_t ret = BAIK_OK;
//   baik_ffi_sig_t *psig = NULL;
//   baik_ffi_ctype_t rtype;
//   baik_val_t sig_v = *vptr(&baik->stack, baik_getretvalpos(baik));

//   int i, nargs;
//   struct ffi_arg res;
//   struct ffi_arg args[FFI_MAX_ARGS_CNT];
//   struct cbdata cbdata;

 
//   baik_val_t resv = baik_mk_undefined();

 
//   baik_val_t argvs[FFI_MAX_ARGS_CNT];
//   struct baik_generic_str argvmgstr[FFI_MAX_ARGS_CNT];

//   if (baik_is_ffi_sig(sig_v)) {
//     psig = baik_get_ffi_sig_struct(sig_v);
//   } else {
//     ret = BAIK_TYPE_ERROR;
//     baik_prepend_errorf(baik, ret, "GALAT :  isi non-ffi-callable");
//     goto clean;
//   }

//   memset(&cbdata, 0, sizeof(cbdata));
//   cbdata.func_idx = -1;
//   cbdata.userdata_idx = -1;

//   rtype = psig->val_types[0];

//   switch (rtype) {
//     case BAIK_FFI_CTYPE_DOUBLE:
//       res.ctype = FFI_CTYPE_DOUBLE;
//       break;
//     case BAIK_FFI_CTYPE_FLOAT:
//       res.ctype = FFI_CTYPE_FLOAT;
//       break;
//     case BAIK_FFI_CTYPE_BOOL:
//       res.ctype = FFI_CTYPE_BOOL;
//       break;
//     case BAIK_FFI_CTYPE_USERDATA:
//     case BAIK_FFI_CTYPE_INT:
//     case BAIK_FFI_CTYPE_CHAR_PTR:
//     case BAIK_FFI_CTYPE_VOID_PTR:
//     case BAIK_FFI_CTYPE_NONE:
//       res.ctype = FFI_CTYPE_WORD;
//       break;
//     case BAIK_FFI_CTYPE_INVALID:
//       ret = BAIK_TYPE_ERROR;
//       baik_prepend_errorf(baik, ret, "GALAT : kesalahan balikan tipe pada fungsi ffi");
//       goto clean;
//   }
//   res.v.i = 0;

//   nargs =
//       baik_stack_size(&baik->stack) - baik_get_int(baik, vtop(&baik->call_stack));

//   if (nargs != psig->args_cnt) {
//     ret = BAIK_TYPE_ERROR;
//     baik_prepend_errorf(baik, ret, "GALAT : got %d actuals, but function takes %d args",
//                        nargs, psig->args_cnt);
//     goto clean;
//   }

//   for (i = 0; i < nargs; i++) {
//     baik_val_t arg = baik_arg(baik, i);

//     switch (psig->val_types[1 + i]) {
//       case BAIK_FFI_CTYPE_NONE:
       
//         ret = BAIK_TYPE_ERROR;
//         if (i == 0) {
         
//           baik_prepend_errorf(baik, ret, "GALAT : fungsi ffi-ed tidak memiliki argumen");
//         } else {
         
//           baik_prepend_errorf(baik, ret, "GALAT : arg ffi bermasalah #%d tipe: \"void\"", i);
//         }

//         goto clean;
//       case BAIK_FFI_CTYPE_USERDATA:
       
//         if (cbdata.userdata_idx != -1) {
//           ret = BAIK_TYPE_ERROR;
//           baik_prepend_errorf(baik, ret, "GALAT dua atau lebih argumen userdata : #%d and %d",
//                              cbdata.userdata_idx, i);

//           goto clean;
//         }
//         cbdata.userdata = arg;
//         cbdata.userdata_idx = i;
//         break;
//       case BAIK_FFI_CTYPE_INT: {
//         int intval = 0;
//         if (baik_is_number(arg)) {
//           intval = baik_get_int(baik, arg);
//         } else if (baik_is_boolean(arg)) {
//           intval = baik_get_bool(baik, arg);
//         } else {
//           ret = BAIK_TYPE_ERROR;
//           baik_prepend_errorf(
//               baik, ret, "GALAT : arg #%d bukan int (tipe idx adalah: %s)", i,
//               baik_typeof(arg));
//         }
//         ffi_set_word(&args[i], intval);
//       } break;
//       case BAIK_FFI_CTYPE_STRUCT_GENERIC_STR_PTR: {
//         if (!baik_is_string(arg)) {
//           ret = BAIK_TYPE_ERROR;
//           baik_prepend_errorf(
//               baik, ret, "GALAT : arg #%d bukan string (tipe idx adalah: %s)",
//               i, baik_typeof(arg));
//           goto clean;
//         }
//         argvs[i] = arg;
//         argvmgstr[i].p = baik_get_string(baik, &argvs[i], &argvmgstr[i].len);
       
//         ffi_set_ptr(&args[i], (void *) &argvmgstr[i]);
//         break;
//       }
//       case BAIK_FFI_CTYPE_BOOL: {
//         int intval = 0;
//         if (baik_is_number(arg)) {
//           intval = !!baik_get_int(baik, arg);
//         } else if (baik_is_boolean(arg)) {
//           intval = baik_get_bool(baik, arg);
//         } else {
//           ret = BAIK_TYPE_ERROR;
//           baik_prepend_errorf(
//               baik, ret, "GALAT : arg #%d bukan bool (tipe idx adalah: %s)", i,
//               baik_typeof(arg));
//         }
//         ffi_set_word(&args[i], intval);
//       } break;
//       case BAIK_FFI_CTYPE_DOUBLE:
//         ffi_set_double(&args[i], baik_get_double(baik, arg));
//         break;
//       case BAIK_FFI_CTYPE_FLOAT:
//         ffi_set_float(&args[i], (float) baik_get_double(baik, arg));
//         break;
//       case BAIK_FFI_CTYPE_CHAR_PTR: {
//         size_t s;
//         if (baik_is_string(arg)) {
         
//           argvs[i] = arg;
//           ffi_set_ptr(&args[i], (void *) baik_get_string(baik, &argvs[i], &s));
//         } else if (baik_is_null(arg)) {
//           ffi_set_ptr(&args[i], NULL);
//         } else {
//           ret = BAIK_TYPE_ERROR;
//           baik_prepend_errorf(
//               baik, ret, "GALAT : arg #%d bukan string (tipe idx adalah: %s)",
//               i, baik_typeof(arg));
//           goto clean;
//         }
//       } break;
//       case BAIK_FFI_CTYPE_VOID_PTR:
//         if (baik_is_string(arg)) {
//           size_t n;
         
//           argvs[i] = arg;
//           ffi_set_ptr(&args[i], (void *) baik_get_string(baik, &argvs[i], &n));
//         } else if (baik_is_foreign(arg)) {
//           ffi_set_ptr(&args[i], (void *) baik_get_ptr(baik, arg));
//         } else if (baik_is_null(arg)) {
//           ffi_set_ptr(&args[i], NULL);
//         } else {
//           ret = BAIK_TYPE_ERROR;
//           baik_prepend_errorf(baik, ret, "GALAT : arg #%d bukan ptr", i);
//           goto clean;
//         }
//         break;
//       case BAIK_FFI_CTYPE_CALLBACK:
//         if (baik_is_function(arg) || baik_is_foreign(arg) ||
//             baik_is_ffi_sig(arg)) {
         
//           cbdata.func = arg;
//           cbdata.func_idx = i;
//         } else {
//           ret = BAIK_TYPE_ERROR;
//           baik_prepend_errorf(baik, ret,
//                              "GALAT : arg #%d bukan fungsi, tetapi %s", i,
//                              baik_stringify_type((enum baik_type) arg));
//           goto clean;
//         }
//         break;
//       case BAIK_FFI_CTYPE_INVALID:
       
//         ret = BAIK_TYPE_ERROR;
//         baik_prepend_errorf(baik, ret, "GALAT : kesalahan tipe arg");
//         goto clean;
//       default:
//         abort();
//         break;
//     }
//   }

//   if (cbdata.userdata_idx >= 0 && cbdata.func_idx >= 0) {
//     struct baik_ffi_cb_args *cbargs = NULL;
//     struct baik_ffi_cb_args **pitem = NULL;

//     pitem = ffi_get_matching(&baik->ffi_cb_args, cbdata.func, cbdata.userdata);
//     if (*pitem == NULL) {
     
//       cbargs = calloc(1, sizeof(*cbargs));
//       cbargs->baik = baik;
//       cbargs->func = cbdata.func;
//       cbargs->userdata = cbdata.userdata;
//       baik_ffi_sig_copy(&cbargs->sig, psig->cb_sig);

//       *pitem = cbargs;
//     } else {
     
//       cbargs = *pitem;
//     }

//     {
//       union {
//         ffi_fn_t *fn;
//         void *p;
//       } u;
//       u.fn = psig->cb_sig->fn;
//       ffi_set_ptr(&args[cbdata.func_idx], u.p);
//       ffi_set_ptr(&args[cbdata.userdata_idx], cbargs);
//     }
//   } else if (!(cbdata.userdata_idx == -1 && cbdata.func_idx == -1)) {
   
//     abort();
//   }

//   ffi_call(psig->fn, nargs, &res, args);

//   switch (rtype) {
//     case BAIK_FFI_CTYPE_CHAR_PTR: {
//       const char *s = (const char *) (uintptr_t) res.v.i;
//       if (s != NULL) {
//         resv = baik_mk_string(baik, s, ~0, 1);
//       } else {
//         resv = BAIK_NULL;
//       }
//       break;
//     }
//     case BAIK_FFI_CTYPE_VOID_PTR:
//       resv = baik_mk_foreign(baik, (void *) (uintptr_t) res.v.i);
//       break;
//     case BAIK_FFI_CTYPE_INT:
//       resv = baik_mk_number(baik, (int) res.v.i);
//       break;
//     case BAIK_FFI_CTYPE_BOOL:
//       resv = baik_mk_boolean(baik, !!res.v.i);
//       break;
//     case BAIK_FFI_CTYPE_DOUBLE:
//       resv = baik_mk_number(baik, res.v.d);
//       break;
//     case BAIK_FFI_CTYPE_FLOAT:
//       resv = baik_mk_number(baik, res.v.f);
//       break;
//     default:
//       resv = baik_mk_undefined();
//       break;
//   }

// clean:
 
//   if (ret != BAIK_OK) {
//     baik_prepend_errorf(baik, ret, "GALAT : gagal memanggil fungsi FFIed");
   
//   }
//   baik_return(baik, resv);

//   return ret;
// }

// BAIK_PRIVATE void baik_ffi_cb_free(struct baik *baik) {
//   baik_val_t ret = baik_mk_number(baik, 0);
//   baik_val_t func = baik_arg(baik, 0);
//   baik_val_t userdata = baik_arg(baik, 1);

//   if (baik_is_function(func)) {
//     struct baik_ffi_cb_args **pitem =
//         ffi_get_matching(&baik->ffi_cb_args, func, userdata);
//     if (*pitem != NULL) {
     
//       struct baik_ffi_cb_args *cbargs = *pitem;
//       *pitem = cbargs->next;
//       baik_ffi_sig_free(&cbargs->sig);
//       free(cbargs);
//       ret = baik_mk_number(baik, 1);
//     }
//   } else {
//     baik_prepend_errorf(baik, BAIK_TYPE_ERROR, "GALAT : kesalahan argumen 'func'");
//   }

//   baik_return(baik, ret);
// }

// void baik_ffi_args_free_list(struct baik *baik) {
//   ffi_cb_args_t *next = baik->ffi_cb_args;

//   while (next != NULL) {
//     ffi_cb_args_t *cur = next;
//     next = next->next;
//     free(cur);
//   }
// }

// BAIK_PRIVATE void baik_ffi_sig_init(baik_ffi_sig_t *sig) {
//   memset(sig, 0, sizeof(*sig));
// }

// BAIK_PRIVATE void baik_ffi_sig_copy(baik_ffi_sig_t *to,
//                                   const baik_ffi_sig_t *from) {
//   memcpy(to, from, sizeof(*to));
//   if (from->cb_sig != NULL) {
//     to->cb_sig = calloc(sizeof(*to->cb_sig), 1);
//     baik_ffi_sig_copy(to->cb_sig, from->cb_sig);
//   }
// }

// BAIK_PRIVATE void baik_ffi_sig_free(baik_ffi_sig_t *sig) {
//   if (sig->cb_sig != NULL) {
//     free(sig->cb_sig);
//     sig->cb_sig = NULL;
//   }
// }

// BAIK_PRIVATE int baik_ffi_sig_set_val_type(baik_ffi_sig_t *sig, int idx,
//                                          baik_ffi_ctype_t type) {
//   if (idx < BAIK_CB_SIGNATURE_MAX_SIZE) {
//     sig->val_types[idx] = type;
//     return 1;
//   } else {
   
//     return 0;
//   }
// }

// BAIK_PRIVATE int baik_ffi_sig_validate(struct baik *baik, baik_ffi_sig_t *sig,
//                                      enum ffi_sig_type sig_type) {
//   int ret = 0;
//   int i;
//   int callback_idx = 0;
//   int userdata_idx = 0;

//   sig->is_valid = 0;

//   switch (sig_type) {
//     case FFI_SIG_FUNC:
     
//       if (sig->val_types[0] != BAIK_FFI_CTYPE_NONE &&
//           sig->val_types[0] != BAIK_FFI_CTYPE_INT &&
//           sig->val_types[0] != BAIK_FFI_CTYPE_BOOL &&
//           sig->val_types[0] != BAIK_FFI_CTYPE_DOUBLE &&
//           sig->val_types[0] != BAIK_FFI_CTYPE_FLOAT &&
//           sig->val_types[0] != BAIK_FFI_CTYPE_VOID_PTR &&
//           sig->val_types[0] != BAIK_FFI_CTYPE_CHAR_PTR) {
//         baik_prepend_errorf(baik, BAIK_TYPE_ERROR, "GALAT : nilai tipe balikan salah");
//         goto clean;
//       }
//       break;
//     case FFI_SIG_CALLBACK:
     
//       if (sig->val_types[0] != BAIK_FFI_CTYPE_NONE &&
//           sig->val_types[0] != BAIK_FFI_CTYPE_INT &&
//           sig->val_types[0] != BAIK_FFI_CTYPE_BOOL &&
//           sig->val_types[0] != BAIK_FFI_CTYPE_DOUBLE &&
//           sig->val_types[0] != BAIK_FFI_CTYPE_FLOAT &&
//           sig->val_types[0] != BAIK_FFI_CTYPE_VOID_PTR) {
//         baik_prepend_errorf(baik, BAIK_TYPE_ERROR, "GALAT : tipe nilai balikan salah");
//         goto clean;
//       }
//   }

 
//   for (i = 1; i < BAIK_CB_SIGNATURE_MAX_SIZE; i++) {
//     baik_ffi_ctype_t type = sig->val_types[i];
//     switch (type) {
//       case BAIK_FFI_CTYPE_USERDATA:
//         if (userdata_idx != 0) {
         
//           baik_prepend_errorf(baik, BAIK_TYPE_ERROR,
//                              "GALAT : lebih dari satu userdata arg: #%d dan #%d",
//                              (userdata_idx - 1), (i - 1));
//           goto clean;
//         }
//         userdata_idx = i;
//         break;
//       case BAIK_FFI_CTYPE_CALLBACK:
//         switch (sig_type) {
//           case FFI_SIG_FUNC:
//             break;
//           case FFI_SIG_CALLBACK:
//             baik_prepend_errorf(baik, BAIK_TYPE_ERROR,
//                                "GALAT : callback tidak mendapatkan callback lainnya");
//             goto clean;
//         }
//         callback_idx = i;
//         break;
//       case BAIK_FFI_CTYPE_INT:
//       case BAIK_FFI_CTYPE_BOOL:
//       case BAIK_FFI_CTYPE_VOID_PTR:
//       case BAIK_FFI_CTYPE_CHAR_PTR:
//       case BAIK_FFI_CTYPE_STRUCT_GENERIC_STR_PTR:
//       case BAIK_FFI_CTYPE_DOUBLE:
//       case BAIK_FFI_CTYPE_FLOAT:
       
//         break;
//       case BAIK_FFI_CTYPE_NONE:
       
//         goto args_over;
//       default:
//         baik_prepend_errorf(baik, BAIK_INTERNAL_ERROR, "GALAT : kesalahan ffi_ctype: %d",
//                            type);
//         goto clean;
//     }

//     sig->args_cnt++;
//   }
// args_over:

//   switch (sig_type) {
//     case FFI_SIG_FUNC:
//       if (!((callback_idx > 0 && userdata_idx > 0) ||
//             (callback_idx == 0 && userdata_idx == 0))) {
//         baik_prepend_errorf(baik, BAIK_TYPE_ERROR,
//                            "GALAT : callback and userdata should be either both "
//                            "present or both absent");
//         goto clean;
//       }
//       break;
//     case FFI_SIG_CALLBACK:
//       if (userdata_idx == 0) {
       
//         baik_prepend_errorf(baik, BAIK_TYPE_ERROR, "GALAT : tanpa userdata arg");
//         goto clean;
//       }
//       break;
//   }

//   ret = 1;

// clean:
//   if (ret) {
//     sig->is_valid = 1;
//   }
//   return ret;
// }

// BAIK_PRIVATE int baik_ffi_is_regular_word(baik_ffi_ctype_t type) {
//   switch (type) {
//     case BAIK_FFI_CTYPE_INT:
//     case BAIK_FFI_CTYPE_BOOL:
//       return 1;
//     default:
//       return 0;
//   }
// }

// BAIK_PRIVATE int baik_ffi_is_regular_word_or_void(baik_ffi_ctype_t type) {
//   return (type == BAIK_FFI_CTYPE_NONE || baik_ffi_is_regular_word(type));
// }

#ifdef _WIN32
void *dlsym(void *handle, const char *name) {
  static HANDLE msvcrt_dll;
  void *sym = NULL;
  if (msvcrt_dll == NULL) msvcrt_dll = GetModuleHandle("msvcrt.dll");
  if ((sym = GetProcAddress(GetModuleHandle(NULL), name)) == NULL) {
    sym = GetProcAddress(msvcrt_dll, name);
  }
  return sym;
}
#elif !defined(__unix__) && !defined(__APPLE__)
void *dlsym(void *handle, const char *name) {
  (void) handle;
  (void) name;
  return NULL;
}
#endif
