# Compile-time Fiddle: the low-level Function.new API. `h = Fiddle.dlopen(nil)`
# binds a handle; `Fiddle::Function.new(h["sym"], [arg_types], ret_type)` binds a
# C function to a local, and `f.call(...)` invokes it. dlopen/Function.new carry
# no runtime value (compile-time bindings); the C symbols resolve at link time.
# TYPE_VOIDP arguments receive a String passed as a C pointer.
require "fiddle"

h = Fiddle.dlopen(nil)
abs    = Fiddle::Function.new(h["abs"],     [Fiddle::TYPE_INT],   Fiddle::TYPE_INT)
strlen = Fiddle::Function.new(h["strlen"],  [Fiddle::TYPE_VOIDP], Fiddle::TYPE_SIZE_T)
sqrt   = Fiddle::Function.new(h["sqrt"],    [Fiddle::TYPE_DOUBLE],Fiddle::TYPE_DOUBLE)
atoi   = Fiddle::Function.new(h["atoi"],    [Fiddle::TYPE_VOIDP], Fiddle::TYPE_INT)

puts abs.call(-7)
puts strlen.call("hello world")
puts sqrt.call(2.0)
puts atoi.call("42")
