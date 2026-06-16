# Forwarding a Proc VALUE to an array iterator: each / map / collect / select /
# reject. Unlike a forwarded block param, the proc is a runtime value called
# once per element via the sp_proc_call ABI. Supported case: int elements with
# int- (or bool-, for filters) returning procs.

# each(&proc): called for side effects, result discarded.
pr = ->(x) { puts x * 10 }
[1, 2, 3].each(&pr)

# map / collect(&proc): collect the per-element results.
dbl = ->(x) { x * 2 }
p [1, 2, 3].map(&dbl)

sq = ->(n) { n * n }
p [1, 2, 3].collect(&sq)

# select / reject(&proc): keep elements where the predicate is truthy / falsy.
pos = ->(x) { x > 0 }
p [-2, -1, 0, 1, 2].select(&pos)

ev = ->(x) { x.even? }
p [1, 2, 3, 4, 5, 6].reject(&ev)

# The same proc reused across two receivers.
neg = ->(x) { 0 - x }
p [1, 2].map(&neg)
p [5, 6, 7].map(&neg)
