# o.extend(M) on a local/param receiver: the receiver responds to module M's
# instance methods after the extend. A synthesized ObjectExt_<M> class holds
# M's methods; the extend returns self. Receivers flow through parameters so
# the runtime path (not folding) is exercised. Only stateless module methods
# are supported (an extended bare Object has no ivar storage).
module Greet; def hi; "hi"; end; end
def f(o); o.extend(Greet); o.hi; end
p f(Object.new)                              # "hi"

# a module method taking arguments
module Calc; def add(a, b); a + b; end; end
def g(o); o.extend(Calc); o.add(2, 3); end
p g(Object.new)                              # 5

# extend returns the receiver (self)
module Marker; def mark; 1; end; end
def r(o); y = o.extend(Marker); y.equal?(o); end
p r(Object.new)                              # true

# two locals, different modules, in one scope
def two
  a = Object.new
  b = Object.new
  a.extend(Greet)
  b.extend(Calc)
  [a.hi, b.add(1, 1)]
end
p two                                        # ["hi", 2]

# a module method calling a sibling module method
module Chain; def one; two_x + 1; end; def two_x; 2; end; end
def q(o); o.extend(Chain); o.one; end
p q(Object.new)                              # 3

# multiple extends on one receiver: the method set is the union
def mu(o)
  o.extend(Greet)
  o.extend(Calc)
  [o.hi, o.add(4, 5)]
end
p mu(Object.new)                             # ["hi", 9]
