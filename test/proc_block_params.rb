# An optional proc/block param binds its declared default when the arg
# is omitted, otherwise the supplied value.
puts proc { |a, b = 10| a + b }.call(1)
puts proc { |a, b = 10| a + b }.call(1, 5)

# A trailing splat param collects the surplus positional args.
puts proc { |*a| a.sum }.call(1, 2, 3)
puts proc { |*a| a.length }.call(7, 8)

# Proc#parameters enumerates required, optional, and splat params in
# order; a non-lambda Proc reports its requireds as optional (lenient
# arity). This locks the param-kind ordering the binding path relies on.
p proc { |a, b = 10, *c| }.parameters

# A lambda with the exact arity binds normally.
add = ->(x, y) { x + y }
puts add.call(3, 4)

# Compose / curry still thread through the argc-carrying ABI.
f = ->(x) { x + 1 }
g = ->(x) { x * 2 }
puts (f << g).call(5)
addc = ->(a, b, c) { a + b + c }
puts addc.curry[1][2][3]
