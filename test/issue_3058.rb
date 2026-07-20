r = Random.new(5)
v = r.rand(2 ** 70)
p v.class
p v >= 0
p v < 2 ** 70
p rand(10 ** 30).class
p Random.rand(10 ** 25).class
p((r.rand(2 ** 70) != r.rand(2 ** 70)) || true)
