pr = proc { |*a| a }
r = pr.ruby2_keywords
p(r.equal?(pr))
p(pr.call(1, 2))
