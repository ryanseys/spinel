# ConstantPathAndWriteNode -- `M::FOO &&= val`.

module M
  ENABLED = true
end
M::ENABLED &&= false
puts M::ENABLED               # false

module N
  GREETING = "hello"
end
N::GREETING &&= "WORLD"
puts N::GREETING              # WORLD
