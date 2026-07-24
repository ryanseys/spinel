# A user definition of a prelude-provided method suppresses the implicit
# splice (textual `def cycle` guard) and, with an explicit require, still
# wins by document order (last def). Both custom classes resolve to the
# USER's cycle here, never the prelude's.
require "prelude/enumerable"

class Wheel
  include Enumerable
  def initialize(*xs); @xs = xs; end
  def each; @xs.each { |x| yield x }; end
  def cycle(n)
    "user cycle x#{n}"
  end
end

p Wheel.new(1, 2).cycle(3)
