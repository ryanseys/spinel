# A `return` inside a direct instance_exec block returns from the
# ENCLOSING method, not just the block -- matching CRuby. The block body
# is spliced at the call site, so the return lowers to a real C return
# from the enclosing function.

class B
  def initialize(v)
    @v = v
  end

  def get
    @v
  end
end

class BP < B
end

class Host
  def initialize(v)
    @box = B.new(v)
  end

  # return short-circuits this method (guard-clause style); the tested
  # value is captured from the enclosing scope.
  def check(n)
    @box.instance_exec { return n if n > 0 }
    -1
  end

  # return a rebound-self method's value out of this method.
  def grab
    @box.instance_exec { return get }
    999
  end
end

h = Host.new(42)
puts h.check(7)    #=> 7
puts h.check(-3)   #=> -1
puts h.grab        #=> 42

# Implicit-self receiver: `instance_exec` with no receiver inside a
# method runs against `self`; the return exits the method.
class Counter
  def initialize
    @n = 10
  end

  def val
    @n
  end

  def fetch
    instance_exec { return val }
    -1
  end
end

class CounterPlus < Counter
end

puts Counter.new.fetch   #=> 10

puts "done"
