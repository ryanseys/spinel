#ifndef SP_ARRAY_H
#define SP_ARRAY_H
/* sp_array.h -- typed array hot core + cold-op surface.
 *
 * The struct layouts (sp_IntArray, ...) live in sp_types.h. The hot
 * accessors (new / push / pop / shift / get / set / length / empty) stay
 * inline here so every generated TU compiles them identically --
 * relocating them out of sp_runtime.h into this shared header is a pure
 * textual move with no codegen change. The cold ops (sort / slice / dup /
 * set algebra / join / ...) are compiled once into libspinel_rt.a
 * (lib/sp_array.c); this header only declares them.
 *
 * sp_sprintf / sp_raise_cls / sp_raise_frozen_array are provided by the
 * generated TU and resolved at the final link, the same way lib/sp_core.c
 * calls them -- so lib/sp_array.c can use them without a runtime include.
 */
#include "sp_gc.h"      /* sp_gc_hdr, sp_gc_bytes, SP_GC_ROOT, sp_oom_die */
#include "sp_alloc.h"   /* sp_gc_alloc, sp_str_alloc, sp_raise_cls, sp_raise_frozen_array */

const char *sp_sprintf(const char *fmt, ...);  /* defined in the generated TU */

/* ============================ sp_IntArray ============================ */
/* `frozen` rides in the struct (not the GC header) so the hot push /
   []= paths read it from the same cache line as len/cap -- no extra
   cache miss vs. the GC-header bit. calloc in sp_gc_alloc zero-inits
   it, so constructors need no change. Issue #918. */
static void sp_IntArray_fin(void*p){free(((sp_IntArray*)p)->data);}
static sp_IntArray*sp_IntArray_new(void){sp_IntArray*a=(sp_IntArray*)sp_gc_alloc(sizeof(sp_IntArray),sp_IntArray_fin,NULL);a->cap=16;a->data=(mrb_int*)malloc(sizeof(mrb_int)*a->cap);if(!a->data)sp_oom_die();a->start=0;a->len=0;{sp_gc_hdr*h=(sp_gc_hdr*)((char*)a-sizeof(sp_gc_hdr));h->size+=sizeof(mrb_int)*a->cap;sp_gc_bytes+=sizeof(mrb_int)*a->cap;}return a;}
static void __attribute__((noinline)) sp_IntArray_push_grow(sp_IntArray*a){if(a->start>0){memmove(a->data,a->data+a->start,sizeof(mrb_int)*a->len);a->start=0;if(a->len<a->cap)return;}{sp_gc_hdr*h=(sp_gc_hdr*)((char*)a-sizeof(sp_gc_hdr));sp_gc_bytes-=sizeof(mrb_int)*a->cap;h->size-=sizeof(mrb_int)*a->cap;a->cap=a->cap*2+1;void*nd=realloc(a->data,sizeof(mrb_int)*a->cap);if(!nd)sp_oom_die();a->data=(mrb_int*)nd;h->size+=sizeof(mrb_int)*a->cap;sp_gc_bytes+=sizeof(mrb_int)*a->cap;}}
static inline void sp_IntArray_push(sp_IntArray*a,mrb_int v){if(a->frozen){sp_raise_frozen_array();return;}if(a->start+a->len>=a->cap)sp_IntArray_push_grow(a);a->data[a->start+a->len]=v;a->len++;}
/* Issue #826/#832: empty pop/shift return SP_INT_NIL (nullable int
   sentinel) to match MRI's nil; callers treat as int?. Without the
   guard, `--a->len` wraps to -1 and reads past the buffer start. */
static inline mrb_int sp_IntArray_pop(sp_IntArray*a){if(!a||a->len<=0)return SP_INT_NIL;if(a->frozen){sp_raise_frozen_array();return SP_INT_NIL;}return a->data[a->start+--a->len];}
static inline mrb_int sp_IntArray_shift(sp_IntArray*a){if(!a||a->len<=0)return SP_INT_NIL;if(a->frozen){sp_raise_frozen_array();return SP_INT_NIL;}mrb_int v=a->data[a->start];a->start++;a->len--;return v;}
static inline mrb_int sp_IntArray_length(sp_IntArray*a){return a->len;}
static inline mrb_bool sp_IntArray_empty(sp_IntArray*a){return a->len==0;}
static inline mrb_int sp_IntArray_get(sp_IntArray*a,mrb_int i){if(!a)return SP_INT_NIL;if(i<0)i+=a->len;if(i<0||i>=a->len)return SP_INT_NIL;return a->data[a->start+i];}
/* Issue #769: a very-negative i leaves i negative after the `i += a->len`
   adjustment. CRuby raises IndexError; spinel no-ops as the safest
   fallback (raising from a typed-array set would need setjmp plumbing
   throughout the call chain). */
static void sp_IntArray_set_slow(sp_IntArray*a,mrb_int i,mrb_int v){if(i<0)return;while(a->start+i>=a->cap){sp_gc_hdr*h=(sp_gc_hdr*)((char*)a-sizeof(sp_gc_hdr));sp_gc_bytes-=sizeof(mrb_int)*a->cap;h->size-=sizeof(mrb_int)*a->cap;a->cap=a->cap*2+1;a->data=(mrb_int*)realloc(a->data,sizeof(mrb_int)*a->cap);h->size+=sizeof(mrb_int)*a->cap;sp_gc_bytes+=sizeof(mrb_int)*a->cap;}while(i>=a->len){a->data[a->start+a->len]=0;a->len++;}a->data[a->start+i]=v;}
/* Issue #839: an extreme negative index (still negative after `i += len`)
   raises IndexError per MRI. */
static inline void sp_IntArray_set(sp_IntArray*a,mrb_int i,mrb_int v){if(!a)return;if(a->frozen){sp_raise_frozen_array();return;}mrb_int orig=i;if(i<0)i+=a->len;if(i<0)sp_raise_cls("IndexError",sp_sprintf("index %lld too small for array; minimum: %lld",(long long)orig,(long long)-a->len));if(i<a->len){a->data[a->start+i]=v;return;}sp_IntArray_set_slow(a,i,v);}

/* ---- sp_IntArray cold ops (compiled in lib/sp_array.c) ---- */
sp_IntArray *sp_IntArray_from_range(mrb_int s, mrb_int e);
sp_IntArray *sp_IntArray_from_range_step(mrb_int s, mrb_int e, mrb_int k);
sp_IntArray *sp_IntArray_dup(sp_IntArray *a);
sp_IntArray *sp_IntArray_slice(sp_IntArray *a, mrb_int start, mrb_int len);
sp_IntArray *sp_IntArray_slice_range(sp_IntArray *a, mrb_int start, mrb_int end_, mrb_int excl);
void sp_IntArray_replace(sp_IntArray *dst, sp_IntArray *src);
void sp_IntArray_reverse_bang(sp_IntArray *a);
void sp_IntArray_rotate_bang(sp_IntArray *a, mrb_int n);
sp_IntArray *sp_IntArray_sort(sp_IntArray *a);
void sp_IntArray_sort_bang(sp_IntArray *a);
void sp_IntArray_uniq_bang(sp_IntArray *a);
void sp_IntArray_shuffle_bang(sp_IntArray *a);
sp_IntArray *sp_IntArray_shuffle(sp_IntArray *a);
mrb_int sp_IntArray_sample(sp_IntArray *a);
mrb_int sp_IntArray_min(sp_IntArray *a);
mrb_int sp_IntArray_max(sp_IntArray *a);
mrb_int sp_IntArray_sum(sp_IntArray *a, mrb_int init);
mrb_bool sp_IntArray_include(sp_IntArray *a, mrb_int v);
mrb_int sp_IntArray_index(sp_IntArray *a, mrb_int v);
mrb_int sp_IntArray_rindex(sp_IntArray *a, mrb_int v);
mrb_int sp_IntArray_delete_at(sp_IntArray *a, mrb_int i);
mrb_int sp_IntArray_delete(sp_IntArray *a, mrb_int v);
void sp_IntArray_insert(sp_IntArray *a, mrb_int i, mrb_int v);
sp_IntArray *sp_IntArray_uniq(sp_IntArray *a);
sp_IntArray *sp_IntArray_intersect(sp_IntArray *a, sp_IntArray *b);
sp_IntArray *sp_IntArray_union(sp_IntArray *a, sp_IntArray *b);
sp_IntArray *sp_IntArray_difference(sp_IntArray *a, sp_IntArray *b);
void sp_IntArray_unshift(sp_IntArray *a, mrb_int v);
const char *sp_IntArray_join(sp_IntArray *a, const char *sep);
mrb_bool sp_IntArray_eq(sp_IntArray *a, sp_IntArray *b);
mrb_int sp_IntArray_cmp(sp_IntArray *a, sp_IntArray *b);

#endif /* SP_ARRAY_H */
