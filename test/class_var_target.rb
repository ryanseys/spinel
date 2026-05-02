# ClassVariableTargetNode -- multi-assign LHS for class vars.
#
# `@@a, @@b = 1, 2` -- each LHS slot is a ClassVariableTargetNode
# routed through emit_multi_write_target's new arm to the same
# cvar_<ClassName>_<var> slot ClassVariableWriteNode uses.

class Pair
  @@a = 0
  @@b = 0

  def self.set(x, y)
    @@a, @@b = x, y
  end

  def self.swap
    @@a, @@b = @@b, @@a
  end

  def self.report
    [@@a, @@b]
  end
end

Pair.set(1, 2)
a, b = Pair.report
puts a                    # 1
puts b                    # 2

Pair.swap
a, b = Pair.report
puts a                    # 2
puts b                    # 1

# Three-slot multi-assign across different cvars.
class Triple
  @@x = 0
  @@y = 0
  @@z = 0

  def self.init
    @@x, @@y, @@z = 10, 20, 30
  end

  def self.report
    [@@x, @@y, @@z]
  end
end

Triple.init
x, y, z = Triple.report
puts x                    # 10
puts y                    # 20
puts z                    # 30
