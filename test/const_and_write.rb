# ConstantAndWriteNode -- `FOO &&= val`.
#
# Lowered at parser side to: if FOO; FOO = val; end.
# Mirror of ConstantOrWriteNode with condition inverted.

ENABLED = true
ENABLED &&= false      # truthy -> fires
puts ENABLED           # false

GREETING = "hello"
GREETING &&= "WORLD"   # truthy -> fires
puts GREETING          # WORLD
