# Rational() with a Bignum operand builds an exact bigint-backed value:
# construction reduces (positive denominator), to_f divides the exact
# components, printing renders (n/d), and a zero denominator raises.
def tf(x) = x.to_f
p tf(Rational(2 ** 64, 1))
p Rational(2 ** 64, 1)
puts Rational(2 ** 64, 1)
p Rational(2 ** 65, 2 ** 64)
p Rational(2 ** 64, 2)
p(Rational(2 ** 65, 2) == Rational(2 ** 64, 1))
p(Rational(2 ** 64, 1) == Rational(2 ** 64, 2))
p Rational(2 ** 64, 1).to_s
begin
  Rational(2 ** 64, 0)
rescue ZeroDivisionError => e
  puts "#{e.class}: #{e.message}"
end
