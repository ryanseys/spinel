# `block_given?` for `&block` parameters.
#
# Pre-fix `block_given?` only returned the right answer inside methods
# that used `yield` (`@in_yield_method == 1`). Methods that captured
# the block via `&block` always saw `block_given?` evaluate to `0`,
# even when a block was provided. Fixed by tracking the enclosing
# method's &block param name (`@current_method_block_param`) at all
# three method-emit sites and emitting `(lv_<param> != NULL)`.
#
# This test pins the no-block-passed observable: a top-level &block
# method called without a block. The receivered + literal-block paths
# go through the yield-inliner's own `block_given?` shortcut
# (compile_expr_remap → "1") so they don't transit the new lowering
# at all — pinning them here would test the inliner, not this commit.

# 1. Top-level `&block` method, called without a block — block_given?
#    must return false. Pre-fix this returned true (the `_block != NULL`
#    check evaluates undefined when the method has no yield ports, and
#    on top of that the handler always emitted "0" so the if took the
#    else branch).
def top(&block)
  if block_given?
    puts "1-yes"
  else
    puts "1-no"
  end
end

top                       #=> 1-no

# 2. Same shape with a regular arg before &block — find_block_param_name
#    must skip the int param and pick the proc-typed slot.
def top2(label, &block)
  if block_given?
    puts label + "-yes"
  else
    puts label + "-no"
  end
end

top2("2")                 #=> 2-no

puts "done"
