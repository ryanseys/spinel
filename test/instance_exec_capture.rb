# Outer-local capture on the direct-path instance_exec lift.
#
#   total = 0
#   obj.instance_exec(5) { |n| total = total + n }
#   obj.instance_exec(7) { |n| total = total + n }
#   puts total   # 12 -- writes propagate back to the caller's local
#
# The lifted `sp_iexec_<N>` function takes a pointer per capture:
#
#   static void sp_iexec_0(sp_C *self, mrb_int lv_n, mrb_int *_cap_total) {
#     lv_total = *_cap_total;        // proxy load
#     ...block body...
#     *_cap_total = lv_total;        // write-back
#   }
#
# The call site passes `&lv_total` for each capture. The block body
# compiles unchanged -- references to `total` resolve to the proxy
# local, mutations propagate to the caller via the pointer.
#
# Before this change the direct path silently dropped writes to
# outer locals (the lifted function couldn't see the caller's
# storage), creating an asymmetry with the trampoline path which
# supported captures via inlining. Both paths now propagate.

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

b = Builder.new
b2 = Builder.new
total = 0
count = 0

b.instance_exec(5) { |n| add(n); total = total + n; count = count + 1 }
b.instance_exec(7) { |n| add(n); total = total + n; count = count + 1 }
b2.instance_exec(3) { |n| add(n); total = total + n; count = count + 1 }

puts total    # 15
puts count    # 3
puts b.total  # 12
puts b2.total # 3

puts "done"
