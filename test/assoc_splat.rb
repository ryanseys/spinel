# AssocSplatNode -- {a: 1, **other, c: 3}
#
# Hash literal merges another hash's entries inline. Spinel's static
# hash typing already paid the polymorphism cost for mixed-value
# literals (sp_SymPolyHash / sp_StrPolyHash exist for {a: 1, b: "x"}
# style hashes). AssocSplat is one new trigger for the existing
# PolyHash path: infer flips to *_poly_hash on splat presence, and
# compile_hash_literal emits sp_*PolyHash_merge per AssocSplatNode.

# Symbol-keyed merge: {a: 1, **{b: 2}, c: 3} -> sym_poly with three keys
src = {b: 2}
merged = {a: 1, **src, c: 3}
puts merged[:a].to_s   #=> 1
puts merged[:b].to_s   #=> 2
puts merged[:c].to_s   #=> 3

# Override: rightmost key wins (CRuby Hash#merge semantics)
override = {a: 99}
later = {a: 1, **override}
puts later[:a].to_s    #=> 99

# Splat at the head
head_splat = {**src, a: 1}
puts head_splat[:a].to_s   #=> 1
puts head_splat[:b].to_s   #=> 2

# Mixed value types prove poly path
mixed = {n: 1, **{s: "two"}}
puts mixed[:n].to_s    #=> 1
puts mixed[:s]         #=> two
