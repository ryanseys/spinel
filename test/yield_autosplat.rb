# Ruby's block-arg auto-splat: a method that yields a single Array to a
# block declaring two or more params destructures the array across the
# params. A one-param block still receives the whole array (no splat).

def pair
  yield [1, 2]
end

pair { |a, b| puts a + b }

def labelled
  yield ["a", "b"]
end

labelled { |s, t| puts s + t }

def coords
  yield [3, 4, 5]
end

coords { |x, y, z| puts x + y + z }

# Single param: no splat, the whole array is bound.
def one
  yield [10, 20]
end

one { |arr| puts arr.length }

# Float elements destructure to float params.
def fpair
  yield [1.5, 2.5]
end

fpair { |g, h| puts g + h }

# More params than the yielded literal has elements: the surplus params
# are nil (Ruby's missing-splat-arg fill).
def short
  yield [1, 2]
end

short { |a, b, c| p a; p b; p c }

def short_str
  yield ["x", "y"]
end

short_str { |m, n, o| p o }
