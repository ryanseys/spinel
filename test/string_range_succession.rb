# A string-endpoint range lowers to its succession array (so it works through
# parameters), while the literal's Range identity survives: inspect/to_s
# format the endpoints and === / cover? are the lexicographic cover check.
def ta(x) = x.to_a
p ta("A".."D")
p ta("a"..."d")

def fr(x) = x.first
p fr("aa".."ac")

p ("a".."e").include?("c")
p ("a".."e").include?("cat")
p ("a".."m") === "cat"
p ("a".."m").cover?("cat")
p ("A".."C") === 20
p ("a"..."m") === "m"
p [*"x".."z"]
p ("a".."c").inspect
puts ("a"..."d").to_s
p ("a".."c")
acc = []
("a".."c").each { |s| acc << s }
p acc
p ("aa".."ad").map { |s| s.upcase }
