# An empty-hash literal constant (`{}` / `{}.freeze`) used as a VALUE -- returned
# directly, or as the `||` fallback for a nil-or-typed ivar. Regression: such a
# constant was dropped from codegen entirely, because an empty `{}` receiver never
# got a hash type the way an empty `[]` receiver gets TY_POLY_ARRAY. Reads then
# emitted `uninitialized constant`. It now types as a str-keyed poly hash (the
# same C type codegen emits for a bare `{}`), so declaration, initializer, and
# readers all agree.
class Node
  EMPTY = {}.freeze
  def initialize = (@attrs = nil)
  def attrs = @attrs || EMPTY
  def set(k, v)
    @attrs ||= {}
    @attrs[k] = v
  end
end

p Node.new.attrs        # the empty constant, read straight out as a value
p Node.new.attrs.empty? # methods dispatch on it

n = Node.new
n.set("level", 2)
p n.attrs               # the typed ivar wins the ||
p n.attrs["level"]

C = {}.freeze           # a top-level empty-hash constant, not just a class one
p C
p C.size
