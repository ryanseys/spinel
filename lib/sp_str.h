#ifndef SP_STR_H
#define SP_STR_H
/* sp_str.h -- cold String transforms compiled once in lib/sp_str.c.
 *
 * These are leaf `const char*` operations (case, strip, split-family,
 * partition, dump/undump, concat, repeat, ...) that depend only on the
 * shared string heap (sp_alloc.h) and the typed arrays (sp_array.h). The
 * hot string core -- the FNV hash cascade (#282), the UTF-8 length cache,
 * and the inline utf8 decode/advance/encode helpers -- stays inline in
 * sp_runtime.h; relocating those would change optcarrot's codegen. As more
 * of that core gets shared, the utf8-dependent transforms can follow.
 *
 * sp_sprintf / sp_raise_cls are provided by the generated TU and resolved
 * at the final link (same as lib/sp_core.c). */
#include "sp_array.h"   /* sp_StrArray, sp_IntArray + sp_alloc.h / sp_gc.h / sp_types.h */

const char *sp_sprintf(const char *fmt, ...);  /* defined in the generated TU */

int sp_utf8_set_has(const uint32_t*cps,size_t n,uint32_t cp);
mrb_int sp_str_casecmp(const char*a,const char*b);
mrb_bool sp_str_valid_encoding(const char*s);
const char*sp_str_field(const char*s,const char*sep,mrb_int n);
mrb_int sp_str_field_count(const char*s,const char*sep);
const char*sp_str_concat(const char*a,const char*b);
const char*sp_str_concat3(const char*a,const char*b,const char*c);
const char*sp_str_concat4(const char*a,const char*b,const char*c,const char*d);
const char*sp_str_concat_arr(const char *const *parts,int n);
const char*sp_str_inspect(const char*s);
const char*sp_str_upcase(const char*s);
const char*sp_str_downcase(const char*s);
const char*sp_str_swapcase(const char*s);
const char*sp_str_dump(const char*s);
const char*sp_str_delete_prefix(const char*s,const char*p);
const char*sp_str_substr(const char*s,mrb_int start,mrb_int len);
const char*sp_str_delete_suffix(const char*s,const char*p);
const char*sp_str_strip(const char*s);
const char*sp_str_chomp(const char*s);
const char *sp_str_chomp_sep(const char *s, const char *sep);
const char*sp_str_chop(const char*s);
mrb_bool sp_str_include(const char*s,const char*sub);
mrb_bool sp_str_start_with(const char*s,const char*p);
mrb_bool sp_str_end_with(const char*s,const char*suf);
sp_StrArray *sp_str_partition(const char *s, const char *sep);
sp_StrArray *sp_str_rpartition(const char *s, const char *sep);
sp_StrArray*sp_str_lines(const char*s);
sp_StrArray*sp_str_lines_chomp(const char*s);
const char*sp_str_byteslice(const char*s,mrb_int start,mrb_int len);
int sp_str_ascii_only(const char*s);
const char*sp_str_format_strarr(const char*fmt,sp_StrArray*a);
const char*sp_str_sub(const char*s,const char*pat,const char*rep);
const char*sp_str_capitalize(const char*s);
const char*sp_str_repeat(const char*s,mrb_int n);
sp_IntArray*sp_str_bytes(const char*s);
const char *sp_str_crypt(const char *s, const char *salt);
const char*sp_str_lstrip(const char*s);
const char*sp_str_rstrip(const char*s);
const char*sp_str_dup(const char*s);

#endif /* SP_STR_H */
