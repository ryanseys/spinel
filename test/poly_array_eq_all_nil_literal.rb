# `<poly_array> == [nil, nil, ...]` shape. The RHS array literal
# would otherwise lower to int_array of zeros and fall through to
# a raw pointer compare. The contextual lift at the == call site
# rebuilds the RHS as a poly_array of box_nil() and dispatches
# through sp_PolyArray_eq, matching the CRuby semantic that
# `Array.new(N) == [nil] * N` is true. Issue #619 puzzle 4.
p Array.new(3) == [nil, nil, nil]   # true
p Array.new(0) == []                # true (empty arrays: handled by the IntArray fallback)
p Array.new(2) == [nil, nil]        # true
p Array.new(3) != [nil, nil, nil]   # false
