# FlipFlopNode edge cases beyond the basic awk-style filter.
#
# Verifies:
# - Same-tick close: when left and right both fire on the same eval,
#   inclusive (..) closes immediately, exclusive (...) stays open.
# - Right never fires: flipflop stays open through the end of the
#   iteration once activated.
# - Left fires multiple times before right: only the first activation
#   matters; subsequent left-true evaluations don't restart the range.

# Same-tick close: i==5 makes both sides true. Inclusive .. closes
# immediately on the same tick; exclusive ... requires a separate tick.
i = 0
incl_count = 0
excl_count = 0
while i < 10
  if (i == 5)..(i == 5)
    incl_count = incl_count + 1
  end
  if (i == 5)...(i == 5)
    excl_count = excl_count + 1
  end
  i = i + 1
end
puts incl_count.to_s    #=> 1   (closes same tick, only i=5 counts)
puts excl_count.to_s    #=> 5   (stays open from i=5 to end)

# Multi-trigger left: once activated, additional left-true evals don't
# extend or reset the range. Only right closes it.
i = 0
hits = 0
while i < 10
  if (i % 2 == 0)..(i == 7)
    hits = hits + 1
  end
  i = i + 1
end
puts hits.to_s           #=> 8  (i=0,1,2,3,4,5,6,7 -- left fires every
                          # even but state stays open until i=7 closes)

# Three independent flipflops in distinct positions maintain
# independent state -- proves per-nid slot allocation.
i = 0
a_hits = 0; b_hits = 0; c_hits = 0
while i < 6
  a_hits = a_hits + 1 if (i == 1)..(i == 3)
  b_hits = b_hits + 1 if (i == 2)..(i == 4)
  c_hits = c_hits + 1 if (i == 0)..(i == 5)
  i = i + 1
end
puts a_hits.to_s         #=> 3   (i=1,2,3)
puts b_hits.to_s         #=> 3   (i=2,3,4)
puts c_hits.to_s         #=> 6   (i=0..5 inclusive)

# Range that opens but right is unreachable: stays active forever.
i = 0
opens = 0
while i < 4
  opens = opens + 1 if (i == 0)..(i == 999)
  i = i + 1
end
puts opens.to_s          #=> 4   (open through full loop)
