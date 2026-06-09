# An escaping Proc that op-assigns an enclosing local captures it by
# reference (not by value): each call mutates the one shared cell, so the
# second call sees the first call's increment.
count = 0
inc = proc { count += 1 }
inc.call
puts inc.call

# A Proc whose own param shares a name with an enclosing captured local
# binds its parameter, not the captured cell: the body compiles in its own
# function where the enclosing cell is out of scope.
base = 100
read_base = proc { base }
double = proc { |base| base * 2 }
puts read_base.call
puts double.call(5)
