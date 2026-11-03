# Data#with on a Data with no initialize of its own applies every keyword
# and every `**hash` in order, the last one naming a member winning as in
# a Hash literal. It read only the last `**hash`, and a literal keyword
# won over a `**hash` after it.
D = Data.define(:a, :b, :c)
d = D.new(a: 1, b: 2, c: 3)
h1 = {a: 10}
h2 = {b: 20}
p d.with(**h1, **h2)
h = {a: 100}
p d.with(a: 1000, **h)
p d.with(**h, a: 1000)
x = 7
p d.with(c: x, **h1)
ab = {a: 5, b: 6}
b7 = {b: 7}
p d.with(**ab, **b7)
p d.with(b: 0, **h1, c: 9, **h2)
p d.with(**{}, b: 4, **{})
both = {a: -1, c: -3}
p d.with(a: 0, b: 0, c: 0, **h1, **both)

S = Data.define(:name, :tag)
s = S.new(name: "n", tag: :t)
nh = {name: "from-hash"}
p s.with(name: "lit", **nh)
tv = {tag: :v}
p s.with(**nh, tag: :u, **tv)
p s.with(**nh, **{name: "later"})

N = Data.define(:v, :w)
n = N.new(v: nil, w: 2)
p n.with(**{w: 5})
w6 = {w: 6}
p n.with(w: 1, **w6)
