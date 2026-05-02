# ClassVariableAndWriteNode -- `@@x &&= val`.
#
# Cvar version of LocalVariable/Global/Instance AndWrite.

class Toggle
  @@enabled = true

  def self.disable
    @@enabled &&= false   # was true -> fires, now false
  end

  def self.enable
    @@enabled &&= true    # was false -> doesn't fire, stays false
  end

  def self.state
    @@enabled
  end
end

puts Toggle.state          # true
Toggle.disable
puts Toggle.state          # false
Toggle.enable
puts Toggle.state          # false (didn't re-enable -- &&= doesn't fire on falsy)

# &&= chain on a numeric cvar.
class Acc
  @@n = 10

  def self.bump
    @@n &&= @@n + 5         # 10 truthy -> fires, becomes 15
  end

  def self.value
    @@n
  end
end

puts Acc.value            # 10
Acc.bump
puts Acc.value            # 15
Acc.bump
puts Acc.value            # 20
