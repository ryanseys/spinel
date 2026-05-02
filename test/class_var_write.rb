# ClassVariableWriteNode -- `@@var = value`.
#
# Spinel stores cvars as per-(class, name) C globals named
# `cvar_<ClassName>_<var>`. The static declaration is emitted at
# file scope alongside constants; ClassVariableReadNode (next
# commit) consumes the same storage.
#
# This commit's test only exercises the write side -- a follow-up
# Read commit prints @@count to verify round-trip.

class Counter
  @@count = 0
end

class Other
  @@count = 99    # independent slot from Counter's @@count
end

puts "ok"
