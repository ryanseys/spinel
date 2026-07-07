# Compile-time Fiddle: the Importer#struct DSL. `Const = struct ["ctype name",...]`
# declares a C struct; Const.malloc allocates it, Const.size is its byte size, and
# instance.field / instance.field= read/write members. This lowers onto the
# native ffi_struct capability (the C compiler owns the layout).
require "fiddle/import"

module M
  extend Fiddle::Importer
  Point = struct ["long x", "long y"]
  Mixed = struct ["int n", "double d"]
end

p = M::Point.malloc
p.x = 3
p.y = 7
puts p.x
puts p.y
puts(p.x + p.y)
puts M::Point.size

m = M::Mixed.malloc
m.n = 42
m.d = 2.5
puts m.n
puts m.d
