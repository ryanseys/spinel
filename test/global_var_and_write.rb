# GlobalVariableAndWriteNode — `$x &&= val`.
#
# Mirror of LocalVariableAndWriteNode. Only assigns if the global
# is currently truthy. Same C-truthy vs Ruby-truthy caveat as
# Or-write — see test/global_var_or_write.rb.

$count = 5
$count &&= 10    # 5 is truthy, fires
puts $count      # 10

$count &&= 0     # 10 is truthy in C, fires
puts $count      # 0
