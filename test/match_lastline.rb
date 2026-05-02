# MatchLastLineNode + InterpolatedMatchLastLineNode + $_ tracking
#
# `if /pat/` is implicit `$_ =~ /pat/`. Spinel updates `sp_lastline`
# at every sp_gets / sp_readlines call site (lib/sp_runtime.h), and
# the codegen emits a null-guarded match against that slot. Without
# stdin input, $_ stays nil/NULL and `if /pat/` returns falsy.
#
# Both CRuby and Spinel agree: bare `if /pat/` against an unset $_
# evaluates to false. This is the test we can run without piping input.

matched = 0
if /anything/
  matched = 1
end
puts matched.to_s     #=> 0

# Interpolated form -- same null-guard path through sp_re_runtime_compile.
x = "z"
matched2 = 0
if /foo_#{x}/
  matched2 = 1
end
puts matched2.to_s    #=> 0
