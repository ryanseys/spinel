# A proc can capture an enclosing inlined-block's loop variable by value: each
# iteration's closure snapshots its own binding, matching CRuby's fresh-per-
# iteration block parameter.

# Immediate call: the proc reads the current iteration's value.
def sum_via_proc
  total = 0
  [10, 20, 30].each { |a| total += proc { a }.call }
  total
end
p sum_via_proc

# Escaping closures: each captured proc keeps its own value (the classic
# closure-in-a-loop idiom).
procs = [1, 2, 3].map { |x| proc { x } }
p procs.map(&:call)

# Escaping via <<, with an expression in the body.
collected = []
[1, 2, 3].each { |n| collected << proc { n * n } }
p collected.map(&:call)

# Non-local return from a proc that also captures the block var.
def first_big
  [10, 20, 30].each { |a| return proc { a + 1 }.call if a >= 20 }
  -1
end
p first_big

# The reported case: a returning proc capturing the each block's loop var.
def reported
  [10, 20].each { |a| proc { return a if a == 20 }.call }
  :done
end
p reported

# String block var captured and used (direct call).
["x", "y", "z"].each { |s| puts proc { s.upcase }.call }

# each_with_index loop vars captured by value.
def index_sum
  acc = 0
  [5, 6, 7].each_with_index { |v, i| acc += proc { v + i }.call }
  acc
end
p index_sum

# Nested inlined blocks: the inner proc captures the outer block's var.
def nested_capture
  out = []
  [1, 2].each { |a| [10, 20].each { |b| out << proc { a + b }.call } }
  out
end
p nested_capture
