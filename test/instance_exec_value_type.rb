# Value-typed receivers (Spinel SRA-promotes single-instance classes
# with simple ivars to stack-allocated value types). The lifted
# `sp_iexec_<N>` / `sp_ieval_<N>` functions must take self by
# pointer so block bodies can mutate the receiver's ivars and the
# changes propagate back to the caller.
#
# Before the value-type splice fix, these tests crashed at C-compile
# time with "operand of type 'sp_Tiny' where arithmetic or pointer
# type is required" -- the splice's pointer cast `(sp_<C> *)<rc>`
# was invalid against a struct value. Now the call site takes the
# receiver's address (`&recv`), the lifted function declares
# `sp_<C> *self`, and the body's `@x = ...` writes through the
# pointer back into the caller's storage.

# 1. instance_eval on a single-instance class
class Tiny
  def initialize
    @n = 0
  end
  def get
    @n
  end
end

t = Tiny.new
t.instance_eval { @n = @n + 5 }
t.instance_eval { @n = @n + 7 }
puts t.get   # 12

# 2. instance_exec on a single-instance class with one arg
class Vec
  def initialize
    @x = 0
  end
  def x
    @x
  end
end

v = Vec.new
v.instance_exec(3) { |n| @x = @x + n }
v.instance_exec(4) { |n| @x = @x + n }
puts v.x     # 7

puts "done"
