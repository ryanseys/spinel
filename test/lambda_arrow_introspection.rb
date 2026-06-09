# After arrow lambdas are backed by the first-class Proc runtime they
# carry the same metadata as proc/lambda literals: arity, lambda?, and
# parameters all report correctly, alongside ordinary multi-arg calls.
add = ->(a, b) { a + b }
puts add.arity
puts add.lambda?
puts add.call(4, 9)

nullary = -> { 42 }
puts nullary.arity
puts nullary.lambda?

square = ->(x) { x * x }
puts square.parameters.inspect
