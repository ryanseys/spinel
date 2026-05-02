# FlipFlopNode -- `if a..b` in conditional context.
#
# Per-site bistable state: each FlipFlop syntax position gets its own
# `static int sp_ff_<id>` slot at file scope. Spinel's AOT model fits
# the per-site requirement exactly -- no fiber-local or per-frame state
# needed; the C linker handles allocation. The classic awk idiom prints
# lines from "BEGIN" through "END" markers.

# Inclusive form (..): emits true on the opening tick AND the closing.
i = 0
while i < 10
  if (i == 3)..(i == 7)
    puts i.to_s
  end
  i = i + 1
end
#=> 3
#=> 4
#=> 5
#=> 6
#=> 7

# Distinct sites maintain independent state -- nesting two flipflops
# proves the per-nid slot allocation works.
i = 0
out_count = 0
in_count = 0
while i < 10
  if (i == 1)..(i == 8)
    out_count = out_count + 1
    if (i == 3)..(i == 5)
      in_count = in_count + 1
    end
  end
  i = i + 1
end
puts out_count.to_s   #=> 8 (i=1..8 inclusive)
puts in_count.to_s    #=> 3 (i=3..5 inclusive)

# Exclusive form (...): the closing right-evaluation does NOT collapse
# into the opening tick. (1==1)...(false) stays open until the right
# side fires.
seen = 0
i = 0
while i < 5
  if (i == 0)...(i == 99)
    seen = seen + 1
  end
  i = i + 1
end
puts seen.to_s   #=> 5 (right never fires; flipflop stays open)
