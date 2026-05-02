# UndefNode -- `undef foo` inside a class body.
#
# CRuby raises NoMethodError if the undef'd method is called. In
# Spinel's AOT model we cannot dispatch at runtime (methods are
# static C functions resolved at compile time), so `undef foo`
# becomes a compile-time error: any call to `.foo` on an instance
# of this class fails to compile with a precise message.
#
# This test verifies that defining + calling another method on the
# same class still works after undef -- undef removes only the
# named method, not the whole class.

class C
  def foo = "foo"
  def bar = "bar"
  undef foo
end

c = C.new
puts c.bar     # bar
