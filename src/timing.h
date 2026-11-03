/* Opt-in compile-phase timing (--timing, or SP_TIMING in the environment).
   One stderr line per phase: `spinel-timing: phase=<name> ms=<wall> [key=val ...]`.
   Wall time on a monotonic clock. A phase nested in another (analysis in
   codegen_program) is counted in both; cc_compile is the wall span of the
   parallel object compiles, not the sum of the workers. SP_TIMING=cpu
   reports this process's CPU time in place of wall time, which a loaded
   host does not inflate (the cc steps run in children, so they read ~0).
   Off: one cached getenv per translation unit, no clock read. */
#ifndef SPINEL_TIMING_H
#define SPINEL_TIMING_H
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* 0 off, 1 wall, 2 cpu */
static inline int sp_timing_on(void) {
  static int on = -1;
  if (on < 0) {
    const char *v = getenv("SP_TIMING");
    on = !v ? 0 : strcmp(v, "cpu") == 0 ? 2 : 1;
  }
  return on;
}

static inline double sp_timing_now(void) {
  if (!sp_timing_on()) return 0;
  struct timespec ts;
  clock_gettime(sp_timing_on() == 2 ? CLOCK_PROCESS_CPUTIME_ID : CLOCK_MONOTONIC, &ts);
  return (double)ts.tv_sec * 1000.0 + (double)ts.tv_nsec / 1e6;
}

/* extra: "" or " key=val ..." with its leading space */
static inline void sp_timing_end(double t0, const char *phase, const char *extra) {
  if (!sp_timing_on()) return;
  fprintf(stderr, "spinel-timing: phase=%s ms=%.1f%s\n", phase, sp_timing_now() - t0, extra ? extra : "");
}
#endif
