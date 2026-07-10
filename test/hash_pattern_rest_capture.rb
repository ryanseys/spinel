# The gap's exact repros: a **rest capture binds the remaining pairs.
case {a: 1, b: 2, c: 3}
in {a:, **rest}
  p a
  p rest
end

case {x: 1, y: 2}
in {**rest}
  p rest
end

# Param-routed (the gap's second form): returns [a, rest].
def f(h); case h; in {a:, **rest}; [a, rest]; end; end
p f({a: 1, b: 2, c: 3})
p f({a: 10, b: 20})

# **rest with an intervening class-checked key still captures the rest.
case {a: 1, b: 2, c: 3}
in {a: Integer => x, **rest}
  p x
  p rest
end

# The strict **nil form (assert no extra keys) is unaffected.
def strict(h); case h; in {a:, **nil}; "exact"; else "extra"; end; end
p strict({a: 1})
p strict({a: 1, b: 2})
