# Pattern matching: FindPatternNode (`case x in [*pre, mid, *post]`).
#
# Scans the array for an offset where the literal middle elements
# match; binds the pre-splat to the prefix slice and the post-splat
# to the suffix slice. Middle elements can be literals (constrained
# scan) or binding targets (wildcards that bind after find succeeds).

# --- Literal middle, both splats bound
case [1, 2, 3, 4, 5]
in [*find_pre1, 3, *find_post1]
  puts find_pre1.length
  puts find_post1.length
end

# --- Two-element literal middle
case [10, 20, 30, 40, 50]
in [*find_pre2, 30, 40, *find_post2]
  puts find_pre2.length
  puts find_post2.length
end

# --- Anonymous splats with literal middle
case [1, 2, 99, 4, 5]
in [*, 99, *]
  puts "found 99"
else
  puts "no 99"
end

# --- No-match falls to else
case [1, 2, 3]
in [*, 9, *]
  puts "nine"
else
  puts "no nine"
end

# --- Middle is a binding (acts as wildcard, binds after find)
case [10, 20, 30]
in [*find_pre3, find_mid_x, *find_post3]
  puts find_pre3.length
  puts find_mid_x
  puts find_post3.length
end

# --- Match at the very start (empty pre)
case [9, 1, 2]
in [*find_start_pre, 9, *find_start_post]
  puts find_start_pre.length
  puts find_start_post.length
end

# --- Match at the very end (empty post)
case [1, 2, 9]
in [*find_end_pre, 9, *find_end_post]
  puts find_end_pre.length
  puts find_end_post.length
end

# --- String array find
case ["a", "b", "MARK", "c"]
in [*find_str_pre, "MARK", *find_str_post]
  puts find_str_pre.length
  puts find_str_post.length
end
