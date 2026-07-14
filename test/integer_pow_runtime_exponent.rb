# Integer ** with a runtime exponent resolves its result class by value,
# like CRuby: Bignum on overflow, Rational for a negative exponent, plain
# Integer otherwise — instead of raising RangeError at the int64 boundary.
def f(n) = 2 ** n
p f(100)
p f(10)
p f(1)
p f(0)
p f(100) + 1
p(f(10) == 1024)
puts "power: #{f(100)}"

def g(base, e) = base ** e
p g(2, -2)
p g(2, -1)
p g(3, 4)
p g(-2, 3)
p g(-2, -3)
p g(10, 30)

begin
  p g(0, -1)
rescue ZeroDivisionError => e
  puts "#{e.class}: #{e.message}"
end
