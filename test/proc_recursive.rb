fact = proc { |n| n <= 1 ? 1 : n * fact.call(n - 1) }
p fact.call(5)

fib = proc { |n| n < 2 ? n : fib.call(n - 1) + fib.call(n - 2) }
p fib.call(10)

f = ->(n) { n <= 1 ? 1 : n * f.call(n - 1) }
p f.call(6)
