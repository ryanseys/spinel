# A single splat arg to a *direct* instance_exec spreads its source
# array across the block's params, exactly as passing the array
# directly does (CRuby auto-splat). The receiver stays heap (Box has a
# subclass), and the block reads the rebound self's ivar alongside the
# spread elements. (The implicit-self trampoline splat is covered by
# instance_exec_splat.rb; this is the explicit-receiver direct lift.)
class Box
  def initialize(v)
    @v = v
  end
end

class BoxPlus < Box
end

b = BoxPlus.new(5)

# splat of an int array across two params
args = [10, 7]
puts b.instance_exec(*args) { |a, c| a + c + @v }

# a directly-passed array auto-splats the same way (regression anchor)
puts b.instance_exec([3, 4]) { |a, c| a * c }

# splat of a string array
words = ["x", "y"]
puts b.instance_exec(*words) { |a, c| a + c }
