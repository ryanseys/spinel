# Forwarding a top-level Method to an array iterator: `arr.each(&method(:m))`
# and `arr.map(&method(:m))` / `.collect` call the method once per element
# through its own typed C signature (no mrb_int laundering). The result of a
# #map is decoded by the method's return type. (Bound instance methods —
# `recv.method(:m)` — are not covered.)

def square(x)
  x * x
end

# #map(&method(:m)): collect per-element results.
p [1, 2, 3].map(&method(:square))

# #collect alias.
def succ_of(n)
  n + 1
end
p [10, 20].collect(&method(:succ_of))

# #each(&method(:m)): called for side effects.
def announce(x)
  puts "got #{x}"
end
[1, 2, 3].each(&method(:announce))

# Through a local binding holding the Method.
sq = method(:square)
p [4, 5].map(&sq)

# Method returning a String -> the mapped array collects strings.
def label(n)
  "n=" + n.to_s
end
p [1, 2, 3].map(&method(:label))
