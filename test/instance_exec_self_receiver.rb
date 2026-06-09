# Explicit `self.instance_exec` / `self.instance_eval` inside a class
# method resolves to the enclosing class -- the same receiver as the
# implicit-self form. Box has a subclass so Spinel keeps it heap.
class Box
  def initialize(v)
    @v = v
  end
  def scaled
    self.instance_exec { @v * 10 }
  end
  def plus
    self.instance_eval { @v + 100 }
  end
end

class BoxPlus < Box
end

b = BoxPlus.new(5)
puts b.scaled
puts b.plus
