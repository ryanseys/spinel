# IndexTargetNode -- multi-assign LHS into indexed slots.
#
# `a[0], b[1] = 1, 2` -- each LHS slot is an IndexTargetNode
# carrying the receiver and index. Routes through emit_multi_write_target's
# new arm via the same per-receiver-type sp_<TYPE>_set dispatch
# compile_index_op_assign uses, but without the read-then-modify
# step (the value is supplied by the caller).

# Two int_array slots written in one multi-assign.
xs = [10, 20, 30]
ys = [100, 200, 300]
xs[0], ys[2] = 1, 999
puts xs[0]                    # 1
puts xs[1]                    # 20 (untouched)
puts ys[2]                    # 999

# Same-array two-slot swap: a[0], a[2] = a[2], a[0].
zs = [1, 2, 3, 4, 5]
zs[0], zs[4] = zs[4], zs[0]
puts zs[0]                    # 5
puts zs[4]                    # 1

# Hash slot mixed with array slot in one multi-assign.
counts = {"a" => 0, "b" => 0}
arr = [0, 0, 0]
counts["a"], arr[1] = 42, 99
puts counts["a"]              # 42
puts arr[1]                   # 99
