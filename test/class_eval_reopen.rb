# `Klass.class_eval do ... end` reopens the class: each `def` and
# `define_method(:lit)` in the block body becomes an instance method on
# the class, exactly as a `class Klass ... end` reopen would. A def may
# read an existing ivar or assign a new one (which gets its own slot),
# subclasses inherit the added methods, and `module_eval` is an alias.
class Box
  def initialize(v)
    @v = v
  end
end

class BoxPlus < Box
end

Box.class_eval do
  def doubled
    @v * 2
  end

  define_method(:tripled) do
    @v * 3
  end
end

# module_eval is the same operation under a different name.
Box.module_eval do
  def labelled
    @label = "n=" + @v.to_s
    @label
  end
end

b = Box.new(21)
puts b.doubled
puts b.tripled
puts b.labelled

# A subclass inherits the class_eval-added instance methods.
bp = BoxPlus.new(5)
puts bp.doubled
puts bp.tripled
