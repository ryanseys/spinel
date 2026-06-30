# RBS seeding spike

A historical bring-up harness, not a shipped subcommand. Unlike the
compiled tools in this directory, it is a plain Ruby script run under
CRuby and it exercises the **legacy** analyzer (`legacy/spinel_analyze.rb`).

`harness.rb` runs the analyzer twice on the same AST — once without a
seed file, once with `box.seed` — and diffs the `Box` class's type-table
rows. The diff is the evidence that RBS seeding flows through to the
analyzer's inferred types (`Box#relabel`'s param shifts from `int` to
`string`).

```
make parse
ruby tools/rbs-seed-spike/harness.rb
```

Exit `0` = the expected diff was observed.

- `harness.rb` — the spike runner
- `box.rb`     — fixture whose `relabel` is never called, so without a
                 seed the analyzer has no call-site evidence for its param
- `box.seed`   — hand-written seed; the worked example referenced from
                 `docs/rbs-extract.md`

The shipped C `--rbs` mechanism this spike originally validated now has
its own coverage: `test/rbs/` (the `spinel_rbs_extract` text transform)
and `test/rbs-seed/` (end-to-end analyzer seeding). This harness is kept
as the original worked example of the seed format and mechanism.
