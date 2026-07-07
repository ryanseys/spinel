#include "analyze_internal.h"
#include <stdio.h>
#include <stdlib.h>

/* Debug: trace a single ivar's type transitions. Gated by SP_IVWATCH=<name>
   (bare name, no @). Zero-cost when the env var is unset. */
void sp_ivwatch(const char *name, const char *where, TyKind old, TyKind nw) {
  if (old == nw || !name) return;
  static const char *want = NULL;
  static int inited = 0;
  if (!inited) { want = getenv("SP_IVWATCH"); inited = 1; }
  if (!want) return;
  if (name[0] == '@') name++;        /* match with or without leading @ */
  if (!sp_streq(want, name)) return;
  fprintf(stderr, "[ivwatch %s] %-28s %d(%s) -> %d(%s)\n",
          name, where, (int)old, ty_name(old < 1000 ? old : TY_POLY),
          (int)nw, ty_name(nw < 1000 ? nw : TY_POLY));
}

/* `...` forwards the caller's args verbatim, so rather than a rest array we
   synthesize concrete positional params whose count is the widest positional
   arg count across this method's call sites (the compiler already knows the
   args it receives). Returns that count. Matches call sites by name -- the
   common free-function / single-definition forwarding case (#1288). */
static int forwarding_call_arity(Compiler *c, const char *mname) {
  const NodeTable *nt = c->nt;
  int maxarg = 0;
  for (int id = 0; id < nt->count; id++) {
    const char *ty = nt_type(nt, id);
    if (!ty || !sp_streq(ty, "CallNode")) continue;
    const char *cn = nt_str(nt, id, "name");
    if (!cn || !sp_streq(cn, mname)) continue;
    int a = nt_ref(nt, id, "arguments");
    int an = 0; const int *av = a >= 0 ? nt_arr(nt, a, "arguments", &an) : NULL;
    if (an == 0) continue;
    /* a `foo(...)` forwarding call is not a concrete arg count */
    if (an == 1 && nt_type(nt, av[0]) && sp_streq(nt_type(nt, av[0]), "ForwardingArgumentsNode")) continue;
    int pos = an;
    if (an > 0 && nt_type(nt, av[an - 1]) && sp_streq(nt_type(nt, av[an - 1]), "KeywordHashNode")) pos = an - 1;
    if (pos > maxarg) maxarg = pos;
  }
  return maxarg;
}

void collect_def_params(Compiler *c, int def_id, Scope *s) {
  int pn = nt_ref(c->nt, def_id, "parameters");
  if (pn < 0) return;
  int rn = 0;
  const int *reqs = nt_arr(c->nt, pn, "requireds", &rn);
  for (int i = 0; i < rn; i++) {
    const char *pname = nt_str(c->nt, reqs[i], "name");
    if (pname) scope_add_param(s, pname, -1);
  }
  int on = 0;
  const int *opts = nt_arr(c->nt, pn, "optionals", &on);
  for (int i = 0; i < on; i++) {
    const char *pname = nt_str(c->nt, opts[i], "name");
    int dv = nt_ref(c->nt, opts[i], "value");
    if (pname) scope_add_param(s, pname, dv);
  }
  int rp = nt_ref(c->nt, pn, "rest");
  if (rp >= 0) {
    const char *rpty = nt_type(c->nt, rp);
    if (rpty && sp_streq(rpty, "RestParameterNode")) {
      const char *rname = nt_str(c->nt, rp, "name");
      if (rname) {
        if (s->nparams % 8 == 0) {
          s->pnames  = realloc(s->pnames,  sizeof(char *) * (size_t)(s->nparams + 8));
          s->pdefault = realloc(s->pdefault, sizeof(int)    * (size_t)(s->nparams + 8));
        }
        s->pdefault[s->nparams] = -1;
        s->pnames[s->nparams++] = strdup(rname);
        LocalVar *lv = scope_local_intern(s, rname);
        lv->is_param = 1;
        lv->type = TY_POLY_ARRAY;
        s->rest_idx = s->nparams - 1;
      }
    }
  }
  /* post-splat required parameters (Prism "posts" array) */
  int postn = 0;
  const int *posts = nt_arr(c->nt, pn, "posts", &postn);
  for (int i = 0; i < postn; i++) {
    const char *pname = nt_str(c->nt, posts[i], "name");
    if (pname) scope_add_param(s, pname, -1);
  }
  if (postn > 0) s->npost_rest = postn;
  int kn = 0;
  const int *kws = nt_arr(c->nt, pn, "keywords", &kn);
  for (int i = 0; i < kn; i++) {
    const char *pty = nt_type(c->nt, kws[i]);
    if (!pty) continue;
    const char *pname = nt_str(c->nt, kws[i], "name");
    int dv = sp_streq(pty, "OptionalKeywordParameterNode") ? nt_ref(c->nt, kws[i], "value") : -1;
    if (pname) scope_add_param(s, pname, dv);
  }
  int kwrp = nt_ref(c->nt, pn, "keyword_rest");
  if (kwrp >= 0) {
    const char *kwrpty = nt_type(c->nt, kwrp);
    if (kwrpty && sp_streq(kwrpty, "KeywordRestParameterNode")) {
      const char *kwrname = nt_str(c->nt, kwrp, "name");
      if (kwrname) {
        LocalVar *lv = scope_local_intern(s, kwrname);
        lv->is_param = 1;
        lv->type = TY_SYM_POLY_HASH;
        if (s->nparams % 8 == 0) {
          s->pnames   = realloc(s->pnames,   sizeof(char *) * (size_t)(s->nparams + 8));
          s->pdefault = realloc(s->pdefault, sizeof(int)    * (size_t)(s->nparams + 8));
        }
        s->pdefault[s->nparams] = -1;
        s->pnames[s->nparams++] = strdup(kwrname);
        s->kwrest_idx = s->nparams - 1;
      }
    }
  }
  int bp = nt_ref(c->nt, pn, "block");
  if (bp >= 0 && nt_type(c->nt, bp) && sp_streq(nt_type(c->nt, bp), "BlockParameterNode")) {
    const char *bn = nt_str(c->nt, bp, "name");
    s->blk_param = strdup(bn ? bn : "");
    /* Register the &block param as a local so mark_proc_captures can find it
       and mark it is_cell when a nested proc body captures it. */
    if (bn && bn[0]) {
      LocalVar *blv = scope_local_intern(s, bn);
      blv->is_param = 1;
      blv->type = TY_PROC;
    }
  }
  /* `def foo(...)`: Prism attaches a ForwardingParameterNode as keyword_rest.
     Synthesize concrete positional params __fwd_0.. (arity from the call
     sites); their types fall out of the normal call-site param seeding and a
     `bar(...)` body forwards them directly -- no rest/splat machinery (#1288). */
  {
    int kwr = nt_ref(c->nt, pn, "keyword_rest");
    if (kwr >= 0 && nt_type(c->nt, kwr) &&
        sp_streq(nt_type(c->nt, kwr), "ForwardingParameterNode") && s->name) {
      int arity = forwarding_call_arity(c, s->name);
      for (int i = 0; i < arity; i++) {
        char nm[24]; snprintf(nm, sizeof nm, "__fwd_%d", i);
        scope_add_param(s, nm, -1);
      }
      /* Keyword args ride the same model: spinel compiles keyword params as
         positional C params mapped at the call site by name, so synthesizing a
         param named after each forwarded key lets the positional forward carry
         it (#1288). Collect the union of keys across the call sites. */
      const NodeTable *nt = c->nt;
      for (int id = 0; id < nt->count; id++) {
        const char *ty = nt_type(nt, id);
        if (!ty || !sp_streq(ty, "CallNode") || !nt_str(nt, id, "name") ||
            !sp_streq(nt_str(nt, id, "name"), s->name)) continue;
        int a = nt_ref(nt, id, "arguments");
        int an = 0; const int *av = a >= 0 ? nt_arr(nt, a, "arguments", &an) : NULL;
        if (an == 0 || !nt_type(nt, av[an - 1]) ||
            !sp_streq(nt_type(nt, av[an - 1]), "KeywordHashNode")) continue;
        int en = 0; const int *els = nt_arr(nt, av[an - 1], "elements", &en);
        for (int e = 0; e < en; e++) {
          int key = nt_ref(nt, els[e], "key");
          const char *kty = key >= 0 ? nt_type(nt, key) : NULL;
          const char *kn = (kty && sp_streq(kty, "SymbolNode")) ? nt_str(nt, key, "value") : NULL;
          if (!kn) continue;
          int dup = 0;
          for (int p = 0; p < s->nparams; p++) if (sp_streq(s->pnames[p], kn)) { dup = 1; break; }
          if (!dup) scope_add_param(s, kn, -1);
        }
      }
    }
  }
}

/* True if `s` is a `def m(...)` forwarding method (keyword_rest is a
   ForwardingParameterNode). */
static int scope_is_forwarding(Compiler *c, Scope *s) {
  if (!s || s->def_node < 0) return 0;
  int pn = nt_ref(c->nt, s->def_node, "parameters");
  if (pn < 0) return 0;
  int kwr = nt_ref(c->nt, pn, "keyword_rest");
  return kwr >= 0 && nt_type(c->nt, kwr) &&
         sp_streq(nt_type(c->nt, kwr), "ForwardingParameterNode");
}

/* The method `s`'s body forwards `...` to (a `callee(...)` call). Returns the
   callee scope index, or -1 if none/unresolved. */
static int forwarding_target_idx(Compiler *c, Scope *s) {
  const NodeTable *nt = c->nt;
  for (int id = 0; id < nt->count; id++) {
    const char *ty = nt_type(nt, id);
    if (!ty || !sp_streq(ty, "CallNode") || comp_scope_of(c, id) != s) continue;
    int a = nt_ref(nt, id, "arguments");
    int an = 0; const int *av = a >= 0 ? nt_arr(nt, a, "arguments", &an) : NULL;
    if (an != 1 || !nt_type(nt, av[0]) ||
        !sp_streq(nt_type(nt, av[0]), "ForwardingArgumentsNode")) continue;
    const char *cn = nt_str(nt, id, "name");
    if (!cn) continue;
    int recv = nt_ref(nt, id, "receiver");
    if (recv >= 0) continue;  /* receiver-qualified target: not resolved here */
    int mi = comp_method_index(c, cn);
    if (mi < 0 && s->class_id >= 0) mi = comp_method_in_chain(c, s->class_id, cn, NULL);
    if (mi < 0 && s->class_id >= 0) mi = comp_cmethod_in_chain(c, s->class_id, cn, NULL);
    if (mi >= 0) return mi;
  }
  return -1;
}

/* Chained `...`: a forwarding method called only via another `f(...)` forward
   has no concrete call site, so its call-site arity is 0. Top its synthesized
   positional params up to its forwarding target's arity, to a fixpoint, so
   `def h(...); f(...); end; def f(...); g(a,b); end` propagates g's arity back
   through f and h (#1288). */
void topup_forwarding_arity(Compiler *c) {
  int changed = 1;
  for (int iter = 0; iter < 32 && changed; iter++) {
    changed = 0;
    for (int s = 1; s < c->nscopes; s++) {
      Scope *sc = &c->scopes[s];
      if (!scope_is_forwarding(c, sc)) continue;
      int tgt = forwarding_target_idx(c, sc);
      if (tgt < 0 || tgt == s) continue;
      int want = c->scopes[tgt].nparams;
      while (sc->nparams < want) {
        char nm[24]; snprintf(nm, sizeof nm, "__fwd_%d", sc->nparams);
        scope_add_param(sc, nm, -1);
        changed = 1;
      }
    }
  }
}

void walk_scope(Compiler *c, int id, int scope_idx, int class_id);

/* String form of an int/string/symbol literal node, for compile-time
   `define_method` name interpolation. Returns malloc'd, or NULL. */
char *dm_lit_str(Compiler *c, int lit) {
  const NodeTable *nt = c->nt;
  const char *ty = nt_type(nt, lit);
  if (!ty) return NULL;
  if (sp_streq(ty, "IntegerNode")) {
    char buf[32]; snprintf(buf, sizeof buf, "%lld", (long long)nt_int(nt, lit, "value", 0));
    return strdup(buf);
  }
  if (sp_streq(ty, "StringNode")) {
    const char *s = nt_str(nt, lit, "content");
    if (!s) s = nt_str(nt, lit, "unescaped");
    return s ? strdup(s) : NULL;
  }
  if (sp_streq(ty, "SymbolNode")) { const char *s = nt_str(nt, lit, "value"); return s ? strdup(s) : NULL; }
  return NULL;
}

/* Evaluate a `define_method(<name-expr>)` name with the each-loop variable
   `bv` bound to literal `lit`. Handles string/symbol literals, a bare loop
   variable, and (interpolated) string/symbol nodes. Returns malloc'd name
   or NULL when not statically resolvable. */
char *dm_eval_name(Compiler *c, int node, const char *bv, int lit) {
  const NodeTable *nt = c->nt;
  const char *ty = nt_type(nt, node);
  if (!ty) return NULL;
  if (sp_streq(ty, "StringNode")) {
    const char *s = nt_str(nt, node, "content");
    if (!s) s = nt_str(nt, node, "unescaped");
    return s ? strdup(s) : NULL;
  }
  if (sp_streq(ty, "SymbolNode")) { const char *s = nt_str(nt, node, "value"); return s ? strdup(s) : NULL; }
  if (sp_streq(ty, "LocalVariableReadNode")) {
    const char *nm = nt_str(nt, node, "name");
    if (nm && bv && sp_streq(nm, bv)) return dm_lit_str(c, lit);
    return NULL;
  }
  if (sp_streq(ty, "EmbeddedStatementsNode")) {
    int body = nt_ref(nt, node, "statements");
    int bn = 0; const int *bb = body >= 0 ? nt_arr(nt, body, "body", &bn) : NULL;
    if (bn != 1) return NULL;
    return dm_eval_name(c, bb[0], bv, lit);
  }
  if (sp_streq(ty, "InterpolatedStringNode") || sp_streq(ty, "InterpolatedSymbolNode")) {
    int pn = 0; const int *parts = nt_arr(nt, node, "parts", &pn);
    char *out = strdup("");
    for (int k = 0; k < pn; k++) {
      char *p = dm_eval_name(c, parts[k], bv, lit);
      if (!p) { free(out); return NULL; }
      size_t no = strlen(out) + strlen(p) + 1;
      char *merged = malloc(no); snprintf(merged, no, "%s%s", out, p);
      free(out); free(p); out = merged;
    }
    return out;
  }
  return NULL;
}

/* TyKind of an int/string/symbol literal node (for the unrolled method's
   subst-var type and return type). */
TyKind dm_lit_type(Compiler *c, int lit) {
  const char *ty = nt_type(c->nt, lit);
  if (!ty) return TY_UNKNOWN;
  if (sp_streq(ty, "IntegerNode")) return TY_INT;
  if (sp_streq(ty, "StringNode"))  return TY_STRING;
  if (sp_streq(ty, "SymbolNode"))  return TY_SYMBOL;
  return TY_UNKNOWN;
}

/* Detect `[lit, ...].each { |v| define_method("m_#{v}") { body } }` in a
   class body and synthesize one method scope per literal element, each with
   a compile-time substitution of `v`. Returns 1 if handled. */
int collect_dm_each_unroll(Compiler *c, int id, int class_id) {
  const NodeTable *nt = c->nt;
  if (class_id < 0) return 0;
  const char *nm = nt_str(nt, id, "name");
  if (!nm || !sp_streq(nm, "each")) return 0;
  int recv = nt_ref(nt, id, "receiver");
  if (recv < 0 || !nt_type(nt, recv) || !sp_streq(nt_type(nt, recv), "ArrayNode")) return 0;
  int blk = nt_ref(nt, id, "block");
  if (blk < 0) return 0;
  /* block parameter name */
  int pn = nt_ref(nt, blk, "parameters");
  int inner = pn >= 0 ? nt_ref(nt, pn, "parameters") : -1;
  int pnode = inner >= 0 ? inner : pn;
  int rnp = 0; const int *reqs = pnode >= 0 ? nt_arr(nt, pnode, "requireds", &rnp) : NULL;
  if (rnp < 1) return 0;
  const char *bv = nt_str(nt, reqs[0], "name");
  if (!bv) return 0;
  /* block body must be a single define_method call */
  int body = nt_ref(nt, blk, "body");
  int bn = 0; const int *bb = body >= 0 ? nt_arr(nt, body, "body", &bn) : NULL;
  if (bn != 1) return 0;
  int dc = bb[0];
  if (!nt_type(nt, dc) || !sp_streq(nt_type(nt, dc), "CallNode")) return 0;
  const char *dcn = nt_str(nt, dc, "name");
  if (!dcn || !sp_streq(dcn, "define_method") || nt_ref(nt, dc, "receiver") >= 0) return 0;
  int dargs = nt_ref(nt, dc, "arguments");
  int dan = 0; const int *dav = dargs >= 0 ? nt_arr(nt, dargs, "arguments", &dan) : NULL;
  if (dan < 1) return 0;
  int dblk = nt_ref(nt, dc, "block");
  if (dblk < 0) return 0;
  int dbody = nt_ref(nt, dblk, "body");
  /* iterate the array literal's elements */
  int en = 0; const int *elems = nt_arr(nt, recv, "elements", &en);
  if (en == 0) return 0;
  for (int k = 0; k < en; k++) {
    TyKind lt = dm_lit_type(c, elems[k]);
    if (lt == TY_UNKNOWN) return 0;  /* non-literal element: bail (unhandled) */
    char *mname = dm_eval_name(c, dav[0], bv, elems[k]);
    if (!mname) return 0;
    Scope *ms = comp_scope_new(c, mname, dc);
    free(mname);
    ms->body = dbody;
    ms->class_id = class_id;
    ms->dm_subst_name = strdup(bv);
    ms->dm_subst_node = elems[k];
    /* the loop var reads inside the body resolve to the literal type */
    LocalVar *lv = scope_local_intern(ms, bv);
    lv->type = lt;
    lv->is_param = 1;  /* not a real C param, but keeps it out of decls */
    /* Walk the (shared) define_method body in this synthetic scope so its
       nodes get nscope attribution. The last element wins for the shared
       body nodes; that is fine since all elements share the value type. */
    int ms_idx = c->nscopes - 1;
    if (dbody >= 0) walk_scope(c, dbody, ms_idx, class_id);
  }
  return 1;
}

/* The class name a TyKind denotes (for `<x>.class` alias resolution). */
const char *builtin_class_of_type(TyKind t) {
  if (t == TY_INT || t == TY_BIGINT) return "Integer";
  if (t == TY_FLOAT) return "Float";
  if (t == TY_STRING) return "String";
  if (t == TY_SYMBOL) return "Symbol";
  return NULL;
}

/* If `cname` is a constant assigned a class value (`CONST = SomeClass` or
   `CONST = <expr>.class`), return the underlying class name so `class CONST`
   reopens that class. Returns NULL if `cname` is a plain new class name. */
/* Index of every ConstantWriteNode id, cached per node table. resolve_class_alias
   is called once per class/module definition during walk_scope; scanning all
   nodes each time made it O(class_defs * nodes) on a flattened runtime. Which
   nodes are ConstantWriteNodes is stable across the pass (only their names may
   have been rewritten earlier, and we re-read those fresh), so the id list can
   be built once and reused. Rebuilt if the node table (pointer or count)
   changes, e.g. a second compile in the same process. */
static const NodeTable *rca_nt = NULL;
static int *rca_ids = NULL;
static int rca_n = 0, rca_ntcount = -1;
const char *resolve_class_alias(Compiler *c, const char *cname) {
  const NodeTable *nt = c->nt;
  if (rca_nt != nt || rca_ntcount != nt->count) {
    free(rca_ids);
    rca_ids = malloc((size_t)nt->count * sizeof(int));
    rca_n = 0;
    if (rca_ids) {
      for (int id = 0; id < nt->count; id++) {
        const char *ty = nt_type(nt, id);
        if (ty && sp_streq(ty, "ConstantWriteNode")) rca_ids[rca_n++] = id;
      }
    }
    rca_nt = nt;
    rca_ntcount = nt->count;
  }
  for (int ii = 0; ii < rca_n; ii++) {
    int id = rca_ids[ii];
    const char *n = nt_str(nt, id, "name");
    if (!n || !sp_streq(n, cname)) continue;
    int v = nt_ref(nt, id, "value");
    if (v < 0) return NULL;
    const char *vty = nt_type(nt, v);
    if (vty && (sp_streq(vty, "ConstantReadNode") || sp_streq(vty, "ConstantPathNode"))) {
      const char *vn = nt_str(nt, v, "name");
      if (vn && (comp_class_index(c, vn) >= 0 || is_builtin_class_name(vn))) return vn;
    }
    if (vty && sp_streq(vty, "CallNode") && nt_str(nt, v, "name") &&
        sp_streq(nt_str(nt, v, "name"), "class")) {
      int r = nt_ref(nt, v, "receiver");
      if (r >= 0) return builtin_class_of_type(infer_type(c, r));
    }
    return NULL;
  }
  return NULL;
}

/* compiler_state_* class macros: declare a bag of typed instance variables
   and auto-synthesize init/dump/set methods.
   The CRuby shim that would define these via define_method is dead code under
   RUBY_ENGINE != "ruby"; spinel recognizes the macros natively here. */
const char *cs_macro_kind(const char *nm) {
  if (!nm) return NULL;
  if (sp_streq(nm, "compiler_state_int")) return "int";
  if (sp_streq(nm, "compiler_state_str")) return "str";
  if (sp_streq(nm, "compiler_state_sa"))  return "sa";
  if (sp_streq(nm, "compiler_state_ia"))  return "ia";
  return NULL;
}
static TyKind cs_field_type(const char *kind) {
  if (sp_streq(kind, "str")) return TY_STRING;
  if (sp_streq(kind, "sa"))  return TY_STR_ARRAY;
  if (sp_streq(kind, "ia"))  return TY_INT_ARRAY;
  return TY_INT;
}
/* CS_SYNTH_* markers; mirrored in codegen. */
enum { CS_INIT = 1, CS_DUMP, CS_SET_INT, CS_SET_STR, CS_SET_SA, CS_SET_IA };
static void cs_synth_method(Compiler *c, int class_id, int def_node, const char *name,
                            int cs_synth, TyKind ret, const char **pnames,
                            const TyKind *ptypes, int nparams) {
  for (int s = 0; s < c->nscopes; s++)
    if (c->scopes[s].class_id == class_id && c->scopes[s].name &&
        sp_streq(c->scopes[s].name, name)) return;  /* already present */
  Scope *s = comp_scope_new(c, name, def_node);
  s->class_id = class_id;
  s->cs_synth = cs_synth;
  s->ret = ret;
  s->body = -1;
  for (int i = 0; i < nparams; i++) {
    scope_add_param(s, pnames[i], -1);
    LocalVar *lv = scope_local(s, pnames[i]);
    if (lv) lv->type = ptypes[i];
  }
}
void collect_compiler_state(Compiler *c, int id, int class_id) {
  const NodeTable *nt = c->nt;
  const char *kind = cs_macro_kind(nt_str(nt, id, "name"));
  if (!kind || class_id < 0) return;
  ClassInfo *ci = &c->classes[class_id];
  /* synthesize the 6 methods once (on the first compiler_state_* decl) */
  { const char *p0[] = {"buf"}; TyKind t0[] = {TY_STRING};
    const char *p2i[] = {"name", "val"}; TyKind t2i[] = {TY_STRING, TY_INT};
    const char *p2s[] = {"name", "val"}; TyKind t2s[] = {TY_STRING, TY_STRING};
    const char *p2sa[] = {"name", "val"}; TyKind t2sa[] = {TY_STRING, TY_STR_ARRAY};
    const char *p2ia[] = {"name", "val"}; TyKind t2ia[] = {TY_STRING, TY_INT_ARRAY};
    cs_synth_method(c, class_id, id, "init_compiler_state", CS_INIT, TY_INT, NULL, NULL, 0);
    cs_synth_method(c, class_id, id, "dump_compiler_state_ir", CS_DUMP, TY_STRING, p0, t0, 1);
    cs_synth_method(c, class_id, id, "compiler_state_set_int", CS_SET_INT, TY_INT, p2i, t2i, 2);
    cs_synth_method(c, class_id, id, "compiler_state_set_str", CS_SET_STR, TY_INT, p2s, t2s, 2);
    cs_synth_method(c, class_id, id, "compiler_state_set_sa", CS_SET_SA, TY_INT, p2sa, t2sa, 2);
    cs_synth_method(c, class_id, id, "compiler_state_set_ia", CS_SET_IA, TY_INT, p2ia, t2ia, 2);
  }
  int args = nt_ref(nt, id, "arguments");
  int an = 0;
  const int *argv = args >= 0 ? nt_arr(nt, args, "arguments", &an) : NULL;
  for (int a = 0; a < an; a++) {
    const char *aty = nt_type(nt, argv[a]);
    if (!aty || !sp_streq(aty, "SymbolNode")) continue;
    const char *fname = nt_str(nt, argv[a], "value");
    if (!fname) continue;
    char ivn[256]; snprintf(ivn, sizeof ivn, "@%s", fname);
    int iv = comp_ivar_intern(ci, ivn);
    ci->ivar_types[iv] = cs_field_type(kind);
    if (ci->ncs >= ci->ccs) {
      ci->ccs = ci->ccs ? ci->ccs * 2 : 16;
      ci->cs_names = realloc(ci->cs_names, sizeof(char *) * (size_t)ci->ccs);
      ci->cs_kinds = realloc(ci->cs_kinds, sizeof(char *) * (size_t)ci->ccs);
    }
    ci->cs_names[ci->ncs] = strdup(fname);
    ci->cs_kinds[ci->ncs] = strdup(kind);
    ci->ncs++;
  }
}

/* If `id` is a receiverless `define_method(:lit) { }` that walk_scope will
   register as a method scope, return that literal method name; else NULL.
   walk_scope only registers when the name is a literal symbol/string AND a block
   is present, so this mirrors that exact gate. Keeping class_eval_reopen_class's
   purity test in lockstep with it prevents a `define_method` that the registrar
   silently skips (blockless, or a dynamic name) from making the block look like a
   pure reopen -- which would no-op the whole call and drop it without a diagnostic. */
static const char *dm_registerable_name(const NodeTable *nt, int id) {
  const char *ty = nt_type(nt, id);
  if (!ty || !sp_streq(ty, "CallNode")) return NULL;
  const char *nm = nt_str(nt, id, "name");
  if (!nm || !sp_streq(nm, "define_method") || nt_ref(nt, id, "receiver") >= 0) return NULL;
  if (nt_ref(nt, id, "block") < 0) return NULL;
  int args = nt_ref(nt, id, "arguments");
  int na = 0;
  const int *argv = args >= 0 ? nt_arr(nt, args, "arguments", &na) : NULL;
  if (na < 1) return NULL;
  const char *aty = nt_type(nt, argv[0]);
  if (aty && sp_streq(aty, "SymbolNode")) return nt_str(nt, argv[0], "value");
  if (aty && sp_streq(aty, "StringNode")) return nt_str(nt, argv[0], "content");
  return NULL;
}

/* `Klass.class_eval { ... }` / `Klass.module_eval { ... }` (and the bare/`self.`
   forms inside a class body) where the target is a known class and the block body
   is purely method definitions (`def` or a registerable `define_method`). Returns
   the target's class index, else -1.

   The receiver may be a constant (`Klass` / `M::Klass`), resolved by short name;
   or `self`/absent, which reopens `enclosing_class` -- the class whose body we are
   directly in (analyze passes g_cbody_direct, codegen passes g_class_body_id, both
   -1 inside method bodies). Restricting to definition-only blocks keeps a
   class_eval that runs other code falling through to the normal (unsupported) path
   instead of being silently dropped. Used by both analyze (to register the methods
   on the target) and codegen (to emit the call as a no-op). */
int class_eval_reopen_class(Compiler *c, int id, int enclosing_class) {
  const NodeTable *nt = c->nt;
  const char *ty = nt_type(nt, id);
  if (!ty || !sp_streq(ty, "CallNode")) return -1;
  const char *nm = nt_str(nt, id, "name");
  if (!nm || (!sp_streq(nm, "class_eval") && !sp_streq(nm, "module_eval"))) return -1;
  int blk = nt_ref(nt, id, "block");
  if (blk < 0) return -1;
  int recv = nt_ref(nt, id, "receiver");
  const char *recv_ty = recv >= 0 ? nt_type(nt, recv) : NULL;
  int ci;
  if (recv < 0 || (recv_ty && sp_streq(recv_ty, "SelfNode"))) {
    /* bare / `self.` receiver reopens the enclosing class -- but only at
       class-body level, where `self` is the class object. */
    if (enclosing_class < 0) return -1;
    ci = enclosing_class;
  }
  else if (recv_ty && (sp_streq(recv_ty, "ConstantReadNode") ||
                         sp_streq(recv_ty, "ConstantPathNode"))) {
    const char *recv_name = nt_str(nt, recv, "name");
    if (!recv_name) return -1;
    ci = comp_class_index(c, recv_name);
    if (ci < 0) return -1;
  }
  else {
    return -1;
  }
  int body = nt_ref(nt, blk, "body");
  int n = 0; const int *stmts = body >= 0 ? nt_arr(nt, body, "body", &n) : NULL;
  for (int k = 0; k < n; k++) {
    const char *sty = nt_type(nt, stmts[k]);
    if (sty && sp_streq(sty, "DefNode")) continue;
    if (dm_registerable_name(nt, stmts[k])) continue;
    return -1;  /* a non-definition statement: not a pure reopen */
  }
  return ci;
}

void walk_scope(Compiler *c, int id, int scope_idx, int class_id) {
  if (id < 0 || id >= c->nt->count) return;
  c->nscope[id] = scope_idx;
  c->node_cbody[id] = g_cbody_class_id;
  const char *ty = nt_type(c->nt, id);
  int child = scope_idx;
  int child_class = class_id;

  /* `class << self; def X; ...; end; end` — treat body defs as class methods. */
  if (ty && sp_streq(ty, "SingletonClassNode")) {
    /* `class << self` inside a class body defines class methods on the
       enclosing class; `class << Const` (a constant naming a class/module)
       defines them on that named class instead. A singleton-class block on an
       arbitrary object (`class << obj`) has no per-object dispatch here, so
       only the resolvable receivers are special-cased; anything else falls
       through to the generic walk and is rejected loudly during codegen. */
    int target_class = class_id;
    int supported = 0;
    int sexpr = nt_ref(c->nt, id, "expression");
    const char *exty = sexpr >= 0 ? nt_type(c->nt, sexpr) : NULL;
    if (exty && sp_streq(exty, "SelfNode")) {
      supported = 1;
    }
    else if (exty && sp_streq(exty, "ConstantReadNode")) {
      const char *cn = nt_str(c->nt, sexpr, "name");
      int ci = cn ? comp_class_index(c, cn) : -1;
      if (ci >= 0) {
        target_class = ci;
        supported = 1;
      }
    }
    if (supported) {
      int sbody = nt_ref(c->nt, id, "body");
      if (sbody >= 0) {
        int n = 0;
        const int *stmts = nt_arr(c->nt, sbody, "body", &n);
        for (int k = 0; k < n; k++) {
          int s = stmts[k];
          const char *sty = nt_type(c->nt, s);
          if (!sty) continue;
          if (sp_streq(sty, "DefNode")) {
            const char *name = nt_str(c->nt, s, "name");
            if (!name) continue;
            Scope *sc = comp_scope_new(c, name, s);
            int new_idx = c->nscopes - 1;
            sc->body = nt_ref(c->nt, s, "body");
            sc->class_id = target_class;
            sc->is_cmethod = 1;
            collect_def_params(c, s, sc);
            /* Assign scope to the def node and its body */
            c->nscope[s] = new_idx;
            if (sc->body >= 0) walk_scope(c, sc->body, new_idx, target_class);
          }
          else {
            walk_scope(c, s, scope_idx, target_class);
          }
        }
        c->nscope[id] = scope_idx;
        c->nscope[sbody] = scope_idx;
      }
      return;
    }
    /* Unsupported receiver: fall through to the generic walk below. */
  }

  if (ty && (sp_streq(ty, "ClassNode") || sp_streq(ty, "ModuleNode"))) {
    int cp = nt_ref(c->nt, id, "constant_path");
    const char *cname = cp >= 0 ? nt_str(c->nt, cp, "name") : NULL;
    /* `module String` reopening a builtin CLASS is CRuby's TypeError; reject
       with that message instead of colliding with the runtime's sp_<Name> C
       type (a raw C error). A lexically nested or path-qualified
       `module A::Encoding` names a fresh constant in CRuby, but the generated
       C type is the bare tail name and still collides -- refuse that loudly
       too, as unsupported rather than TypeError. */
    if (sp_streq(ty, "ModuleNode") && cname &&
        is_builtin_class_name(cname) && !is_builtin_module_name(cname)) {
      int ln = (int)nt_int(c->nt, id, "node_line", 0);
      const char *file = c->nt->source_file ? c->nt->source_file : "source.rb";
      int toplevel = class_id < 0 && cp >= 0 && nt_type(c->nt, cp) &&
                     sp_streq(nt_type(c->nt, cp), "ConstantReadNode");
      if (toplevel)
        fprintf(stderr, "spinel: %s:%d: %s is not a module (TypeError)\n", file, ln, cname);
      else
        fprintf(stderr, "spinel: %s:%d: unsupported module name '%s': "
                        "collides with the builtin class of that name\n", file, ln, cname);
      exit(1);
    }
    /* `class CONST` where CONST aliases an existing class reopens that class.
       Rewrite the AST name so every later pass (registration, includes) agrees. */
    if (cname && cp >= 0 && comp_class_index(c, cname) < 0) {
      const char *real = resolve_class_alias(c, cname);
      if (real) {
        char buf[256]; snprintf(buf, sizeof buf, "%s", real);  /* copy: set frees cname */
        nt_set_str((NodeTable *)c->nt, cp, "name", buf);
        cname = nt_str(c->nt, cp, "name");
      }
    }
    if (cname && comp_class_index(c, cname) < 0) {
      comp_class_new(c, cname, id);
      child_class = c->nclasses - 1;
      c->classes[child_class].enclosing_class = class_id;
    }
    else if (cname) {
      child_class = comp_class_index(c, cname);  /* reopened class/module */
      /* A class can be opened bare first (no superclass -- e.g. just to hold a
         nested class) and reopened later with `< Super`. The parent link is read
         from def_node's "superclass" ref, so prefer the opening that declares one;
         otherwise the superclass is lost and a subclass's overrides aren't
         dispatched against the right ancestor chain (matz/spinel#1477). */
      if (child_class >= 0 && nt_ref(c->nt, id, "superclass") >= 0 &&
          c->classes[child_class].def_node >= 0 &&
          nt_ref(c->nt, c->classes[child_class].def_node, "superclass") < 0) {
        c->classes[child_class].def_node = id;
      }
    }
  }
  else if (ty && sp_streq(ty, "DefNode")) {
    const char *name = nt_str(c->nt, id, "name");
    Scope *s = comp_scope_new(c, name, id);
    int new_idx = c->nscopes - 1;
    s->body = nt_ref(c->nt, id, "body");
    s->class_id = class_id;   /* instance method of the enclosing class */
    /* `def self.foo` / `def Klass.foo`: a class (singleton) method. */
    int defrecv = nt_ref(c->nt, id, "receiver");
    if (defrecv >= 0) {
      s->is_cmethod = 1;
      /* `def Klass.foo` with an explicit constant receiver (typically defined
         outside the class body, where the enclosing class_id is -1) attaches
         to that class's singleton chain rather than becoming a top-level free
         function. `def self.foo` keeps the enclosing class. */
      const char *rty = nt_type(c->nt, defrecv);
      if (rty && sp_streq(rty, "ConstantReadNode")) {
        int rci = comp_class_index(c, nt_str(c->nt, defrecv, "name"));
        if (rci >= 0) s->class_id = rci;
      }
    }
    collect_def_params(c, id, s);
    child = new_idx;
  }
  else if (ty && sp_streq(ty, "CallNode")) {
    /* `Klass.class_eval/module_eval { defs }` (and the bare/`self.` forms in a
       class body) reopens the class: its block body's `def` and `define_method`
       become instance methods on it, exactly like a `class Klass ... end` reopen.
       Set child_class to the target so the generic recursion below registers them
       there (and register_locals interns any ivars first assigned inside those
       methods). g_cbody_direct gives the enclosing class for bare/self receivers. */
    {
      int ce_ci = class_eval_reopen_class(c, id, g_cbody_direct);
      if (ce_ci >= 0) child_class = ce_ci;
    }
    /* [lits].each { |v| define_method("m_#{v}") { body } } -- unroll into one
       method per element. Handled wholesale; skip the generic recursion so
       the inner define_method isn't also processed as a normal call. */
    if (class_id >= 0 && collect_dm_each_unroll(c, id, class_id)) return;
    /* compiler_state_int/str/sa/ia :fields -- declare ivars + synthesize the
       init/dump/set methods (the metaprogramming is native, not define_method). */
    if (class_id >= 0 && cs_macro_kind(nt_str(c->nt, id, "name"))) {
      collect_compiler_state(c, id, class_id);
      return;
    }
    /* define_method(:literal_name) { ... }: register as a method scope.
       At class scope it becomes an instance method; at top level a free
       function (class_id stays -1), matching `def`. */
    const char *dm_cn = nt_str(c->nt, id, "name");
    int dm_recv = nt_ref(c->nt, id, "receiver");
    int dm_is_dm  = dm_cn && sp_streq(dm_cn, "define_method") && dm_recv < 0;
    int dm_is_dsm = dm_cn && sp_streq(dm_cn, "define_singleton_method");
    /* define_singleton_method registers a class method on the resolved target:
       a class constant receiver, `self` in a class body, or no receiver (the
       enclosing class). An arbitrary-instance singleton has no compile-time
       class, so it is not registered (the later call rejects). */
    int dm_cmethod = 0, dm_cls = class_id, dm_ok = dm_is_dm;
    if (dm_is_dsm) {
      dm_cmethod = 1;
      const char *dsm_rty = dm_recv >= 0 ? nt_type(c->nt, dm_recv) : NULL;
      if (dm_recv < 0) dm_cls = class_id;
      else if (dsm_rty && (sp_streq(dsm_rty, "ConstantReadNode") || sp_streq(dsm_rty, "ConstantPathNode")))
        dm_cls = comp_class_index(c, nt_str(c->nt, dm_recv, "name"));
      else if (dsm_rty && sp_streq(dsm_rty, "SelfNode")) dm_cls = class_id;
      else dm_cls = -1;
      dm_ok = dm_cls >= 0;
    }
    if (dm_ok) {
      int dm_args = nt_ref(c->nt, id, "arguments");
      int dm_na = 0;
      const int *dm_argv = dm_args >= 0 ? nt_arr(c->nt, dm_args, "arguments", &dm_na) : NULL;
      if (dm_na >= 1) {
        const char *dm_aty = nt_type(c->nt, dm_argv[0]);
        const char *dm_mname = NULL;
        if (dm_aty && sp_streq(dm_aty, "SymbolNode"))
          dm_mname = nt_str(c->nt, dm_argv[0], "value");
        else if (dm_aty && sp_streq(dm_aty, "StringNode"))
          dm_mname = nt_str(c->nt, dm_argv[0], "content");
        int dm_blk = nt_ref(c->nt, id, "block");
        if (dm_mname && dm_blk >= 0) {
          Scope *dm_s = comp_scope_new(c, dm_mname, id);
          int dm_new_idx = c->nscopes - 1;
          dm_s->body = nt_ref(c->nt, dm_blk, "body");
          dm_s->class_id = dm_cls;
          dm_s->is_cmethod = dm_cmethod;
          /* the block's params are the defined method's params (e.g. the
             `&:to_s`-rewritten `{ |_spx| _spx.to_s }`'s _spx). */
          int dm_pn = nt_ref(c->nt, dm_blk, "parameters");
          int dm_inner = dm_pn >= 0 ? nt_ref(c->nt, dm_pn, "parameters") : -1;
          int dm_pnode = dm_inner >= 0 ? dm_inner : dm_pn;
          int dm_rn = 0; const int *dm_reqs = dm_pnode >= 0 ? nt_arr(c->nt, dm_pnode, "requireds", &dm_rn) : NULL;
          for (int p = 0; p < dm_rn; p++) {
            const char *pnm = nt_str(c->nt, dm_reqs[p], "name");
            if (pnm) scope_add_param(dm_s, pnm, -1);
          }
          child = dm_new_idx;
        }
      }
    }
  }

  int saved_cbody = g_cbody_class_id;
  int saved_direct = g_cbody_direct;
  if (child_class >= 0) g_cbody_class_id = child_class;
  /* g_cbody_direct tracks the class whose body we are *directly* in (where `self`
     is the class). A method/block scope is entered exactly when `child` was
     reassigned (DefNode/define_method/block); there `self` is no longer the class,
     so clear it. ClassNode/ModuleNode leave child == scope_idx. */
  if (child != scope_idx) g_cbody_direct = -1;
  else if (child_class >= 0) g_cbody_direct = child_class;

  int nr = nt_num_refs(c->nt, id);
  for (int i = 0; i < nr; i++) {
    int r = nt_ref_at(c->nt, id, i);
    if (r >= 0) walk_scope(c, r, child, child_class);
  }
  int na = nt_num_arrs(c->nt, id);
  for (int i = 0; i < na; i++) {
    int n = 0;
    const int *ids = nt_arr_at(c->nt, id, i, &n);
    for (int j = 0; j < n; j++)
      if (ids[j] >= 0) walk_scope(c, ids[j], child, child_class);
  }
  g_cbody_class_id = saved_cbody;
  g_cbody_direct = saved_direct;
}

/* Mark methods following `module_function` in a module body as class-level
   (is_cmethod=1, no self param). This lets them be called as bare functions
   when their module is included at the top level. */
void register_module_functions(Compiler *c) {
  const NodeTable *nt = c->nt;
  for (int ci = 0; ci < c->nclasses; ci++) {
    int dn = c->classes[ci].def_node;
    const char *dt = dn >= 0 ? nt_type(nt, dn) : NULL;
    if (!dt || !sp_streq(dt, "ModuleNode")) continue;
    int body = nt_ref(nt, dn, "body");
    if (body < 0) continue;
    int bn = 0;
    const int *stmts = nt_arr(nt, body, "body", &bn);
    int in_module_function = 0;
    for (int k = 0; k < bn; k++) {
      int s = stmts[k];
      const char *sty = nt_type(nt, s);
      if (!sty) continue;
      if (sp_streq(sty, "CallNode") && nt_ref(nt, s, "receiver") < 0) {
        const char *nm = nt_str(nt, s, "name");
        if (nm && sp_streq(nm, "module_function")) {
          /* `module_function :m1, :m2` form: mark named methods */
          int an = 0;
          int anode = nt_ref(nt, s, "arguments");
          const int *aargs = anode >= 0 ? nt_arr(nt, anode, "arguments", &an) : NULL;
          if (an == 0) { in_module_function = 1; continue; }
          for (int ai = 0; ai < an; ai++) {
            const char *aty = nt_type(nt, aargs[ai]);
            const char *aval = NULL;
            if (aty && sp_streq(aty, "SymbolNode")) aval = nt_str(nt, aargs[ai], "value");
            if (!aval) continue;
            for (int mi = 0; mi < c->nscopes; mi++) {
              if (c->scopes[mi].class_id == ci && !c->scopes[mi].is_cmethod &&
                  c->scopes[mi].name && sp_streq(c->scopes[mi].name, aval))
                c->scopes[mi].is_cmethod = 1;
            }
          }
          continue;
        }
      }
      if (sp_streq(sty, "DefNode") && in_module_function) {
        const char *mname = nt_str(nt, s, "name");
        if (!mname) continue;
        for (int mi = 0; mi < c->nscopes; mi++) {
          if (c->scopes[mi].def_node == s) { c->scopes[mi].is_cmethod = 1; break; }
        }
      }
    }
  }
}

/* Method name carried by a `private`/`public`/`protected` symbol/string arg. */
static const char *vis_arg_name(const NodeTable *nt, int arg) {
  const char *aty = nt_type(nt, arg);
  if (!aty) return NULL;
  if (sp_streq(aty, "SymbolNode")) return nt_str(nt, arg, "value");
  if (sp_streq(aty, "StringNode")) {
    const char *s = nt_str(nt, arg, "content");
    return s ? s : nt_str(nt, arg, "unescaped");
  }
  return NULL;
}

/* Record `kind` for the methods an attr_reader/writer/accessor call declares
   (writers as "x="), e.g. for `private attr_reader :x` or a bare attr under a
   private/protected section. */
static void vis_apply_attr(Compiler *c, ClassInfo *cls, int call, int kind) {
  const NodeTable *nt = c->nt;
  const char *nm = nt_str(nt, call, "name");
  if (!nm) return;
  int reader = sp_streq(nm, "attr_reader") || sp_streq(nm, "attr_accessor");
  int writer = sp_streq(nm, "attr_writer") || sp_streq(nm, "attr_accessor");
  if (!reader && !writer) return;
  int args = nt_ref(nt, call, "arguments");
  int an = 0;
  const int *argv = args >= 0 ? nt_arr(nt, args, "arguments", &an) : NULL;
  for (int i = 0; i < an; i++) {
    const char *base = vis_arg_name(nt, argv[i]);
    if (!base) continue;
    if (reader) comp_method_vis_set(cls, base, kind);
    if (writer) {
      char buf[256];
      snprintf(buf, sizeof buf, "%s=", base);
      comp_method_vis_set(cls, buf, kind);
    }
  }
}

/* Walk one class/module body in lexical order, recording each method's
   visibility (default public). Handles a bare `private`/`protected`/`public`
   (switches the mode for following defs/attrs), the `private :a, :b` /
   `private def m;end` / `private attr_reader :x` argument forms, and plain
   `def`/`attr_*` declarations under the active mode. Class (`def self.x`)
   methods are a separate axis and left alone. */
static void register_method_visibility_body(Compiler *c, ClassInfo *cls, int body) {
  const NodeTable *nt = c->nt;
  int n = 0;
  const int *stmts = body >= 0 ? nt_arr(nt, body, "body", &n) : NULL;
  int cur = SP_VIS_PUBLIC;
  for (int k = 0; k < n; k++) {
    int s = stmts[k];
    const char *sty = nt_type(nt, s);
    if (!sty) continue;
    if (sp_streq(sty, "DefNode")) {
      const char *mname = nt_str(nt, s, "name");
      if (mname && nt_ref(nt, s, "receiver") < 0)
        comp_method_vis_set(cls, mname, cur);
      continue;
    }
    if (!sp_streq(sty, "CallNode") || nt_ref(nt, s, "receiver") >= 0) continue;
    const char *nm = nt_str(nt, s, "name");
    if (!nm) continue;
    int kind = sp_streq(nm, "private")   ? SP_VIS_PRIVATE   :
               sp_streq(nm, "protected") ? SP_VIS_PROTECTED :
               sp_streq(nm, "public")    ? SP_VIS_PUBLIC : -1;
    if (kind >= 0) {
      int args = nt_ref(nt, s, "arguments");
      int an = 0;
      const int *argv = args >= 0 ? nt_arr(nt, args, "arguments", &an) : NULL;
      if (an == 0) { cur = kind; continue; }  /* bare: switch the section mode */
      for (int i = 0; i < an; i++) {
        const char *aty = nt_type(nt, argv[i]);
        const char *mn = vis_arg_name(nt, argv[i]);
        if (mn) { comp_method_vis_set(cls, mn, kind); continue; }
        if (aty && sp_streq(aty, "DefNode")) {
          const char *dn = nt_str(nt, argv[i], "name");
          if (dn && nt_ref(nt, argv[i], "receiver") < 0)
            comp_method_vis_set(cls, dn, kind);
        }
        else if (aty && sp_streq(aty, "CallNode")) {
          vis_apply_attr(c, cls, argv[i], kind);  /* private attr_reader :x */
        }
      }
      continue;
    }
    /* Record attr visibility unconditionally (like a plain `def`), so a public
       attr in a subclass overrides an inherited private/protected method rather
       than resolving up the chain to the ancestor's visibility. */
    if (sp_streq(nm, "attr_reader") || sp_streq(nm, "attr_writer") ||
        sp_streq(nm, "attr_accessor"))
      vis_apply_attr(c, cls, s, cur);
  }
}

/* Record per-method visibility for every class/module body, including reopened
   bodies (each `class Foo ... end` opening starts public, like CRuby). */
void register_method_visibility(Compiler *c) {
  const NodeTable *nt = c->nt;
  for (int ci = 0; ci < c->nclasses; ci++) {
    ClassInfo *cls = &c->classes[ci];
    register_method_visibility_body(c, cls, nt_ref(nt, cls->def_node, "body"));
  }
  for (int id = 0; id < nt->count; id++) {
    const char *ty = nt_type(nt, id);
    if (!ty || (!sp_streq(ty, "ClassNode") && !sp_streq(ty, "ModuleNode"))) continue;
    int cp = nt_ref(nt, id, "constant_path");
    const char *cname = cp >= 0 ? nt_str(nt, cp, "name") : NULL;
    if (!cname) continue;
    int ci = comp_class_index(c, cname);
    if (ci < 0) continue;
    if (id == c->classes[ci].def_node) continue;  /* canonical body already done */
    register_method_visibility_body(c, &c->classes[ci], nt_ref(nt, id, "body"));
  }
}

void register_locals(Compiler *c) {
  const NodeTable *nt = c->nt;
  for (int id = 0; id < nt->count; id++) {
    const char *ty = nt_type(nt, id);
    if (!ty) continue;
    if (sp_streq(ty, "LocalVariableWriteNode") ||
        sp_streq(ty, "LocalVariableTargetNode") ||
        sp_streq(ty, "LocalVariableReadNode") ||
        sp_streq(ty, "LocalVariableOperatorWriteNode") ||
        sp_streq(ty, "LocalVariableOrWriteNode") ||
        sp_streq(ty, "LocalVariableAndWriteNode")) {
      const char *nm = nt_str(nt, id, "name");
      if (nm) scope_local_intern(comp_scope_of(c, id), nm);
    }
    if (sp_streq(ty, "InstanceVariableWriteNode") ||
        sp_streq(ty, "InstanceVariableReadNode") ||
        sp_streq(ty, "InstanceVariableOperatorWriteNode")) {
      const char *nm = nt_str(nt, id, "name");
      Scope *s = comp_scope_of(c, id);
      if (nm && s->class_id >= 0) comp_ivar_intern(&c->classes[s->class_id], nm);
    }
  }
}

/* `Const = Struct.new(:a, :b)` / `Const = Data.define(:a, :b)` defines a
   class named Const whose positional members are attr_accessors. Register
   it as a class with one ivar + reader + writer per member. */
int is_c_ident(const char *s);

/* Is CallNode `val` a `Struct.new(...)` / `Data.define(...)`? */
int is_struct_call(Compiler *c, int val) {
  const NodeTable *nt = c->nt;
  if (val < 0 || !nt_type(nt, val) || !sp_streq(nt_type(nt, val), "CallNode")) return 0;
  const char *mn = nt_str(nt, val, "name");
  int vr = nt_ref(nt, val, "receiver");
  const char *rn = vr >= 0 && nt_type(nt, vr) && sp_streq(nt_type(nt, vr), "ConstantReadNode")
                   ? nt_str(nt, vr, "name") : NULL;
  return rn && ((sp_streq(rn, "Struct") && mn && sp_streq(mn, "new")) ||
                (sp_streq(rn, "Data") && mn && sp_streq(mn, "define")));
}

/* Register the symbol members of a Struct.new(...) call onto `cls`. */
void register_struct_members(Compiler *c, ClassInfo *cls, int val) {
  const NodeTable *nt = c->nt;
  cls->is_struct = 1;
  {
    int vr = nt_ref(nt, val, "receiver");
    const char *rn = vr >= 0 && nt_type(nt, vr) && sp_streq(nt_type(nt, vr), "ConstantReadNode")
                     ? nt_str(nt, vr, "name") : NULL;
    if (rn && sp_streq(rn, "Data")) cls->is_data = 1;
  }
  int args = nt_ref(nt, val, "arguments");
  int an = 0;
  const int *argv = args >= 0 ? nt_arr(nt, args, "arguments", &an) : NULL;
  for (int a = 0; a < an; a++) {
    if (!nt_type(nt, argv[a]) || !sp_streq(nt_type(nt, argv[a]), "SymbolNode")) continue;
    const char *m = nt_str(nt, argv[a], "value");
    if (!m) continue;
    char ivn[256]; snprintf(ivn, sizeof ivn, "@%s", m);
    comp_ivar_intern(cls, ivn);
    comp_add_reader(cls, m);
    comp_add_writer(cls, m);
  }
}

void register_structs(Compiler *c) {
  const NodeTable *nt = c->nt;
  for (int id = 0; id < nt->count; id++) {
    const char *ty = nt_type(nt, id);
    if (!ty) continue;
    /* Const = Struct.new(:a, :b) */
    if (sp_streq(ty, "ConstantWriteNode")) {
      const char *cname = nt_str(nt, id, "name");
      int val = nt_ref(nt, id, "value");
      if (!cname || !is_c_ident(cname) || !is_struct_call(c, val)) continue;
      if (comp_class_index(c, cname) >= 0) continue;
      register_struct_members(c, comp_class_new(c, cname, id), val);
    }
    /* class X < Struct.new(:a, :b); ... end */
    else if (sp_streq(ty, "ClassNode")) {
      int sup = nt_ref(nt, id, "superclass");
      if (!is_struct_call(c, sup)) continue;
      int cp = nt_ref(nt, id, "constant_path");
      const char *cname = cp >= 0 ? nt_str(nt, cp, "name") : NULL;
      int ci = cname ? comp_class_index(c, cname) : -1;
      if (ci >= 0) register_struct_members(c, &c->classes[ci], sup);
    }
  }
}

/* Fix scope class_id for DefNodes inside Struct.new { } blocks.
   walk_scope runs before register_structs, so defs in struct blocks get
   class_id=-1. This pass corrects them after the class is registered. */
void fix_struct_block_scopes(Compiler *c) {
  const NodeTable *nt = c->nt;
  NT_FOREACH_KIND(nt, NK_ConstantWriteNode, id) {
    const char *cname = nt_str(nt, id, "name");
    int val = nt_ref(nt, id, "value");
    if (!cname || val < 0 || !is_struct_call(c, val)) continue;
    int blk = nt_ref(nt, val, "block");
    if (blk < 0) continue;
    int ci = comp_class_index(c, cname);
    if (ci < 0) continue;
    /* Walk the block body and fix any DefNode scopes */
    int bbody = nt_ref(nt, blk, "body");
    if (bbody < 0) continue;
    int bn = 0;
    const int *stmts = nt_arr(nt, bbody, "body", &bn);
    for (int k = 0; k < bn; k++) {
      const char *sty = nt_type(nt, stmts[k]);
      if (!sty || !sp_streq(sty, "DefNode")) continue;
      int dn = stmts[k];
      /* Find the scope whose def_node == dn and fix its class_id */
      for (int s = 0; s < c->nscopes; s++) {
        if (c->scopes[s].def_node == dn) {
          c->scopes[s].class_id = ci;
          break;
        }
      }
    }
  }
}

/* Process attr_accessor/reader/writer call: register ivars + reader/writer names.
   If `singleton` is non-zero, registers singleton (class-level) accessors instead. */
void register_attr_call(Compiler *c, ClassInfo *cls, int s, int singleton) {
  const NodeTable *nt = c->nt;
  const char *nm = nt_str(nt, s, "name");
  if (!nm) return;
  int accessor = sp_streq(nm, "attr_accessor") ||
                 sp_streq(nm, "attribute") || sp_streq(nm, "attributes");
  int reader = sp_streq(nm, "attr_reader") || accessor;
  int writer = sp_streq(nm, "attr_writer") || accessor;
  if (!reader && !writer) return;
  int args = nt_ref(nt, s, "arguments");
  int an = 0;
  const int *argv = args >= 0 ? nt_arr(nt, args, "arguments", &an) : NULL;
  for (int a = 0; a < an; a++) {
    const char *aty = nt_type(nt, argv[a]);
    if (!aty || !sp_streq(aty, "SymbolNode")) continue;
    const char *base = nt_str(nt, argv[a], "value");
    if (!base) continue;
    if (singleton) {
      if (reader) comp_add_sg_reader(cls, base);
      if (writer) comp_add_sg_writer(cls, base);
    }
else {
      char ivname[256];
      snprintf(ivname, sizeof ivname, "@%s", base);
      comp_ivar_intern(cls, ivname);
      if (reader) comp_add_reader(cls, base);
      if (writer) comp_add_writer(cls, base);
    }
  }
}

/* Collect attr_reader/attr_writer/attr_accessor declarations in class
   bodies, registering backing ivars + reader/writer method names.
   Also scans class << self bodies for singleton-level attr_accessors. */
void register_attrs_body(Compiler *c, ClassInfo *cls, int body) {
  const NodeTable *nt = c->nt;
  int n = 0;
  const int *stmts = body >= 0 ? nt_arr(nt, body, "body", &n) : NULL;
  for (int k = 0; k < n; k++) {
    int s = stmts[k];
    const char *sty = nt_type(nt, s);
    if (!sty) continue;
    if (sp_streq(sty, "CallNode")) {
      register_attr_call(c, cls, s, 0);
    }
    else if (sp_streq(sty, "SingletonClassNode")) {
      /* class << self; attr_accessor :x; end */
      int sbody = nt_ref(nt, s, "body");
      if (sbody < 0) continue;
      int sn = 0;
      const int *sstmts = nt_arr(nt, sbody, "body", &sn);
      for (int j = 0; j < sn; j++) {
        int ss = sstmts[j];
        const char *ssty = nt_type(nt, ss);
        if (ssty && sp_streq(ssty, "CallNode"))
          register_attr_call(c, cls, ss, 1);
      }
    }
  }
}

void register_attrs(Compiler *c) {
  const NodeTable *nt = c->nt;
  /* Pass 1: process primary definition bodies. */
  for (int ci = 0; ci < c->nclasses; ci++) {
    ClassInfo *cls = &c->classes[ci];
    register_attrs_body(c, cls, nt_ref(nt, cls->def_node, "body"));
  }
  /* Pass 2: scan all ClassNode/ModuleNode reopenings. */
  for (int id = 0; id < nt->count; id++) {
    const char *ty = nt_type(nt, id);
    if (!ty || (!sp_streq(ty, "ClassNode") && !sp_streq(ty, "ModuleNode"))) continue;
    int cp = nt_ref(nt, id, "constant_path");
    const char *cname = cp >= 0 ? nt_str(nt, cp, "name") : NULL;
    if (!cname) continue;
    int ci = comp_class_index(c, cname);
    if (ci < 0) continue;
    if (id == c->classes[ci].def_node) continue;  /* already handled above */
    register_attrs_body(c, &c->classes[ci], nt_ref(nt, id, "body"));
  }
}

/* Classify a modifier/if predicate as a compile-time constant: 1 = always
   truthy, 0 = always falsy, -1 = a runtime value. Only literal true / non-nil
   literals and false / nil fold; anything else (a call, constant, variable) is
   runtime. */
static int alias_pred_const(const NodeTable *nt, int pred) {
  if (pred < 0) return -1;
  const char *t = nt_type(nt, pred);
  if (!t) return -1;
  /* unwrap parentheses: `if (cond)`. An empty `()` is nil (falsy); multiple
     statements are not folded (conservative). */
  while (sp_streq(t, "ParenthesesNode")) {
    int stmts = nt_ref(nt, pred, "body");  /* ParenthesesNode -> StatementsNode */
    int n = 0;
    const int *body = stmts >= 0 ? nt_arr(nt, stmts, "body", &n) : NULL;
    if (n == 0 || !body) return 0;
    if (n != 1) return -1;
    pred = body[0];
    if (pred < 0) return -1;
    t = nt_type(nt, pred);
    if (!t) return -1;
  }
  if (sp_streq(t, "FalseNode") || sp_streq(t, "NilNode")) return 0;
  if (sp_streq(t, "TrueNode") || sp_streq(t, "IntegerNode") || sp_streq(t, "FloatNode") ||
      sp_streq(t, "StringNode") || sp_streq(t, "SymbolNode") || sp_streq(t, "ArrayNode") ||
      sp_streq(t, "HashNode") || sp_streq(t, "RegularExpressionNode"))
    return 1;
  return -1;
}

/* Collect `alias new old` (AliasMethodNode) and `alias_method :new, :old`
   (CallNode) statements in class bodies into the class alias table. */
void register_aliases_body(Compiler *c, ClassInfo *cls, int body) {
  const NodeTable *nt = c->nt;
  int n = 0;
  const int *stmts = body >= 0 ? nt_arr(nt, body, "body", &n) : NULL;
  for (int k = 0; k < n; k++) {
    int s = stmts[k];
    const char *sty = nt_type(nt, s);
    if (!sty) continue;
    if (sp_streq(sty, "AliasMethodNode")) {
      int nn = nt_ref(nt, s, "new_name");
      int on = nt_ref(nt, s, "old_name");
      const char *nw = nn >= 0 ? nt_str(nt, nn, "value") : NULL;
      const char *od = on >= 0 ? nt_str(nt, on, "value") : NULL;
      comp_add_alias(cls, nw, od);
    }
    else if (sp_streq(sty, "CallNode")) {
      const char *nm = nt_str(nt, s, "name");
      if (!nm || !sp_streq(nm, "alias_method")) continue;
      int args = nt_ref(nt, s, "arguments");
      int an = 0;
      const int *argv = args >= 0 ? nt_arr(nt, args, "arguments", &an) : NULL;
      if (an >= 2 && nt_type(nt, argv[0]) && sp_streq(nt_type(nt, argv[0]), "SymbolNode") &&
          nt_type(nt, argv[1]) && sp_streq(nt_type(nt, argv[1]), "SymbolNode"))
        comp_add_alias(cls, nt_str(nt, argv[0], "value"), nt_str(nt, argv[1], "value"));
    }
    else if (sp_streq(sty, "IfNode") || sp_streq(sty, "UnlessNode")) {
      /* A statement modifier (`alias a b if cond`) wraps the alias in an IfNode;
         a full if/elsif/else chains through `subsequent`. An alias resolves at
         compile time, so register only the branch the conditions statically
         select, following the chain. A non-constant guard selects nothing: the
         alias cannot be created conditionally with static method tables, so the
         name is left unresolved and rejects loudly if used. */
      int curr = s;
      while (curr >= 0) {
        const char *cty = nt_type(nt, curr);
        if (!cty) break;
        if (sp_streq(cty, "ElseNode")) {
          register_aliases_body(c, cls, nt_ref(nt, curr, "statements"));
          break;
        }
        if (!sp_streq(cty, "IfNode") && !sp_streq(cty, "UnlessNode")) break;
        int is_unless = sp_streq(cty, "UnlessNode");
        int pc = alias_pred_const(nt, nt_ref(nt, curr, "predicate"));
        int then_runs = is_unless ? (pc == 0) : (pc == 1);
        int else_runs = is_unless ? (pc == 1) : (pc == 0);
        if (then_runs) { register_aliases_body(c, cls, nt_ref(nt, curr, "statements")); break; }
        if (else_runs) curr = nt_ref(nt, curr, is_unless ? "else_clause" : "subsequent");
        else break;  /* non-constant: select nothing */
      }
    }
  }
}

void register_aliases(Compiler *c) {
  const NodeTable *nt = c->nt;
  /* Pass 1: primary definition bodies. */
  for (int ci = 0; ci < c->nclasses; ci++) {
    ClassInfo *cls = &c->classes[ci];
    register_aliases_body(c, cls, nt_ref(nt, cls->def_node, "body"));
  }
  /* Pass 2: reopened class/module bodies. */
  for (int id = 0; id < nt->count; id++) {
    const char *ty = nt_type(nt, id);
    if (!ty || (!sp_streq(ty, "ClassNode") && !sp_streq(ty, "ModuleNode"))) continue;
    int cp = nt_ref(nt, id, "constant_path");
    const char *cname = cp >= 0 ? nt_str(nt, cp, "name") : NULL;
    if (!cname) continue;
    int ci = comp_class_index(c, cname);
    if (ci < 0) continue;
    if (id == c->classes[ci].def_node) continue;
    register_aliases_body(c, &c->classes[ci], nt_ref(nt, id, "body"));
  }
}

void register_undefs_body(Compiler *c, ClassInfo *cls, int body) {
  const NodeTable *nt = c->nt;
  int n = 0;
  const int *stmts = body >= 0 ? nt_arr(nt, body, "body", &n) : NULL;
  for (int k = 0; k < n; k++) {
    int s = stmts[k];
    const char *sty = nt_type(nt, s);
    if (!sty || !sp_streq(sty, "UndefNode")) continue;
    int names_n = 0;
    const int *names = nt_arr(nt, s, "names", &names_n);
    for (int j = 0; j < names_n; j++) {
      const char *mname = nt_str(nt, names[j], "value");
      if (mname) comp_add_undef(cls, mname);
    }
  }
}

void register_undefs(Compiler *c) {
  const NodeTable *nt = c->nt;
  for (int ci = 0; ci < c->nclasses; ci++) {
    ClassInfo *cls = &c->classes[ci];
    register_undefs_body(c, cls, nt_ref(nt, cls->def_node, "body"));
  }
  for (int id = 0; id < nt->count; id++) {
    const char *ty = nt_type(nt, id);
    if (!ty || (!sp_streq(ty, "ClassNode") && !sp_streq(ty, "ModuleNode"))) continue;
    int cp = nt_ref(nt, id, "constant_path");
    const char *cname = cp >= 0 ? nt_str(nt, cp, "name") : NULL;
    if (!cname) continue;
    int ci = comp_class_index(c, cname);
    if (ci < 0) continue;
    if (id == c->classes[ci].def_node) continue;
    register_undefs_body(c, &c->classes[ci], nt_ref(nt, id, "body"));
  }
}

int is_c_ident(const char *s) {
  if (!s || !*s) return 0;
  for (const char *p = s; *p; p++)
    if (!((*p >= 'a' && *p <= 'z') || (*p >= 'A' && *p <= 'Z') ||
          (*p >= '0' && *p <= '9') || *p == '_')) return 0;
  return 1;
}

/* Register global variables ($g) and top-level constants (FOO). */
void register_globals_consts(Compiler *c) {
  const NodeTable *nt = c->nt;
  /* Pass 1: collect alias $copy $orig mappings first so pass 2 can skip them. */
  NT_FOREACH_KIND(nt, NK_AliasGlobalVariableNode, id) {
    int nw_id  = nt_ref(nt, id, "new_name");
    int old_id = nt_ref(nt, id, "old_name");
    const char *nw  = nw_id  >= 0 ? nt_str(nt, nw_id,  "name") : NULL;
    const char *old = old_id >= 0 ? nt_str(nt, old_id, "name") : NULL;
    if (nw && nw[0] == '$' && is_c_ident(nw + 1) &&
        old && old[0] == '$' && is_c_ident(old + 1)) {
      comp_gvar_intern(c, old + 1);             /* intern the original */
      comp_add_gvar_alias(c, nw + 1, old + 1); /* $new -> $old */
    }
  }
  /* Pass 2: intern all other globals (skipping alias names). */
  for (int id = 0; id < nt->count; id++) {
    const char *ty = nt_type(nt, id);
    if (!ty) continue;
    if (sp_streq(ty, "GlobalVariableWriteNode") || sp_streq(ty, "GlobalVariableReadNode") ||
        sp_streq(ty, "GlobalVariableOperatorWriteNode") || sp_streq(ty, "GlobalVariableTargetNode") ||
        sp_streq(ty, "GlobalVariableOrWriteNode") || sp_streq(ty, "GlobalVariableAndWriteNode")) {
      const char *nm = nt_str(nt, id, "name");
      /* skip alias names - they resolve to the original and need no separate slot */
      if (nm && nm[0] == '$' && is_c_ident(nm + 1) &&
          sp_streq(nm + 1, comp_resolve_gvar(c, nm + 1)))
        comp_gvar_intern(c, nm + 1);
    }
    else if (sp_streq(ty, "AliasGlobalVariableNode")) {
      /* already handled in pass 1 */
    }
    else if (sp_streq(ty, "ConstantTargetNode")) {
      /* target in a multi-write: A, B = expr (a definite write) */
      const char *nm = nt_str(nt, id, "name");
      if (nm && is_c_ident(nm) && comp_class_index(c, nm) < 0)
        comp_const_intern(c, nm)->const_def_write = 1;
    }
    else if (sp_streq(ty, "ConstantPathWriteNode") || sp_streq(ty, "ConstantPathOrWriteNode") ||
             sp_streq(ty, "ConstantPathAndWriteNode") || sp_streq(ty, "ConstantPathOperatorWriteNode")) {
      /* `Mod::X = v` / `Mod::X ||= v`: register the leaf constant by name so it
         gets a runtime slot. The module path is not modeled as a namespace; the
         leaf name is interned flat like a top-level constant. */
      int tgt = nt_ref(nt, id, "target");
      const char *nm = tgt >= 0 ? nt_str(nt, tgt, "name") : NULL;
      if (nm && is_c_ident(nm) && comp_class_index(c, nm) < 0) {
        LocalVar *cv = comp_const_intern(c, nm);
        if (sp_streq(ty, "ConstantPathWriteNode")) cv->const_def_write = 1;
      }
    }
    else if (sp_streq(ty, "ConstantWriteNode")) {
      const char *nm = nt_str(nt, id, "name");
      /* a constant bound to a regex literal is resolved at compile time to a
         precompiled pattern, not stored as a runtime value */
      int rv = nt_ref(nt, id, "value");
      if (rv >= 0 && nt_type(nt, rv) && sp_streq(nt_type(nt, rv), "CallNode") &&
          nt_str(nt, rv, "name") && sp_streq(nt_str(nt, rv, "name"), "freeze"))
        rv = nt_ref(nt, rv, "receiver");
      int is_regex_const = rv >= 0 && nt_type(nt, rv) && sp_streq(nt_type(nt, rv), "RegularExpressionNode");
      /* regex constants: store with type TY_REGEX so call-type inference works */
      if (nm && is_regex_const) {
        LocalVar *cv = comp_const_intern(c, nm);
        cv->type = TY_REGEX;
      }
      /* a Struct/Data const names a class, not a value constant.
         Do NOT skip when the name collides with a module: M::V = "str" is a
         value constant even though top-level `module V` exists. */
      if (nm && is_c_ident(nm) && !is_regex_const) {
        LocalVar *cv = comp_const_intern(c, nm);
        cv->const_def_write = 1;
        /* `CONST = SomeClass.new(...)`: reads of CONST during the new()
           (i.e. inside initialize or anything it calls) must raise
           NameError, since CONST is not yet bound. */
        int v = nt_ref(nt, id, "value");
        const char *vty = v >= 0 ? nt_type(nt, v) : NULL;
        if (vty && sp_streq(vty, "CallNode") && nt_str(nt, v, "name") &&
            sp_streq(nt_str(nt, v, "name"), "new")) {
          int vr = nt_ref(nt, v, "receiver");
          if (vr >= 0 && nt_type(nt, vr) && sp_streq(nt_type(nt, vr), "ConstantReadNode") &&
              nt_str(nt, vr, "name") && comp_class_index(c, nt_str(nt, vr, "name")) >= 0)
            cv->init_guarded = 1;
        }
      }
    }
  }
}

/* Extract a symbol or string literal text from a node, or NULL. */
const char *ffi_arg_str(const NodeTable *nt, int nid) {
  if (nid < 0) return NULL;
  const char *ty = nt_type(nt, nid);
  if (!ty) return NULL;
  if (sp_streq(ty, "SymbolNode")) return nt_str(nt, nid, "value");
  if (sp_streq(ty, "StringNode")) return nt_str(nt, nid, "content");
  return NULL;
}

/* Extract an integer literal value, or -1. */
int ffi_arg_int(const NodeTable *nt, int nid) {
  if (nid < 0) return -1;
  const char *ty = nt_type(nt, nid);
  if (!ty) return -1;
  if (sp_streq(ty, "IntegerNode")) return (int)nt_int(nt, nid, "value", 0);
  return -1;
}

/* Map an FFI spec string to the Spinel TyKind used for return types. */
TyKind ffi_spec_to_ty(const char *spec) {
  if (!spec) return TY_UNKNOWN;
  if (sp_streq(spec,"int")||sp_streq(spec,"uint32")||sp_streq(spec,"int32")||
      sp_streq(spec,"uint16")||sp_streq(spec,"int16")||sp_streq(spec,"uint8")||
      sp_streq(spec,"size_t")||sp_streq(spec,"long")||sp_streq(spec,"int64"))
    return TY_INT;
  if (sp_streq(spec,"float")||sp_streq(spec,"double")) return TY_FLOAT;
  if (sp_streq(spec,"str")||sp_streq(spec,"binstr")) return TY_STRING;
  if (sp_streq(spec,"bool")) return TY_BOOL;
  if (sp_streq(spec,"ptr")) return TY_POLY;
  if (sp_streq(spec,"void")) return TY_NIL;
  if (sp_streq(spec,"float_array")) return TY_FLOAT_ARRAY;
  if (sp_streq(spec,"int_array")) return TY_INT_ARRAY;
  return TY_UNKNOWN;
}

/* Register a ffi_func / ffi_const / ffi_buffer / ffi_read_* declared in
   module bodies. Called during analyze_program before fixpoint. */
void register_ffi_decls(Compiler *c) {
  const NodeTable *nt = c->nt;
  NT_FOREACH_KIND(nt, NK_ModuleNode, id) {
    int cp = nt_ref(nt, id, "constant_path");
    const char *mname = cp >= 0 ? nt_str(nt, cp, "name") : NULL;
    if (!mname) continue;
    int body = nt_ref(nt, id, "body");
    int sn = 0;
    const int *stmts = body >= 0 ? nt_arr(nt, body, "body", &sn) : NULL;
    /* Pre-scan for `native_lib "feat"`: its require-gate feature name is
       stamped onto every native_func of this module regardless of order. */
    const char *mod_feat = NULL;
    for (int k = 0; k < sn; k++) {
      int s = stmts[k];
      const char *sty = nt_type(nt, s);
      if (!sty || !sp_streq(sty, "CallNode")) continue;
      if (nt_ref(nt, s, "receiver") >= 0) continue;
      const char *dn = nt_str(nt, s, "name");
      if (dn && sp_streq(dn, "native_lib")) {
        int a = nt_ref(nt, s, "arguments");
        int na = 0;
        const int *av = a >= 0 ? nt_arr(nt, a, "arguments", &na) : NULL;
        if (na >= 1) mod_feat = ffi_arg_str(nt, av[0]);
        break;
      }
    }
    /* Pre-scan for `native_struct "Name", "sp_CStruct"[, "free_sym"]`: registers
       Name as a native (C-backed) class so native_new/native_method below can
       bind to its class index, regardless of declaration order. */
    int native_cid = -1;
    for (int k = 0; k < sn; k++) {
      int s = stmts[k];
      const char *sty = nt_type(nt, s);
      if (!sty || !sp_streq(sty, "CallNode")) continue;
      if (nt_ref(nt, s, "receiver") >= 0) continue;
      const char *dn = nt_str(nt, s, "name");
      if (!dn || !sp_streq(dn, "native_struct")) continue;
      int a = nt_ref(nt, s, "arguments");
      int na = 0;
      const int *av = a >= 0 ? nt_arr(nt, a, "arguments", &na) : NULL;
      if (na < 2) break;
      const char *clsname = ffi_arg_str(nt, av[0]);
      const char *cstruct = ffi_arg_str(nt, av[1]);
      const char *freesym = na >= 3 ? ffi_arg_str(nt, av[2]) : NULL;
      if (!clsname || !cstruct) break;
      int ex = comp_class_index(c, clsname);
      if (ex >= 0) native_cid = ex;
      else { comp_class_new(c, clsname, -1); native_cid = c->nclasses - 1; }
      ClassInfo *nc = &c->classes[native_cid];
      nc->is_native_class = 1;
      free(nc->c_struct); nc->c_struct = strdup(cstruct);
      if (freesym) { free(nc->native_free); nc->native_free = strdup(freesym); }
      break;
    }
    for (int k = 0; k < sn; k++) {
      int s = stmts[k];
      const char *sty = nt_type(nt, s);
      if (!sty || !sp_streq(sty, "CallNode")) continue;
      if (nt_ref(nt, s, "receiver") >= 0) continue;
      const char *dname = nt_str(nt, s, "name");
      if (!dname) continue;
      int anode = nt_ref(nt, s, "arguments");
      int an = 0;
      const int *args = anode >= 0 ? nt_arr(nt, anode, "arguments", &an) : NULL;

      /* native_func :name, [arg_specs], ret_spec, "c_symbol" (Path B).
         Specs are the spinel type language (any/string/int/float/bool). */
      if (sp_streq(dname, "native_func")) {
        if (an < 4) continue;
        const char *fname = ffi_arg_str(nt, args[0]);
        const char *arr_ty = nt_type(nt, args[1]);
        const char *ret_spec = ffi_arg_str(nt, args[2]);
        const char *csym = ffi_arg_str(nt, args[3]);
        if (!fname || !ret_spec || !csym || !arr_ty || !sp_streq(arr_ty, "ArrayNode")) continue;
        int en = 0;
        const int *elems = nt_arr(nt, args[1], "elements", &en);
        char **arg_specs = malloc(sizeof(char *) * (size_t)(en + 1));
        for (int ei = 0; ei < en; ei++) {
          const char *spec = ffi_arg_str(nt, elems[ei]);
          arg_specs[ei] = strdup(spec ? spec : "");
        }
        if (c->n_native_funcs >= c->c_native_funcs) {
          c->c_native_funcs = c->c_native_funcs ? c->c_native_funcs * 2 : 16;
          c->native_funcs = realloc(c->native_funcs, sizeof(NativeFunc) * (size_t)c->c_native_funcs);
        }
        int ni = c->n_native_funcs++;
        c->native_funcs[ni].mod  = strdup(mname);
        c->native_funcs[ni].name = strdup(fname);
        c->native_funcs[ni].ret  = strdup(ret_spec);
        c->native_funcs[ni].csym = strdup(csym);
        c->native_funcs[ni].feat = strdup(mod_feat ? mod_feat : "");
        c->native_funcs[ni].args = arg_specs;
        c->native_funcs[ni].nargs = en;
        continue;
      }

      /* native_new [arg_specs], "csym"  and
         native_method :name, [arg_specs], ret_spec, "csym"
         bind a native class's constructor / instance methods to C symbols. */
      if ((sp_streq(dname, "native_new") || sp_streq(dname, "native_method")) && native_cid >= 0) {
        int is_ctor = sp_streq(dname, "native_new");
        int need = is_ctor ? 2 : 4;
        if (an < need) continue;
        const char *mname_m = is_ctor ? "new" : ffi_arg_str(nt, args[0]);
        int arr_i = is_ctor ? 0 : 1;
        const char *arr_ty = nt_type(nt, args[arr_i]);
        const char *ret_spec = is_ctor ? "" : ffi_arg_str(nt, args[2]);
        const char *csym = ffi_arg_str(nt, args[is_ctor ? 1 : 3]);
        if (!mname_m || !csym || !ret_spec || !arr_ty || !sp_streq(arr_ty, "ArrayNode")) continue;
        int en = 0;
        const int *elems = nt_arr(nt, args[arr_i], "elements", &en);
        char **arg_specs = malloc(sizeof(char *) * (size_t)(en + 1));
        for (int ei = 0; ei < en; ei++) {
          const char *spec = ffi_arg_str(nt, elems[ei]);
          arg_specs[ei] = strdup(spec ? spec : "");
        }
        if (c->n_native_methods >= c->c_native_methods) {
          c->c_native_methods = c->c_native_methods ? c->c_native_methods * 2 : 16;
          c->native_methods = realloc(c->native_methods, sizeof(NativeMethod) * (size_t)c->c_native_methods);
        }
        int mi = c->n_native_methods++;
        c->native_methods[mi].class_id = native_cid;
        c->native_methods[mi].kind = is_ctor ? 1 : 0;
        c->native_methods[mi].name = strdup(mname_m);
        c->native_methods[mi].ret  = strdup(ret_spec);
        c->native_methods[mi].csym = strdup(csym);
        c->native_methods[mi].args = arg_specs;
        c->native_methods[mi].nargs = en;
        continue;
      }
      /* native_struct is handled in the pre-scan above; skip it here. */
      if (sp_streq(dname, "native_struct")) continue;

      /* native_obj_reflect: the package consumes the generic object->hash
         reflection (sp_obj_to_hash); codegen installs it when Structs exist. */
      if (sp_streq(dname, "native_obj_reflect")) { c->native_obj_reflect = 1; continue; }

      /* native_obj "packages/<pkg>/<file>.o": a carried C object linked only
         when this module's require-gate feature is present (Path B). */
      if (sp_streq(dname, "native_obj")) {
        if (an < 1) continue;
        const char *objp = ffi_arg_str(nt, args[0]);
        if (!objp) continue;
        if (c->n_native_objs >= c->c_native_objs) {
          c->c_native_objs = c->c_native_objs ? c->c_native_objs * 2 : 8;
          c->native_objs = realloc(c->native_objs, sizeof(NativeObj) * (size_t)c->c_native_objs);
        }
        int oi = c->n_native_objs++;
        c->native_objs[oi].mod  = strdup(mname);
        c->native_objs[oi].path = strdup(objp);
        c->native_objs[oi].feat = strdup(mod_feat ? mod_feat : "");
        continue;
      }

      if (sp_streq(dname, "ffi_lib")) {
        if (an < 1) continue;
        const char *libname = ffi_arg_str(nt, args[0]);
        if (!libname) continue;
        /* find or create lib entry */
        int mi = -1;
        for (int li = 0; li < c->n_ffi_libs; li++)
          if (sp_streq(c->ffi_libs[li].mod, mname)) { mi = li; break; }
        if (mi < 0) {
          if (c->n_ffi_libs >= c->c_ffi_libs) {
            c->c_ffi_libs = c->c_ffi_libs ? c->c_ffi_libs * 2 : 8;
            c->ffi_libs = realloc(c->ffi_libs, sizeof(FfiLib) * (size_t)c->c_ffi_libs);
          }
          c->ffi_libs[c->n_ffi_libs].mod  = strdup(mname);
          c->ffi_libs[c->n_ffi_libs].names = strdup(libname);
          mi = c->n_ffi_libs++;
        }
        else {
          /* append with semicolon */
          size_t old_len = strlen(c->ffi_libs[mi].names);
          size_t new_len = old_len + 1 + strlen(libname) + 1;
          char *merged = malloc(new_len);
          snprintf(merged, new_len, "%s;%s", c->ffi_libs[mi].names, libname);
          free(c->ffi_libs[mi].names);
          c->ffi_libs[mi].names = merged;
        }
        continue;
      }

      if (sp_streq(dname, "ffi_cflags")) {
        if (an < 1) continue;
        const char *cflag = ffi_arg_str(nt, args[0]);
        if (!cflag) continue;
        /* find or create cflag entry, semicolon-merged per module */
        int mi = -1;
        for (int ci = 0; ci < c->n_ffi_cflags; ci++)
          if (sp_streq(c->ffi_cflags[ci].mod, mname)) { mi = ci; break; }
        if (mi < 0) {
          if (c->n_ffi_cflags >= c->c_ffi_cflags) {
            c->c_ffi_cflags = c->c_ffi_cflags ? c->c_ffi_cflags * 2 : 8;
            c->ffi_cflags = realloc(c->ffi_cflags, sizeof(FfiCflag) * (size_t)c->c_ffi_cflags);
          }
          c->ffi_cflags[c->n_ffi_cflags].mod = strdup(mname);
          c->ffi_cflags[c->n_ffi_cflags].val = strdup(cflag);
          mi = c->n_ffi_cflags++;
        }
        else {
          size_t old_len = strlen(c->ffi_cflags[mi].val);
          size_t new_len = old_len + 1 + strlen(cflag) + 1;
          char *merged = malloc(new_len);
          snprintf(merged, new_len, "%s;%s", c->ffi_cflags[mi].val, cflag);
          free(c->ffi_cflags[mi].val);
          c->ffi_cflags[mi].val = merged;
        }
        continue;
      }

      if (sp_streq(dname, "ffi_func")) {
        if (an < 3) continue;
        const char *fname = ffi_arg_str(nt, args[0]);
        if (!fname) continue;
        /* arg type array */
        int arr_id = args[1];
        const char *arr_ty = nt_type(nt, arr_id);
        if (!arr_ty || !sp_streq(arr_ty, "ArrayNode")) continue;
        int en = 0;
        const int *elems = nt_arr(nt, arr_id, "elements", &en);
        char **arg_specs = malloc(sizeof(char*) * (size_t)(en + 1));
        for (int ei = 0; ei < en; ei++) {
          const char *spec = ffi_arg_str(nt, elems[ei]);
          arg_specs[ei] = strdup(spec ? spec : "");
        }
        const char *ret_spec = ffi_arg_str(nt, args[2]);
        if (!ret_spec) { free(arg_specs); continue; }
        /* grow array */
        if (c->n_ffi_funcs >= c->c_ffi_funcs) {
          c->c_ffi_funcs = c->c_ffi_funcs ? c->c_ffi_funcs * 2 : 16;
          c->ffi_funcs = realloc(c->ffi_funcs, sizeof(FfiFunc) * (size_t)c->c_ffi_funcs);
        }
        int fi = c->n_ffi_funcs++;
        c->ffi_funcs[fi].mod  = strdup(mname);
        c->ffi_funcs[fi].name = strdup(fname);
        c->ffi_funcs[fi].ret   = strdup(ret_spec);
        c->ffi_funcs[fi].args  = arg_specs;
        c->ffi_funcs[fi].nargs = en;
        c->ffi_funcs[fi].is_fiddle = 0;
        continue;
      }

      if (sp_streq(dname, "ffi_const")) {
        if (an < 2) continue;
        const char *kname = ffi_arg_str(nt, args[0]);
        if (!kname) continue;
        int val = ffi_arg_int(nt, args[1]);
        if (c->n_ffi_consts >= c->c_ffi_consts) {
          c->c_ffi_consts = c->c_ffi_consts ? c->c_ffi_consts * 2 : 16;
          c->ffi_consts = realloc(c->ffi_consts, sizeof(FfiConst) * (size_t)c->c_ffi_consts);
        }
        int ci2 = c->n_ffi_consts++;
        c->ffi_consts[ci2].mod  = strdup(mname);
        c->ffi_consts[ci2].name = strdup(kname);
        c->ffi_consts[ci2].val  = val;
        continue;
      }

      if (sp_streq(dname, "ffi_buffer")) {
        if (an < 2) continue;
        const char *bname = ffi_arg_str(nt, args[0]);
        if (!bname) continue;
        int bsize = ffi_arg_int(nt, args[1]);
        if (bsize <= 0) continue;
        if (c->n_ffi_bufs >= c->c_ffi_bufs) {
          c->c_ffi_bufs = c->c_ffi_bufs ? c->c_ffi_bufs * 2 : 8;
          c->ffi_bufs = realloc(c->ffi_bufs, sizeof(FfiBuf) * (size_t)c->c_ffi_bufs);
        }
        int bi = c->n_ffi_bufs++;
        c->ffi_bufs[bi].mod  = strdup(mname);
        c->ffi_bufs[bi].name = strdup(bname);
        c->ffi_bufs[bi].size = bsize;
        continue;
      }

      if (!strncmp(dname, "ffi_read_", 9)) {
        if (an < 2) continue;
        const char *rname = ffi_arg_str(nt, args[0]);
        if (!rname) continue;
        int roff = ffi_arg_int(nt, args[1]);
        if (roff < 0) roff = 0;
        const char *kind = dname + 9;  /* "u32", "i32", "ptr" */
        if (c->n_ffi_readers >= c->c_ffi_readers) {
          c->c_ffi_readers = c->c_ffi_readers ? c->c_ffi_readers * 2 : 8;
          c->ffi_readers = realloc(c->ffi_readers, sizeof(FfiReader) * (size_t)c->c_ffi_readers);
        }
        int ri = c->n_ffi_readers++;
        c->ffi_readers[ri].mod    = strdup(mname);
        c->ffi_readers[ri].name   = strdup(rname);
        c->ffi_readers[ri].offset = roff;
        c->ffi_readers[ri].kind   = strdup(kind);
        continue;
      }

      /* ffi_callback :name, [arg_specs], ret_spec -- declares a C
         function-pointer type usable as an ffi_func arg spec. */
      if (sp_streq(dname, "ffi_callback")) {
        if (an < 3) continue;
        const char *cbname = ffi_arg_str(nt, args[0]);
        const char *arr_ty = nt_type(nt, args[1]);
        const char *ret_spec = ffi_arg_str(nt, args[2]);
        if (!cbname || !ret_spec || !arr_ty || !sp_streq(arr_ty, "ArrayNode")) continue;
        int en = 0; const int *elems = nt_arr(nt, args[1], "elements", &en);
        char **arg_specs = malloc(sizeof(char *) * (size_t)(en + 1));
        if (!arg_specs) { perror("malloc"); exit(1); }
        for (int ei = 0; ei < en; ei++) {
          const char *spec = ffi_arg_str(nt, elems[ei]);
          arg_specs[ei] = strdup(spec ? spec : "");
        }
        if (c->n_ffi_callbacks >= c->c_ffi_callbacks) {
          c->c_ffi_callbacks = c->c_ffi_callbacks ? c->c_ffi_callbacks * 2 : 8;
          FfiCallback *grown = realloc(c->ffi_callbacks, sizeof(FfiCallback) * (size_t)c->c_ffi_callbacks);
          if (!grown) { perror("realloc"); exit(1); }
          c->ffi_callbacks = grown;
        }
        int ci = c->n_ffi_callbacks++;
        c->ffi_callbacks[ci].mod       = strdup(mname);
        c->ffi_callbacks[ci].name      = strdup(cbname);
        c->ffi_callbacks[ci].arg_specs = arg_specs;
        c->ffi_callbacks[ci].nargs     = en;
        c->ffi_callbacks[ci].ret_spec  = strdup(ret_spec);
        continue;
      }

      /* ffi_struct :Name, [[:field, :spec], ...] -- a named C struct with
         generated field accessors: Name_new / Name_get_<f> / Name_set_<f>. */
      if (sp_streq(dname, "ffi_struct")) {
        if (an < 2) continue;
        const char *sname = ffi_arg_str(nt, args[0]);
        const char *arr_ty = nt_type(nt, args[1]);
        if (!sname || !arr_ty || !sp_streq(arr_ty, "ArrayNode")) continue;
        int en = 0; const int *elems = nt_arr(nt, args[1], "elements", &en);
        FfiField *fields = malloc(sizeof(FfiField) * (size_t)(en > 0 ? en : 1));
        if (!fields) { perror("malloc"); exit(1); }
        int nf = 0;
        for (int ei = 0; ei < en; ei++) {
          const char *pty = nt_type(nt, elems[ei]);
          if (!pty || !sp_streq(pty, "ArrayNode")) continue;
          int pn = 0; const int *pair = nt_arr(nt, elems[ei], "elements", &pn);
          if (pn < 2) continue;
          const char *fn = ffi_arg_str(nt, pair[0]);
          const char *fs = ffi_arg_str(nt, pair[1]);
          if (!fn || !fs) continue;
          fields[nf].name = strdup(fn);
          fields[nf].spec = strdup(fs);
          nf++;
        }
        if (nf == 0) { free(fields); continue; }
        if (c->n_ffi_structs >= c->c_ffi_structs) {
          c->c_ffi_structs = c->c_ffi_structs ? c->c_ffi_structs * 2 : 8;
          FfiStruct *grown = realloc(c->ffi_structs, sizeof(FfiStruct) * (size_t)c->c_ffi_structs);
          if (!grown) { perror("realloc"); exit(1); }
          c->ffi_structs = grown;
        }
        int sidx = c->n_ffi_structs++;
        c->ffi_structs[sidx].mod     = strdup(mname);
        c->ffi_structs[sidx].name    = strdup(sname);
        c->ffi_structs[sidx].fields  = fields;
        c->ffi_structs[sidx].nfields = nf;
        continue;
      }
    }
  }
}

/* Look up an ffi_callback by (module, name). Returns index or -1. */
int ffi_find_callback(Compiler *c, const char *mod, const char *name) {
  for (int i = 0; i < c->n_ffi_callbacks; i++)
    if (sp_streq(c->ffi_callbacks[i].mod, mod) && sp_streq(c->ffi_callbacks[i].name, name))
      return i;
  return -1;
}

/* Resolve Module.<method> against ffi_struct declarations. See compiler.h. */
int ffi_struct_method(Compiler *c, const char *mod, const char *method, int *si, int *fi) {
  for (int i = 0; i < c->n_ffi_structs; i++) {
    if (!sp_streq(c->ffi_structs[i].mod, mod)) continue;
    const char *nm = c->ffi_structs[i].name;
    size_t nl = strlen(nm);
    if (strncmp(method, nm, nl) != 0 || method[nl] != '_') continue;
    const char *rest = method + nl + 1;
    if (sp_streq(rest, "new")) { *si = i; *fi = -1; return FFI_SM_NEW; }
    int isget = !strncmp(rest, "get_", 4);
    int isset = !strncmp(rest, "set_", 4);
    if (isget || isset) {
      const char *field = rest + 4;
      for (int f = 0; f < c->ffi_structs[i].nfields; f++)
        if (sp_streq(c->ffi_structs[i].fields[f].name, field)) {
          *si = i; *fi = f; return isget ? FFI_SM_GET : FFI_SM_SET;
        }
    }
  }
  return FFI_SM_NONE;
}

/* ---- compile-time Fiddle lowering (see register_fiddle_decls) ----
   `require "fiddle"` programs that use only statically-resolvable idioms are
   lowered onto the FFI machinery above: the Importer `extern "C sig"` DSL
   becomes a synthetic FfiFunc so `Module.func(...)` dispatches like any
   ffi_func. Anything not statically lowerable is simply left unregistered, so
   the resulting call fails loudly at codegen (`unsupported`) -- never a silent
   wrong result. */

static int fiddle_ident_ch(char ch) {
  return (ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') ||
         (ch >= '0' && ch <= '9') || ch == '_';
}
static char *fiddle_dup_range(const char *a, const char *b) {
  size_t n = (size_t)(b - a);
  char *s = malloc(n + 1);
  if (!s) { perror("malloc"); exit(1); }
  memcpy(s, a, n); s[n] = '\0';
  return s;
}

/* Map a C type spelling (from an `extern` signature or a `typealias`) to the
   Spinel FFI spec vocabulary understood by ffi_spec_to_ty / ffi_c_type. LP64
   target. `is_arg` distinguishes a `const char *` argument -- a Ruby String
   passed as a C string, spec "str" -- from a pointer return, which stays a raw
   pointer ("ptr", Fiddle returns a Pointer). `af`/`at` are the module's
   typealias pairs (resolved one level). Returns NULL for a spelling we cannot
   lower (caller skips the decl). */
static const char *fiddle_ctype_to_spec(const char *ty, int is_arg,
                                        char **af, char **at, int na) {
  if (!ty) return NULL;
  int is_ptr = 0, is_char = 0, is_const = 0, longs = 0;
  const char *base = NULL;
  char word[64];
  for (const char *p = ty; *p; ) {
    if (*p == '*' || *p == '[') { is_ptr = 1; p++; continue; }
    if (*p == ']' || *p == ' ' || *p == '\t') { p++; continue; }
    size_t w = 0;
    while (*p && fiddle_ident_ch(*p)) { if (w < sizeof(word) - 1) word[w++] = *p; p++; }
    if (w == 0) { p++; continue; }
    word[w] = '\0';
    if (sp_streq(word, "const")) { is_const = 1; continue; }
    if (sp_streq(word, "signed") || sp_streq(word, "unsigned") ||
        sp_streq(word, "struct") || sp_streq(word, "union") || sp_streq(word, "enum"))
      continue;                                    /* spec vocab is sign-agnostic */
    if (sp_streq(word, "long"))  { longs++; base = "long"; continue; }
    if (sp_streq(word, "char"))  { is_char = 1; base = "char"; continue; }
    base = word;   /* void/int/short/float/double/size_t/bool/int32_t/alias/... */
  }
  if (is_ptr) {
    if (is_char && is_const && is_arg) return "str";   /* const char* arg = C string */
    return "ptr";
  }
  if (!base) return NULL;
  if (sp_streq(base, "void"))    return "void";
  if (sp_streq(base, "float"))   return "float";
  if (sp_streq(base, "double"))  return "double";
  if (sp_streq(base, "bool") || sp_streq(base, "_Bool")) return "bool";
  if (sp_streq(base, "size_t"))  return "size_t";
  if (sp_streq(base, "ssize_t")) return "long";
  if (sp_streq(base, "long"))    return longs >= 2 ? "int64" : "long";
  if (sp_streq(base, "char") || sp_streq(base, "short") || sp_streq(base, "int"))
    return "int";
  if (sp_streq(base, "int8_t")  || sp_streq(base, "uint8_t"))  return "int8";
  if (sp_streq(base, "int16_t") || sp_streq(base, "uint16_t")) return "int16";
  if (sp_streq(base, "int32_t") || sp_streq(base, "uint32_t")) return "int";
  if (sp_streq(base, "int64_t") || sp_streq(base, "uint64_t")) return "int64";
  if (sp_streq(base, "intptr_t") || sp_streq(base, "uintptr_t") ||
      sp_streq(base, "ptrdiff_t")) return "long";
  /* one-level typealias resolution */
  for (int i = 0; i < na; i++)
    if (sp_streq(af[i], base)) return fiddle_ctype_to_spec(at[i], is_arg, NULL, NULL, 0);
  return NULL;
}

/* Parse a Fiddle signature "ret name(arg, ...)" into FFI specs. Returns 1 and
   sets the out params (caller owns the allocations) on success; 0 for a form we
   cannot lower (function-pointer params, varargs, unknown types). */
static int fiddle_parse_signature(const char *sig, char **af, char **at, int na,
                                  char **fname_out, char **ret_out,
                                  char ***args_out, int *nargs_out) {
  if (!sig) return 0;
  const char *lp = strchr(sig, '(');
  const char *rp = lp ? strrchr(sig, ')') : NULL;
  if (!lp || !rp || rp < lp) return 0;
  for (const char *p = lp + 1; p < rp; p++)     /* nested () = fn-pointer param */
    if (*p == '(' || *p == ')') return 0;
  /* name = last identifier run before '('; return spelling = everything before */
  const char *ne = lp;
  while (ne > sig && ne[-1] == ' ') ne--;
  const char *ns = ne;
  while (ns > sig && fiddle_ident_ch(ns[-1])) ns--;
  if (ns == ne) return 0;
  char *retsp = fiddle_dup_range(sig, ns);
  const char *ret = fiddle_ctype_to_spec(retsp, 0, af, at, na);
  free(retsp);
  if (!ret) return 0;
  char *fname = fiddle_dup_range(ns, ne);
  /* arg list */
  char **args = NULL; int nargs = 0, cap = 0;
  const char *p = lp + 1;
  int ok = 1;
  while (p < rp) {
    while (p < rp && (*p == ' ' || *p == ',')) p++;
    if (p >= rp) break;
    const char *start = p;
    while (p < rp && *p != ',') p++;
    const char *end = p;
    while (end > start && end[-1] == ' ') end--;
    char *tok = fiddle_dup_range(start, end);
    if (sp_streq(tok, "void") || tok[0] == '\0') { free(tok); continue; }
    const char *spec = fiddle_ctype_to_spec(tok, 1, af, at, na);
    free(tok);
    if (!spec) { ok = 0; break; }
    if (nargs >= cap) {
      cap = cap ? cap * 2 : 4;
      char **grown = realloc(args, sizeof(char *) * (size_t)cap);
      if (!grown) { perror("realloc"); exit(1); }
      args = grown;
    }
    args[nargs++] = strdup(spec);
  }
  if (!ok) {
    for (int i = 0; i < nargs; i++) free(args[i]);
    free(args); free(fname);
    return 0;
  }
  *fname_out = fname;
  *ret_out = strdup(ret);
  *args_out = args;
  *nargs_out = nargs;
  return 1;
}

/* Register the Importer `extern` DSL as synthetic ffi_funcs. A module that
   `extend Fiddle::Importer` and declares `extern "C sig"` gets one FfiFunc per
   extern, so `Module.func(args)` resolves through the unchanged FFI inference +
   codegen. `dlload "lib"` appends an FfiLib (a `-llib` link marker); `dlload
   nil` resolves against the linked image and needs no marker. Gated on the
   fiddle require. */
void register_fiddle_decls(Compiler *c) {
  if (!sp_feature_enabled("fiddle")) return;
  const NodeTable *nt = c->nt;
  NT_FOREACH_KIND(nt, NK_ModuleNode, id) {
    int cp = nt_ref(nt, id, "constant_path");
    const char *mname = cp >= 0 ? nt_str(nt, cp, "name") : NULL;
    if (!mname) continue;
    int body = nt_ref(nt, id, "body");
    int sn = 0;
    const int *stmts = body >= 0 ? nt_arr(nt, body, "body", &sn) : NULL;

    /* Pre-scan: only lower modules that `extend Fiddle::Importer`. */
    int is_importer = 0;
    for (int k = 0; k < sn && !is_importer; k++) {
      int s = stmts[k];
      if (!nt_type(nt, s) || !sp_streq(nt_type(nt, s), "CallNode")) continue;
      if (nt_ref(nt, s, "receiver") >= 0) continue;
      const char *dn = nt_str(nt, s, "name");
      if (!dn || !sp_streq(dn, "extend")) continue;
      int a = nt_ref(nt, s, "arguments"); int an = 0;
      const int *av = a >= 0 ? nt_arr(nt, a, "arguments", &an) : NULL;
      if (an < 1) continue;
      int cpn = av[0];
      if (!nt_type(nt, cpn) || !sp_streq(nt_type(nt, cpn), "ConstantPathNode")) continue;
      const char *cn = nt_str(nt, cpn, "name");
      int par = nt_ref(nt, cpn, "parent");
      const char *pn = par >= 0 ? nt_str(nt, par, "name") : NULL;
      if (cn && pn && sp_streq(cn, "Importer") && sp_streq(pn, "Fiddle")) is_importer = 1;
    }
    if (!is_importer) continue;

    /* Pre-scan: typealias pairs (both string literals). */
    char *afrom[64]; char *ato[64]; int nalias = 0;
    for (int k = 0; k < sn; k++) {
      int s = stmts[k];
      if (!nt_type(nt, s) || !sp_streq(nt_type(nt, s), "CallNode")) continue;
      if (nt_ref(nt, s, "receiver") >= 0) continue;
      const char *dn = nt_str(nt, s, "name");
      if (!dn || !sp_streq(dn, "typealias")) continue;
      int a = nt_ref(nt, s, "arguments"); int an = 0;
      const int *av = a >= 0 ? nt_arr(nt, a, "arguments", &an) : NULL;
      if (an < 2 || nalias >= 64) continue;
      const char *from = ffi_arg_str(nt, av[0]);
      const char *to   = ffi_arg_str(nt, av[1]);
      if (from && to) { afrom[nalias] = (char *)from; ato[nalias] = (char *)to; nalias++; }
    }

    /* Process dlload + extern. */
    for (int k = 0; k < sn; k++) {
      int s = stmts[k];
      if (!nt_type(nt, s) || !sp_streq(nt_type(nt, s), "CallNode")) continue;
      if (nt_ref(nt, s, "receiver") >= 0) continue;
      const char *dn = nt_str(nt, s, "name");
      if (!dn) continue;
      int a = nt_ref(nt, s, "arguments"); int an = 0;
      const int *av = a >= 0 ? nt_arr(nt, a, "arguments", &an) : NULL;

      if (sp_streq(dn, "dlload")) {
        if (an < 1) continue;
        const char *ty0 = nt_type(nt, av[0]);
        if (ty0 && sp_streq(ty0, "NilNode")) continue;   /* this image, no -l marker */
        const char *libname = ffi_arg_str(nt, av[0]);
        if (!libname) continue;                          /* computed lib: no marker */
        int mi = -1;
        for (int li = 0; li < c->n_ffi_libs; li++)
          if (sp_streq(c->ffi_libs[li].mod, mname)) { mi = li; break; }
        if (mi < 0) {
          if (c->n_ffi_libs >= c->c_ffi_libs) {
            c->c_ffi_libs = c->c_ffi_libs ? c->c_ffi_libs * 2 : 8;
            FfiLib *grown = realloc(c->ffi_libs, sizeof(FfiLib) * (size_t)c->c_ffi_libs);
            if (!grown) { perror("realloc"); exit(1); }
            c->ffi_libs = grown;
          }
          c->ffi_libs[c->n_ffi_libs].mod   = strdup(mname);
          c->ffi_libs[c->n_ffi_libs].names = strdup(libname);
          c->n_ffi_libs++;
        } else {
          size_t nl = strlen(c->ffi_libs[mi].names) + 1 + strlen(libname) + 1;
          char *merged = malloc(nl);
          if (!merged) { perror("malloc"); exit(1); }
          snprintf(merged, nl, "%s;%s", c->ffi_libs[mi].names, libname);
          free(c->ffi_libs[mi].names);
          c->ffi_libs[mi].names = merged;
        }
        continue;
      }

      if (sp_streq(dn, "extern")) {
        if (an < 1) continue;
        const char *sig = ffi_arg_str(nt, av[0]);
        if (!sig) continue;                              /* non-literal: codegen rejects the call */
        char *fn = NULL, *ret = NULL, **as = NULL; int nas = 0;
        if (!fiddle_parse_signature(sig, afrom, ato, nalias, &fn, &ret, &as, &nas))
          continue;                                      /* unlowerable: call rejects downstream */
        if (c->n_ffi_funcs >= c->c_ffi_funcs) {
          c->c_ffi_funcs = c->c_ffi_funcs ? c->c_ffi_funcs * 2 : 16;
          FfiFunc *grown = realloc(c->ffi_funcs, sizeof(FfiFunc) * (size_t)c->c_ffi_funcs);
          if (!grown) { perror("realloc"); exit(1); }
          c->ffi_funcs = grown;
        }
        int fi = c->n_ffi_funcs++;
        c->ffi_funcs[fi].mod   = strdup(mname);
        c->ffi_funcs[fi].name  = fn;
        c->ffi_funcs[fi].ret   = ret;
        c->ffi_funcs[fi].args  = as;
        c->ffi_funcs[fi].nargs = nas;
        c->ffi_funcs[fi].is_fiddle = 1;
        continue;
      }
    }
  }
}

/* Synthetic module tag for ffi_funcs created by the low-level Fiddle API; never
   a real receiver, so it is reached only through a FiddleLocal (find_fiddle_local),
   not the module-name FFI dispatch. */
#define FIDDLE_MOD "__fiddle__"

/* Map a Fiddle TYPE_* constant name to a Spinel FFI spec (LP64). NULL = reject
   (e.g. TYPE_VARIADIC). Unsigned variants share the signed spec (sign-agnostic). */
static const char *fiddle_type_to_spec(const char *name) {
  if (!name) return NULL;
  if (sp_streq(name, "TYPE_VOID"))         return "void";
  if (sp_streq(name, "TYPE_VOIDP"))        return "ptr";
  if (sp_streq(name, "TYPE_CONST_STRING")) return "str";
  if (sp_streq(name, "TYPE_BOOL"))         return "bool";
  if (sp_streq(name, "TYPE_FLOAT"))        return "float";
  if (sp_streq(name, "TYPE_DOUBLE"))       return "double";
  if (sp_streq(name, "TYPE_SIZE_T"))       return "size_t";
  if (sp_streq(name, "TYPE_CHAR")  || sp_streq(name, "TYPE_UCHAR") ||
      sp_streq(name, "TYPE_INT8_T")|| sp_streq(name, "TYPE_UINT8_T"))  return "int8";
  if (sp_streq(name, "TYPE_SHORT") || sp_streq(name, "TYPE_USHORT") ||
      sp_streq(name, "TYPE_INT16_T")|| sp_streq(name, "TYPE_UINT16_T")) return "int16";
  if (sp_streq(name, "TYPE_INT")   || sp_streq(name, "TYPE_UINT") ||
      sp_streq(name, "TYPE_INT32_T")|| sp_streq(name, "TYPE_UINT32_T")) return "int";
  if (sp_streq(name, "TYPE_LONG")  || sp_streq(name, "TYPE_ULONG") ||
      sp_streq(name, "TYPE_SSIZE_T")|| sp_streq(name, "TYPE_PTRDIFF_T") ||
      sp_streq(name, "TYPE_INTPTR_T")|| sp_streq(name, "TYPE_UINTPTR_T")) return "long";
  if (sp_streq(name, "TYPE_LONG_LONG")|| sp_streq(name, "TYPE_ULONG_LONG") ||
      sp_streq(name, "TYPE_INT64_T")|| sp_streq(name, "TYPE_UINT64_T")) return "int64";
  return NULL;
}

/* Resolve a Fiddle type argument node -- a `Fiddle::TYPE_*` constant, possibly
   negated (`-Fiddle::TYPE_INT`, the unsigned form) -- to a spec string or NULL. */
static const char *fiddle_typearg_spec(const NodeTable *nt, int node) {
  if (node < 0) return NULL;
  const char *ty = nt_type(nt, node);
  if (!ty) return NULL;
  if (sp_streq(ty, "CallNode")) {
    const char *nm = nt_str(nt, node, "name");
    if (nm && sp_streq(nm, "-@")) return fiddle_typearg_spec(nt, nt_ref(nt, node, "receiver"));
    return NULL;
  }
  if (!sp_streq(ty, "ConstantPathNode")) return NULL;
  const char *cn = nt_str(nt, node, "name");
  int par = nt_ref(nt, node, "parent");
  const char *pn = par >= 0 ? nt_str(nt, par, "name") : NULL;
  if (!cn || !pn || !sp_streq(pn, "Fiddle")) return NULL;
  return fiddle_type_to_spec(cn);
}

/* Record that dlopen/Function.new call node `id` was bound (so it lowers to
   nil). Deduped. */
static void fiddle_ctor_mark(Compiler *c, int id) {
  for (int i = 0; i < c->n_fiddle_ctors; i++) if (c->fiddle_ctors[i] == id) return;
  if (c->n_fiddle_ctors >= c->c_fiddle_ctors) {
    c->c_fiddle_ctors = c->c_fiddle_ctors ? c->c_fiddle_ctors * 2 : 8;
    int *grown = realloc(c->fiddle_ctors, sizeof(int) * (size_t)c->c_fiddle_ctors);
    if (!grown) { perror("realloc"); exit(1); }
    c->fiddle_ctors = grown;
  }
  c->fiddle_ctors[c->n_fiddle_ctors++] = id;
}

/* Was call node `id` a dlopen/Function.new that register_fiddle_locals bound? */
int fiddle_ctor_is_bound(Compiler *c, int id) {
  for (int i = 0; i < c->n_fiddle_ctors; i++) if (c->fiddle_ctors[i] == id) return 1;
  return 0;
}

/* Is `node` a `Fiddle::<leaf>` constant path (ConstantPathNode name==leaf,
   parent ConstantReadNode "Fiddle")? Used for Fiddle::Pointer / Fiddle::NULL. */
static int is_fiddle_const_path(const NodeTable *nt, int node, const char *leaf) {
  if (node < 0 || !nt_type(nt, node) || !sp_streq(nt_type(nt, node), "ConstantPathNode")) return 0;
  const char *cn = nt_str(nt, node, "name");
  int par = nt_ref(nt, node, "parent");
  const char *pn = par >= 0 ? nt_str(nt, par, "name") : NULL;
  return cn && pn && sp_streq(cn, leaf) && sp_streq(pn, "Fiddle");
}
int is_fiddle_pointer_const(const NodeTable *nt, int node) { return is_fiddle_const_path(nt, node, "Pointer"); }
int is_fiddle_null_const(const NodeTable *nt, int node)    { return is_fiddle_const_path(nt, node, "NULL"); }

/* The class index of the Fiddle::Pointer native class, or -1 (fiddle not required). */
int fiddle_pointer_cid(Compiler *c) { return comp_class_index(c, "Fiddle::Pointer"); }

/* Find a Fiddle local binding by (scope, name); returns its index or -1. */
int find_fiddle_local(Compiler *c, int scope, const char *name) {
  if (!name) return -1;
  for (int i = 0; i < c->n_fiddle_locals; i++)
    if (c->fiddle_locals[i].scope == scope && sp_streq(c->fiddle_locals[i].name, name))
      return i;
  return -1;
}

static void fiddle_local_append(Compiler *c, int scope, const char *name,
                                int kind, const char *lib, int ffi_idx) {
  if (c->n_fiddle_locals >= c->c_fiddle_locals) {
    c->c_fiddle_locals = c->c_fiddle_locals ? c->c_fiddle_locals * 2 : 8;
    FiddleLocal *grown = realloc(c->fiddle_locals, sizeof(FiddleLocal) * (size_t)c->c_fiddle_locals);
    if (!grown) { perror("realloc"); exit(1); }
    c->fiddle_locals = grown;
  }
  int i = c->n_fiddle_locals++;
  c->fiddle_locals[i].scope   = scope;
  c->fiddle_locals[i].name    = strdup(name);
  c->fiddle_locals[i].kind    = kind;
  c->fiddle_locals[i].lib     = lib ? strdup(lib) : NULL;
  c->fiddle_locals[i].ffi_idx = ffi_idx;
}

/* Register the low-level Fiddle API bound to locals: `h = Fiddle.dlopen(...)`
   and `f = Fiddle::Function.new(h["sym"], [TYPE...], TYPE)`. A Function local
   gets a synthetic ffi_func (keyed by the C symbol), so `f.call(args)` lowers
   through the same FFI codegen as an ffi_func. Only literal shapes are bound;
   anything else is left unbound so the later `.call` fails loudly at codegen. */
void register_fiddle_locals(Compiler *c) {
  if (!sp_feature_enabled("fiddle")) return;
  const NodeTable *nt = c->nt;
  /* Pass 0: handles (dlopen).  Pass 1: functions (may reference a handle). */
  for (int pass = 0; pass < 2; pass++) {
    NT_FOREACH_KIND(nt, NK_LocalVariableWriteNode, w) {
      const char *lname = nt_str(nt, w, "name");
      int val = nt_ref(nt, w, "value");
      if (!lname || val < 0) continue;
      if (!nt_type(nt, val) || !sp_streq(nt_type(nt, val), "CallNode")) continue;
      int scope = c->nscope[w];
      const char *cname = nt_str(nt, val, "name");
      int recv = nt_ref(nt, val, "receiver");
      int anode = nt_ref(nt, val, "arguments");
      int an = 0;
      const int *av = anode >= 0 ? nt_arr(nt, anode, "arguments", &an) : NULL;

      if (pass == 0) {
        if (!cname || !sp_streq(cname, "dlopen")) continue;
        if (recv < 0 || !nt_type(nt, recv) || !sp_streq(nt_type(nt, recv), "ConstantReadNode")) continue;
        const char *rn = nt_str(nt, recv, "name");
        if (!rn || !sp_streq(rn, "Fiddle") || an < 1) continue;
        const char *lib = "";
        const char *a0ty = nt_type(nt, av[0]);
        if (!(a0ty && sp_streq(a0ty, "NilNode"))) {
          lib = ffi_arg_str(nt, av[0]);
          if (!lib) continue;                       /* computed lib: leave unbound */
        }
        fiddle_ctor_mark(c, val);
        if (find_fiddle_local(c, scope, lname) >= 0) continue;
        fiddle_local_append(c, scope, lname, FIDDLE_HANDLE, lib, -1);
        continue;
      }

      /* pass 1: f = Fiddle::Function.new(h["sym"], [TYPE...], TYPE) */
      if (!cname || !sp_streq(cname, "new")) continue;
      if (recv < 0 || !nt_type(nt, recv) || !sp_streq(nt_type(nt, recv), "ConstantPathNode")) continue;
      const char *rcn = nt_str(nt, recv, "name");
      int rpar = nt_ref(nt, recv, "parent");
      const char *rpn = rpar >= 0 ? nt_str(nt, rpar, "name") : NULL;
      if (!rcn || !rpn || !sp_streq(rcn, "Function") || !sp_streq(rpn, "Fiddle") || an < 3) continue;
      /* args[0] = h["sym"] */
      int hn = av[0];
      if (!nt_type(nt, hn) || !sp_streq(nt_type(nt, hn), "CallNode")) continue;
      const char *hname = nt_str(nt, hn, "name");
      if (!hname || !sp_streq(hname, "[]")) continue;
      int hrecv = nt_ref(nt, hn, "receiver");
      if (hrecv < 0 || !nt_type(nt, hrecv) || !sp_streq(nt_type(nt, hrecv), "LocalVariableReadNode")) continue;
      const char *handle_name = nt_str(nt, hrecv, "name");
      int hanode = nt_ref(nt, hn, "arguments"); int han = 0;
      const int *hav = hanode >= 0 ? nt_arr(nt, hanode, "arguments", &han) : NULL;
      if (han < 1) continue;
      const char *sym = ffi_arg_str(nt, hav[0]);
      if (!sym) continue;                            /* computed symbol: unbound */
      /* args[1] = [TYPE...] */
      if (!nt_type(nt, av[1]) || !sp_streq(nt_type(nt, av[1]), "ArrayNode")) continue;
      int en = 0;
      const int *elems = nt_arr(nt, av[1], "elements", &en);
      char **argspecs = malloc(sizeof(char *) * (size_t)(en + 1));
      if (!argspecs) { perror("malloc"); exit(1); }
      int ok = 1;
      for (int ei = 0; ei < en; ei++) {
        const char *sp = fiddle_typearg_spec(nt, elems[ei]);
        if (!sp) { ok = 0; for (int j = 0; j < ei; j++) free(argspecs[j]); break; }
        argspecs[ei] = strdup(sp);
      }
      const char *ret = ok ? fiddle_typearg_spec(nt, av[2]) : NULL;
      if (!ok || !ret) { free(argspecs); continue; }
      /* link marker from the handle's library (dlopen(nil) -> none) */
      int hidx = find_fiddle_local(c, scope, handle_name);
      const char *lib = (hidx >= 0 && c->fiddle_locals[hidx].kind == FIDDLE_HANDLE)
                        ? c->fiddle_locals[hidx].lib : NULL;
      if (lib && lib[0]) {
        int mi = -1;
        for (int li = 0; li < c->n_ffi_libs; li++)
          if (sp_streq(c->ffi_libs[li].mod, FIDDLE_MOD) && strstr(c->ffi_libs[li].names, lib)) { mi = li; break; }
        if (mi < 0) {
          if (c->n_ffi_libs >= c->c_ffi_libs) {
            c->c_ffi_libs = c->c_ffi_libs ? c->c_ffi_libs * 2 : 8;
            FfiLib *grown = realloc(c->ffi_libs, sizeof(FfiLib) * (size_t)c->c_ffi_libs);
            if (!grown) { perror("realloc"); exit(1); }
            c->ffi_libs = grown;
          }
          c->ffi_libs[c->n_ffi_libs].mod   = strdup(FIDDLE_MOD);
          c->ffi_libs[c->n_ffi_libs].names = strdup(lib);
          c->n_ffi_libs++;
        }
      }
      /* dedupe the synthetic ffi_func by C symbol */
      int fi = ffi_find_func(c, FIDDLE_MOD, sym);
      if (fi < 0) {
        if (c->n_ffi_funcs >= c->c_ffi_funcs) {
          c->c_ffi_funcs = c->c_ffi_funcs ? c->c_ffi_funcs * 2 : 16;
          FfiFunc *grown = realloc(c->ffi_funcs, sizeof(FfiFunc) * (size_t)c->c_ffi_funcs);
          if (!grown) { perror("realloc"); exit(1); }
          c->ffi_funcs = grown;
        }
        fi = c->n_ffi_funcs++;
        c->ffi_funcs[fi].mod   = strdup(FIDDLE_MOD);
        c->ffi_funcs[fi].name  = strdup(sym);
        c->ffi_funcs[fi].ret   = strdup(ret);
        c->ffi_funcs[fi].args  = argspecs;
        c->ffi_funcs[fi].nargs = en;
        c->ffi_funcs[fi].is_fiddle = 1;
      } else {
        for (int j = 0; j < en; j++) free(argspecs[j]);
        free(argspecs);
      }
      fiddle_ctor_mark(c, val);
      if (find_fiddle_local(c, scope, lname) >= 0) continue;
      fiddle_local_append(c, scope, lname, FIDDLE_FUNC, NULL, fi);
    }
  }
}

/* Look up an FFI func by (module, name). Returns index or -1. */
int ffi_find_func(Compiler *c, const char *mod, const char *name) {
  for (int i = 0; i < c->n_ffi_funcs; i++)
    if (sp_streq(c->ffi_funcs[i].mod, mod) && sp_streq(c->ffi_funcs[i].name, name))
      return i;
  return -1;
}

/* Look up an FFI buffer by (module, name). Returns index or -1. */
int ffi_find_buf(Compiler *c, const char *mod, const char *name) {
  for (int i = 0; i < c->n_ffi_bufs; i++)
    if (sp_streq(c->ffi_bufs[i].mod, mod) && sp_streq(c->ffi_bufs[i].name, name))
      return i;
  return -1;
}

/* Look up an FFI reader by (module, name). Returns index or -1. */
int ffi_find_reader(Compiler *c, const char *mod, const char *name) {
  for (int i = 0; i < c->n_ffi_readers; i++)
    if (sp_streq(c->ffi_readers[i].mod, mod) && sp_streq(c->ffi_readers[i].name, name))
      return i;
  return -1;
}

int infer_global_const_types(Compiler *c) {
  const NodeTable *nt = c->nt;
  int changed = 0;
  for (int id = 0; id < nt->count; id++) {
    const char *ty = nt_type(nt, id);
    if (!ty) continue;
    LocalVar *lv = NULL;
    TyKind vt = TY_UNKNOWN;
    if (sp_streq(ty, "GlobalVariableWriteNode")) {
      const char *nm = nt_str(nt, id, "name");
      const char *rn = nm ? comp_resolve_gvar(c, nm + 1) : NULL;
      if (rn) lv = comp_gvar(c, rn);
      vt = infer_type(c, nt_ref(nt, id, "value"));
      if (vt == TY_NIL) continue;
    }
    else if (sp_streq(ty, "GlobalVariableOperatorWriteNode")) {
      const char *nm = nt_str(nt, id, "name");
      const char *rn = nm ? comp_resolve_gvar(c, nm + 1) : NULL;
      if (rn) lv = comp_gvar(c, rn);
      TyKind cur = lv ? lv->type : TY_UNKNOWN;
      TyKind v = infer_type(c, nt_ref(nt, id, "value"));
      if (cur == TY_STRING) vt = TY_STRING;
      else if (ty_is_numeric(cur) && ty_is_numeric(v)) vt = (cur == TY_FLOAT || v == TY_FLOAT) ? TY_FLOAT : TY_INT;
      else vt = cur;
    }
    else if (sp_streq(ty, "GlobalVariableOrWriteNode") || sp_streq(ty, "GlobalVariableAndWriteNode")) {
      const char *nm = nt_str(nt, id, "name");
      const char *rn = nm ? comp_resolve_gvar(c, nm + 1) : NULL;
      if (rn) lv = comp_gvar(c, rn);
      vt = infer_type(c, nt_ref(nt, id, "value"));
      if (vt == TY_NIL) continue;
    }
    else if (sp_streq(ty, "ConstantWriteNode")) {
      const char *nm = nt_str(nt, id, "name");
      if (nm) lv = comp_const(c, nm);
      vt = infer_type(c, nt_ref(nt, id, "value"));
    }
    else if (sp_streq(ty, "ConstantPathWriteNode") || sp_streq(ty, "ConstantPathOrWriteNode") ||
             sp_streq(ty, "ConstantPathAndWriteNode") || sp_streq(ty, "ConstantPathOperatorWriteNode")) {
      int tgt = nt_ref(nt, id, "target");
      const char *nm = tgt >= 0 ? nt_str(nt, tgt, "name") : NULL;
      if (nm) lv = comp_const(c, nm);
      int is_orand = sp_streq(ty, "ConstantPathOrWriteNode") || sp_streq(ty, "ConstantPathAndWriteNode");
      /* An or/and-write-only constant has no definite value before its first
         use, so it must default to nil (poly) for the truthiness check. */
      if (is_orand && lv && !lv->const_def_write)
        vt = TY_POLY;
      else
        vt = infer_type(c, nt_ref(nt, id, "value"));
    }
    else if (sp_streq(ty, "MultiWriteNode")) {
      int ln = 0;
      const int *lefts = nt_arr(nt, id, "lefts", &ln);
      int value = nt_ref(nt, id, "value");
      const char *vty = nt_type(nt, value);
      int en = 0;
      const int *els = (vty && sp_streq(vty, "ArrayNode")) ? nt_arr(nt, value, "elements", &en) : NULL;
      int rn_count = 0;
      nt_arr(nt, id, "rights", &rn_count);
      for (int i = 0; i < ln; i++) {
        const char *lty2 = nt_type(nt, lefts[i]);
        if (!lty2 || !sp_streq(lty2, "GlobalVariableTargetNode")) continue;
        const char *gnm = nt_str(nt, lefts[i], "name");
        const char *rn2 = gnm ? comp_resolve_gvar(c, gnm + 1) : NULL;
        LocalVar *glv = rn2 ? comp_gvar(c, rn2) : NULL;
        if (!glv) continue;
        TyKind vt2 = (els && i < en) ? infer_type(c, els[i]) : TY_UNKNOWN;
        if (vt2 == TY_NIL || vt2 == TY_UNKNOWN) continue;
        TyKind merged2 = ty_unify(glv->type, vt2);
        if (merged2 != glv->type) { glv->type = merged2; changed = 1; }
      }
      /* handle splat-rest global target (*$rest = ...) */
      int rest_nid2 = nt_ref(nt, id, "rest");
      if (rest_nid2 >= 0) {
        const char *rsty2 = nt_type(nt, rest_nid2);
        int rest_inner2 = (rsty2 && sp_streq(rsty2, "SplatNode")) ? nt_ref(nt, rest_nid2, "expression") : -1;
        const char *rinty2 = rest_inner2 >= 0 ? nt_type(nt, rest_inner2) : NULL;
        if (rinty2 && sp_streq(rinty2, "GlobalVariableTargetNode")) {
          const char *gnm2 = nt_str(nt, rest_inner2, "name");
          const char *rn3 = gnm2 ? comp_resolve_gvar(c, gnm2 + 1) : NULL;
          LocalVar *glv2 = rn3 ? comp_gvar(c, rn3) : NULL;
          if (glv2 && els) {
            TyKind rest_elem = TY_UNKNOWN;
            for (int i = ln; i < en - rn_count; i++)
              rest_elem = ty_unify(rest_elem, infer_type(c, els[i]));
            TyKind rest_arr_t = (rest_elem != TY_UNKNOWN) ? ty_array_of(rest_elem) : TY_UNKNOWN;
            if (rest_arr_t != TY_UNKNOWN) {
              TyKind merged3 = ty_unify(glv2->type, rest_arr_t);
              if (merged3 != glv2->type) { glv2->type = merged3; changed = 1; }
            }
          }
        }
      }
      continue;
    }
    else if (sp_streq(ty, "CallNode")) {
      /* CONST << v / CONST.push(v) / CONST.append(v): infer CONST as an
         array whose element type comes from v's type. Only applies when
         the receiver is a direct ConstantReadNode. */
      const char *cnm = nt_str(nt, id, "name");
      if (!cnm) continue;
      int is_push = (sp_streq(cnm, "<<") || sp_streq(cnm, "push") || sp_streq(cnm, "append"));
      if (!is_push) continue;
      int crecv = nt_ref(nt, id, "receiver");
      if (crecv < 0) continue;
      const char *rty = nt_type(nt, crecv);
      if (!rty || !sp_streq(rty, "ConstantReadNode")) continue;
      const char *cnm2 = nt_str(nt, crecv, "name");
      if (!cnm2) continue;
      lv = comp_const(c, cnm2);
      if (!lv || lv->type != TY_UNKNOWN) continue;
      int cargs = nt_ref(nt, id, "arguments");
      int cac = 0;
      const int *cav = cargs >= 0 ? nt_arr(nt, cargs, "arguments", &cac) : NULL;
      if (cac < 1 || !cav) continue;
      TyKind et = infer_type(c, cav[0]);
      if (et == TY_UNKNOWN || et == TY_NIL) continue;
      vt = ty_array_of(et);
      if (vt == TY_UNKNOWN) vt = TY_POLY_ARRAY;
    }
    else {
      continue;
    }
    if (!lv) continue;
    TyKind merged = ty_unify(lv->type, vt);
    if (merged != lv->type) { lv->type = merged; changed = 1; }
  }
  return changed;
}

/* Re-infer constants assigned via multi-write with a call/variable RHS.
   The existing infer_write_types pass widened them to TY_POLY early (before
   block params converged); this pass overrides with the now-stable element
   type once it is known and not poly. */
int infer_multiwrite_const_types(Compiler *c) {
  const NodeTable *nt = c->nt;
  int changed = 0;
  NT_FOREACH_KIND(nt, NK_MultiWriteNode, id) {
    int value = nt_ref(nt, id, "value");
    if (value < 0) continue;
    const char *vty = nt_type(nt, value);
    if (vty && sp_streq(vty, "ArrayNode")) continue; /* literal handled in infer_write_types */
    TyKind st = infer_type(c, value);
    if (!ty_is_array(st)) continue;
    TyKind elem = ty_array_elem(st);
    if (elem == TY_POLY || elem == TY_UNKNOWN) continue; /* not yet settled */
    int ln = 0;
    const int *lefts = nt_arr(nt, id, "lefts", &ln);
    for (int i = 0; i < ln; i++) {
      const char *lty = nt_type(nt, lefts[i]) ? nt_type(nt, lefts[i]) : "";
      if (!sp_streq(lty, "ConstantTargetNode")) continue;
      const char *nm = nt_str(nt, lefts[i], "name");
      LocalVar *cv = nm ? comp_const(c, nm) : NULL;
      if (!cv || cv->type == elem) continue;
      cv->type = elem; changed = 1;
    }
    int rn = 0;
    const int *rights = nt_arr(nt, id, "rights", &rn);
    for (int j = 0; j < rn; j++) {
      const char *rty2 = nt_type(nt, rights[j]) ? nt_type(nt, rights[j]) : "";
      if (!sp_streq(rty2, "ConstantTargetNode")) continue;
      const char *nm = nt_str(nt, rights[j], "name");
      LocalVar *cv = nm ? comp_const(c, nm) : NULL;
      if (!cv || cv->type == elem) continue;
      cv->type = elem; changed = 1;
    }
  }
  return changed;
}

/* Resolve each class's superclass index from its ClassNode. */
void resolve_parents(Compiler *c) {
  const NodeTable *nt = c->nt;
  for (int i = 0; i < c->nclasses; i++) {
    int sc = nt_ref(nt, c->classes[i].def_node, "superclass");
    if (sc < 0) continue;
    const char *sty = nt_type(nt, sc);
    /* A module-qualified superclass (`class Child < M::Handler`) is a
       ConstantPathNode whose `name` is the last segment ("Handler").
       Classes are registered under that bare last name, so resolve it the
       same way as an unqualified ConstantReadNode superclass. */
    if (sty && (sp_streq(sty, "ConstantReadNode") || sp_streq(sty, "ConstantPathNode"))) {
      int p = comp_class_index(c, nt_str(nt, sc, "name"));
      if (p >= 0 && p != i) c->classes[i].parent = p;
    }
  }
}

/* True if the method scope's body contains a `super` (an explicit-arg SuperNode
   or a bare ForwardingSuperNode). */
static int scope_body_has_super(Compiler *c, int scope_idx) {
  const NodeTable *nt = c->nt;
  for (int id = 0; id < nt->count; id++) {
    if (c->nscope[id] != scope_idx) continue;
    const char *ty = nt_type(nt, id);
    if (ty && (sp_streq(ty, "SuperNode") || sp_streq(ty, "ForwardingSuperNode"))) return 1;
  }
  return 0;
}

/* Process include calls in a single class body, creating scope copies for each
   included module method. We copy (not mutate) so multiple classes can include
   the same module independently. */
int g_inc_did_clone = 0;
void process_include_body(Compiler *c, int ci, int body_node) {
  const NodeTable *nt = c->nt;
  int n = 0;
  const int *stmts = body_node >= 0 ? nt_arr(nt, body_node, "body", &n) : NULL;
  for (int k = 0; k < n; k++) {
    int s = stmts[k];
    const char *sty = nt_type(nt, s);
    if (!sty || !sp_streq(sty, "CallNode")) continue;
    const char *nm = nt_str(nt, s, "name");
    if (!nm || !sp_streq(nm, "include")) continue;
    if (nt_ref(nt, s, "receiver") >= 0) continue;
    int anode = nt_ref(nt, s, "arguments");
    int an = 0;
    const int *args = anode >= 0 ? nt_arr(nt, anode, "arguments", &an) : NULL;
    for (int j = 0; j < an; j++) {
      const char *aty = nt_type(nt, args[j]);
      const char *mname = NULL;
      if (aty && sp_streq(aty, "ConstantReadNode")) mname = nt_str(nt, args[j], "name");
      else if (aty && sp_streq(aty, "ConstantPathNode")) mname = nt_str(nt, args[j], "name");
      int mod_id = mname ? comp_class_index(c, mname) : -1;
      if (mod_id < 0) continue;
      /* snapshot count before adding new scopes to avoid re-scanning them */
      int snap = c->nscopes;
      for (int ms = 0; ms < snap; ms++) {
        Scope *src = &c->scopes[ms];
        if (src->class_id != mod_id || src->is_cmethod || !src->name) continue;
        const char *dst_name = src->name;
        char inc_shadow[256];
        int own = comp_method_in_class(c, ci, src->name);
        if (own >= 0) {
          /* The class overrides the module method. If the override calls super,
             the module method is the super target: copy it under a shadow name
             and chain to it so emit_super reaches it via the prepend-super path.
             Otherwise the module method is simply shadowed -- nothing to emit. */
          if (!scope_body_has_super(c, own)) continue;
          const char *existing = comp_prep_chain_target(c, ci, src->name);
          snprintf(inc_shadow, sizeof inc_shadow, "__inc_%d_%s",
                   c->classes[ci].prep_shadow_count++, src->name);
          if (existing) {
            /* Another included module already supplies the super target for this
               method. A later include takes precedence (Ruby MRO: C -> Mlast ->
               ... -> Mfirst), so retarget the class's super to this module's copy
               and chain this copy to the previously included one:
               name -> new_shadow -> earlier_shadow. */
            char *prev = strdup(existing);  /* stable copy: the slot is freed below */
            ClassInfo *cif = &c->classes[ci];
            for (int kk = 0; kk < cif->nprep_chain; kk++)
              if (sp_streq(cif->prep_from[kk], src->name)) {
                free(cif->prep_to[kk]);
                cif->prep_to[kk] = strdup(inc_shadow);
                break;
              }
            comp_prep_chain_add(cif, inc_shadow, prev);
            free(prev);
          }
          else {
            comp_prep_chain_add(&c->classes[ci], src->name, inc_shadow);
          }
          dst_name = inc_shadow;
        }
        /* Create a new scope sharing the same AST nodes but owned by ci. */
        Scope *dst = comp_scope_new(c, dst_name, src->def_node);
        int dst_idx = c->nscopes - 1;
        /* comp_scope_new may realloc c->scopes; re-derive src pointer. */
        src = &c->scopes[ms];
        /* Clone the body and re-attribute it to the target when either:
           (a) the target is a built-in class, where `self` has a different
           (scalar) type than the module's object self -- otherwise the shared
           SelfNode resolves to the module and e.g. `self.to_s` mis-dispatches; or
           (b) the copied method itself calls `super` (a multi-module chain), so
           its super node resolves to this shadow scope (and thus the class's prep
           chain) rather than to the source module, where the chain isn't set. */
        if ((is_builtin_class_name(c->classes[ci].name) || scope_body_has_super(c, ms)) && src->body >= 0) {
          int nb = nt_clone_subtree((NodeTable *)nt, src->body);
          if (nb >= 0) {
            comp_grow_node_arrays(c);
            src = &c->scopes[ms]; dst = &c->scopes[dst_idx];
            dst->body = nb;
            walk_scope(c, nb, dst_idx, ci);
            g_inc_did_clone = 1;
          }
          else dst->body = src->body;
        }
else {
          dst->body = src->body;
        }
        dst->class_id = ci;
        dst->is_cmethod = 0;
        dst->reachable = src->reachable;
        dst->yields = src->yields;
        dst->nrequired = src->nrequired;
        dst->rest_idx = src->rest_idx;
        dst->kwrest_idx = src->kwrest_idx;
        if (src->blk_param) dst->blk_param = strdup(src->blk_param);
        src->is_transplanted_source = 1;
        /* Copy parameter names and defaults. */
        dst->nparams = src->nparams;
        if (src->nparams > 0) {
          dst->pnames = malloc(sizeof(char *) * (size_t)src->nparams);
          dst->pdefault = malloc(sizeof(int) * (size_t)src->nparams);
          for (int p = 0; p < src->nparams; p++) {
            dst->pnames[p] = src->pnames[p] ? strdup(src->pnames[p]) : NULL;
            dst->pdefault[p] = src->pdefault ? src->pdefault[p] : -1;
          }
          /* Register param locals so infer_param_types can update types. */
          for (int p = 0; p < src->nparams; p++) {
            if (dst->pnames[p]) {
              LocalVar *lv = scope_local_intern(dst, dst->pnames[p]);
              lv->is_param = 1;
            }
          }
        }
        /* Scan source body for ivar accesses and register them in the
           destination class so codegen's struct layout includes them. */
        for (int id2 = 0; id2 < nt->count; id2++) {
          if (c->nscope[id2] != ms) continue;
          const char *bty = nt_type(nt, id2);
          if (!bty) continue;
          if (sp_streq(bty, "InstanceVariableWriteNode") ||
              sp_streq(bty, "InstanceVariableReadNode") ||
              sp_streq(bty, "InstanceVariableOperatorWriteNode") ||
              sp_streq(bty, "InstanceVariableOrWriteNode")) {
            const char *ivnm = nt_str(nt, id2, "name");
            if (ivnm) comp_ivar_intern(&c->classes[ci], ivnm);
          }
        }
      }
    }
  }
}

/* For each class, find `include M` declarations in ALL class bodies
   (including reopenings) and transplant M's instance methods into the
   class so they are reachable via comp_method_in_chain. */
void register_includes(Compiler *c) {
  const NodeTable *nt = c->nt;
  g_inc_did_clone = 0;
  /* First pass: process def_node bodies (first class definition). */
  for (int ci = 0; ci < c->nclasses; ci++) {
    int body = nt_ref(nt, c->classes[ci].def_node, "body");
    process_include_body(c, ci, body);
  }
  /* Second pass: scan all ClassNode/ModuleNode in the AST for reopenings. */
  for (int id = 0; id < nt->count; id++) {
    const char *ty = nt_type(nt, id);
    if (!ty || (!sp_streq(ty, "ClassNode") && !sp_streq(ty, "ModuleNode"))) continue;
    int cp = nt_ref(nt, id, "constant_path");
    const char *cname = cp >= 0 ? nt_str(nt, cp, "name") : NULL;
    if (!cname) continue;
    int ci = comp_class_index(c, cname);
    if (ci < 0) continue;
    if (id == c->classes[ci].def_node) continue;  /* already processed above */
    int body = nt_ref(nt, id, "body");
    process_include_body(c, ci, body);
  }
  if (g_inc_did_clone) register_locals(c);
}

/* For each class, find `extend M` declarations and transplant M's instance
   methods as class methods (is_cmethod=1) so they are callable as C.m. */
void register_extends(Compiler *c) {
  const NodeTable *nt = c->nt;
  for (int ci = 0; ci < c->nclasses; ci++) {
    int body = nt_ref(nt, c->classes[ci].def_node, "body");
    int n = 0;
    const int *stmts = body >= 0 ? nt_arr(nt, body, "body", &n) : NULL;
    for (int k = 0; k < n; k++) {
      int s = stmts[k];
      const char *sty = nt_type(nt, s);
      if (!sty || !sp_streq(sty, "CallNode")) continue;
      const char *nm = nt_str(nt, s, "name");
      if (!nm || !sp_streq(nm, "extend")) continue;
      if (nt_ref(nt, s, "receiver") >= 0) continue;
      int anode = nt_ref(nt, s, "arguments");
      int an = 0;
      const int *args = anode >= 0 ? nt_arr(nt, anode, "arguments", &an) : NULL;
      for (int j = 0; j < an; j++) {
        const char *aty = nt_type(nt, args[j]);
        const char *mname = NULL;
        if (aty && sp_streq(aty, "ConstantReadNode")) mname = nt_str(nt, args[j], "name");
        else if (aty && sp_streq(aty, "ConstantPathNode")) mname = nt_str(nt, args[j], "name");
        int mod_id = mname ? comp_class_index(c, mname) : -1;
        if (mod_id < 0) continue;
        int snap = c->nscopes;
        for (int ms = 0; ms < snap; ms++) {
          Scope *src = &c->scopes[ms];
          /* Only transplant instance methods; self.* on the module stay on it. */
          if (src->class_id != mod_id || src->is_cmethod || !src->name) continue;
          if (comp_cmethod_in_class(c, ci, src->name) >= 0) continue;
          Scope *dst = comp_scope_new(c, src->name, src->def_node);
          src = &c->scopes[ms];
          dst->body = src->body;
          dst->class_id = ci;
          dst->is_cmethod = 1;  /* transplanted as a class method */
          dst->reachable = src->reachable;
          dst->yields = src->yields;
          dst->nrequired = src->nrequired;
          dst->rest_idx = src->rest_idx;
          dst->kwrest_idx = src->kwrest_idx;
          if (src->blk_param) dst->blk_param = strdup(src->blk_param);
          dst->nparams = src->nparams;
          if (src->nparams > 0) {
            dst->pnames = malloc(sizeof(char *) * (size_t)src->nparams);
            dst->pdefault = malloc(sizeof(int) * (size_t)src->nparams);
            for (int p = 0; p < src->nparams; p++) {
              dst->pnames[p] = src->pnames[p] ? strdup(src->pnames[p]) : NULL;
              dst->pdefault[p] = src->pdefault ? src->pdefault[p] : -1;
            }
            for (int p = 0; p < src->nparams; p++) {
              if (dst->pnames[p]) {
                LocalVar *lv = scope_local_intern(dst, dst->pnames[p]);
                lv->is_param = 1;
              }
            }
          }
          src->is_transplanted_source = 1;
        }
      }
    }
  }
}

/* True if class method scope `mi`'s body contains a bare `new` call (which
   must rebind to the calling subclass, not the defining class). */
int cmethod_has_bare_new(Compiler *c, int mi) {
  const NodeTable *nt = c->nt;
  for (int id = 0; id < nt->count; id++) {
    if (c->nscope[id] != mi) continue;
    const char *ty = nt_type(nt, id);
    if (ty && sp_streq(ty, "CallNode") && nt_ref(nt, id, "receiver") < 0 &&
        nt_str(nt, id, "name") && sp_streq(nt_str(nt, id, "name"), "new"))
      return 1;
  }
  return 0;
}

/* Does the inherited cls method `mi` (defined on def_cls), run as a class method
   of `ci`, reach a bare cmethod call that resolves to a DIFFERENT method for ci
   -- directly, or TRANSITIVELY through a non-overriding intermediate cmethod?
   E.g. `last_row` calls `all_rows` (which ci does not override) calls
   `table_name` (which ci does): `last_row` reaches an override and so must be
   specialized for ci too, or its implicit-self chain stays bound to the base and
   the override is skipped (#1451). The depth cap bounds the walk and doubles as a
   cycle guard for mutually-recursive cmethods. */
static int cmethod_reaches_override(Compiler *c, int mi, int ci, int def_cls, int depth) {
  if (depth > 64) return 0;
  const NodeTable *nt = c->nt;
  NT_FOREACH_KIND(nt, NK_CallNode, id) {
    if (c->nscope[id] != mi) continue;
    if (nt_ref(nt, id, "receiver") >= 0) continue;   /* receiverless only */
    const char *nm = nt_str(nt, id, "name");
    if (!nm || sp_streq(nm, "new")) continue;          /* direct `new` is the caller's has_new path */
    int sub_def = -1;
    int mci = comp_cmethod_in_chain(c, ci, nm, NULL);
    int mdef = comp_cmethod_in_chain(c, def_cls, nm, &sub_def);
    if (mci >= 0 && mci != mdef) return 1;            /* ci overrides nm directly */
    /* nm is inherited unchanged by ci -- but its own body may still reach an
       override; descend into the def-chain version. */
    if (mdef >= 0 && sub_def >= 0 &&
        cmethod_reaches_override(c, mdef, ci, sub_def, depth + 1)) return 1;
  }
  return 0;
}

/* Does the inherited cls method `mi` (defined on def_cls) contain a bare call
   that would resolve differently when run as a class method of `ci`? That is:
   a bare `new` (constructs ci), or a bare cmethod call that resolves to ci's
   own version directly or transitively (cmethod_reaches_override). */
int cmethod_needs_specialization(Compiler *c, int mi, int ci, int def_cls, int *has_new) {
  const NodeTable *nt = c->nt;
  int need = 0;
  if (has_new) *has_new = 0;
  NT_FOREACH_KIND(nt, NK_CallNode, id) {
    if (c->nscope[id] != mi) continue;
    if (nt_ref(nt, id, "receiver") >= 0) continue;   /* receiverless only */
    const char *nm = nt_str(nt, id, "name");
    if (!nm) continue;
    if (sp_streq(nm, "new")) { if (has_new) *has_new = 1; need = 1; }
  }
  if (cmethod_reaches_override(c, mi, ci, def_cls, 0)) need = 1;
  return need;
}

/* Clone inherited cls method `mi` (defined on def_cls) as a ci-owned copy whose
   body is re-attributed to ci, so its bare `new` constructs ci and its
   implicit-self cmethod calls resolve in ci's chain. Then recurse: any bare
   cmethod call in mi's body that ci inherits unchanged but that itself needs
   specialization (reaches an override, or does `new`) is cloned for ci too, so
   the cloned body's implicit-self call rebinds to ci's copy instead of staying
   on the base -- that transitive rebind is the #1451 fix. The ci-already-owns
   guard makes this idempotent and terminates mutually-recursive cmethods. */
static void specialize_cmethod_for(Compiler *c, int mi, int def_cls, int ci) {
  if (comp_cmethod_in_class(c, ci, c->scopes[mi].name) >= 0) return;
  NodeTable *nt = (NodeTable *)c->nt;
  int has_new = 0;
  (void)cmethod_needs_specialization(c, mi, ci, def_cls, &has_new);
  int src_body = c->scopes[mi].body;
  int new_body = src_body >= 0 ? nt_clone_subtree(nt, src_body) : -1;
  if (src_body >= 0 && new_body < 0) return;  /* clone failed: skip */
  comp_grow_node_arrays(c);
  Scope *src = &c->scopes[mi];
  Scope *dst = comp_scope_new(c, src->name, src->def_node);
  src = &c->scopes[mi];  /* realloc-safe */
  int dst_idx = c->nscopes - 1;
  dst->body = new_body;
  if (new_body >= 0) walk_scope(c, new_body, dst_idx, ci);
  dst->class_id = ci;
  dst->is_cmethod = 1;
  dst->yields = src->yields;
  dst->nrequired = src->nrequired;
  dst->rest_idx = src->rest_idx;
  dst->kwrest_idx = src->kwrest_idx;
  /* A bare-`new` create method returns the specialized subclass instance, so
     pin its return type. Other specializations let normal return inference
     compute the type from the cloned, ci-attributed body. */
  if (has_new) {
    dst->ret = ty_object(ci);
    dst->ret_specialized = 1;
  }
  if (src->blk_param) dst->blk_param = strdup(src->blk_param);
  dst->nparams = src->nparams;
  if (src->nparams > 0) {
    dst->pnames = malloc(sizeof(char *) * (size_t)src->nparams);
    dst->pdefault = malloc(sizeof(int) * (size_t)src->nparams);
    for (int p = 0; p < src->nparams; p++) {
      dst->pnames[p] = src->pnames[p] ? strdup(src->pnames[p]) : NULL;
      dst->pdefault[p] = src->pdefault ? src->pdefault[p] : -1;
      if (dst->pnames[p]) { LocalVar *lv = scope_local_intern(dst, dst->pnames[p]); lv->is_param = 1; }
    }
  }
  /* Recurse into the inherited intermediates this body reaches. Scan the
     ORIGINAL mi body (cloned nodes are attributed to dst, not mi); a sub-clone
     reallocs c->scopes/c->nscope, so use indices and refetch. */
  int scan_n = nt->count;
  for (int id = 0; id < scan_n; id++) {
    if (c->nscope[id] != mi) continue;
    const char *ty = nt_type(nt, id);
    if (!ty || !sp_streq(ty, "CallNode")) continue;
    if (nt_ref(nt, id, "receiver") >= 0) continue;
    const char *nm = nt_str(nt, id, "name");
    if (!nm || sp_streq(nm, "new")) continue;
    int sub_def = -1;
    int sub_mi = comp_cmethod_in_chain(c, ci, nm, &sub_def);
    if (sub_mi < 0 || sub_def == ci) continue;   /* unresolved, or ci-native */
    int sub_new = 0;
    if (cmethod_needs_specialization(c, sub_mi, ci, sub_def, &sub_new))
      specialize_cmethod_for(c, sub_mi, sub_def, ci);
  }
}

/* `Subclass.create` where `create` is an inherited class method whose body
   does `new(...)`: Ruby's bare `new` constructs the *calling* class, so copy
   the inherited cls method into each calling subclass (the copy's class_id
   makes codegen's `new` resolve to that subclass). The defining-class source
   is DCE'd unless it is itself called directly. Covers #224 / #229 / #1451. */
void specialize_inherited_cls_new(Compiler *c) {
  NodeTable *nt = (NodeTable *)c->nt;
  int snap = c->nscopes;
  int node_count = nt->count;   /* don't scan nodes appended by cloning */
  int did_clone = 0;
  for (int id = 0; id < node_count; id++) {
    const char *ty = nt_type(nt, id);
    if (!ty || !sp_streq(ty, "CallNode")) continue;
    int recv = nt_ref(nt, id, "receiver");
    if (recv < 0) continue;
    const char *rty = nt_type(nt, recv);
    if (!rty || (!sp_streq(rty, "ConstantReadNode") && !sp_streq(rty, "ConstantPathNode"))) continue;
    const char *cn = nt_str(nt, recv, "name");
    int ci = cn ? comp_class_index(c, cn) : -1;
    if (ci < 0) continue;
    const char *mname = nt_str(nt, id, "name");
    if (!mname || sp_streq(mname, "new")) continue;
    if (comp_cmethod_in_class(c, ci, mname) >= 0) continue;  /* defined on ci */
    int def_cls = -1;
    int mi = comp_cmethod_in_chain(c, ci, mname, &def_cls);
    if (mi < 0 || def_cls == ci || mi >= snap) continue;     /* not inherited */
    int has_new = 0;
    if (!cmethod_needs_specialization(c, mi, ci, def_cls, &has_new)) continue;
    /* Clone mi for ci and, transitively, the inherited intermediates it reaches
       (#1451). nscopes growth below stands in for the old did_clone flag. */
    specialize_cmethod_for(c, mi, def_cls, ci);
  }
  did_clone = (c->nscopes > snap);
  /* Index of every CallNode with a constant receiver, built once: the
     called-direct check below otherwise rescans all nodes per shadowed cmethod
     (O(cmethods * nodes)). */
  int *ccall = malloc((size_t)node_count * sizeof(int));
  int nccall = 0;
  if (ccall) {
    for (int id = 0; id < node_count; id++) {
      if (!nt_type(nt, id) || !sp_streq(nt_type(nt, id), "CallNode")) continue;
      int r = nt_ref(nt, id, "receiver");
      const char *rty = r >= 0 ? nt_type(nt, r) : NULL;
      if (rty && (sp_streq(rty, "ConstantReadNode") || sp_streq(rty, "ConstantPathNode")))
        ccall[nccall++] = id;
    }
  }
  /* DCE the now-shadowed source cls methods that are never called on their
     own defining class. */
  for (int s = 0; s < snap; s++) {
    Scope *src = &c->scopes[s];
    if (!src->is_cmethod || !src->name || src->class_id < 0) continue;
    /* did we specialize this one into a subclass? (a fresh cmethod copy with
       the same name was appended in a descendant class). Match on the class
       hierarchy, not just the name: an unrelated class's cmethod that merely
       shares the name must not be treated as a transplanted source (and DCE'd). */
    int specialized = 0;
    for (int d = snap; d < c->nscopes; d++)
      if (c->scopes[d].is_cmethod && c->scopes[d].name && c->scopes[d].class_id >= 0 &&
          sp_streq(c->scopes[d].name, src->name) &&
          is_descendant(c, c->scopes[d].class_id, src->class_id)) { specialized = 1; break; }
    if (!specialized) continue;
    /* keep it if called directly as <DefiningClass>.<name> */
    int called_direct = 0;
    for (int ii = 0; ii < nccall && !called_direct; ii++) {
      int id = ccall[ii];
      if (!nt_str(nt, id, "name") || !sp_streq(nt_str(nt, id, "name"), src->name)) continue;
      int r = nt_ref(nt, id, "receiver");
      if (comp_class_index(c, nt_str(nt, r, "name")) == src->class_id) called_direct = 1;
    }
    if (!called_direct) src->is_transplanted_source = 1;
  }
  free(ccall);
  /* The cloned bodies introduced new local/ivar nodes; intern them. */
  if (did_clone) register_locals(c);
}

/* For each class, find `prepend M` declarations and transplant M's instance
   methods into the class with shadow-chain renaming so `super` can route
   from M's body to the original (now renamed) class body. */
void register_prepends(Compiler *c) {
  const NodeTable *nt = c->nt;
  for (int ci = 0; ci < c->nclasses; ci++) {
    int body = nt_ref(nt, c->classes[ci].def_node, "body");
    int n = 0;
    const int *stmts = body >= 0 ? nt_arr(nt, body, "body", &n) : NULL;
    for (int k = 0; k < n; k++) {
      int s = stmts[k];
      const char *sty = nt_type(nt, s);
      if (!sty || !sp_streq(sty, "CallNode")) continue;
      const char *nm = nt_str(nt, s, "name");
      if (!nm || !sp_streq(nm, "prepend")) continue;
      if (nt_ref(nt, s, "receiver") >= 0) continue;
      int anode = nt_ref(nt, s, "arguments");
      int an = 0;
      const int *args = anode >= 0 ? nt_arr(nt, anode, "arguments", &an) : NULL;
      for (int j = 0; j < an; j++) {
        const char *aty = nt_type(nt, args[j]);
        const char *mname = NULL;
        if (aty && sp_streq(aty, "ConstantReadNode")) mname = nt_str(nt, args[j], "name");
        else if (aty && sp_streq(aty, "ConstantPathNode")) mname = nt_str(nt, args[j], "name");
        int mod_id = mname ? comp_class_index(c, mname) : -1;
        if (mod_id < 0) continue;
        /* Transplant each instance method of the module into class ci. */
        for (int ms = 0; ms < c->nscopes; ms++) {
          Scope *sc = &c->scopes[ms];
          if (sc->class_id != mod_id || sc->is_cmethod || !sc->name) continue;
          const char *method_name = sc->name;
          int active_mi = comp_method_in_class(c, ci, method_name);
          if (active_mi >= 0) {
            Scope *active = &c->scopes[active_mi];
            char shadow[256];
            snprintf(shadow, sizeof shadow, "__prep_%d_%s",
                     c->classes[ci].prep_shadow_count++, method_name);
            /* Rename any existing chain entry for method_name to use shadow. */
            ClassInfo *cif = &c->classes[ci];
            for (int kk = 0; kk < cif->nprep_chain; kk++) {
              if (sp_streq(cif->prep_from[kk], method_name)) {
                free(cif->prep_from[kk]);
                cif->prep_from[kk] = strdup(shadow);
                break;
              }
            }
            /* Rename the currently active scope to the shadow name. */
            free(active->name);
            active->name = strdup(shadow);
            /* Record the new dispatch chain entry: method_name -> shadow. */
            comp_prep_chain_add(&c->classes[ci], method_name, shadow);
          }
          /* Transplant the module scope into class ci. */
          sc->class_id = ci;
        }
      }
    }
  }
}

/* Merge inherited ivar/reader/writer NAMES into subclasses so the struct
   layout is [parent ivars..., own ivars...] (cast-compatible). Types are
   propagated later in the fixpoint. Parent-first order. */
void inherit_members(Compiler *c) {
  for (int i = 0; i < c->nclasses; i++) {
    ClassInfo *ci = &c->classes[i];
    int p = ci->parent;
    if (p < 0 || p >= i) continue;  /* parent defined earlier; already merged */
    ClassInfo *pc = &c->classes[p];

    char **old = ci->ivars; TyKind *oldt = ci->ivar_types; int oldn = ci->nivars;
    ci->ivars = NULL; ci->ivar_types = NULL; ci->nivars = ci->civars = 0;
    for (int k = 0; k < pc->nivars; k++) {
      int idx = comp_ivar_intern(ci, pc->ivars[k]);
      ci->ivar_types[idx] = pc->ivar_types[k];
    }
    for (int k = 0; k < oldn; k++) {
      int idx = comp_ivar_intern(ci, old[k]);
      ci->ivar_types[idx] = ty_unify(ci->ivar_types[idx], oldt[k]);
      free(old[k]);
    }
    free(old); free(oldt);

    for (int k = 0; k < pc->nreaders; k++) comp_add_reader(ci, pc->readers[k]);
    for (int k = 0; k < pc->nwriters; k++) comp_add_writer(ci, pc->writers[k]);
  }
}

/* Propagate inherited @ivar types parent -> child. */
int infer_inherited_ivars(Compiler *c) {
  int changed = 0;
  for (int i = 0; i < c->nclasses; i++) {
    ClassInfo *ci = &c->classes[i];
    if (ci->parent < 0) continue;
    ClassInfo *pc = &c->classes[ci->parent];
    for (int k = 0; k < pc->nivars; k++) {
      int idx = comp_ivar_index(ci, pc->ivars[k]);
      if (idx < 0) continue;
      TyKind merged = ty_unify(ci->ivar_types[idx], pc->ivar_types[k]);
      if (merged != ci->ivar_types[idx]) { ci->ivar_types[idx] = merged; changed = 1; }
    }
  }
  return changed;
}

/* @ivar types from their assignments across the class's methods. */
/* Register each class variable (@@x) in its owning class and infer its type
   from the write sites' RHS. */
int infer_cvar_types(Compiler *c) {
  const NodeTable *nt = c->nt;
  int changed = 0;
  /* Pass 1: class body-level writes (comp_scope_of returns scope 0, class_id=-1,
     so use the class's def_node to find which class owns them). */
  for (int ci = 0; ci < c->nclasses; ci++) {
    int body = nt_ref(nt, c->classes[ci].def_node, "body");
    int n = 0;
    const int *stmts = body >= 0 ? nt_arr(nt, body, "body", &n) : NULL;
    for (int k = 0; k < n; k++) {
      int s = stmts[k];
      const char *sty = nt_type(nt, s);
      if (!sty) continue;
      if (sp_streq(sty, "ClassVariableWriteNode")) {
        const char *nm = nt_str(nt, s, "name");
        if (!nm) continue;
        int idx = comp_cvar_intern(&c->classes[ci], nm);
        TyKind vt = infer_type(c, nt_ref(nt, s, "value"));
        if (vt == TY_NIL) continue;
        TyKind merged = ty_unify(c->classes[ci].cvar_types[idx], vt);
        if (merged != c->classes[ci].cvar_types[idx]) { c->classes[ci].cvar_types[idx] = merged; changed = 1; }
      }
      else if (sp_streq(sty, "MultiWriteNode")) {
        int mln = 0;
        const int *mlefts = nt_arr(nt, s, "lefts", &mln);
        int mval = nt_ref(nt, s, "value");
        const char *mvty = nt_type(nt, mval);
        int men = 0;
        const int *mels = (mvty && sp_streq(mvty, "ArrayNode")) ? nt_arr(nt, mval, "elements", &men) : NULL;
        for (int mi = 0; mi < mln; mi++) {
          const char *mlty = nt_type(nt, mlefts[mi]);
          if (!mlty || !sp_streq(mlty, "ClassVariableTargetNode")) continue;
          const char *cnm = nt_str(nt, mlefts[mi], "name");
          if (!cnm) continue;
          int midx = comp_cvar_intern(&c->classes[ci], cnm);
          TyKind mvt2 = (mels && mi < men) ? infer_type(c, mels[mi]) : TY_UNKNOWN;
          if (mvt2 == TY_NIL || mvt2 == TY_UNKNOWN) continue;
          TyKind mmerged = ty_unify(c->classes[ci].cvar_types[midx], mvt2);
          if (mmerged != c->classes[ci].cvar_types[midx]) { c->classes[ci].cvar_types[midx] = mmerged; changed = 1; }
        }
      }
    }
  }
  /* Pass 2: method-level writes (comp_scope_of has class_id set). */
  NT_FOREACH_KIND(nt, NK_ClassVariableWriteNode, id) {
    const char *nm = nt_str(nt, id, "name");
    Scope *s = comp_scope_of(c, id);
    if (!nm || s->class_id < 0) continue;
    ClassInfo *ci = &c->classes[s->class_id];
    int idx = comp_cvar_intern(ci, nm);
    TyKind vt = infer_type(c, nt_ref(nt, id, "value"));
    if (vt == TY_NIL) continue;
    TyKind merged = ty_unify(ci->cvar_types[idx], vt);
    if (merged != ci->cvar_types[idx]) { ci->cvar_types[idx] = merged; changed = 1; }
  }
  /* Pass 3: top-level writes (class_id == -1 in scope 0) -- use Toplevel pseudo-class. */
  NT_FOREACH_KIND(nt, NK_ClassVariableWriteNode, id) {
    const char *nm = nt_str(nt, id, "name");
    Scope *s = comp_scope_of(c, id);
    if (!nm || s->class_id >= 0) continue;
    int tl_idx = comp_class_index(c, "Toplevel");
    if (tl_idx < 0) { comp_class_new(c, "Toplevel", -1); tl_idx = c->nclasses - 1; }
    ClassInfo *ci = &c->classes[tl_idx];
    int idx = comp_cvar_intern(ci, nm);
    TyKind vt = infer_type(c, nt_ref(nt, id, "value"));
    if (vt == TY_NIL) continue;
    TyKind merged = ty_unify(ci->cvar_types[idx], vt);
    if (merged != ci->cvar_types[idx]) { ci->cvar_types[idx] = merged; changed = 1; }
  }
  return changed;
}

/* def_node -> scopes sharing it (transplanted module copies), cached per scope
   count. infer_ivar_types propagates each ivar write to copies of its method in
   other classes; without this it rescanned every scope per ivar write
   (O(ivar_writes * scopes)). Built once per fixpoint run (scope shape is fixed
   there); dn_head is indexed by node id, dn_next chains scopes. */
static int dn_nscopes = -1, dn_count = -1;
static int *dn_head = NULL, *dn_next = NULL;
static void dn_build(Compiler *c) {
  int nc = c->nt->count, ns = c->nscopes;
  free(dn_head); free(dn_next);
  dn_head = malloc((size_t)(nc > 0 ? nc : 1) * sizeof(int));
  dn_next = malloc((size_t)(ns > 0 ? ns : 1) * sizeof(int));
  dn_count = nc; dn_nscopes = ns;
  if (!dn_head || !dn_next) { dn_nscopes = -1; return; }
  for (int i = 0; i < nc; i++) dn_head[i] = -1;
  for (int s = 0; s < ns; s++) {
    int d = c->scopes[s].def_node;
    if (d >= 0 && d < nc) { dn_next[s] = dn_head[d]; dn_head[d] = s; }
    else dn_next[s] = -1;
  }
}

/* `@iv = cond ? nil : <int>` (a literal-nil ternary arm) pins the ivar as a
   nullable int -- the SP_INT_NIL sentinel in an unboxed int slot, the same
   representation a direct `@iv = nil` / `@iv = <int>` pair already yields
   (a bare `@iv = nil` is skipped below, leaving the int writes) -- rather than
   widening to poly. Scoped to the ivar write so the nullable value never
   escapes as a bare ternary expression, where a non-ivar consumer would not be
   sentinel-aware. Returns TY_INT for that shape, else TY_UNKNOWN. */
static TyKind ivar_nullable_int_ternary(Compiler *c, int vnode) {
  int tn, en;
  if (!comp_ternary_arms(c->nt, vnode, &tn, &en)) return TY_UNKNOWN;
  const char *tt = nt_type(c->nt, tn), *et = nt_type(c->nt, en);
  int t_nil = tt && sp_streq(tt, "NilNode");
  int e_nil = et && sp_streq(et, "NilNode");
  if (t_nil == e_nil) return TY_UNKNOWN;  /* exactly one arm a literal nil */
  return infer_type(c, t_nil ? en : tn) == TY_INT ? TY_INT : TY_UNKNOWN;
}

int infer_ivar_types(Compiler *c) {
  const NodeTable *nt = c->nt;
  int changed = 0;
  if (dn_nscopes != c->nscopes || dn_count != nt->count) dn_build(c);
  for (int id = 0; id < nt->count; id++) {
    const char *ty = nt_type(nt, id);
    if (!ty) continue;
    if (sp_streq(ty, "InstanceVariableWriteNode") ||
        sp_streq(ty, "InstanceVariableOrWriteNode") ||
        sp_streq(ty, "InstanceVariableAndWriteNode") ||
        sp_streq(ty, "InstanceVariableOperatorWriteNode")) {
      const char *nm = nt_str(nt, id, "name");
      int vnode = nt_ref(nt, id, "value");
      TyKind vt = infer_type(c, vnode);
      if (vt == TY_NIL) continue;  /* nil write doesn't pin the ivar type */
      /* `@a = @b = nil`: the chain writes nil to every target; don't let the
         inner slot's unified type (from its other writes) pin this ivar. */
      if (comp_nil_chain_bottom(nt, vnode) >= 0) continue;
      if (vt == TY_POLY && ivar_nullable_int_ternary(c, vnode) == TY_INT) vt = TY_INT;
      Scope *s = comp_scope_of(c, id);
      int cls_id2 = s->class_id;
      if (!nm) continue;
      /* A `@ivar = v` directly in a class/module body (not in a method) belongs
         to that class/module object, like its class methods see it -- attribute
         it to the enclosing class-body rather than the Toplevel pseudo-class. */
      if (cls_id2 < 0 && c->node_cbody[id] >= 0) cls_id2 = c->node_cbody[id];
      if (cls_id2 < 0) {
        /* Top-level method: track ivars in the Toplevel pseudo-class */
        int old_nc = c->nclasses;
        cls_id2 = comp_class_index(c, "Toplevel");
        if (cls_id2 < 0) { comp_class_new(c, "Toplevel", -1); cls_id2 = c->nclasses - 1; }
        if (c->nclasses != old_nc) changed = 1;  /* new class created, need another pass */
      }
      ClassInfo *ci = &c->classes[cls_id2];
      int old_ni = ci->nivars;
      int iv = comp_ivar_intern(ci, nm);
      if (ci->nivars != old_ni) changed = 1;  /* new ivar registered, need another pass */
      /* For operator-write (@b += rhs), vt is the RHS type, not the result type.
         When the slot holds a user object, the result is the method's return type. */
      if (sp_streq(ty, "InstanceVariableOperatorWriteNode") && ty_is_object(ci->ivar_types[iv])) {
        const char *op2 = nt_str(nt, id, "binary_operator");
        int cid2 = ty_object_class(ci->ivar_types[iv]);
        int mi2 = op2 ? comp_method_in_chain(c, cid2, op2, NULL) : -1;
        if (mi2 >= 0 && c->scopes[mi2].ret != TY_UNKNOWN)
          vt = c->scopes[mi2].ret;
        else
          vt = ci->ivar_types[iv];  /* keep existing type, don't widen */
      }
      if (!class_ivar_pinned(ci, nm)) {
        TyKind merged = ty_unify(ci->ivar_types[iv], vt);
        sp_ivwatch(nm, "ivar_write_merge", ci->ivar_types[iv], merged);
        if (merged != ci->ivar_types[iv]) { ci->ivar_types[iv] = merged; changed = 1; }
      }
      /* Propagate to transplanted copies (module included into a class).
         Body nodes still point to the module scope, so cls_id2 is the module.
         Any scope sharing the same def_node but with a different class_id is
         a transplanted copy that must see the same ivar type. */
      if (s->class_id >= 0 && s->def_node >= 0) {
        int sdef = s->def_node;
        int orig_cid = s->class_id;
        int use_idx = dn_head && dn_nscopes == c->nscopes && sdef < dn_count;
        int si = use_idx ? dn_head[sdef] : 0;
        for (; use_idx ? (si >= 0) : (si < c->nscopes); si = use_idx ? dn_next[si] : si + 1) {
          Scope *ts = &c->scopes[si];
          if (ts->def_node != sdef || ts->class_id == orig_cid || ts->class_id < 0) continue;
          ClassInfo *tc = &c->classes[ts->class_id];
          if (class_ivar_pinned(tc, nm)) continue;
          int tiv = comp_ivar_intern(tc, nm);
          TyKind tmerged = ty_unify(tc->ivar_types[tiv], vt);
          sp_ivwatch(nm, "transplant_merge", tc->ivar_types[tiv], tmerged);
          if (tmerged != tc->ivar_types[tiv]) { tc->ivar_types[tiv] = tmerged; changed = 1; }
        }
      }
    }
    else if (sp_streq(ty, "CallNode")) {
      /* attr-writer assignment: obj.x = v  (CallNode "x=") */
      const char *nm = nt_str(nt, id, "name");
      int recv = nt_ref(nt, id, "receiver");
      size_t ln = nm ? strlen(nm) : 0;
      if (!nm || recv < 0 || ln < 2 || nm[ln - 1] != '=') continue;
      /* not a comparison that happens to end in '=' (==, !=, <=, >=) */
      if (nm[ln - 2] == '=' || nm[ln - 2] == '!' || nm[ln - 2] == '<' || nm[ln - 2] == '>') continue;
      char base[256];
      if (ln - 1 >= sizeof base) continue;
      memcpy(base, nm, ln - 1); base[ln - 1] = '\0';
      int args = nt_ref(nt, id, "arguments");
      int an = 0;
      const int *argv = args >= 0 ? nt_arr(nt, args, "arguments", &an) : NULL;
      if (an < 1) continue;
      TyKind vt = infer_type(c, argv[0]);
      if (vt == TY_NIL) continue;  /* a nil write doesn't pin the ivar type */
      char ivname[256];
      snprintf(ivname, sizeof ivname, "@%s", base);
      TyKind rt = infer_type(c, recv);
      if (ty_is_object(rt)) {
        /* concrete receiver: attribute to its class. */
        ClassInfo *ci = &c->classes[ty_object_class(rt)];
        if (!comp_is_writer(ci, base)) continue;
        int iv = comp_ivar_index(ci, ivname);
        if (iv < 0 || class_ivar_pinned(ci, ivname)) continue;
        TyKind merged = ty_unify(ci->ivar_types[iv], vt);
        if (merged != ci->ivar_types[iv]) { ci->ivar_types[iv] = merged; changed = 1; }
      }
      else {
        /* Poly/unknown receiver -- e.g. `cell` read from a poly array/hash
           (`@cells.each_value { |cell| cell.neighbours = ... }`). The static
           class is unknown, but if exactly ONE class defines this attr-writer
           with a matching ivar, the runtime object must be of that class, so
           attribute the write to it. (Skip when ambiguous: zero or several
           classes share the attr name -- over-widening an unrelated same-named
           ivar would be unsound to attribute.) ty_unify only widens. */
        int only = -1;
        for (int ci2 = 0; ci2 < c->nclasses; ci2++) {
          if (comp_is_writer(&c->classes[ci2], base) &&
              comp_ivar_index(&c->classes[ci2], ivname) >= 0) {
            if (only >= 0) { only = -2; break; }   /* ambiguous */
            only = ci2;
          }
        }
        if (only < 0) continue;
        ClassInfo *ci = &c->classes[only];
        int iv = comp_ivar_index(ci, ivname);
        if (iv < 0 || class_ivar_pinned(ci, ivname)) continue;
        TyKind merged = ty_unify(ci->ivar_types[iv], vt);
        if (merged != ci->ivar_types[iv]) { ci->ivar_types[iv] = merged; changed = 1; }
      }
    }
  }
  return changed;
}

/* ---- fixpoint passes ---- */

