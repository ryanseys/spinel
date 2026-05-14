# Bignum literal arithmetic: Integer("<big>") returns a bigint-typed
# value that flows through spinel's existing bigint arithmetic dispatch
# (sp_bigint_add/sub/mul/div/mod/cmp). Verified against CRuby 4.0.4
# which auto-promotes/demotes seamlessly.
#
# Note: comparing a literal Bignum source value (`100_000_000_000_000_000_000`
# in source code) against an Integer("...") result requires source-level
# bignum literal parsing, which is a separate gap (Group D in the plan).
# This test compares against bignum values produced by Integer("...") only.

# Setup: a single bigint from literal
b = Integer("100000000000000000000")    # 10^20

# Arithmetic with int literals
puts b + 1
puts b - 1
puts b * 2
puts b / 100
puts b % 7

# Comparison with int (fixnum) literals
puts b > 1
puts b > 99999999999999999999
puts b < 0
puts b == 0
puts b != 1

# Comparison with another Integer("...") bignum
b2 = Integer("100000000000000000000")
puts b == b2
puts b <= b2
puts b >= b2

b3 = Integer("100000000000000000001")
puts b < b3
puts b3 > b
