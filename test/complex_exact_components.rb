# Complex arithmetic against a Rational (or with a Rational component) keeps
# exact Rational components like CRuby, rather than coercing to machine floats.
# Receivers/arguments flow through method parameters so the runtime path (not
# constant folding) is exercised. Each helper is monomorphic so the compiler
# keeps concrete Complex/Rational types rather than unifying to a poly.
def mcr(a, b) = a * b   # Complex(int) * Rational
def mcf(a, b) = a * b   # Complex(float) * Rational
def mrc(a, b) = a * b   # Rational * Complex
def arc(a, b) = a + b   # Rational + Complex
def acr(a, b) = a + b   # Complex + Rational
def scr(a, b) = a - b   # Complex - Rational
def dci(a, b) = a / b   # Complex / Integer
def dcr(a, b) = a / b   # Complex / Rational
def qci(a, b) = a.quo(b)
def qcr(a, b) = a.quo(b)
def cxc(a, b) = a * b   # Complex * Complex
def eqc(a, b) = a == b

# Complex * Rational: an integer receiver keeps (1/1) under multiplication.
p mcr(Complex(1, 2), Rational(1, 2))            # ((1/2)+(1/1)*i)
p mcr(Complex(1, 2), Rational(3, 2))            # ((3/2)+(3/1)*i)
p mcr(Complex(1, -2), Rational(1, 2))           # ((1/2)+(-1/1)*i)
# Float receiver component: scalar (component-wise) mul preserves the exact part.
p mcf(Complex(0.5, 1), Rational(1, 2))          # (0.25+(1/2)*i)
# Rational * Complex (commutative; scales the Complex component-wise).
p mrc(Rational(1, 2), Complex(1, 2))            # ((1/2)+(1/1)*i)
p mrc(Rational(3, 2), Complex(2, 4))            # (3+6i)

# Rational + Complex and Complex + Rational; subtraction.
p arc(Rational(1, 2), Complex(1, 2))            # ((3/2)+2i)
p acr(Complex(1, 2), Rational(1, 2))            # ((3/2)+2i)
p acr(Complex(1, 2), Rational(-3, 2))           # ((-1/2)+2i)
p scr(Complex(1, 2), Rational(3, 2))            # ((-1/2)+2i)

# Division by an Integer canonicalizes whole components to Integer.
p dci(Complex(1, 2), 2)                          # ((1/2)+1i)
p dci(Complex(2, 4), 2)                          # (1+2i)
# Division by a Rational.
p dcr(Complex(1, 2), Rational(1, 2))            # (2+4i)
p dcr(Complex(2, 4), Rational(2, 1))            # (1+2i)
# quo mirrors division.
p qci(Complex(3, 4), 2)                          # ((3/2)+2i)
p qcr(Complex(3, 4), Rational(1, 2))            # (6+8i)

# Constructor with Rational components.
p Complex(Rational(1, 2), 0)                     # ((1/2)+0i)
p Complex(Rational(-1, 2), Rational(1, 3))       # ((-1/2)+(1/3)*i)
p Complex(Rational(1, 2))                        # ((1/2)+0i)

# Component classes: an exact component reports Rational and coerces exactly.
p mcr(Complex(1, 2), Rational(1, 2)).real.class       # Rational
p mcr(Complex(1, 2), Rational(1, 2)).imaginary.class  # Rational
p mcr(Complex(1, 2), Rational(1, 2)).real.to_f        # 0.5
p mcr(Complex(1, 2), Rational(1, 2)).imaginary.to_f   # 1.0

# to_s / puts render the bare (no-paren) rational form.
p mcr(Complex(1, 2), Rational(1, 2)).to_s        # "1/2+1/1i"
puts mcr(Complex(1, -2), Rational(1, 2))         # 1/2-1/1i

# Equality across representations.
p eqc(mcr(Complex(1, 2), Rational(1, 2)), Complex(Rational(1, 2), 1))  # true
p eqc(Complex(Rational(1, 2), 0), Complex(0.5, 0))                     # true
p eqc(Complex(1, 2), Complex(1.0, 2.0))                               # true

# Regressions: genuine Complex * Complex uses the full cross formula; plain
# integer/float Complexes and abs are unchanged.
p cxc(Complex(1, 2), Complex(3, 4))              # (-5+10i)
p Complex(1, 2)                                  # (1+2i)
p Complex(1.5, 2)                                # (1.5+2i)
p Complex(3, 4).abs                              # 5.0
