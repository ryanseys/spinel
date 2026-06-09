# `recv.instance_exec(&proc_var)` forwards a runtime Proc. Inlining the
# splice would require invoking that Proc with `self` rebound to the
# receiver, but Spinel's Proc carries no bound-self representation, so
# this is a compile-time error rather than a silent miscompile. (CRuby
# runs the Proc's body with self set to the receiver.)
class Box
  def initialize(v)
    @v = v
  end
end

b = Box.new(5)
p = proc { 1 }
b.instance_exec(&p)
