# ConstantPathOperatorWriteNode -- `M::FOO += val`.
#
# Lowered at parser side to ConstantPathWriteNode whose value is a
# CallNode binary-op against ConstantPathNode of the same path.

module M
  COUNT = 10
end
M::COUNT += 5
puts M::COUNT     # 15

M::COUNT *= 2
puts M::COUNT     # 30

module N
  GREETING = "hi"
end
N::GREETING += " there"
puts N::GREETING  # hi there
