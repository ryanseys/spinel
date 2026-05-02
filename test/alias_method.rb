# AliasMethodNode -- `alias greet hello` inside a class body.
#
# CRuby snapshots the method at alias time -- if `hello` is later
# redefined, `greet` still calls the original. In Spinel's AOT model
# methods are static C functions; we register a compile-time
# synonym so dispatch on `.greet` routes to the same C function as
# `.hello`. Out of scope: alias inside a method body (CRuby allows
# but the semantics differ); alias of an inherited method.

class Greeter
  def hello = "hi from hello"
  alias greet hello
end

g = Greeter.new
puts g.hello   # hi from hello
puts g.greet   # hi from hello

# Two aliases of the same method.
class Counter
  def value = 42
  alias get value
  alias read value
end
c = Counter.new
puts c.value   # 42
puts c.get     # 42
puts c.read    # 42
