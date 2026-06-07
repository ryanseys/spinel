#include "compiler.h"

#include <stdlib.h>
#include <string.h>

Compiler *comp_new(const NodeTable *nt) {
  Compiler *c = calloc(1, sizeof(Compiler));
  if (!c) return NULL;
  c->nt = nt;
  int n = nt->count > 0 ? nt->count : 1;
  c->ntype = calloc((size_t)n, sizeof(TyKind));
  c->nscope = calloc((size_t)n, sizeof(int));   /* default scope 0 */
  return c;
}

void comp_free(Compiler *c) {
  if (!c) return;
  for (int s = 0; s < c->nscopes; s++) {
    Scope *sc = &c->scopes[s];
    free(sc->name);
    for (int i = 0; i < sc->nlocals; i++) free(sc->locals[i].name);
    free(sc->locals);
    for (int i = 0; i < sc->nparams; i++) free(sc->pnames[i]);
    free(sc->pnames);
  }
  free(c->scopes);
  free(c->nscope);
  free(c->ntype);
  free(c);
}

Scope *comp_scope_new(Compiler *c, const char *name, int def_node) {
  if (c->nscopes >= c->cscopes) {
    c->cscopes = c->cscopes ? c->cscopes * 2 : 8;
    c->scopes = realloc(c->scopes, sizeof(Scope) * (size_t)c->cscopes);
  }
  Scope *s = &c->scopes[c->nscopes++];
  memset(s, 0, sizeof(*s));
  s->name = name ? strdup(name) : NULL;
  s->def_node = def_node;
  s->body = -1;
  s->ret = TY_UNKNOWN;
  return s;
}

Scope *comp_scope_of(Compiler *c, int node_id) {
  if (node_id < 0 || node_id >= c->nt->count) return &c->scopes[0];
  int idx = c->nscope[node_id];
  if (idx < 0 || idx >= c->nscopes) idx = 0;
  return &c->scopes[idx];
}

int comp_method_index(Compiler *c, const char *name) {
  if (!name) return -1;
  for (int s = 0; s < c->nscopes; s++)
    if (c->scopes[s].name && strcmp(c->scopes[s].name, name) == 0) return s;
  return -1;
}

LocalVar *scope_local(Scope *s, const char *name) {
  for (int i = 0; i < s->nlocals; i++)
    if (strcmp(s->locals[i].name, name) == 0) return &s->locals[i];
  return NULL;
}

LocalVar *scope_local_intern(Scope *s, const char *name) {
  LocalVar *lv = scope_local(s, name);
  if (lv) return lv;
  if (s->nlocals >= s->clocals) {
    s->clocals = s->clocals ? s->clocals * 2 : 8;
    s->locals = realloc(s->locals, sizeof(LocalVar) * (size_t)s->clocals);
  }
  lv = &s->locals[s->nlocals++];
  lv->name = strdup(name);
  lv->type = TY_UNKNOWN;
  lv->gc_root = 0;
  lv->is_param = 0;
  return lv;
}
