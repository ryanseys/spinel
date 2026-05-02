# ImaginaryNode -- 1i, 2.5i literals.
#
# Spinel emits the canonical "0+Ni" string form at compile time.
# Same string-not-typed trade-off as RationalNode: arithmetic on
# Complex values would require a runtime sp_Complex struct + numeric
# ops, which is deferred. Today's coverage is "literal compiles,
# puts/p/== work, string-context use works."

# Integer imaginaries
puts 1i      #=> 0+1i
puts 5i      #=> 0+5i
puts 0i      #=> 0+0i

# Float imaginaries
puts 2.5i    #=> 0+2.5i
puts 0.5i    #=> 0+0.5i

# Negative imaginaries -- Prism keeps the minus inside the numeric
# child, codegen pulls it out and flips the sign.
puts (-1i)   #=> 0-1i
puts (-2.5i) #=> 0-2.5i
