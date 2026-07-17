/* sp_random.h -- the shared Kernel-level PRNG stream (lib/sp_random.c).
 *
 * PCG-XSH-RR, the generator mruby's mruby-random uses: much better
 * statistical quality than libc rand(), no cryptographic claims. One
 * stream is shared by every runtime TU -- Kernel#rand/#srand, the array
 * shuffle/sample paths (generated TU and libspinel_rt.a alike) and the
 * Random default instance all draw from it, so a single `srand(n)`
 * makes every consumer reproducible. */
#ifndef SP_RANDOM_H
#define SP_RANDOM_H

#include <stdint.h>
#include "sp_types.h"

uint32_t sp_pcg32_adv(uint64_t *state);   /* one generator step */
void sp_pcg_seed(uint64_t *state, uint64_t seed);
uint64_t sp_krand_next(void);             /* 64-bit draw, lazily self-seeded */
void sp_krand_srand(uint64_t seed);
mrb_int sp_krand_below(mrb_int n);        /* uniform [0, n); 0 when n <= 0 */
mrb_float sp_krand_float(void);           /* uniform [0, 1) */

#endif
