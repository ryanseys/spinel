# MultiTargetNode -- nested LHS in destructuring multi-assign.
#
#   a, (b, c), d = 1, [2, 3], 4
#
# Each parenthesized group on the LHS is a MultiTargetNode that
# recursively unpacks its slot of the RHS. Spinel routes through
# emit_multi_write_target which dispatches on the target node type.
# The inner-array RHS slot must be a typed-array (int_array, str_array,
# or float_array). Two-level nesting where intermediate slots are
# heterogeneous (poly_array) is out of scope -- documented inline.

a, (b, c), d = 1, [2, 3], 4
puts a    # 1
puts b    # 2
puts c    # 3
puts d    # 4

# String-typed inner array
x, (s, t), y = 10, ["hello", "world"], 20
puts x    # 10
puts s    # hello
puts t    # world
puts y    # 20
