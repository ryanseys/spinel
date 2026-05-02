# InterpolatedRegularExpressionNode -- /foo_#{x}/
#
# The pattern is only known at execution time, so each evaluation goes
# through sp_re_runtime_compile() (the runtime wrapper around the same
# onig engine that backs static /lit/ literals). Match-site dispatch
# uses the new regex_pat_c_expr helper so =~, match?, match, gsub, sub,
# scan, and split all accept either form.

# match? predicate (boolean -- safe across CRuby/Spinel int/value
# differences in =~ which returns a position vs a count).
x = "bar"
puts "foo_bar".match?(/foo_#{x}/)   #=> true
puts "foo_baz".match?(/foo_#{x}/)   #=> false

# Captures populate $1
if "foo_bar_42" =~ /foo_#{x}_(\d+)/
  puts $1   #=> 42
end

# gsub with interpolated pattern
who = "world"
puts "hello world".gsub(/#{who}/, "Ruby")   #=> hello Ruby

# sub
puts "abc 123 def".sub(/(\d+)/, "X")        #=> abc X def

# Interpolation that varies per-iteration (proves no stale cache and
# that runtime compilation actually re-fires per evaluation).
i = 0
while i < 3
  pat_seed = i.to_s
  puts ("v_" + pat_seed).match?(/v_#{pat_seed}/)
  i = i + 1
end
#=> true
#=> true
#=> true
