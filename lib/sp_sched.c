/* sp_sched.c -- cooperative M:N thread scheduler bodies, Phase 0 (N=1).
 * See sp_sched.h. Built on the sp_fiber context switch: the main thread runs on
 * the root fiber and pumps a run queue of green threads whenever it blocks. */
#include "sp_sched.h"
#include "sp_alloc.h"   /* sp_box_nil / sp_box_obj */
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* Reached by name (defined in lib/sp_alloc.c or the generated TU), exactly as
   lib/sp_fiber.c reaches them. */
void *sp_gc_alloc(size_t sz, void (*fin)(void *), void (*scn)(void *));
SP_NORETURN void sp_raise_cls(const char *cls, const char *msg);
void sp_fiber_reraise(const char *cls, const char *msg, void *obj);

/* ---- scheduler state (single OS worker, so plain globals) ---- */
static sp_Fiber  *g_root_fiber = NULL;   /* the main thread's context, captured at init */
static sp_thread  g_main_thread;         /* the main thread: runs on root, fiber == NULL */
static sp_thread *g_current = NULL;      /* the green thread running right now */
static sp_thread *g_rq_head = NULL, *g_rq_tail = NULL;  /* FIFO run queue (RUNNABLE) */
static sp_thread *g_all = NULL;          /* registry of live threads, for GC rooting */
static unsigned   g_next_id = 1;

static void rq_push(sp_thread *t) {
  t->state = SP_TH_RUNNABLE;
  t->rq_next = NULL;
  if (g_rq_tail) g_rq_tail->rq_next = t; else g_rq_head = t;
  g_rq_tail = t;
}
static sp_thread *rq_pop(void) {
  sp_thread *t = g_rq_head;
  if (t) { g_rq_head = t->rq_next; if (!g_rq_head) g_rq_tail = NULL; t->rq_next = NULL; }
  return t;
}

static void reg_add(sp_thread *t) {
  t->all_prev = NULL; t->all_next = g_all;
  if (g_all) g_all->all_prev = t;
  g_all = t;
}
static void reg_remove(sp_thread *t) {
  if (t->all_prev) t->all_prev->all_next = t->all_next;
  else if (g_all == t) g_all = t->all_next;
  if (t->all_next) t->all_next->all_prev = t->all_prev;
  t->all_prev = t->all_next = NULL;
}

/* GC: root every live green thread (and thus its fiber, stack roots, and
   pending result) so a fire-and-forget thread with no user reference is not
   collected mid-run. Chained ahead of whatever globals hook was installed. */
static void (*g_prev_globals_hook)(void) = NULL;
static void sp_sched_globals_mark(void) {
  for (sp_thread *t = g_all; t; t = t->all_next) sp_gc_mark(t);
  if (g_prev_globals_hook) g_prev_globals_hook();
}

void sp_sched_init(void) {
  /* Called from main() on the root fiber, so sp_fiber_current is the root. */
  g_root_fiber = sp_fiber_current;
  memset(&g_main_thread, 0, sizeof g_main_thread);
  g_main_thread.fiber = NULL;
  g_main_thread.state = SP_TH_RUNNING;
  g_main_thread.report_on_exception = 1;
  g_current = &g_main_thread;
  g_prev_globals_hook = sp_gc_mark_globals_hook;
  sp_gc_mark_globals_hook = sp_sched_globals_mark;
}

static void sp_thread_report(sp_thread *t) {
  fprintf(stderr, "#<Thread:%u> terminated with exception: %s (%s)\n",
          t->id, t->exc_msg ? t->exc_msg : "", t->exc_cls ? t->exc_cls : "Exception");
}

/* A finished thread's parked joiners become runnable again (the main thread,
   if it was waiting, is released by the pump's target check, not the queue). */
static void sp_thread_wake_joiners(sp_thread *t) {
  sp_thread *j = t->joiners;
  t->joiners = NULL;
  while (j) {
    sp_thread *n = j->join_next;
    j->join_next = NULL;
    if (j != &g_main_thread) rq_push(j);
    j = n;
  }
}

/* Run runnable green threads until `target` is DEAD, or (target == NULL) until
   the run queue drains. Runs on the root fiber (the main thread). */
static void sp_sched_pump(sp_thread *target) {
  for (;;) {
    if (target && target->state == SP_TH_DEAD) return;
    sp_thread *t = rq_pop();
    if (!t) return;   /* nothing runnable: drained, or a deadlock the caller observes */
    sp_thread *saved = g_current;
    g_current = t;
    t->state = SP_TH_RUNNING;
    int raised = 0;
    const char *ec = NULL, *em = NULL;
    void *eo = NULL;
    sp_Fiber_transfer_catch(t->fiber, sp_box_nil(), &raised, &ec, &em, &eo);
    g_current = saved;
    if (t->fiber->state == 3) {   /* the body returned (terminated) */
      t->retval = t->fiber->yielded_value;
      t->state = SP_TH_DEAD;
      if (raised) {
        t->has_exc = 1; t->exc_cls = ec; t->exc_msg = em; t->exc_obj = eo;
        if (t->report_on_exception) sp_thread_report(t);
      }
      sp_thread_wake_joiners(t);
      reg_remove(t);   /* collectable once no user reference remains */
    }
    /* otherwise t yielded back: it parked itself (joiners list) or re-queued
       itself (Thread.pass) before transferring, so the queue state is correct. */
  }
}

static void sp_thread_scan(void *p) {
  sp_thread *t = (sp_thread *)p;
  if (t->fiber) sp_gc_mark(t->fiber);
  sp_mark_rbval(t->retval);
  if (t->exc_obj) sp_gc_mark(t->exc_obj);
}

sp_thread *sp_Thread_spawn_fiber(sp_Fiber *f) {
  SP_GC_ROOT(f);   /* root the freshly-built fiber across the allocation below */
  sp_thread *volatile t = (sp_thread *)sp_gc_alloc(sizeof(sp_thread), NULL, sp_thread_scan);
  memset(t, 0, sizeof *t);
  t->fiber = f;
  t->retval = sp_box_nil();
  t->report_on_exception = 1;
  t->id = g_next_id++;
  reg_add(t);
  rq_push(t);
  return t;
}

/* Block the calling thread until `t` is dead. The main thread pumps the queue;
   a spawned thread parks on t's joiners and yields to the scheduler. */
static void sp_thread_await(sp_thread *t) {
  if (t->state == SP_TH_DEAD) return;
  sp_thread *self = g_current;
  if (self == &g_main_thread) {
    sp_sched_pump(t);
    if (t->state != SP_TH_DEAD)
      sp_raise_cls("ThreadError", "deadlock detected: no runnable thread");
  } else {
    self->state = SP_TH_BLOCKED;
    self->join_next = t->joiners;
    t->joiners = self;
    sp_Fiber_transfer(g_root_fiber, sp_box_nil());   /* resumes once t is dead */
  }
}

/* CRuby: #join and #value re-raise the thread's unhandled exception in the
   joining thread. */
static void sp_thread_reraise_if_exc(sp_thread *t) {
  if (t->has_exc) sp_fiber_reraise(t->exc_cls, t->exc_msg, t->exc_obj);
}

sp_thread *sp_Thread_join(sp_thread *t) {
  sp_thread_await(t);
  sp_thread_reraise_if_exc(t);
  return t;
}

sp_RbVal sp_Thread_value(sp_thread *t) {
  sp_thread_await(t);
  sp_thread_reraise_if_exc(t);
  return t->retval;
}

void sp_Thread_pass(void) {
  sp_thread *self = g_current;
  if (self == &g_main_thread) {
    sp_sched_pump(NULL);   /* give every currently-runnable thread a turn */
  } else {
    rq_push(self);
    sp_Fiber_transfer(g_root_fiber, sp_box_nil());
  }
}

sp_thread *sp_Thread_current(void) { return g_current; }

mrb_bool sp_Thread_alive(sp_thread *t) { return t->state != SP_TH_DEAD; }

void sp_sched_drain(void) {
  /* main() is finishing: run remaining runnable threads so fire-and-forget
     side effects happen. Only meaningful when called from the main thread. */
  if (g_current == &g_main_thread) sp_sched_pump(NULL);
}
