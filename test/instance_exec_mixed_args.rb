# Mixed-args instance_exec trampolines: the body of the trampoline
# method can forward a mix of method params, literals, and ivar
# reads into the block's params.
#
#   def emit(x, &b)
#     instance_exec(x, 99, @magic, &b)   # mix: param, literal, ivar
#   end
#   b.emit("hi") { |a, b, c| ... }       # a="hi", b=99, c=b.@magic
#
# Before this generalisation the trampoline detector required all
# args to be LocalVariableReads of the method's required params in
# matching order. The relaxed detector accepts literal nodes and
# InstanceVariableReadNode too; the inliner evaluates each arg per
# kind at the call site.

class Wrap
  def initialize
    @magic = 42
  end

  # Mix of param ("x"), literal (99), and ivar (@magic).
  def emit(x, &b)
    instance_exec(x, 99, @magic, &b)
  end

  def magic
    @magic
  end
end

w1 = Wrap.new
w2 = Wrap.new   # keep heap-allocated
w1.emit(10) { |a, b, c| puts a + b + c }   # 10 + 99 + 42 = 151
w1.emit(7)  { |a, b, c| puts a * b - c }   # 7  * 99 - 42 = 651

# All-literal int trampoline. (String and Symbol literals also pass
# the detector, but printing them inside the spliced block currently
# trips a separate type-inference gap -- the splice's block-param
# locals are declared after analyze has typed the block body, so
# `puts <str-arg>` doesn't pick up the string format. Tracked as a
# follow-up; this test stays on ints for now.)
# Inherit from Wrap so Tag stays heap-allocated (Wrap is already
# escaped above via w1/w2 with both used). Value-typed splice has
# a known write-through gap with @ivar writes inside the spliced
# block; documented separately.
class Tag < Wrap
  def initialize
    @count = 0
  end
  def fire(&b)
    instance_exec(1, 2, 3, &b)
  end
  def count
    @count
  end
end

t1 = Tag.new
t2 = Tag.new
t1.fire { |a, b, c| puts a + b + c; @count = @count + 1 }   # 6
puts t1.count                                                # 1

# Trampoline forwards only ivars + a literal (no method params).
# Int-typed ivars stay simple; string-concat-in-splice has a
# separate type-inference gap tracked elsewhere.
class Source
  def initialize
    @ttl = 5
    @base = 100
  end
  def with(&b)
    instance_exec(@ttl, @base, 0, &b)
  end
end

s1 = Source.new
s2 = Source.new
s1.with { |ttl, base, extra| puts ttl + base + extra }   # 105
puts "done"
