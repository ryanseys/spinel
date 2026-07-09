# Integer ** / Integer#pow with a statically-NEGATIVE literal exponent evaluates
# to a Rational (CRuby: 2 ** -2 == (1/4)). A non-negative or runtime exponent
# keeps the plain-Integer result -- typing every `base ** k` as a
# sometimes-Rational would force the result poly and cascade through inference.
# (A runtime negative exponent still raises RangeError, a deliberate divergence
# documented in docs/limitations.md.)
puts 2 ** 10
puts 0 ** 0
puts 3 ** 4
puts(2.0 ** -1)

p 2 ** -1
p 2 ** -2
p 10 ** -3
p 2.pow(-2)

# a non-negative runtime exponent stays a plain Integer (base ** index)
def ipow(b, e) = b ** e
p ipow(2, 3)
p ipow(5, 0)

# a literal non-negative exponent stays a plain Integer
n = 4
p n ** 2

# modular pow is unaffected
p 2.pow(10, 1000)
