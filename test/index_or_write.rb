# IndexOrWriteNode -- `a[i] ||= val`.
#
# Reads `a[i]`; if falsy, sets a[i] = val. Receiver and index are
# evaluated exactly once even though the source has them on both
# sides of the implicit `a[i] = a[i] || val`.
#
# Same C-truthy vs Ruby-truthy caveat as the other ||= forms:
# numeric 0 reads as falsy at the C level. For string-valued
# slots (NULL falsy, anything else truthy) C and Ruby agree.

# str_int_hash: the cvar slot starts unset (sp_StrIntHash_get
# returns 0 = falsy), so the first ||= initializes; subsequent
# ||= against the now-truthy value is a no-op.
counts = {"alice" => 0, "bob" => 5}
counts["carol"] ||= 10    # never set -> fires
counts["bob"]   ||= 99    # 5 is truthy -> no-op

puts counts["alice"]      # 0  (was set by literal; ||= would re-fire,
                          #     but we don't ||= alice in this test)
puts counts["bob"]        # 5
puts counts["carol"]      # 10
