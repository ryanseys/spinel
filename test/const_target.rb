# ConstantTargetNode -- `FOO, BAR = 1, 2` (multi-assign LHS).
#
# Each LHS slot is a ConstantTargetNode; the multi-write splat
# routes through emit_multi_write_target which now has an arm for
# this case.

FOO = 0
BAR = 0
FOO, BAR = 100, 200
puts FOO   # 100
puts BAR   # 200

# Three-slot mix
A = 0
B = 0
C = 0
A, B, C = 1, 2, 3
puts A     # 1
puts B     # 2
puts C     # 3
