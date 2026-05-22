# Pattern matching: ArrayPatternNode with full binding support.
#
# Covers literals, LocalVariableTarget bindings, rest splats (with
# and without binding), posts (post-rest fixed elements), and the
# common typed-array variants (int_array, str_array, sym_array).

# --- Literal-only requireds
case [1, 2, 3]
in [1, 2, 3]
  puts "lit exact"
in [1, 2, 4]
  puts "off"
end

# --- Length mismatch falls through
case [1, 2]
in [1, 2, 3]
  puts "len3"
in [1, 2]
  puts "len2"
end

# --- Bindings: simple positional
case [10, 20, 30]
in [a, b, c]
  puts a
  puts b
  puts c
end

# --- Mix of literal + binding
case [1, 99, 3]
in [1, lit_x, 3]
  puts "mix"
  puts lit_x
end

# --- Rest splat with binding (int_array)
case [1, 2, 3, 4, 5]
in [int_head, *int_tail]
  puts int_head
  puts int_tail.length
  puts int_tail.first
end

# --- Rest splat anonymous
case [10, 20, 30]
in [anon_head, *]
  puts anon_head
end

# --- Rest + posts (binding both sides)
case [1, 2, 3, 4, 5]
in [mid_a, *mid_rest, mid_z]
  puts mid_a
  puts mid_rest.length
  puts mid_z
end

# --- Posts only (rest empty / no binding)
case [1, 9, 9]
in [1, *_, post_last]
  puts post_last
end

# --- String array bindings (different names to avoid type collision)
case ["alpha", "beta", "gamma"]
in [str_head, *str_tail]
  puts str_head
  puts str_tail.length
end

# --- Symbol array bindings
case [:a, :b, :c]
in [sym_x, sym_y, sym_z]
  puts sym_x
  puts sym_y
  puts sym_z
end

# --- Empty array
case []
in []
  puts "empty"
end

# --- No match falls to else
case [1, 2, 3]
in [9, 9, 9]
  puts "nine"
in [1, 1, 1]
  puts "one"
else
  puts "no match"
end

# --- Four-element binding
case [1, 2, 3, 4]
in [four_a, four_b, four_c, four_d]
  puts four_a + four_b + four_c + four_d
end
