# NoKeywordsParameterNode — `def f(**nil)` explicit kwarg rejection.
#
# In CRuby this declares the method rejects keyword arguments;
# attempting `f(k: 1)` raises ArgumentError. Spinel's keyword-arg
# handling is already conservative (only known keys accepted), so
# the explicit `**nil` marker is effectively a no-op at the codegen
# level. The parser must still recognize the node so Phase 0's
# UnsupportedNode loud-error doesn't fire.

def add(a, b, **nil)
  a + b
end

puts add(1, 2)     # 3
puts add(10, 20)   # 30
