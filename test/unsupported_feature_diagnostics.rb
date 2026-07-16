# A deliberately-unsupported feature (docs/limitations.md) names itself rather
# than dumping compiler internals. A user-defined method of the same name is
# that method, not the builtin, so it still works (#2652 / #2667 / #2668).
class Widget
  def extend(x); "user-extend:#{x}"; end
  def ruby2_keywords; "user-r2k"; end
  def define_singleton_method(n); "user-dsm:#{n}"; end
end
w = Widget.new
p w.extend(1)
p w.ruby2_keywords
p w.define_singleton_method(:z)
def extend(a); "toplevel:#{a}"; end
p extend(9)
