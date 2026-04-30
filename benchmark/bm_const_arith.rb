# bm_const_arith - codegen-time integer constant-fold (#17)
#
# Hot loop computes a constant subexpression `60 * 60 * 24` per
# iteration. The chained `% 99991` modulo on `acc` enforces a
# sequential data dependency so the C compiler cannot collapse
# the loop to a closed form.
#
# At -O2, clang already folds `(60 * 60) * 24 -> 86400` itself,
# so the codegen-time fold's wall-clock benefit is expected to
# be in the noise — this bench primarily catches regressions
# where Spinel emits a slower path (e.g. a runtime arithmetic
# helper) for constant integer arithmetic.
#
# Pattern shape: `acc = (acc + <const>) % <prime>` in a counted
# while loop, mirroring real-world hashing/folding kernels.

i = 0
acc = 1
while i < 500_000_000
  acc = (acc + 60 * 60 * 24) % 99991
  i = i + 1
end
puts acc
