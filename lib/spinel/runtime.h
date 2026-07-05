/* spinel/runtime.h -- the stable ABI surface for carried package C (Path B).
 *
 * A spin package that carries C (compiled and linked on demand, see the
 * native_* binding DSL) includes THIS header, not the compiler's internal
 * lib/sp_*.h. Only the symbols documented here are a stable contract; anything
 * reachable through the umbrella includes but not listed below is an internal
 * of the current runtime and may change without notice.
 *
 * Frozen surface (grown per Path B target; this is the json set):
 *
 *   Values / tags:
 *     sp_RbVal            boxed value       (sp_gc.h)
 *     SP_TAG_INT/FLT/BOOL/NIL/STR/SYM/OBJ   value tags
 *
 *   Container reflection hooks (installed by the generated program at startup;
 *   a package reads a program's typed containers only through these):
 *     sp_json_kind_fn, sp_json_len_fn, sp_json_aref_fn, sp_json_hpair_fn
 *     sp_sym_name_fn
 *
 *   String heap + scalar formatting (sp_alloc.h):
 *     sp_str_alloc, sp_str_alloc_raw, sp_str_set_len, sp_str_empty
 *     sp_int_to_s, sp_float_to_s
 *     sp_oom_die
 *
 *   Errors:
 *     sp_raise_cls   (raise a named exception class)
 */
#ifndef SPINEL_RUNTIME_H
#define SPINEL_RUNTIME_H

#include "sp_gc.h"      /* sp_RbVal, SP_TAG_*, container + sym reflection hooks */
#include "sp_alloc.h"   /* sp_str_alloc, sp_str_set_len, sp_int_to_s, sp_float_to_s, sp_oom_die */

#endif /* SPINEL_RUNTIME_H */
