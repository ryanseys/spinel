# AssocSplatNode edge cases: chained splats, mixed source types,
# deeply nested merges, splat-only literals.

# Chained splats: rightmost wins per CRuby Hash#merge semantics.
a = {x: 1, y: 2}
b = {y: 99, z: 3}
chained = {**a, **b}
puts chained[:x].to_s   #=> 1
puts chained[:y].to_s   #=> 99    (b's y wins over a's y)
puts chained[:z].to_s   #=> 3

# Splat-only literal: {**other} clones the hash entries into a fresh poly.
src = {p: 10, q: 20, r: 30}
clone = {**src}
puts clone[:p].to_s     #=> 10
puts clone[:q].to_s     #=> 20
puts clone[:r].to_s     #=> 30

# Three-way chain with override at the end.
base = {name: "default", count: 0}
extra = {count: 5}
final = {**base, **extra, count: 100}
puts final[:name]       #=> default
puts final[:count].to_s #=> 100   (the trailing literal wins)

# Splat from a hash whose value type differs from literal entries.
# infer_hash_val_type forces sym_poly because the splat operand
# carries unknown value types at compile time.
nums = {a: 1, b: 2}
mixed = {first: "hi", **nums}
puts mixed[:first]      #=> hi
puts mixed[:a].to_s     #=> 1
puts mixed[:b].to_s     #=> 2

# Splat-into-empty-with-trailing-literal: trailing literal still wins.
trailing = {**{}, only: 42}
puts trailing[:only].to_s   #=> 42
