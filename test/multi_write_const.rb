# Multi-write to constants — `A, B, C = expr` — used to silently
# drop the targets because the parser emitted ConstantTargetNode as
# `UnknownNode_43` and the codegen had no MultiWriteNode branch in
# the constant-collection passes. Subsequent reads of A/B/C compiled
# to undefined-identifier errors.
#
# Two RHS shapes are exercised: a literal ArrayNode (split element
# by element at emit time) and a call returning int_array (evaluated
# once into a temp, then sp_IntArray_get for each target).

class C
  N1, N2, N3 = [10, 20, 30]
end

puts C::N1
puts C::N2
puts C::N3

class D
  ARR = [100, 200, 300, 400]
  M1, M2, M3, M4 = ARR
end

puts D::M1
puts D::M2
puts D::M3
puts D::M4

# RHS with a block — the block param introduces a local that
# must be declared in main() (or the enclosing scope) so the
# emitted `lv_<bp>` reference resolves. Single-const init already
# scans @const_expr_ids; the multi-write form needs the same scan
# over @multi_const_inits.
P, Q = [1, 2].map { |n| n * 10 }
puts P
puts Q
