# Pin matz's 81bb92b (Closes #133) — "heterogeneous Hash rebuild via
# each-loop now compiles".
#
# History:
#   - a952cd8 (#138) introduced empty-hash promotion based on the
#     first []= site, but only widened on str_int_hash.
#   - 81bb92b extended that to all hash variants AND added
#     scan_locals_arg_type so block-param value types feed the
#     promotion AND fixed the scan_locals_first_type ↔ scan_locals
#     pass merge for hash → poly_hash escalation.
#
# test/empty_hash_promote.rb covers the pure []= case (single direct
# write per fresh hash). This file pins the each-loop rebuild shape
# that fails without the additional fixes — block params, promoted
# locals across method boundaries, and heterogeneous values.

# 1. Sym-keyed source with heterogeneous values rebuilt via each.
#    Pre-81bb92b: out was typed sp_SymIntHash (from the first
#    seen-int []= once block-param inference fell back to int) and
#    never escalated to sp_SymPolyHash on the string write.
src = {name: "alice", age: 30}
out = {}
src.each { |k, v| out[k] = v }
puts out[:name]              # alice
puts out[:age]               # 30
puts out.length              # 2

# 2. Same shape but the accumulator is a method-local. Exercises the
#    declare_method_locals merge between scan_locals_first_type and
#    scan_locals (both passes need the hash → poly escalation).
def rebuild(src)
  acc = {}
  src.each { |k, v| acc[k] = v }
  acc
end

r = rebuild({status: "ok", count: 7})
puts r[:status]              # ok
puts r[:count]               # 7
puts r.length                # 2

puts "done"
