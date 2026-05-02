# GlobalVariableOrWriteNode — `$x ||= val`.
#
# Mirror of LocalVariableOrWriteNode. Only assigns if the global
# is currently falsy.
#
# Note: Spinel uses C-truthy semantics (any zero is falsy), which
# diverges from CRuby (only nil/false are falsy). For unassigned
# globals (default 0/nil) the two agree on the first assignment;
# subsequent ||= against truthy non-zero ints also agree. This
# test exercises only the agreement region.

$count ||= 5    # never assigned, fires
puts $count     # 5

$count ||= 99   # already 5 (truthy in both C and Ruby), doesn't fire
puts $count     # 5
