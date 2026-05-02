# FindPatternNode -- `[*pre, x, y, *post]` finds a contiguous window
# matching the requireds, binds splats to slices before and after.
#
# Spinel's simplified implementation handles the LocalVariableTarget-
# only case: requireds bind to the first N elements and splats to
# before (empty) / after (rest). Literal-anchored finds (e.g.
# `[*, 3, *]`) need a runtime scan loop and stay deferred. This is
# enough for the common idiom of "split into head/middle/tail" without
# manual indexing.

case [10, 20, 30, 40, 50]
in [*pre, x, y, *post]
  puts x.to_s             #=> 10
  puts y.to_s             #=> 20
  puts pre.length.to_s    #=> 0
  puts post.length.to_s   #=> 3
  post.each { |v| puts v.to_s }
  #=> 30
  #=> 40
  #=> 50
end

# Single required, splats both sides.
case [1, 2, 3]
in [*pre, only, *post]
  puts only.to_s          #=> 1
  puts pre.length.to_s    #=> 0
  puts post.length.to_s   #=> 2
end

# String array works the same way through sp_StrArray_get/slice.
case ["a", "b", "c", "d"]
in [*lhs, first, second, *rhs]
  puts first              #=> a
  puts second             #=> b
  puts lhs.length.to_s    #=> 0
  puts rhs.length.to_s    #=> 2
end
