K = Struct.new(*%i[a b].reverse)
x = K.new("foo")
p x.a
p x.b
ks = %i[c a b]
K2 = Struct.new(*ks.sort)
y = K2.new(1, 2, 3)
p y.a
p y.b
p y.c
K3 = Struct.new(*%i[x x y].uniq)
p K3.new(1, 2).x
p K3.new(1, 2).y
