# ConstantPathTargetNode -- `M::FOO, M::BAR = 1, 2` (multi-assign LHS).

module M
  X = 0
  Y = 0
end
M::X, M::Y = 100, 200
puts M::X    # 100
puts M::Y    # 200
