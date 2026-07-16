# Comparable operators/clamp validate the user <=> result type: a Float
# result compares by sign, a non-numeric result raises ArgumentError.
class FDiff
  include Comparable
  attr_reader :n
  def initialize(n); @n = n; end
  def <=>(o); (@n - o.n).to_f; end
end
p(FDiff.new(9) < FDiff.new(5))
p(FDiff.new(5).between?(FDiff.new(1), FDiff.new(9)))
p(FDiff.new(12).clamp(FDiff.new(1), FDiff.new(9)).n)
class Bad
  include Comparable
  def <=>(o); "x"; end
end
a = Bad.new; b = Bad.new
p((a < b rescue $!.class))
p((a == b rescue $!.class))
