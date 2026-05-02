# GlobalVariableOperatorWriteNode — `$x += v`, `$x -= v`, etc.
#
# Mirror of LocalVariableOperatorWriteNode but the storage is a
# C-level global symbol via sanitize_gvar(name) rather than a
# fiber-local slot.

$counter = 0
$counter += 1
$counter += 5
puts $counter  # 6
$counter -= 2
puts $counter  # 4
$counter *= 3
puts $counter  # 12
$counter /= 4
puts $counter  # 3
