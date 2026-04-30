# Integer-only constant arithmetic should fold at codegen time.
# Output is the same as the runtime path; this test pins the
# semantic equivalence — the perf win shows up in the generated C
# (no runtime multiply chain), not in observable output.

# GAPS.md reproducer
SECONDS_PER_DAY = 60 * 60 * 24
puts SECONDS_PER_DAY

# Chained adds, subs, muls
puts 1 + 2 + 3 + 4
puts 100 - 25 - 25 - 25
puts 2 * 3 * 4 * 5
puts 10 + 20 * 3
puts (5 + 5) * (4 + 4)

# Negative literals
puts -7 * 3
puts 100 - 200
puts -10 + 5

# Mixed (some folded, some not — variable poisons the chain)
x = 7
puts x + 3 + 4
puts (3 + 4) + x

# Larger but still in-range
puts 1000 * 1000
puts 60 * 60 * 24 * 7

# Division and modulo are NOT folded (different sign semantics)
puts 10 / 3
puts -10 / 3
puts 10 % 3
puts -10 % 3
