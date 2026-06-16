# Forwarding a Proc *value* to an array iterator: `arr.each(&proc)` and
# `arr.map(&proc)` / `arr.collect(&proc)` call the proc once per element.
# Unlike a forwarded block param (`&blk`, handled by inlining), the proc is a
# runtime value, so the element rides its mrb_int arg slot and the result rides
# its mrb_int return slot, decoded by the proc's body return type. Int elements
# with int-returning procs are the supported case (a standalone lambda's param
# type must be inferable; arithmetic infers int).

# #each(&proc): called for side effects, result discarded.
pr = ->(x) { puts x * 10 }
[1, 2, 3].each(&pr)

# #map(&proc): collect the per-element results.
dbl = ->(x) { x * 2 }
p [1, 2, 3].map(&dbl)

# #collect alias.
inc = ->(n) { n + 1 }
p [10, 20].collect(&inc)

# Result fed into a further computation; receiver is a variable.
sq = ->(n) { n * n }
nums = [1, 2, 3, 4]
p nums.map(&sq)

# The same proc reused across two receivers.
neg = ->(x) { 0 - x }
p [1, 2].map(&neg)
p [5, 6, 7].map(&neg)
