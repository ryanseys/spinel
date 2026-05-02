# RationalNode -- 1r, 3.5r literals.
#
# Spinel emits the canonical "num/den" string form (reduced by GCD,
# denominator > 0) at compile time. This satisfies puts/string-context
# use without yet introducing a runtime sp_Rational struct -- arithmetic
# on Rational values would require that and is deferred. Tests verify
# the literal renders identically to CRuby's `puts` form.

# Integer rationals: 1r, 5r
puts 1r        #=> 1/1
puts 5r        #=> 5/1
puts 0r        #=> 0/1
puts (-3r)     #=> -3/1

# Float rationals: Prism splits 1.5r -> 3/2, 0.25r -> 1/4
puts 1.5r      #=> 3/2
puts 0.25r     #=> 1/4
puts 3.5r      #=> 7/2
puts 0.125r    #=> 1/8

# GCD reduction confirms canonical form
puts 6r        #=> 6/1   (already reduced; den=1 is a degenerate GCD case)
