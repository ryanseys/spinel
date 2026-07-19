r = ([[1, 2], [3]].transpose rescue $!.class); p r
p([[1, 2], [3, 4]].transpose)
p([].transpose)
p([[1], [2], [3]].transpose)
