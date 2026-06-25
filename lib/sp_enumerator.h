/* sp_enumerator.h -- Enumerator runtime surface.
 *
 * An sp_Enumerator wraps a generator body -- an `Enumerator.new { |y| ... }`
 * block lowered to a fiber body function -- and layers pull-model iteration on
 * top of the fiber runtime: #next / #peek drive the fiber and surface each
 * value the body yielded, raising StopIteration past the end; #rewind restarts
 * it. The backing fiber is created lazily on first use, so an unconsumed
 * Enumerator costs only the wrapper.
 *
 * The generated `y << v` / `y.yield(v)` lowers to a plain sp_Fiber_yield, which
 * operates on the running fiber (sp_fiber_current) -- so a yield works no matter
 * how deeply it nests inside the body (loop/times/each lower to inline loops in
 * the same C function). See src/codegen.c emit_enumerator_new. */
#ifndef SP_ENUMERATOR_H
#define SP_ENUMERATOR_H

#include "sp_fiber.h"   /* sp_Fiber, sp_RbVal */
#include "sp_alloc.h"   /* sp_PolyArray */

typedef struct sp_Enumerator {
  void (*body)(sp_Fiber *);  /* the Enumerator.new generator block */
  void *user_data;           /* captured-variable struct, or NULL */
  sp_Fiber *fib;             /* backing fiber, created lazily */
  int started;               /* fib has been created for this run */
  int done;                  /* the generator has reached its end */
  int has_peek;              /* `peeked` holds a buffered lookahead value */
  sp_RbVal peeked;
} sp_Enumerator;

sp_Enumerator *sp_Enumerator_new(void (*body)(sp_Fiber *), void *user_data);
/* A fresh Enumerator over the same body + captures, with its own cursor. The
   materializers (#to_a / #first / #each) iterate from the start each call, so
   they run a fresh one -- independent of this Enumerator's #next cursor. */
sp_Enumerator *sp_Enumerator_fresh(sp_Enumerator *e);
/* Pull one value; raise StopIteration past the end. */
sp_RbVal sp_Enumerator_next(sp_Enumerator *e);
/* Like #next, but the value stays buffered for the following #next / #peek. */
sp_RbVal sp_Enumerator_peek(sp_Enumerator *e);
/* Restart from the beginning -- the next #next recreates the fiber. Returns e. */
sp_Enumerator *sp_Enumerator_rewind(sp_Enumerator *e);
/* Non-raising pull, used by the bounded drivers and `each`: returns 1 and sets
   *out when a value is produced, 0 at the end. */
mrb_bool sp_Enumerator_advance(sp_Enumerator *e, sp_RbVal *out);
/* #first -> first value or nil. */
sp_RbVal sp_Enumerator_first(sp_Enumerator *e);
/* #first(n) / #take(n) -> an Array of up to n values. */
sp_PolyArray *sp_Enumerator_first_n(sp_Enumerator *e, mrb_int n);
/* #to_a / #entries -> an Array of every value (loops forever on an unbounded
   generator, matching CRuby). */
sp_PolyArray *sp_Enumerator_to_a(sp_Enumerator *e);

#endif
