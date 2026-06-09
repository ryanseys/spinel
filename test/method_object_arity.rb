def f(a, b)
  a + b
end

def g(a, b = 1)
  a + b
end

def h(*a)
  a.sum
end

def z
  0
end

puts method(:f).arity
puts method(:g).arity
puts method(:h).arity
puts method(:z).arity

class C
  def add(x, y)
    x + y
  end

  def one(x)
    x
  end
end

c = C.new
puts c.method(:add).arity
puts c.method(:one).arity
