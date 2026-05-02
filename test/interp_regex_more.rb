# InterpolatedRegularExpressionNode -- additional coverage beyond
# test/interp_regex.rb. Exercises captures with each, scan return shape,
# gsub with multiple matches, and dispatch through helper methods.

# Captures populate $1..$N and survive into the next statement.
text = "alice=30 bob=25 carol=42"
key = "bob"
if text =~ /#{key}=(\d+)/
  puts $1   #=> 25
end

# scan with interpolated pattern returns flat StrArray of matches.
needle = "x"
hits = "abc x def x ghi x".scan(/#{needle}/)
puts hits.length.to_s   #=> 3

# gsub iterates -- replaces every match.
who = "world"
puts "world world world".gsub(/#{who}/, "Ruby")
#=> Ruby Ruby Ruby

# Dispatch via helper method (proves the regex_pat_c_expr helper
# routes through method calls, not just inline =~).
def first_match(s, pat)
  s =~ /#{pat}/ ? "yes" : "no"
end
puts first_match("foo_x", "_x")        #=> yes  ("foo_x" contains "_x")
puts first_match("noexx", "_x")        #=> no   ("noexx" lacks underscore)
puts first_match("foo", "_x")          #=> no

# Same interpolated pattern inside a loop -- runtime compile fires
# every iteration (no cache today).
seeds = ["a", "b", "c"]
seeds.each { |s| puts ("v_" + s).match?(/v_#{s}/) }
#=> true
#=> true
#=> true
