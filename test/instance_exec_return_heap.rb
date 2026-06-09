# A direct instance_exec used in expression position must type the outer
# local by the block's real last-expression type -- including heap types
# (arrays / hashes), not just scalars. The lift is always inlined, so the
# result temp can hold the heap value. Box has a subclass to stay heap.
class Box
  def initialize(v)
    @v = v
  end
  def name
    "box"
  end
end

class BoxPlus < Box
end

b = BoxPlus.new(3)

# int array literal in expression position
a = b.instance_exec { [1, 2, 3] }
puts a.inspect

# array built from the rebound self's ivar
a2 = b.instance_exec { [@v, @v + 1] }
puts a2.inspect

# string array
sa = b.instance_exec { ["a", "b"] }
puts sa.inspect

# scalar/string return still works (regression anchor)
puts b.instance_exec { name }
