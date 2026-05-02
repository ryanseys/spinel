# SourceFileNode — the `__FILE__` keyword.
#
# Spinel inlines `require_relative` at parse time, so call sites
# in different source files are not distinguished. `__FILE__` always
# returns the toplevel script path passed to spinel_parse, matching
# CRuby's behavior for top-level uses.

puts __FILE__
