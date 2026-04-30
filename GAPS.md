# Spinel Gaps

A working catalog of identified contribution opportunities. Each entry
is sized to a single small/medium PR (~50-400 LoC). The
[`Array#intersection` PR](https://github.com/matz/spinel/commit/c31b618)
is the reference template: single feature, scope-limited, reuses
runtime helpers, has tests with edge cases.

Entries are grouped by tier (when to ship) then by category. Each
includes a minimal reproducer, where to plug in, and a PR size
estimate (S = ~50-100 LoC, M = ~100-300, L = >300).

## Now — next ~5 PRs

These are the highest-impact / lowest-risk items. All have a clear
fix shape, a 3-5 line reproducer, and an existing dispatcher to plug
into.

### Bugs

#### Sym-keyed hash misinference for string-valued shorthand · M

```ruby
first = "ada"
who = {first:, last: "lovelace"}  # → declared sp_SymIntHash, built as sp_SymStrHash
puts who[:first]                   # C compile error: incompatible pointer types
```

`scan_locals` calls `infer_type` on the AssocNode value before the
local `first` is added to `@scope_names`, so `find_var_type("first")`
returns `""` and `infer_type` falls back to `"int"`. Fix: pre-scan
local writes in the enclosing scope before resolving hash literal
value types, or split inference into two passes.

Plug in: `spinel_codegen.rb:1691` (`infer_hash_val_type`),
`spinel_codegen.rb:12093` (`scan_locals`).

#### Method calls only inside `#{...}` skip parameter widening · S

```ruby
def cap(s); s + "_cap"; end
name = "frob"
puts "lv_#{cap(name)}"   # cap's param `s` defaults to int → C error
```

`scan_features` doesn't visit `EmbeddedStatementsNode` children, so a
method called only inside interpolation never anchors its parameter
types. Fix: walk `EmbeddedStatementsNode` body in
`scan_features_children`.

Plug in: `spinel_codegen.rb:7953` area (`scan_features`),
`compile_interpolated` near `13000`.

### Stdlib methods

#### `Array#union` · S — int/sym/str/float arrays

```ruby
puts [1, 2, 3].union([2, 3, 4]).inspect   # → [1, 2, 3, 4]
```

Same template as `Array#intersection` (commit `c31b618`). Reuse
`sp_*Array_include` for membership; build a fresh array with elements
from both inputs, deduplicated.

Plug in: `spinel_codegen.rb:16331` area (next to the `intersection`
branch).

#### `Array#difference` · S — int/sym/str/float arrays

```ruby
puts [1, 2, 3, 4].difference([2, 4]).inspect   # → [1, 3]
```

Inverse of intersection: keep only elements **not** in the other
array. Same dispatch + runtime pattern.

#### `Array#take_while` / `Array#drop_while` · S each

```ruby
puts [1, 2, 3, 1].take_while { |x| x < 3 }.inspect   # → [1, 2]
puts [1, 2, 3, 1].drop_while { |x| x < 3 }.inspect   # → [3, 1]
```

Block-driven variants of `take` / `drop`. Existing `take` / `drop`
runtime helpers + a per-element block dispatch loop.

#### `String#each_byte` · S

```ruby
"ab".each_byte { |b| puts b }   # 97, 98
```

Trivial loop over `sp_str_length` + char dereference. Mirror existing
`each_char` if present; if not, ship both at once (pair PR).

#### `Range#cover?`, `Range#min`, `Range#max`, `Range#count` · S each

```ruby
puts (1..10).cover?(5)   # true
puts (1..10).min          # 1
puts (1..10).max          # 10
puts (1..10).count        # 10
```

For numeric ranges these reduce to `start`/`last`/arithmetic.
`cover?` reuses the existing `include?` runtime path. Bundle as one
PR (Range completeness).

Plug in: `spinel_codegen.rb:16070` area (`compile_range_method_expr`).

### Tooling

#### Source-mapped error messages · M

Currently a codegen error prints a sentinel string (or silently
returns 0). Goal: every codegen-side `raise` / `STDERR.puts` includes
`<file>:<line>` from the offending Ruby source.

Plug in: thread `@nd_start_line[nid]` (or add it) through
`@source_path` (already known) into every diagnostic emission point.
Audit `STDERR.puts` and `raise` sites in `spinel_codegen.rb`.

## Next — once Now lands

Higher-leverage but slightly bigger. Each is one PR.

### Bugs

#### Multi-arg numbered params don't auto-destructure · M

```ruby
[[1, 10], [2, 20]].each { puts "#{_1}=#{_2}" }   # prints garbage=0
```

When the block has `NumberedParametersNode { maximum: 2+ }` and the
iteration yields a tuple/sub-array, the codegen needs to destructure
the yielded element into `_1`, `_2`, ... before running the block
body. Currently `_1` is bound to the whole element and `_2` is
uninitialized.

Plug in: `get_block_param` (`spinel_codegen.rb:23015` area),
`compile_each_block` (`spinel_codegen.rb:23202`).

#### Uninitialized ivar fall-back to `int` · S

```ruby
class Box
  def show; puts @x; end   # @x never written → silently treated as int 0
end
Box.new.show
```

`cls_ivar_type` returns `"int"` when an ivar isn't in the class's
ivar list. Better: emit a compile-time warning `"undefined ivar @x in
class Box"` and either fail or default explicitly to `nil` of the
matching type.

Plug in: `spinel_codegen.rb:1264` (`cls_ivar_type`).

#### `infer_type` end-of-function `int` fallback · S

Any unhandled AST node returns `"int"` silently. Hides bugs.

End of `infer_type` (`spinel_codegen.rb:1560`-ish). Replace
unconditional `"int"` with a guarded `"unknown:" + node_type` and add
a validation pass that rejects unknown markers, surfacing the true
node type in the error.

#### Empty hash literal type promotion runs too late · M

```ruby
h = {}
h["k"] = 1   # h was already declared sp_SymIntHash; first []= can't promote
```

`promote_empty_hash_for` runs during statement compilation, not
during inference. If the first `[]=` uses a string key, the
declaration may already be wrong. Pair with the empty-array tracking
that already exists.

### Stdlib methods

#### `Array#tally` for str/sym/int element types · S

```ruby
puts ["a", "b", "a"].tally.inspect   # → {"a"=>2, "b"=>1}
```

Trivial: build a `*_int_hash` from the input array's element type.

#### `Hash#filter_map` · S

```ruby
puts({a: 1, b: 2, c: 3}.filter_map { |k, v| [k, v*2] if v > 1 }.inspect)
```

Combine `select` + `map` shapes. Reuse hash iteration; conditional
push to a `poly_array` of `[k, v]` tuples.

#### `Hash#invert` (sym_int variants) · S

`str_str_hash#invert` already works. Add `sym_int_hash`,
`int_str_hash`, `str_int_hash` variants.

#### `Hash#transform_keys`, `Hash#transform_values` block forms · S each

```ruby
puts({a: 1, b: 2}.transform_keys { |k| k.to_s }.inspect)
```

Already partly there for `transform_values`; extend to `transform_keys`
and the block forms.

#### `Symbol#upcase`, `Symbol#downcase` · S each

```ruby
puts :hello.upcase   # → :HELLO
```

Call `sp_str_upcase` on the symbol's name, re-intern via the existing
dynamic pool path.

Plug in: `spinel_codegen.rb:16122` (`compile_symbol_method_expr`).

#### `Float#ceil(n)`, `Float#floor(n)`, `Float#round(n)`, `Float#truncate(n)` · S each

```ruby
puts 3.14159.round(2)   # → 3.14
```

Existing zero-arg variants at `spinel_codegen.rb:16276`-ish; add the
precision argument via shift-by-`10**n` then back.

#### `Integer#gcd`, `Integer#lcm`, `Integer#digits` · S each

```ruby
puts 12.gcd(8)              # 4
puts 4.lcm(6)               # 12
puts 1234.digits.inspect    # [4, 3, 2, 1]
```

Plain arithmetic. Watch for negative-input edge cases per Ruby spec.

### Performance

#### Integer-only constant folding · S

```ruby
SECONDS_PER_DAY = 60 * 60 * 24   # currently emits runtime arithmetic
```

Detect when all operands are integer literals (or already-folded
constants) and evaluate at codegen time. Touches the operator
expression compile path.

#### `Range#each` over numeric range compiles to tight C `for` · M

```ruby
sum = 0
(1..100_000).each { |i| sum += i }   # currently allocates an sp_Range
```

Specialize `compile_each_block` when the receiver is a numeric range
literal: skip the `sp_Range` allocation and emit `for (mrb_int i = lo;
i <= hi; i++)` directly.

Plug in: `compile_each_block` range branch (`spinel_codegen.rb:23450`
area).

### Syntax

#### `**opts` double-splat at call sites · M

```ruby
def show(a:, b:); puts a, b; end
opts = {a: 1, b: 2}
show(**opts)
```

Call-site splat for keyword arguments. Pairs naturally with the
existing splat infrastructure (commit `e7cd0ca`).

#### `__FILE__`, `__dir__` magic constants · S

```ruby
puts __FILE__   # "/path/to/script.rb"
puts __dir__    # "/path/to"
```

Already-supported `__LINE__` shows the pattern: emit a static C
string built from the parser's source-path metadata.

#### Endless method with rescue modifier · S

```ruby
def parse(s) = Integer(s) rescue nil
```

Combine endless-def (already supported) with `rescue` modifier. Most
work already in place; verify with a focused test, fix any gaps.

### Tooling

#### `spinel --version` and `spinel --help` · S

Standard CLI flags. `bin/spinel` is a POSIX shell wrapper; add two
flag handlers up top.

#### `make bench-time` — wall-clock benchmark harness · M

`make bench` only verifies output correctness against CRuby. Add a
sibling `bench-time` target that also runs each benchmark under
`hyperfine` (or `time` fallback) and emits a markdown table. Lets
README benchmarks be regenerated reproducibly.

#### Cleanup on bootstrap interrupt · S

```bash
make bootstrap
^C   # half-finished gen2.c left around
make bootstrap   # confusing failure modes
```

Add explicit cleanup guards / `.PHONY` discipline to the bootstrap
chain so a partial run doesn't poison the next one.

## Later — ideas, larger work, or out-of-scope

### Bugs

#### Heterogeneous-keyed hash inference

`{1 => "a", :b => "c"}` currently coerces; could specialize to a
poly-keyed hash variant. Significant runtime work — defer.

### Stdlib methods

- `Array#combination`, `Array#permutation`, `Array#chunk_while`
  (Enumerator-style; needs lazy evaluation infra)
- `String#unpack`, `Array#pack` (template grammar)
- `String#scrub` (UTF-8 validation)
- `Array#bsearch`, `Array#group_by`, `Array#cycle` (block + control flow)
- `String#%` (full sprintf-style formatter)
- `String#to_r`, `String#to_c`, `Float#to_r` (need Rational/Complex types)
- `Symbol#to_proc` (needs Proc allocation + send dispatch)

### Performance

- **Inline user-class operator methods** (`Vec#+` at hot call sites)
- **Type-specialized `int_int_hash`** variant for tight numeric loops
- **Loop-invariant hash/array literal hoisting**
- **Inline method threshold tuning** (3 → 4-6 stmts for leaf methods)
- **`String#+=` accumulation promotion to mutable + reuse**
- **Symbol array inline buffer** (mirror existing `StrArray` opt)
- **GC threshold adaptive heuristics** for allocation-heavy workloads
- **Interpolation-only allocation removal**: a `String#+`-only chain
  inside `#{}` could fold into the `sp_sprintf` directly

### Syntax

- `Data.define` (Ruby 3.2 immutable value object — needs new value-type infra)
- Full pattern matching extensions: array patterns with `*rest`, find
  patterns `[*, :flag, *]`, hash patterns with `^pin`
- Pattern matching in rightward assign (`expr => [a, b]`,
  `expr => {key:}`)
- `Comparable` mixin via user `<=>` (already partly works; full
  audit needed)
- `Enumerable` mixin via user `each` (already partly works; verify
  full delegation: `map`, `select`, `reduce`, etc.)
- `Hash#deconstruct_keys` for user-class pattern matching
- Refinements (out of scope — document explicitly)
- Frozen-string-literal pragma enforcement
- Ractor (out of scope — threading; document)

### Tooling / DX

- Generated C source maps (Ruby line → C line comments for `-g`)
- Recursive AST depth guard (anti-fuzz; finite recursion budget)
- `make bench-valgrind` (memleak detection harness)
- CI matrix: bootstrap under multiple CRuby versions
- Compile-time graph validation: dangling refs, unresolved methods,
  unknown ivars
- REPL — long-shot, would need stable-ABI runtime + dlopen pattern

---

## How to use this list

When picking the next PR:

1. Start from **Now**. Each entry has a reproducer; copy it into
   `test/<name>.rb` first, confirm CRuby's expected output, then make
   it pass.
2. Stay close to the `c31b618` template: one feature, ~100-200 LoC
   diff, a focused test file, no scope creep.
3. After landing, move the entry to a "Shipped" section at the
   bottom of this file (or strike it through) so the next contributor
   sees what's still open.
4. When promoting items from **Next** → **Now**, prefer items where
   the prerequisites in **Now** have already shipped (e.g. don't
   pick "multi-arg numbered params auto-destructure" before the
   simpler bugs land — they're better warm-up work).
