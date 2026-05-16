# Toplevel `def foo(x)` bodies containing instance_exec / instance_eval.
# The rewrite walks toplevel methods alongside class methods so receiver
# resolution works when the receiver is a method param.
#
# Without ieval_walk_toplevel_methods (and its
# propagate_toplevel_meth_arg_types_for_ieval pre-pass), a toplevel
# method like `def fill(b, k); b.instance_exec(k) { |n| add(n) }; end`
# silently bypasses the rewrite -- the receiver `b`'s type isn't in
# scope when the analyze pass walks the def body.

class Builder
  def initialize
    @s = 0
  end
  def add(n)
    @s = @s + n
  end
  def total
    @s
  end
end

# Toplevel def receives a typed param and calls instance_exec.
def fill(b, k)
  b.instance_exec(k) { |n| add(n) }
end

builder = Builder.new
builder2 = Builder.new
fill(builder, 10)
fill(builder, 20)
fill(builder2, 100)
puts builder.total    # 30
puts builder2.total   # 100

# Same shape for instance_eval (no args).
def apply(r)
  r.instance_eval { add(7) }
end

apply(builder)
puts builder.total    # 37

puts "done"
