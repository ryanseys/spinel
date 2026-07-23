/* sp_proc.c -- cold sp_Proc/sp_Curry ops (see sp_proc.h). 0 optcarrot uses. */
#include "sp_proc.h"

void sp_Proc_scan(void *p) { sp_Proc *pr = (sp_Proc *)p; if (pr->cap && pr->cap_scan) pr->cap_scan(pr->cap); }
sp_Proc *sp_proc_new_meta(void *fn, void *cap, void (*cap_scan)(void *), mrb_int arity, mrb_bool lambda_p, mrb_int param_count, const sp_sym *param_kinds, const sp_sym *param_names) { sp_Proc *p = (sp_Proc *)sp_gc_alloc(sizeof(sp_Proc), NULL, sp_Proc_scan); p->fn = fn; p->cap = cap; p->cap_scan = cap_scan; p->arity = arity; p->lambda_p = lambda_p; p->param_count = param_count; p->param_kinds = param_kinds; p->param_names = param_names; return p; }
sp_Proc *sp_proc_dup(sp_Proc *p, int keep_frozen) {
  if (!p) return p;
  SP_GC_ROOT(p);
  sp_Proc *r = (sp_Proc *)sp_gc_alloc(sizeof(sp_Proc), NULL, sp_Proc_scan);
  *r = *p;
  if (!keep_frozen) r->frozen = 0;
  r->origin = sp_proc_root(p);   /* share the lineage root so dup == original */
  return r;
}
sp_Proc *sp_proc_new(void *fn, void *cap, void (*cap_scan)(void *)) { return sp_proc_new_meta(fn, cap, cap_scan, 0, FALSE, 0, NULL, NULL); }
mrb_int sp_proc_arity(sp_Proc *p) { return p ? p->arity : 0; }
mrb_bool sp_proc_lambda_p(sp_Proc *p) { return p ? p->lambda_p : FALSE; }
const char *sp_proc_inspect(sp_Proc *p) {
  if (!p) return "nil";
  return sp_sprintf(p->lambda_p ? "#<Proc:0x%016llx (lambda)>" : "#<Proc:0x%016llx>",
                    (unsigned long long)(uintptr_t)p);
}
void sp_proc_lambda_arity_check(mrb_int argc, mrb_int req, mrb_int opt, mrb_bool has_rest, mrb_bool has_kw) {
  /* a lambda with keyword parameters accepts one extra trailing argument (the
     keyword hash) beyond its positional maximum. */
  mrb_int max = req + opt + (has_kw ? 1 : 0);
  if (argc < req || (!has_rest && argc > max)) sp_raise_cls("ArgumentError", "wrong number of arguments");
}
sp_PolyArray *sp_proc_parameters_ids(sp_Proc *p, int mode, sp_sym req_id, sp_sym opt_id) {
  sp_PolyArray *r = sp_PolyArray_new();
  if (!p || p->param_count <= 0 || !p->param_kinds) return r;
  SP_GC_ROOT(r);
  int want_lambda = mode >= 0 ? mode : (p->lambda_p ? 1 : 0);
  for (mrb_int i = 0; i < p->param_count; i++) {
    sp_sym k = p->param_kinds[i];
    if (!want_lambda && k == req_id) k = opt_id;
    sp_PolyArray *pair = sp_PolyArray_new();
    sp_PolyArray_push(pair, sp_box_sym(k));
    if (p->param_names && p->param_names[i] >= 0) sp_PolyArray_push(pair, sp_box_sym(p->param_names[i]));
    sp_PolyArray_push(r, sp_box_poly_array(pair));
  }
  return r;
}
sp_PolyArray *sp_proc_parameters(sp_Proc *p) { sp_PolyArray *r = sp_PolyArray_new(); if (!p || p->param_count <= 0 || !p->param_kinds) return r; SP_GC_ROOT(r); for (mrb_int i = 0; i < p->param_count; i++) { sp_PolyArray *pair = sp_PolyArray_new(); sp_PolyArray_push(pair, sp_box_sym(p->param_kinds[i])); if (p->param_names && p->param_names[i] >= 0) sp_PolyArray_push(pair, sp_box_sym(p->param_names[i])); sp_PolyArray_push(r, sp_box_poly_array(pair)); } return r; }
void sp_curry_scan(void *p) { sp_Curry *c = (sp_Curry *)p; if (c->target) sp_gc_mark(c->target); for (mrb_int i = 0; i < c->nargs && i < 16; i++) sp_mark_rbval(c->args[i]); }
sp_Curry *sp_curry_new(sp_Proc *p) {
  SP_GC_ROOT(p);  /* the target proc has no other root across this alloc */
  sp_Curry *c = (sp_Curry *)sp_gc_alloc(sizeof(sp_Curry), NULL, sp_curry_scan);
  c->target = p; c->nargs = 0;
  return c;
}
sp_Curry *sp_curry_apply(sp_Curry *c, sp_RbVal arg) {
  /* root the source accumulator: in a chained apply it is only referenced
     from a C argument slot, and this allocation can collect */
  SP_GC_ROOT(c);
  sp_Curry *n = (sp_Curry *)sp_gc_alloc(sizeof(sp_Curry), NULL, sp_curry_scan);
  *n = *c;
  if (n->nargs < 16) n->args[n->nargs++] = arg;
  return n;
}
void sp_curry_publish_args(sp_Curry *c) {
  for (mrb_int i = 0; i < c->nargs && i < 16; i++)
    _sp_proc_poly_args[i] = c->args[i];
}
