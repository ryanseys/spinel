# IndexAndWriteNode -- `a[i] &&= val`.
#
# Reads `a[i]`; if truthy, sets a[i] = val. Mirror of IndexOrWriteNode
# with condition inverted. Same temp pattern keeps receiver and
# index evaluated exactly once.

# Int array with non-zero slots: avoid the C-truthy vs Ruby-truthy
# divergence by sticking to non-zero values that are truthy in both.
xs = [1, 2, 3, 4, 5]
xs[0] &&= 10                  # truthy -> fires
xs[2] &&= 30                  # truthy -> fires
xs[4] &&= xs[4] * 100         # truthy -> fires; receiver/index evaluated once

puts xs[0]                    # 10
puts xs[1]                    # 2 (untouched)
puts xs[2]                    # 30
puts xs[3]                    # 4 (untouched)
puts xs[4]                    # 500

# Hash with int values: each set value is non-zero, so &&= behaves
# the same in C and Ruby for these slots.
counts = {"alice" => 30, "bob" => 25}
counts["alice"] &&= 31        # truthy -> fires
counts["carol"] &&= 50        # never-set (sp_StrIntHash_get returns 0) -> doesn't fire
puts counts["alice"]          # 31
puts counts["bob"]            # 25 (untouched)
puts counts.length            # 2 (carol not added)
