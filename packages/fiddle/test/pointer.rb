# Compile-time Fiddle: the Fiddle::Pointer runtime type (a carried-C native
# class carrying a raw pointer + byte size). Pointer.malloc allocates a
# zero-filled buffer; p[off,len] reads/writes bytes; p+n/p-n do pointer
# arithmetic (size adjusts by the offset); Fiddle::NULL is a null pointer; a
# Pointer passed to a TYPE_VOIDP argument hands off its raw pointer.
require "fiddle"

p = Fiddle::Pointer.malloc(16)
p[0, 8] = "ABCDEFGH"
puts p[0, 4]
puts p[4, 4]
puts p.null?
puts p.to_s

q = p + 4
puts q[0, 4]
puts (q - 4)[0, 4]

puts Fiddle::NULL.null?

h = Fiddle.dlopen(nil)
strlen = Fiddle::Function.new(h["strlen"], [Fiddle::TYPE_VOIDP], Fiddle::TYPE_SIZE_T)
s = Fiddle::Pointer.malloc(8)
s[0, 5] = "hello"
puts strlen.call(s)

p.free
s.free
