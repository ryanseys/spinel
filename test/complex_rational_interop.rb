# Mixed Complex/Rational arithmetic keeps exact Rational components (see
# complex_exact_components.rb). This pin normalizes each component through
# #to_f, so it checks the numeric value independently of the component class.
def comps(c)
  [c.real.to_f, c.imaginary.to_f]
end
p comps(Complex(1, 2) + Rational(1, 2))
p comps(Complex(1, 2) - Rational(1, 2))
p comps(Complex(1, 2) * Rational(1, 2))
p comps(Complex(1, 2) / Rational(1, 2))
p comps(Rational(1, 2) + Complex(1, 2))
p comps(Rational(3, 2) * Complex(2, 4))
p comps(Complex(Rational(1, 2), Rational(1, 3)))
p comps(Complex(Rational(1, 2)))
p comps(Complex(3, 4).quo(2))
p comps(Complex(3, 4).quo(Rational(1, 2)))
p(10.quo(4))
