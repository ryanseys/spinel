# Numeric tower remainder: Integer#[start, len] (the len-bit field starting at
# bit `start`) and Math.lgamma (which returns [log(|gamma(x)|), sign]). The
# integer receiver goes through a method param to exercise the runtime path.
def i(x); x; end

# bit-range field extraction
p i(0b1011010)[1, 3]
p i(255)[0, 4]
p i(255)[4, 4]
p i(0b110100)[2, 4]
p i(42)[2]

# Out-of-range start/len must not trigger an undefined shift: a len at/above
# the word width keeps every shifted bit, a non-positive len or a start past
# the width behaves as CRuby's two's-complement model does.
def br(n, s, l); n[s, l]; end
p br(255, 2, 100)
p br(5, 2, -1)
p br(5, -1, 3)
p br(255, 0, -5)
p br(-1, 60, 8)
p br(255, 100, 4)
p br(1, 64, 1)
p br(7, 0, 0)

# Math.lgamma -> [value, sign]. The value is computed in-tree (Stirling's
# series + the Gamma recurrence) and ultimately depends on libm log()/sin(),
# whose last ULP is not portable across implementations, so assert it within a
# tight tolerance rather than pinning the exact bits. The exact points
# (lgamma(1) = lgamma(2) = 0) and the sign are bit-stable and checked directly.
g5 = Math.lgamma(5)                                  # ~ log(24)
p(g5[0] > 3.1780538303 && g5[0] < 3.1780538304)
p g5[1]
gh = Math.lgamma(0.5)                                # ~ log(sqrt(pi))
p(gh[0] > 0.5723649429 && gh[0] < 0.5723649430)
p gh[1]
p Math.lgamma(1)
p Math.lgamma(2)

# Edge cases match CRuby exactly: +Infinity and every non-positive integer are
# poles -> [Infinity, 1]; -Infinity raises Math::DomainError. The exact values
# and signs here are bit-stable, so they're checked directly.
p Math.lgamma(Float::INFINITY)
p Math.lgamma(-1.0)
p Math.lgamma(-2.0)
gm = Math.lgamma(-0.5)                                # ~ log(2*sqrt(pi)), sign -1
p(gm[0] > 1.2655121234 && gm[0] < 1.2655121235)
p gm[1]
begin
  Math.lgamma(-Float::INFINITY)
  p "no raise"
rescue Math::DomainError
  p "domain error"
end
