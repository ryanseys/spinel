# `class_eval { |mod| ... }` passes the class to the block. The reopen
# splice registers the def bodies directly on the class and never
# threads that block parameter into scope, so a parameterized block is
# rejected rather than silently binding `mod` to nothing.
class Box
  def initialize(v)
    @v = v
  end
end

Box.class_eval do |klass|
  def doubled
    @v * 2
  end
end
