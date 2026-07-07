# Compile-time Fiddle: Fiddle::Closure::BlockCaller wraps a Ruby block as a C
# function pointer. A non-capturing block is desugared to a synthetic top-level
# method and reuses the ffi_callback trampoline; :ptr callback params arrive as
# Fiddle::Pointers so the block reads through them. Capturing blocks are rejected.
require "fiddle"

h = Fiddle.dlopen(nil)

# qsort with a block comparator: the canonical Fiddle::Closure idiom.
qsort = Fiddle::Function.new(h["qsort"],
  [Fiddle::TYPE_VOIDP, Fiddle::TYPE_SIZE_T, Fiddle::TYPE_SIZE_T, Fiddle::TYPE_VOIDP],
  Fiddle::TYPE_VOID)
cmp = Fiddle::Closure::BlockCaller.new(Fiddle::TYPE_INT, [Fiddle::TYPE_VOIDP, Fiddle::TYPE_VOIDP]) do |a, b|
  a[0, 4].unpack1("l") <=> b[0, 4].unpack1("l")
end
buf = Fiddle::Pointer.malloc(20)
buf[0, 20] = [5, 3, 1, 4, 2].pack("l*")
qsort.call(buf, 5, 4, cmp)
puts buf[0, 20].unpack("l*").inspect

# descending comparator (inline closure, no local)
buf2 = Fiddle::Pointer.malloc(16)
buf2[0, 16] = [10, 40, 20, 30].pack("l*")
qsort.call(buf2, 4, 4,
  Fiddle::Closure::BlockCaller.new(Fiddle::TYPE_INT, [Fiddle::TYPE_VOIDP, Fiddle::TYPE_VOIDP]) do |a, b|
    b[0, 4].unpack1("l") <=> a[0, 4].unpack1("l")
  end)
puts buf2[0, 16].unpack("l*").inspect
