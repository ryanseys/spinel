/*
 * sp_fiddle.c -- Fiddle::Pointer for Spinel.
 *
 * A carried-C spin package (Path B typed object), linked on demand when
 * `require "fiddle"` appears. Fiddle::Pointer wraps a raw C pointer together
 * with a byte size (both needed for slicing, arithmetic, and to_str, which the
 * bare foreign-pointer boxing can't carry). `owned` pointers were malloc'd here
 * and are freed by the GC finalizer / #free; wrapped raw pointers are not.
 *
 * Instance methods (#[], #[]=, #null?, #to_s, #to_str, #free) follow the native
 * ABI `(self, args...)`. Class-level / operator forms (Pointer.malloc, +, -,
 * Fiddle::NULL) are emitted by codegen with the class id, since they mint a new
 * Pointer and the native-method ABI has no class-id parameter.
 */

#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "spinel/runtime.h"

typedef struct sp_FiddlePtr_s {
  mrb_int cls_id;   /* object header: runtime class id, compiler-stamped */
  void   *ptr;
  long    size;
  int     owned;    /* 1 if malloc'd here: the finalizer / #free release it */
} sp_FiddlePtr;

/* GC finalizer (native_struct's free symbol): release an owned buffer. */
void sp_FiddlePtr_gc_free(void *p) {
  sp_FiddlePtr *fp = (sp_FiddlePtr *)p;
  if (fp && fp->owned && fp->ptr) { free(fp->ptr); fp->ptr = NULL; fp->owned = 0; }
}

static sp_FiddlePtr *fp_alloc(mrb_int cls_id) {
  sp_FiddlePtr *fp = (sp_FiddlePtr *)sp_gc_alloc(sizeof(sp_FiddlePtr), sp_FiddlePtr_gc_free, NULL);
  fp->cls_id = cls_id;
  fp->ptr = NULL;
  fp->size = 0;
  fp->owned = 0;
  return fp;
}

/* Fiddle::Pointer.malloc(n): a zero-filled owned buffer. */
sp_FiddlePtr *sp_FiddlePtr_malloc(mrb_int cls_id, mrb_int n) {
  sp_FiddlePtr *fp = fp_alloc(cls_id);
  fp->ptr = n > 0 ? calloc(1, (size_t)n) : NULL;
  fp->size = (long)n;
  fp->owned = 1;
  return fp;
}

/* Wrap an existing raw pointer (e.g. an FFI :ptr return, or Fiddle::NULL). */
sp_FiddlePtr *sp_FiddlePtr_new_raw(mrb_int cls_id, void *raw, long size) {
  sp_FiddlePtr *fp = fp_alloc(cls_id);
  fp->ptr = raw;
  fp->size = size;
  fp->owned = 0;
  return fp;
}

/* #free: release an owned buffer now (idempotent). */
mrb_int sp_FiddlePtr_free(sp_FiddlePtr *fp) {
  if (fp && fp->owned && fp->ptr) { free(fp->ptr); fp->ptr = NULL; fp->owned = 0; }
  return 0;
}

mrb_bool sp_FiddlePtr_null_p(sp_FiddlePtr *fp) { return fp && fp->ptr == NULL; }

/* p[off, len] -> a String of len bytes (binary-safe). */
sp_RbVal sp_FiddlePtr_slice(sp_FiddlePtr *fp, mrb_int off, mrb_int len) {
  if (!fp || !fp->ptr || off < 0 || len < 0) return sp_box_nil();
  return sp_box_str(sp_str_from_bytes((const char *)fp->ptr + off, (size_t)len));
}

/* p[off, len] = str: copy min(len, str.bytesize) bytes into the buffer. */
mrb_int sp_FiddlePtr_slice_set(sp_FiddlePtr *fp, mrb_int off, mrb_int len, const char *val) {
  if (!fp || !fp->ptr || !val || off < 0 || len < 0) return 0;
  size_t vl = sp_str_byte_len(val);
  size_t n = (size_t)len < vl ? (size_t)len : vl;
  memcpy((char *)fp->ptr + off, val, n);
  return 0;
}

/* #to_str: exactly `size` bytes. #to_s: bytes up to the first NUL. */
const char *sp_FiddlePtr_to_str(sp_FiddlePtr *fp) {
  if (!fp || !fp->ptr || fp->size <= 0) return sp_str_empty;
  return sp_str_from_bytes((const char *)fp->ptr, (size_t)fp->size);
}
const char *sp_FiddlePtr_to_s(sp_FiddlePtr *fp) {
  if (!fp || !fp->ptr) return sp_str_empty;
  const char *p = (const char *)fp->ptr;
  size_t n = 0;
  while (p[n]) n++;
  return sp_str_from_bytes(p, n);
}

/* p + n / p - n: a new pointer with the size adjusted by the offset (CRuby). */
sp_FiddlePtr *sp_FiddlePtr_plus(mrb_int cls_id, sp_FiddlePtr *fp, mrb_int n) {
  return sp_FiddlePtr_new_raw(cls_id, (fp && fp->ptr) ? (char *)fp->ptr + n : NULL,
                              fp ? fp->size - (long)n : 0);
}
sp_FiddlePtr *sp_FiddlePtr_minus(mrb_int cls_id, sp_FiddlePtr *fp, mrb_int n) {
  return sp_FiddlePtr_new_raw(cls_id, (fp && fp->ptr) ? (char *)fp->ptr - n : NULL,
                              fp ? fp->size + (long)n : 0);
}

/* Unbox a Fiddle::Pointer to its raw void* for an FFI argument. */
void *sp_FiddlePtr_raw(sp_FiddlePtr *fp) { return fp ? fp->ptr : NULL; }
