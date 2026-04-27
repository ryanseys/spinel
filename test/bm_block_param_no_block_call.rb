# Method declares `&block` parameter but is called without a block.
# Spinel must pass NULL for the block parameter at the C call site
# instead of omitting the argument entirely (which produced
# "too few arguments" compile errors).
#
# Two scenarios:
#   1. With a positional arg + no block at the call site
#   2. Block-only method (zero positional args), no block at the call site
#      — tests the trailing-comma edge case (must emit `sp_f(NULL)`,
#      not `sp_f(, NULL)`).

def maybe_inc(n, &block)
  if block
    return block.call(n)
  end
  n + 1
end

def maybe_const(&block)
  if block
    return block.call
  end
  42
end

puts maybe_inc(5)
puts maybe_inc(5) { |x| x * 10 }
puts maybe_const
puts maybe_const { 99 }
