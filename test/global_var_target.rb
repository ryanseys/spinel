# GlobalVariableTargetNode — multi-assign LHS for globals.
#
# `$a, $b = 1, 2` -- each LHS slot is a GlobalVariableTargetNode.
# Routes through MultiWriteNode's emit_multi_write_target to the
# same g_<name> C-level storage GlobalVariableWriteNode uses.

$a, $b = 1, 2
puts $a    # 1
puts $b    # 2

$a, $b = $b, $a   # swap
puts $a    # 2
puts $b    # 1
