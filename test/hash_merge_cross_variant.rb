# Issue #661: Hash#merge across hash variants with different value
# types. Previously the codegen used the receiver's prefix
# unconditionally (sp_SymIntHash_merge(h1, h2) where h2 was
# sp_SymStrHash *), failing C compile.
#
# Fix: when recv and arg are both sym-keyed (or both str-keyed) but
# their value types differ, the analyzer pulls the call's return
# type up to sym_poly_hash / str_poly_hash, and codegen promotes
# both sides via the existing *_to_sym_poly / *_from_str_int_hash
# converters before dispatching sp_*PolyHash_merge.

# Sym + Sym with mixed value types.
h1 = { a: 1, b: 2 }
h2 = { c: "x", d: "y" }
h3 = h1.merge(h2)
puts h3[:a].inspect
puts h3[:c].inspect

# Str + Str with mixed value types.
s1 = { "a" => 1, "b" => 2 }
s2 = { "c" => "x", "d" => "y" }
s3 = s1.merge(s2)
puts s3["a"].inspect
puts s3["c"].inspect

puts "ok"
