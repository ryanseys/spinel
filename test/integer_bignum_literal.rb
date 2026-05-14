# CRuby 4.0.4: Integer("<string-literal-exceeding-mrb_int>") returns a
# Bignum. Spinel statically classifies the literal at codegen time and
# emits the bigint path so existing bigint arithmetic / puts dispatch
# carries the value.

# Positive bignums (just outside mrb_int range, far outside, hex form)
puts Integer("9223372036854775808")   # LLONG_MAX + 1
puts Integer("99999999999999999999999")
puts Integer("100000000000000000000")

# Negative bignums
puts Integer("-9223372036854775809")  # LLONG_MIN - 1
puts Integer("-99999999999999999999999")

# Exact LLONG_MAX / LLONG_MIN still fixnum (round-trip)
puts Integer("9223372036854775807")
puts Integer("-9223372036854775808")

# Underscore-separated bignum literal
puts Integer("100_000_000_000_000_000_000")

# Non-decimal bignum literals: codegen must route through the matching
# base, not blindly call sp_bigint_new_str(..., 10). Hex with 17+
# digits, binary with 64+ digits, octal with 22+ digits all classify
# as bigint per integer_literal_classify's thresholds.
puts Integer("0xFFFFFFFFFFFFFFFFFFFF")
puts Integer("-0xFFFFFFFFFFFFFFFFFFFF")
puts Integer("0o7777777777777777777777")
puts Integer("01777777777777777777777")
