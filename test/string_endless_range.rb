# #543. `s[1..]` (Ruby 2.6+ endless range) returned an empty
# string under spinel-AOT instead of the substring from index 1
# to end. CRuby returns "id" for ":id"[1..]; spinel returned "".
# Trigger: the codegen lowered `s[1..]`'s RangeNode `right` (an
# AST -1 sentinel meaning "no value") to literal `0`, then
# `sp_str_sub_range_r(s, 1, 0, 0)` produced a zero-length slice.
#
# Sam Ruby caught this when Roundhouse's router stripped the
# leading `:` from `:id` pattern segments via `pp[1..]` and the
# capture key became "" -- every `/articles/N` 404'd because
# params["id"] was always the default fallback.
#
# Fix: in the bracket-call's RangeNode branch, treat the AST -1
# sentinel for the endpoints as the corresponding end:
#   - missing left  -> "0"  (s[..k] == s[0..k])
#   - missing right -> "-1" (s[k..] == s[k..-1])
# The exclusive flag is ignored on the missing endpoint (CRuby
# semantics: `s[1..]` == `s[1...]` == `s[1..-1]`).

s = "hello"

# Endless from inclusive lower bound.
puts s[1..]      # ello
puts s[1...]     # ello  (CRuby treats endless ... same as ..)
puts s[-2..]     # lo

# Beginless to inclusive upper bound.
puts s[..2]      # hel
puts s[...2]     # he   (exclusive upper)

# The Sam Ruby repro shape: strip the leading char.
pp = ":id"
puts pp[1..]     # id
