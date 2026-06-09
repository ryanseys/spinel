# A non-lambda Proc has lenient arity: a required param the caller omits is
# nil (not a default/zero). Lambdas are strict and aren't covered here.
# (Each block uses distinct param names: same-named block params in one
# scope share a single typed slot, an unrelated limitation.)

# A missing required param is nil, rendered via p and string interpolation.
proc { |a, b| p b }.call(5)
Proc.new { |x, y, z| puts "x=#{x} y=#{y} z=#{z}" }.call(10)
Proc.new { |x2, y2, z2| puts "x=#{x2} y=#{y2} z=#{z2}" }.call(10, 20)

# Exact arity is unchanged.
p proc { |e, f| e + f }.call(3, 4)

# A single Array yielded to a block with more params than the array has
# elements destructures and nil-fills the surplus, for a non-literal
# (runtime-length) array too.
def give(arr)
  yield arr
end

give([1, 2]) { |m, n, o| p m; p n; p o }
give([10, 20]) { |p1, q1| p p1 + q1 }

# A yield of fewer scalars than the block's params nil-fills the rest,
# and the missing value renders as nil inside an array.
def one
  yield 7
end

one { |s, t| p [s, t] }
