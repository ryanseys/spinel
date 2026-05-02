# ClassVariableOperatorWriteNode -- `@@x += val`, `@@x -= val`, etc.
#
# Mirror of GlobalVariableOperatorWriteNode but the storage is the
# per-(class,name) cvar slot routed through cvar_qname rather than
# sanitize_gvar.

class Stats
  @@total = 0
  @@max = 0

  def self.record(n)
    @@total += n          # +=
    @@max *= 2 if n > @@max
    @@max = n if n > @@max
  end

  def self.report
    [@@total, @@max]
  end
end

Stats.record(5)
Stats.record(10)
Stats.record(3)

t, m = Stats.report
puts t                   # 18 (5 + 10 + 3)
puts m                   # 10

# All-operator coverage on a single cvar.
class Calc
  @@x = 100
  def self.add(n);  @@x += n; end
  def self.sub(n);  @@x -= n; end
  def self.mul(n);  @@x *= n; end
  def self.div(n);  @@x /= n; end
  def self.modulo(n); @@x = @@x % n; end
  def self.shl(n);  @@x <<= n; end
  def self.shr(n);  @@x >>= n; end
  def self.bor(n);  @@x |= n; end
  def self.band(n); @@x &= n; end
  def self.bxor(n); @@x ^= n; end
  def self.value;   @@x; end
end

Calc.add(50)             # 150
puts Calc.value          # 150
Calc.sub(30)             # 120
puts Calc.value          # 120
Calc.mul(2)              # 240
puts Calc.value          # 240
Calc.div(4)              # 60
puts Calc.value          # 60
Calc.shl(1)              # 120
puts Calc.value          # 120
Calc.shr(2)              # 30
puts Calc.value          # 30
Calc.bor(1)              # 31
puts Calc.value          # 31
Calc.band(15)            # 15
puts Calc.value          # 15
Calc.bxor(255)           # 240
puts Calc.value          # 240
