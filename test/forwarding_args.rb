# ForwardingParameterNode + ForwardingArgumentsNode -- def f(...); g(...); end
#
# `...` is sugar for `*__fwd_a, **__fwd_kw, &__fwd_b`. Spinel's AOT model
# requires no new varargs ABI -- the synthetic triple is just three
# existing param slots (rest, kwrest, block) under fixed names. Both the
# forwarding method's signature and any g(...) call inside it pivot on
# the same names, so the call site can pass them through directly without
# per-param matching.
#
# Restriction: forwarding only works callee-to-callee where both ends
# use `(...)`. Calling a fixed-arity method with `(...)` would require
# unbundling the triple, which Spinel doesn't support today.

# Two-tier forwarding: outer hands off to inner via the synthetic triple.
def inner(...)
  puts "inner called"
end

def outer(...)
  inner(...)
end

outer
#=> inner called

# Three-tier forwarding still threads through the same slots.
def deep_inner(...)
  puts "deep"
end

def deep_middle(...)
  deep_inner(...)
end

def deep_outer(...)
  deep_middle(...)
end

deep_outer
#=> deep
