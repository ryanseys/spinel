# [WIP] KNOWN-FAILING INVESTIGATION RECORD -- do not upstream.
# Lives under wip/ (not test/) so the Makefile's `wildcard test/*.rb` never
# picks it up and CI stays green. Run by hand: `./bin/spinel wip/poly_slice_nested_gap.rb`.
#
# BUG: a poly-array rest-slice pattern (`in [*pre, last]`) renders `pre` as
# garbage (a 16-element array of raw pointers/zeros) when the poly array's
# inferred element union includes a NESTED ARRAY type (e.g. Int | Str | IntArray).
#
# It reproduces with ZERO pattern-matching of the nested elements -- purely from
# element-type widening of the shared `poly` helper. The bug is in the poly-array
# slice + boxing path (sp_poly_slice / how the sliced poly array is boxed for the
# binding target), NOT in posts-binding (which is correct -- see matz PR #1802).

def poly(a) = a

# Widen poly()'s inferred element type to Int | Str | IntArray via extra call
# sites. None of these are pattern-matched; they only affect inference.
_z1 = poly([1, 2, [3, 4]])
_z2 = poly([1, 2, 5])
_z3 = poly([1, [2, 3]])
_z4 = poly([1, [2, 3], 4])

# The single pattern match. Expected (CRuby 4.0.5): [[1, "x", 3], 4].
# Actual (spinel): pre = garbage, e.g. [[<ptr>, 0, 3, 0, 0, ...], 4].
case poly([1, "x", 3, 4])
in [*pre, last]; p [pre, last]; end

# CONTROL: remove the _z widening calls above and this prints correctly,
# which is why matz PR #1802's test isolates its nested cases behind a separate
# `pnest` helper to keep `poly` narrow.
