# #566 (T.Yamada). The original report was a segfault from
# downstream `Array#drop` / `take` running on a NULL pointer
# emitted by then-unimplemented `each_cons` / `with_index` /
# `max` (all warned-and-emitted-0 at the time). The NULL
# guards in `sp_PtrArray_*` (commit 729b9cb) stopped the
# segfault first; subsequently the missing Enumerator-chain
# methods were implemented via chain fusion so the full
# expression now produces CRuby's `3` instead of the
# warned-fallback `0`.
#
# This test now covers two things together:
# 1. The original NULL-safe guards on `sp_PtrArray_*` (a
#    later regression that emits NULL where a PtrArray is
#    expected must not crash here).
# 2. The end-to-end Enumerator-chain `each_cons(n).
#    with_index(off).map { ... }.drop.take.max[i]` lowering.

q = [100, 90, 82, 70, 65]
a = 2
b = 4
p q.each_cons(2).with_index(1).map { |(x, y), i| [x - y, i] }.drop(a - 1).take(b - a + 1).max[1]
