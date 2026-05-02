# MatchLastLineNode + $_ -- additional coverage beyond the
# null-guard checks in test/match_lastline.rb.
#
# Note: actually piping stdin through the test harness isn't easy
# without modifying the Makefile, so we focus on cases that exercise
# the codegen + runtime without needing real gets() input. The
# null-guard branches are well-tested; this adds variations on
# pattern flags and the Boolean false case.

# Bare regex with /i flag against unset $_ still returns false.
matched = 0
matched = 1 if /WORLD/i
puts matched.to_s        #=> 0

# Multi-line pattern flag /m -- same null-guard, same result.
multi = 0
multi = 1 if /^foo/m
puts multi.to_s          #=> 0

# Negation: `unless /pat/` is the inverse of `if /pat/`. With $_ NULL,
# /pat/ returns false, so `unless` body fires.
fired = 0
fired = 1 unless /anything/
puts fired.to_s          #=> 1

# Multiple if /pat/ at distinct sites in same scope -- each site gets
# its own scan_features registration, no slot collision.
a = 0; b = 0
a = 1 if /alpha/
b = 1 if /beta/
puts a.to_s              #=> 0
puts b.to_s              #=> 0
