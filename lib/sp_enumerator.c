/* sp_enumerator.c -- Enumerator runtime bodies. See sp_enumerator.h.
 *
 * Built entirely on the fiber runtime (lib/sp_fiber.c): an Enumerator is a lazy
 * fiber plus a one-slot lookahead buffer and a `done` latch. sp_gc_alloc is
 * reached by name (forward-declared below); sp_raise_cls comes from sp_alloc.h. */
#include "sp_enumerator.h"
#include "sp_gc.h"      /* sp_gc_roots / _sp_gc_root_push, sp_gc_mark, sp_mark_rbval */

void *sp_gc_alloc(size_t sz, void (*fin)(void *), void (*scn)(void *));

/* The collector reaches an Enumerator's live references through this hook: the
   capture struct, the backing fiber, and any buffered peek value. */
static void sp_Enumerator_scan(void *p) {
  sp_Enumerator *e = (sp_Enumerator *)p;
  if (e->user_data) sp_gc_mark(e->user_data);
  if (e->fib) sp_gc_mark((void *)e->fib);
  if (e->has_peek) sp_mark_rbval(e->peeked);
}

sp_Enumerator *sp_Enumerator_new(void (*body)(sp_Fiber *), void *user_data) {
  sp_Enumerator *e = (sp_Enumerator *)sp_gc_alloc(sizeof(sp_Enumerator), NULL, sp_Enumerator_scan);
  e->body = body;
  e->user_data = user_data;
  e->fib = NULL;
  e->started = 0;
  e->done = 0;
  e->has_peek = 0;
  e->peeked = sp_box_nil();
  return e;
}

sp_Enumerator *sp_Enumerator_fresh(sp_Enumerator *e) {
  return sp_Enumerator_new(e->body, e->user_data);
}

mrb_bool sp_Enumerator_advance(sp_Enumerator *e, sp_RbVal *out) {
  if (e->has_peek) { *out = e->peeked; e->has_peek = 0; e->peeked = sp_box_nil(); return 1; }
  if (e->done) return 0;
  if (!e->started) {
    /* No allocation between sp_Fiber_new returning and storing it, so the fresh
       fiber cannot be collected before the Enumerator's scan can reach it. */
    e->fib = sp_Fiber_new(e->body);
    e->fib->user_data = e->user_data;
    e->started = 1;
  }
  if (!sp_Fiber_alive(e->fib)) { e->done = 1; return 0; }
  sp_RbVal v = sp_Fiber_resume(e->fib, sp_box_nil());
  /* A resume that terminated the fiber ran the body's tail (or an empty body)
     without a fresh yield -- its return value is not an element, so stop. */
  if (!sp_Fiber_alive(e->fib)) { e->done = 1; return 0; }
  *out = v;
  return 1;
}

sp_RbVal sp_Enumerator_next(sp_Enumerator *e) {
  sp_RbVal v;
  if (!sp_Enumerator_advance(e, &v)) sp_raise_cls("StopIteration", "iteration reached an end");
  return v;
}

sp_RbVal sp_Enumerator_peek(sp_Enumerator *e) {
  if (e->has_peek) return e->peeked;
  sp_RbVal v;
  if (!sp_Enumerator_advance(e, &v)) sp_raise_cls("StopIteration", "iteration reached an end");
  e->peeked = v;
  e->has_peek = 1;
  return v;
}

sp_Enumerator *sp_Enumerator_rewind(sp_Enumerator *e) {
  /* Abandon the current fiber (the collector reclaims it) and reset; the next
     advance lazily creates a fresh one from the same body + captures. */
  e->fib = NULL;
  e->started = 0;
  e->done = 0;
  e->has_peek = 0;
  e->peeked = sp_box_nil();
  return e;
}

sp_RbVal sp_Enumerator_first(sp_Enumerator *e) {
  /* #first iterates from the start, independent of the #next cursor. */
  sp_Enumerator *fe = sp_Enumerator_fresh(e);
  int rooted = _sp_gc_root_push((void **)&fe);
  sp_RbVal v, out = sp_box_nil();
  if (sp_Enumerator_advance(fe, &v)) out = v;
  _sp_gc_root_pop(&rooted);
  return out;
}

/* Drive a fresh run of the generator into a new Array, stopping at `limit`
   values (or at the end first); limit < 0 means unbounded. Both the fresh
   Enumerator and the in-flight array are rooted across each resume, which can
   allocate and collect. */
static sp_PolyArray *sp_enum_collect(sp_Enumerator *e, mrb_int limit) {
  sp_Enumerator *fe = sp_Enumerator_fresh(e);
  int r1 = _sp_gc_root_push((void **)&fe);
  sp_PolyArray *a = sp_PolyArray_new();
  int r2 = _sp_gc_root_push((void **)&a);
  sp_RbVal v;
  while ((limit < 0 || a->len < limit) && sp_Enumerator_advance(fe, &v)) sp_PolyArray_push(a, v);
  _sp_gc_root_pop(&r2);
  _sp_gc_root_pop(&r1);
  return a;
}

sp_PolyArray *sp_Enumerator_first_n(sp_Enumerator *e, mrb_int n) {
  return sp_enum_collect(e, n < 0 ? 0 : n);
}

sp_PolyArray *sp_Enumerator_to_a(sp_Enumerator *e) {
  return sp_enum_collect(e, -1);
}
