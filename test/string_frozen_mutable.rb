puts String.new("x").frozen?

s = String.new("hi")
puts s.frozen?
s << " there"
puts s.frozen?
puts s
