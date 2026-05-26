# Hash#inspect on poly-valued hashes (sym_poly / str_poly) with large,
# heterogeneous values. Exercises the accumulator-string append loop in
# sp_SymPolyHash_inspect / sp_StrPolyHash_inspect at scale — the helpers
# that root their accumulator string with SP_GC_ROOT for GC-safety. The
# existing test/hash_inspect_poly.rb only covers 3-entry hashes; this adds
# a multi-hundred-KB value to keep the full result intact under heavy
# allocation. Assert exact length plus the head/tail so a truncation or
# mis-format anywhere in the body is caught.

big = "z" * 300000
h = { a: big, b: 1, c: 2.5, d: :sym, e: "tail" }
s = h.inspect
puts s.length
puts s.start_with?("{a: \"zzz")
puts s.end_with?(", b: 1, c: 2.5, d: :sym, e: \"tail\"}")

bigs = "y" * 300000
g = { "a" => bigs, "b" => 1, "c" => :s }
t = g.inspect
puts t.length
puts t.start_with?("{\"a\"=>\"yyy")
puts t.end_with?(", \"b\"=>1, \"c\"=>:s}")
