# CallOperatorWriteNode -- `obj.attr += val`, `obj.attr -= val`, etc.
#
# CRuby evaluates the receiver exactly once even though the source
# `obj.bar += v` expands conceptually to `obj.bar = obj.bar + v`.
# The temp pattern in compile_call_assign_typed mirrors that.
#
# Spinel restriction: only typed instance receivers with
# attr_accessor (or struct field) on the class are supported. For
# everything else the codegen exits with a precise error rather
# than emit silently-wrong C.

class Counter
  attr_accessor :n
  def initialize
    @n = 0
  end
end

c = Counter.new
c.n += 5
puts c.n            # 5
c.n += 10
puts c.n            # 15
c.n -= 3
puts c.n            # 12
c.n *= 2
puts c.n            # 24
c.n /= 4
puts c.n            # 6
c.n |= 1
puts c.n            # 7
c.n &= 5
puts c.n            # 5
c.n ^= 4
puts c.n            # 1
c.n <<= 3
puts c.n            # 8
c.n >>= 1
puts c.n            # 4

# String-valued attr -- `+=` becomes string concat.
class Greeting
  attr_accessor :msg
  def initialize
    @msg = "hello"
  end
end

g = Greeting.new
g.msg += " world"
puts g.msg          # hello world
