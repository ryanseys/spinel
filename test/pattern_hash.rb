# Pattern matching: HashPatternNode (`case x in {a:, b: 1, **rest}`).
#
# Per CRuby spec, hash patterns require Symbol keys in the scrutinee.
# Spinel supports the three sym-keyed typed-Hash variants
# (sym_int_hash / sym_str_hash / sym_poly_hash); non-Symbol-keyed
# hashes never match the pattern.

# --- sym_int_hash: bind shorthand
case {a: 1, b: 2, c: 3}
in {a: hi_a, b: hi_b}
  puts hi_a
  puts hi_b
end

# --- sym_int_hash: literal value match
case {x: 10, y: 20}
in {x: 10, y: 20}
  puts "exact"
end

# --- sym_int_hash: literal mismatch falls through
case {x: 1, y: 2}
in {x: 1, y: 9}
  puts "y9"
in {x: 1, y: 2}
  puts "y2"
end

# --- Missing key falls through
case {a: 1}
in {b: hi_missing}
  puts "got b"
else
  puts "no b"
end

# --- Mix literal + binding
case {name: "alice", age: 30}
in {name: hi_name, age: 30}
  puts hi_name
end

# --- sym_str_hash
case {city: "tokyo", country: "jp"}
in {city: hs_city, country: hs_country}
  puts hs_city
  puts hs_country
end

# --- sym_poly_hash (mixed values)
case {kind: :ok, code: 200, msg: "fine"}
in {kind: hp_kind, code: hp_code, msg: hp_msg}
  puts hp_kind
  puts hp_code
  puts hp_msg
end

# --- **rest binding
case {a: 1, b: 2, c: 3, d: 4}
in {a: hr_a, **hr_rest}
  puts hr_a
  puts hr_rest.length
end
