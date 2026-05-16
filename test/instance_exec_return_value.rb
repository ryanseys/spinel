# Expression-position return value for instance_exec: the call
# evaluates to the block's last expression rather than the receiver.
#
# Before infer_iexec_body_return_types landed, the lift emitted a
# void-returning function and the call-site comma-expression hack
# returned the receiver. CRuby always returns the block's value,
# and `x = obj.instance_exec(...) { ... }` is the common pattern
# this enables.

class Vec
  def initialize
    @x = 0
  end
  def x
    @x
  end
end

# Subclass forces Vec to stay heap-allocated; value-typed receivers
# lose @ivar mutations across the lifted-function boundary because
# self is passed by value (the lifted function sees a copy).
class VecPlus < Vec
end

v1 = Vec.new
v2 = Vec.new

# Block ends with an int expression -- the call's value is that int.
result_int = v1.instance_exec(3, 4) { |a, b| @x = a + b; a * b }
puts result_int          # 12
puts v1.x                # 7 (side effect on @x preserved)

# Block ends with a string expression.
result_str = v2.instance_exec(5) { |n| "n is " + n.to_s }
puts result_str          # n is 5

# Used inline in an arithmetic expression.
final = v1.instance_exec(10) { |k| @x + k }
puts final               # 17 (7 + 10)

puts "done"
