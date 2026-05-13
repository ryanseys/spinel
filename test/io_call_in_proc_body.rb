# `puts` / `print` / `printf` reached in expression context — the
# last statement of a `proc { ... }` body. The expression-context
# dispatcher previously had no path for these no-recv IO methods,
# so the call fell through to warn_unresolved_call and the IO
# side-effect was silently dropped. Fix: bridge through
# compile_io_call_stmt and pass "0" up as the C expression value.
#
# To exercise all three IO methods concisely, the test iterates an
# array of procs and `.call`s each. This relies on two adjacent
# fixes also in this PR:
#
# 1. Heterogeneous array literals of procs (`[proc, proc, proc]`)
#    are inferred as `poly_array` (boxed via sp_box_proc) rather
#    than falling through to `int_array`. Without this, the array
#    construction emitted `sp_IntArray_push` with a `sp_Proc *`
#    argument and the C compiler rejected the int-from-pointer
#    conversion.
#
# 2. Poly `.call` dispatch in `compile_poly_method_call` — when a
#    block param iterates a poly array carrying procs, the recv's
#    static type is `poly`. Unbox via `(sp_Proc *)recv.v.p` and
#    invoke `sp_proc_call`. Without this, `p.call` inside the
#    block fell through to warn_unresolved_call on a poly recv.
#
# Symbol#to_proc (`&:call`) — the more idiomatic Ruby form for this
# iteration — is still TODO in spinel (`find_block_arg` returns -1
# for the SymbolNode shape). The explicit-block form `{ |p| p.call }`
# is the closest equivalent that works today; the &:sym lowering is
# a separate concern.

[
  proc { puts "a" },
  proc { print "b\n" },
  proc { printf("c=%d\n", 3) }
].each { |p| p.call }
