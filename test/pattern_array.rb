# ArrayPatternNode -- destructuring array patterns in case/in.
#
# Spinel emits a chained boolean expression: type check + length check
# + comma-bound bindings. Each LocalVariableTargetNode in the pattern
# becomes an `lv_<name> = sp_*Array_get(tmp, i)` expression chained via
# the comma operator inside `&& (...)`. Rest splat bindings use
# sp_*Array_slice for the tail.

# Exact-length destructure -- bind a, b, c.
case [1, 2, 3]
in [a, b, c]
  puts a.to_s    #=> 1
  puts b.to_s    #=> 2
  puts c.to_s    #=> 3
end

# Length mismatch falls through to else.
case [10, 20]
in [a, b, c]
  puts "matched (wrong)"
else
  puts "fell through"   #=> fell through
end

# Rest splat captures the tail.
case [100, 200, 300, 400]
in [first, second, *rest]
  puts first.to_s        #=> 100
  puts second.to_s       #=> 200
  puts rest.length.to_s  #=> 2
  rest.each { |v| puts v.to_s }
  #=> 300
  #=> 400
end

# String array works the same way through sp_StrArray_get.
case ["foo", "bar", "baz"]
in [x, y, z]
  puts x   #=> foo
  puts y   #=> bar
  puts z   #=> baz
end
