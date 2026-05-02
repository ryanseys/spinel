# ConstantOperatorWriteNode -- `FOO += val`, etc.
#
# Lowered at parser side to ConstantWriteNode whose value is a
# CallNode binary-op against ConstantReadNode of the same name.
# CRuby would warn "already initialized constant"; Spinel does not
# warn but updates the slot (Spinel constants are mutable C globals).

COUNT = 0
COUNT += 1
COUNT += 5
puts COUNT       # 6

COUNT -= 2
puts COUNT       # 4

COUNT *= 3
puts COUNT       # 12

GREETING = "hello"
GREETING += " world"
puts GREETING    # hello world
