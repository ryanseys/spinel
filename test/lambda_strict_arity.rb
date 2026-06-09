# A lambda raises ArgumentError on the wrong argument count and aborts,
# so "after" is never printed (matching CRuby). stderr differs but the
# suite compares stdout only.
puts "before"
f = ->(x) { x }
f.call(1, 2)
puts "after"
