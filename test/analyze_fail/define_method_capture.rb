# A direct `define_method(:name) { ... }` body compiles as a standalone
# method, so it cannot close over a local from the enclosing class body
# (only the literal-unrolled `.each` form substitutes its loop variable).
# Reject precisely instead of emitting undeclared-identifier C.
class K
  n = 7
  define_method(:g) { n }
end

puts K.new.g
