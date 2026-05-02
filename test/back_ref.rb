# BackReferenceReadNode -- `$&`, `$~`, `$'`, $`.
#
# Special globals populated by regex matches. Spinel's regex engine
# already populates sp_re_match* state for $1..$9 (NumberedReferenceReadNode);
# this extends to the symbolic back-references.
#
#   $&  -- the entire matched substring
#   $~  -- the MatchData object (Spinel exposes as MatchData wrapper)
#   $`  -- the substring before the match
#   $'  -- the substring after the match

"hello world" =~ /lo wo/
puts $&     # lo wo
puts $`     # hel
puts $'     # rld

# Re-match updates the state
"abcdef" =~ /cd/
puts $&     # cd
puts $`     # ab
puts $'     # ef
