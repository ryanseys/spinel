/* Per-type method-name sets for respond_to? on primitive receivers.
   GENERATED from Ruby 4.0.5 `Type.instance_methods(true)` — do not edit by hand.
   Each array is sorted so the lookup can binary-search. To regenerate a type,
   emit `Klass.instance_methods(true).map(&:to_s).sort.uniq` as a C string array. */
#ifndef SP_RESPOND_TO_TABLE_H
#define SP_RESPOND_TO_TABLE_H
#include <string.h>

static const char *const sp_rt_string[] = {
  "!", "!=", "!~", "%", "*", "+",
  "+@", "-@", "<", "<<", "<=", "<=>",
  "==", "===", "=~", ">", ">=", "[]",
  "[]=", "__id__", "__send__", "append_as_bytes", "ascii_only?", "b",
  "between?", "byteindex", "byterindex", "bytes", "bytesize", "byteslice",
  "bytesplice", "capitalize", "capitalize!", "casecmp", "casecmp?", "center",
  "chars", "chomp", "chomp!", "chop", "chop!", "chr",
  "clamp", "class", "clear", "clone", "codepoints", "concat",
  "count", "crypt", "dedup", "define_singleton_method", "delete", "delete!",
  "delete_prefix", "delete_prefix!", "delete_suffix", "delete_suffix!", "display", "downcase",
  "downcase!", "dump", "dup", "each_byte", "each_char", "each_codepoint",
  "each_grapheme_cluster", "each_line", "empty?", "encode", "encode!", "encoding",
  "end_with?", "enum_for", "eql?", "equal?", "extend", "force_encoding",
  "freeze", "frozen?", "getbyte", "grapheme_clusters", "gsub", "gsub!",
  "hash", "hex", "include?", "index", "insert", "inspect",
  "instance_eval", "instance_exec", "instance_of?", "instance_variable_defined?", "instance_variable_get", "instance_variable_set",
  "instance_variables", "intern", "is_a?", "itself", "kind_of?", "length",
  "lines", "ljust", "lstrip", "lstrip!", "match", "match?",
  "method", "methods", "next", "next!", "nil?", "object_id",
  "oct", "ord", "partition", "prepend", "private_methods", "protected_methods",
  "public_method", "public_methods", "public_send", "remove_instance_variable", "replace", "respond_to?",
  "reverse", "reverse!", "rindex", "rjust", "rpartition", "rstrip",
  "rstrip!", "scan", "scrub", "scrub!", "send", "setbyte",
  "singleton_class", "singleton_method", "singleton_methods", "size", "slice", "slice!",
  "split", "squeeze", "squeeze!", "start_with?", "strip", "strip!",
  "sub", "sub!", "succ", "succ!", "sum", "swapcase",
  "swapcase!", "tap", "then", "to_c", "to_enum", "to_f",
  "to_i", "to_r", "to_s", "to_str", "to_sym", "tr",
  "tr!", "tr_s", "tr_s!", "undump", "unicode_normalize", "unicode_normalize!",
  "unicode_normalized?", "unpack", "unpack1", "upcase", "upcase!", "upto",
  "valid_encoding?", "yield_self",
};
static const int sp_rt_string_n = 182;

static const char *const sp_rt_integer[] = {
  "!", "!=", "!~", "%", "&", "*",
  "**", "+", "+@", "-", "-@", "/",
  "<", "<<", "<=", "<=>", "==", "===",
  ">", ">=", ">>", "[]", "^", "__id__",
  "__send__", "abs", "abs2", "allbits?", "angle", "anybits?",
  "arg", "between?", "bit_length", "ceil", "ceildiv", "chr",
  "clamp", "class", "clone", "coerce", "conj", "conjugate",
  "define_singleton_method", "denominator", "digits", "display", "div", "divmod",
  "downto", "dup", "enum_for", "eql?", "equal?", "even?",
  "extend", "fdiv", "finite?", "floor", "freeze", "frozen?",
  "gcd", "gcdlcm", "hash", "i", "imag", "imaginary",
  "infinite?", "inspect", "instance_eval", "instance_exec", "instance_of?", "instance_variable_defined?",
  "instance_variable_get", "instance_variable_set", "instance_variables", "integer?", "is_a?", "itself",
  "kind_of?", "lcm", "magnitude", "method", "methods", "modulo",
  "negative?", "next", "nil?", "nobits?", "nonzero?", "numerator",
  "object_id", "odd?", "ord", "phase", "polar", "positive?",
  "pow", "pred", "private_methods", "protected_methods", "public_method", "public_methods",
  "public_send", "quo", "rationalize", "real", "real?", "rect",
  "rectangular", "remainder", "remove_instance_variable", "respond_to?", "round", "send",
  "singleton_class", "singleton_method", "singleton_method_added", "singleton_methods", "size", "step",
  "succ", "tap", "then", "times", "to_c", "to_enum",
  "to_f", "to_i", "to_int", "to_r", "to_s", "truncate",
  "upto", "yield_self", "zero?", "|", "~",
};
static const int sp_rt_integer_n = 137;

static const char *const sp_rt_float[] = {
  "!", "!=", "!~", "%", "*", "**",
  "+", "+@", "-", "-@", "/", "<",
  "<=", "<=>", "==", "===", ">", ">=",
  "__id__", "__send__", "abs", "abs2", "angle", "arg",
  "between?", "ceil", "clamp", "class", "clone", "coerce",
  "conj", "conjugate", "define_singleton_method", "denominator", "display", "div",
  "divmod", "dup", "enum_for", "eql?", "equal?", "extend",
  "fdiv", "finite?", "floor", "freeze", "frozen?", "hash",
  "i", "imag", "imaginary", "infinite?", "inspect", "instance_eval",
  "instance_exec", "instance_of?", "instance_variable_defined?", "instance_variable_get", "instance_variable_set", "instance_variables",
  "integer?", "is_a?", "itself", "kind_of?", "magnitude", "method",
  "methods", "modulo", "nan?", "negative?", "next_float", "nil?",
  "nonzero?", "numerator", "object_id", "phase", "polar", "positive?",
  "prev_float", "private_methods", "protected_methods", "public_method", "public_methods", "public_send",
  "quo", "rationalize", "real", "real?", "rect", "rectangular",
  "remainder", "remove_instance_variable", "respond_to?", "round", "send", "singleton_class",
  "singleton_method", "singleton_method_added", "singleton_methods", "step", "tap", "then",
  "to_c", "to_enum", "to_f", "to_i", "to_int", "to_r",
  "to_s", "truncate", "yield_self", "zero?",
};
static const int sp_rt_float_n = 112;

static const char *const sp_rt_symbol[] = {
  "!", "!=", "!~", "<", "<=", "<=>",
  "==", "===", "=~", ">", ">=", "[]",
  "__id__", "__send__", "between?", "capitalize", "casecmp", "casecmp?",
  "clamp", "class", "clone", "define_singleton_method", "display", "downcase",
  "dup", "empty?", "encoding", "end_with?", "enum_for", "eql?",
  "equal?", "extend", "freeze", "frozen?", "hash", "id2name",
  "inspect", "instance_eval", "instance_exec", "instance_of?", "instance_variable_defined?", "instance_variable_get",
  "instance_variable_set", "instance_variables", "intern", "is_a?", "itself", "kind_of?",
  "length", "match", "match?", "method", "methods", "name",
  "next", "nil?", "object_id", "private_methods", "protected_methods", "public_method",
  "public_methods", "public_send", "remove_instance_variable", "respond_to?", "send", "singleton_class",
  "singleton_method", "singleton_methods", "size", "slice", "start_with?", "succ",
  "swapcase", "tap", "then", "to_enum", "to_proc", "to_s",
  "to_sym", "upcase", "yield_self",
};
static const int sp_rt_symbol_n = 81;

static const char *const sp_rt_array[] = {
  "!", "!=", "!~", "&", "*", "+",
  "-", "<<", "<=>", "==", "===", "[]",
  "[]=", "__id__", "__send__", "all?", "any?", "append",
  "assoc", "at", "bsearch", "bsearch_index", "chain", "chunk",
  "chunk_while", "class", "clear", "clone", "collect", "collect!",
  "collect_concat", "combination", "compact", "compact!", "concat", "count",
  "cycle", "deconstruct", "define_singleton_method", "delete", "delete_at", "delete_if",
  "detect", "difference", "dig", "display", "drop", "drop_while",
  "dup", "each", "each_cons", "each_entry", "each_index", "each_slice",
  "each_with_index", "each_with_object", "empty?", "entries", "enum_for", "eql?",
  "equal?", "extend", "fetch", "fetch_values", "fill", "filter",
  "filter!", "filter_map", "find", "find_all", "find_index", "first",
  "flat_map", "flatten", "flatten!", "freeze", "frozen?", "grep",
  "grep_v", "group_by", "hash", "include?", "index", "inject",
  "insert", "inspect", "instance_eval", "instance_exec", "instance_of?", "instance_variable_defined?",
  "instance_variable_get", "instance_variable_set", "instance_variables", "intersect?", "intersection", "is_a?",
  "itself", "join", "keep_if", "kind_of?", "last", "lazy",
  "length", "map", "map!", "max", "max_by", "member?",
  "method", "methods", "min", "min_by", "minmax", "minmax_by",
  "nil?", "none?", "object_id", "one?", "pack", "partition",
  "permutation", "pop", "prepend", "private_methods", "product", "protected_methods",
  "public_method", "public_methods", "public_send", "push", "rassoc", "reduce",
  "reject", "reject!", "remove_instance_variable", "repeated_combination", "repeated_permutation", "replace",
  "respond_to?", "reverse", "reverse!", "reverse_each", "rfind", "rindex",
  "rotate", "rotate!", "sample", "select", "select!", "send",
  "shift", "shuffle", "shuffle!", "singleton_class", "singleton_method", "singleton_methods",
  "size", "slice", "slice!", "slice_after", "slice_before", "slice_when",
  "sort", "sort!", "sort_by", "sort_by!", "sum", "take",
  "take_while", "tally", "tap", "then", "to_a", "to_ary",
  "to_enum", "to_h", "to_s", "to_set", "transpose", "union",
  "uniq", "uniq!", "unshift", "values_at", "yield_self", "zip",
  "|",
};
static const int sp_rt_array_n = 187;

static const char *const sp_rt_hash[] = {
  "!", "!=", "!~", "<", "<=", "<=>",
  "==", "===", ">", ">=", "[]", "[]=",
  "__id__", "__send__", "all?", "any?", "assoc", "chain",
  "chunk", "chunk_while", "class", "clear", "clone", "collect",
  "collect_concat", "compact", "compact!", "compare_by_identity", "compare_by_identity?", "count",
  "cycle", "deconstruct_keys", "default", "default=", "default_proc", "default_proc=",
  "define_singleton_method", "delete", "delete_if", "detect", "dig", "display",
  "drop", "drop_while", "dup", "each", "each_cons", "each_entry",
  "each_key", "each_pair", "each_slice", "each_value", "each_with_index", "each_with_object",
  "empty?", "entries", "enum_for", "eql?", "equal?", "except",
  "extend", "fetch", "fetch_values", "filter", "filter!", "filter_map",
  "find", "find_all", "find_index", "first", "flat_map", "flatten",
  "freeze", "frozen?", "grep", "grep_v", "group_by", "has_key?",
  "has_value?", "hash", "include?", "inject", "inspect", "instance_eval",
  "instance_exec", "instance_of?", "instance_variable_defined?", "instance_variable_get", "instance_variable_set", "instance_variables",
  "invert", "is_a?", "itself", "keep_if", "key", "key?",
  "keys", "kind_of?", "lazy", "length", "map", "max",
  "max_by", "member?", "merge", "merge!", "method", "methods",
  "min", "min_by", "minmax", "minmax_by", "nil?", "none?",
  "object_id", "one?", "partition", "private_methods", "protected_methods", "public_method",
  "public_methods", "public_send", "rassoc", "reduce", "rehash", "reject",
  "reject!", "remove_instance_variable", "replace", "respond_to?", "reverse_each", "select",
  "select!", "send", "shift", "singleton_class", "singleton_method", "singleton_methods",
  "size", "slice", "slice_after", "slice_before", "slice_when", "sort",
  "sort_by", "store", "sum", "take", "take_while", "tally",
  "tap", "then", "to_a", "to_enum", "to_h", "to_hash",
  "to_proc", "to_s", "to_set", "transform_keys", "transform_keys!", "transform_values",
  "transform_values!", "uniq", "update", "value?", "values", "values_at",
  "yield_self", "zip",
};
static const int sp_rt_hash_n = 170;

static const char *const sp_rt_range[] = {
  "!", "!=", "!~", "%", "<=>", "==",
  "===", "__id__", "__send__", "all?", "any?", "begin",
  "bsearch", "chain", "chunk", "chunk_while", "class", "clone",
  "collect", "collect_concat", "compact", "count", "cover?", "cycle",
  "define_singleton_method", "detect", "display", "drop", "drop_while", "dup",
  "each", "each_cons", "each_entry", "each_slice", "each_with_index", "each_with_object",
  "end", "entries", "enum_for", "eql?", "equal?", "exclude_end?",
  "extend", "filter", "filter_map", "find", "find_all", "find_index",
  "first", "flat_map", "freeze", "frozen?", "grep", "grep_v",
  "group_by", "hash", "include?", "inject", "inspect", "instance_eval",
  "instance_exec", "instance_of?", "instance_variable_defined?", "instance_variable_get", "instance_variable_set", "instance_variables",
  "is_a?", "itself", "kind_of?", "last", "lazy", "map",
  "max", "max_by", "member?", "method", "methods", "min",
  "min_by", "minmax", "minmax_by", "nil?", "none?", "object_id",
  "one?", "overlap?", "partition", "private_methods", "protected_methods", "public_method",
  "public_methods", "public_send", "reduce", "reject", "remove_instance_variable", "respond_to?",
  "reverse_each", "select", "send", "singleton_class", "singleton_method", "singleton_methods",
  "size", "slice_after", "slice_before", "slice_when", "sort", "sort_by",
  "step", "sum", "take", "take_while", "tally", "tap",
  "then", "to_a", "to_enum", "to_h", "to_s", "to_set",
  "uniq", "yield_self", "zip",
};
static const int sp_rt_range_n = 123;

static const char *const sp_rt_nil[] = {
  "!", "!=", "!~", "&", "<=>", "==",
  "===", "=~", "^", "__id__", "__send__", "class",
  "clone", "define_singleton_method", "display", "dup", "enum_for", "eql?",
  "equal?", "extend", "freeze", "frozen?", "hash", "inspect",
  "instance_eval", "instance_exec", "instance_of?", "instance_variable_defined?", "instance_variable_get", "instance_variable_set",
  "instance_variables", "is_a?", "itself", "kind_of?", "method", "methods",
  "nil?", "object_id", "private_methods", "protected_methods", "public_method", "public_methods",
  "public_send", "rationalize", "remove_instance_variable", "respond_to?", "send", "singleton_class",
  "singleton_method", "singleton_methods", "tap", "then", "to_a", "to_c",
  "to_enum", "to_f", "to_h", "to_i", "to_r", "to_s",
  "yield_self", "|",
};
static const int sp_rt_nil_n = 62;

static const char *const sp_rt_true[] = {
  "!", "!=", "!~", "&", "<=>", "==",
  "===", "^", "__id__", "__send__", "class", "clone",
  "define_singleton_method", "display", "dup", "enum_for", "eql?", "equal?",
  "extend", "freeze", "frozen?", "hash", "inspect", "instance_eval",
  "instance_exec", "instance_of?", "instance_variable_defined?", "instance_variable_get", "instance_variable_set", "instance_variables",
  "is_a?", "itself", "kind_of?", "method", "methods", "nil?",
  "object_id", "private_methods", "protected_methods", "public_method", "public_methods", "public_send",
  "remove_instance_variable", "respond_to?", "send", "singleton_class", "singleton_method", "singleton_methods",
  "tap", "then", "to_enum", "to_s", "yield_self", "|",
};
static const int sp_rt_true_n = 54;

static const char *const sp_rt_false[] = {
  "!", "!=", "!~", "&", "<=>", "==",
  "===", "^", "__id__", "__send__", "class", "clone",
  "define_singleton_method", "display", "dup", "enum_for", "eql?", "equal?",
  "extend", "freeze", "frozen?", "hash", "inspect", "instance_eval",
  "instance_exec", "instance_of?", "instance_variable_defined?", "instance_variable_get", "instance_variable_set", "instance_variables",
  "is_a?", "itself", "kind_of?", "method", "methods", "nil?",
  "object_id", "private_methods", "protected_methods", "public_method", "public_methods", "public_send",
  "remove_instance_variable", "respond_to?", "send", "singleton_class", "singleton_method", "singleton_methods",
  "tap", "then", "to_enum", "to_s", "yield_self", "|",
};
static const int sp_rt_false_n = 54;

/* Binary-search a sorted method-name table. */
static int sp_rt_in(const char *const *a, int n, const char *q) {
  int lo = 0, hi = n - 1;
  while (lo <= hi) {
    int mid = (lo + hi) / 2;
    int cmp = strcmp(a[mid], q);
    if (cmp == 0) return 1;
    if (cmp < 0) lo = mid + 1; else hi = mid - 1;
  }
  return 0;
}

#endif
