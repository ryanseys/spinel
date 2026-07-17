/* sp_random.c -- the shared Kernel-level PRNG stream (see sp_random.h). */
#include <time.h>
#include "sp_random.h"

/* PCG-XSH-RR constants (64-bit state, 32-bit output). */
#define SP_PCG_MULT 6364136223846793005ULL
#define SP_PCG_INC  1442695040888963407ULL

uint32_t sp_pcg32_adv(uint64_t *state) {
  uint64_t old = *state;
  *state = old * SP_PCG_MULT + SP_PCG_INC;
  /* output: xorshift-high folds the state, then a random rotate */
  uint32_t xs = (uint32_t)(((old >> 18u) ^ old) >> 27u);
  uint32_t rot = (uint32_t)(old >> 59u);
  return (xs >> rot) | (xs << ((32 - rot) & 31));
}

/* The PCG init sequence: zero the state, step, add the seed, then mix.
   The extra mixing rounds keep nearby seeds from producing correlated
   early output. */
void sp_pcg_seed(uint64_t *state, uint64_t seed) {
  *state = 0;
  (void)sp_pcg32_adv(state);
  *state += seed;
  for (int i = 0; i < 10; i++) (void)sp_pcg32_adv(state);
}

/* Per-worker (SP_TLS) in the threaded build: the generator has no
   internal lock, so a shared stream would race across workers. */
static SP_TLS uint64_t sp_krand_state;
static SP_TLS int sp_krand_seeded;

void sp_krand_srand(uint64_t seed) {
  sp_pcg_seed(&sp_krand_state, seed);
  sp_krand_seeded = 1;
}

uint64_t sp_krand_next(void) {
  if (!sp_krand_seeded) {
    /* Auto-seed on first draw, mirroring CRuby's startup seeding so
       unseeded rand/shuffle/sample vary per run. time() alone repeats
       within a second; mix in clock() and the per-worker state address. */
    sp_krand_srand((uint64_t)time(NULL) ^ ((uint64_t)clock() << 24) ^
                   (uint64_t)(uintptr_t)&sp_krand_state);
  }
  uint64_t hi = sp_pcg32_adv(&sp_krand_state);
  return (hi << 32) | sp_pcg32_adv(&sp_krand_state);
}

mrb_int sp_krand_below(mrb_int n) {
  if (n <= 0) return 0;
  /* uniform in [0, n) without modulo bias: reject draws below the
     wrap-around threshold (0 for powers of two, so the loop is rare) */
  uint64_t umax = (uint64_t)n;
  uint64_t threshold = (uint64_t)(-(int64_t)umax) % umax;
  uint64_t r;
  do {
    r = sp_krand_next();
  } while (r < threshold);
  return (mrb_int)(r % umax);
}

mrb_float sp_krand_float(void) {
  return (mrb_float)(sp_krand_next() >> 11) / (mrb_float)(1ULL << 53);
}
