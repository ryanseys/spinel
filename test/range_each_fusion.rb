# Range#each over a literal numeric range should fuse to a tight C
# for-loop with bounds inlined. Output is identical to the unfused
# path; this test pins behavioural parity. The perf win shows up in
# the generated C — no sp_Range struct copy on the loop's hot path.

# Inclusive range, accumulator
sum = 0
(1..10).each { |i| sum = sum + i }
puts sum

# Exclusive range — last value is excluded
sum_excl = 0
(1...10).each { |i| sum_excl = sum_excl + i }
puts sum_excl

# Negative bounds
neg_sum = 0
(-3..3).each { |i| neg_sum = neg_sum + i }
puts neg_sum

# Single-element inclusive range
hits = 0
(7..7).each { |i| hits = hits + 1 }
puts hits

# Empty exclusive range
empties = 0
(5...5).each { |i| empties = empties + 1 }
puts empties

# Block param shadows nothing — fresh local
count = 0
(1..100).each { |i| count = count + 1 }
puts count

# Nested ranges
total = 0
(1..3).each { |i| (1..3).each { |j| total = total + i * j } }
puts total
