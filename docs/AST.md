# Spinel AST format

`spinel_parse` consumes a Ruby source file and emits a text-based AST
that the rest of the pipeline (`spinel_analyze`, `spinel_codegen`)
reads. This file documents that text format -- the record types, the
flattening conventions, and the per-node fields the consumers rely on.

## Pipeline position

```
.rb  ──[spinel_parse]──▶  .ast  ──[spinel_analyze]──▶  .ir  ──[spinel_codegen]──▶  .c
```

`spinel_parse` is the only stage that talks to libprism. The downstream
binaries never see Prism's data structures -- they read the flattened
text AST and operate on parallel integer-indexed arrays. The text format
is the stable contract between Prism (a moving upstream) and Spinel's
fixed-shape backend.

Two implementations of the parser exist and produce byte-identical AST:

- **`spinel_parse.c`** -- links libprism directly. The default; no Ruby
  needed at build time once `make deps` has fetched the sources.
- **`spinel_parse.rb`** -- uses the Prism gem from CRuby. Bootstrap
  fallback when no compiled parser is available.

## File format

Plain UTF-8 text, line-oriented (`\n`-terminated). Every line is one
record. Fields are space-separated.

Records are not length-limited by the format. Large source constructs
can produce `A` records longer than 4096 bytes, so parser emitters must
size output records dynamically instead of formatting through a fixed
line buffer.

The first record is always:

```
ROOT <int>
```

naming the AST node id that the rest of the file roots at (always `0`
in practice, since `flatten()` walks the tree top-down and assigns ids
in DFS order).

Every subsequent record begins with a single-letter tag:

| Tag | Shape                              | Meaning                                                              |
|-----|------------------------------------|----------------------------------------------------------------------|
| `N` | `N <id> <NodeType>`                | Node header. Introduces a new node; binds its id to its Prism type.  |
| `S` | `S <id> <field> <string>`          | String-valued attribute (`name`, `content`, `pattern`, etc.).        |
| `I` | `I <id> <field> <int>`             | Integer attribute (`value`, `flags`).                                |
| `F` | `F <id> <field> <float>`           | Float attribute (`value`).                                           |
| `R` | `R <id> <field> <child-id-or--1>`  | Single child reference. `-1` means the child slot is empty.          |
| `A` | `A <id> <field> <id,id,…>`         | Array of child references. Body is comma-separated; empty allowed.   |

A node is defined by its `N` line followed by zero or more attribute
lines (`S`/`I`/`F`/`R`/`A`) that share the same `<id>`. Attribute lines
appear in source order -- but downstream code addresses attributes by
*field name*, not position, so the order is informational rather than
load-bearing.

### Example

```ruby
puts "hi"
```

flattens to:

```
ROOT 0
N 0 ProgramNode
N 1 StatementsNode
N 2 CallNode
S 2 name puts
R 2 receiver -1
N 3 ArgumentsNode
N 4 StringNode
S 4 content hi
A 3 arguments 4
R 2 arguments 3
R 2 block -1
A 1 body 2
R 0 statements 1
```

Node ids are assigned in DFS pre-order during parse, so a child's id is
always greater than its parent's `N` line id. The attribute lines for a
parent may interleave with the child subtree -- the consumer matches by
id, not by position.

### Encoding rules

String values (`S` records) are produced by `escape_str()` in
`spinel_parse.c`:

| Char           | Encoded as |
|----------------|------------|
| `\n`           | `\\n`      |
| `\t`           | `\\t`      |
| `\r`           | `\\r`      |
| `\\`           | `\\\\`     |
| `"`            | `\\"`      |
| `\0`           | `\\0`      |
| byte ≥ `0x80`  | passed through verbatim (UTF-8) |
| other          | passed through |

The encoding is reversible by the consumer's `unescape_str` helper.

The space between fields is a single ASCII space; any spaces inside the
string payload survive (they are not separators after the third field).
Newlines do not appear in string payloads -- they are encoded as `\n`.

## Node-type taxonomy

`spinel_parse.c`'s `flatten()` enumerates every Prism node it
understands. Adding support for a new Ruby idiom usually means adding
one `case PM_<NODE>:` arm to the switch and a matching consumer-side
handler in `spinel_analyze.rb` / `spinel_codegen.rb`. The list below
groups the recognised nodes by category; field names per node are the
ones Prism documents, with a few Spinel-specific aliases for clarity.

### Program / scopes

| Node                | Fields used                              | Notes                                                 |
|---------------------|------------------------------------------|-------------------------------------------------------|
| `ProgramNode`       | `statements` (R)                         | Top-level wrapper. The root node.                     |
| `StatementsNode`    | `body` (A)                               | Sequence of statements -- methods bodies, blocks, etc.|
| `ClassNode`         | `constant_path` (R), `superclass` (R), `body` (R) | Class definition. `superclass` is `-1` for no parent. |
| `ModuleNode`        | `constant_path` (R), `body` (R)          | Module definition.                                    |
| `SingletonClassNode`| `expression` (R), `body` (R)             | `class << recv; ... end`.                             |
| `DefNode`           | `name` (S), `parameters` (R), `body` (R), `receiver` (R) | Method definition. `receiver` is non-`-1` for `def self.x` / `def obj.x`. |
| `LambdaNode`        | `parameters` (R), `body` (R)             | `-> { ... }` / `lambda { ... }`.                      |
| `BlockNode`         | `parameters` (R), `body` (R)             | `do ... end` / `{ ... }` attached to a call.          |
| `ParametersNode`    | `requireds` (A), `optionals` (A), `keywords` (A), `posts` (A), `rest` (R), `block` (R) | Per-method or per-block parameter list. |
| `NumberedParametersNode` | `value` (I) (max param `_1` .. `_N`) | `{ _1 + _2 }` shorthand.                              |

### Control flow

| Node                | Fields used                                          |
|---------------------|------------------------------------------------------|
| `IfNode`            | `predicate` (R), `statements` (R), `subsequent` (R)  |
| `UnlessNode`        | `predicate` (R), `statements` (R), `consequent` (R)  |
| `ElseNode`          | `statements` (R)                                     |
| `CaseNode`          | `predicate` (R), `conditions` (A), `else_clause` (R) |
| `CaseMatchNode`     | `predicate` (R), `conditions` (A), `else_clause` (R) |
| `WhenNode`          | `conditions` (A), `statements` (R)                   |
| `InNode`            | `pattern` (R), `statements` (R)                      |
| `WhileNode`         | `predicate` (R), `statements` (R)                    |
| `UntilNode`         | `predicate` (R), `statements` (R)                    |
| `BreakNode` / `NextNode` / `RedoNode` / `ReturnNode` | `arguments` (R)         |
| `BeginNode`         | `statements` (R), `rescue_clause` (R), `else_clause` (R), `ensure_clause` (R) |
| `RescueNode`        | `exceptions` (A), `reference` (R), `statements` (R), `subsequent` (R) |
| `EnsureNode`        | `statements` (R)                                     |
| `YieldNode`         | `arguments` (R)                                      |
| `SuperNode` / `ForwardingSuperNode` | `arguments` (R), `block` (R)         |
| `CatchNode` / `ThrowNode` | (encoded as CallNode-style; no dedicated Prism node -- the parser injects helpers) |

### Calls

`CallNode` is the workhorse: every method invocation -- including
operators, attribute access, indexing -- compiles to one. The
disambiguating field is `name`, plus the recv / args / block triple.

| Node                | Fields used                                          | Notes                                       |
|---------------------|------------------------------------------------------|---------------------------------------------|
| `CallNode`          | `name` (S), `receiver` (R), `arguments` (R), `block` (R), `flags` (I) | Universal method call. `flags` bit 0 = `&.` safe navigation. |
| `ArgumentsNode`     | `arguments` (A)                                      | Positional + keyword + splat args.          |
| `KeywordHashNode`   | `elements` (A)                                       | Trailing keyword hash literal.              |
| `AssocNode`         | `key` (R), `value` (R)                               | One key=>value pair inside a Hash or kwargs.|
| `SplatNode`         | `expression` (R)                                     | `*args` -- splat in args or LHS.            |
| `BlockArgumentNode` | `expression` (R)                                     | `&block` -- pass block by name.             |
| `ForwardingArgumentsNode` | (no fields)                                    | `def f(...); g(...); end` shape.            |
| `BlockParameterNode`| `name` (S)                                           | `&blk` in a method's parameter list.        |

### Literals

| Node                  | Fields used                                | Notes                          |
|-----------------------|--------------------------------------------|--------------------------------|
| `IntegerNode`         | `value` (I)                                | Decimal, hex, oct, bin.        |
| `FloatNode`           | `value` (F)                                | Includes exponent forms.       |
| `RationalNode` / `ImaginaryNode` | `numeric` (R)                   | Wrapping number suffix.        |
| `StringNode`          | `content` (S)                              | Literal string.                |
| `SymbolNode`          | `content` (S)                              | Symbol literal.                |
| `InterpolatedStringNode` / `InterpolatedSymbolNode` | `parts` (A) | Each part is either a StringNode or an `EmbeddedStatementsNode`. |
| `EmbeddedStatementsNode` | `statements` (R)                        | `#{...}` inside a string.      |
| `RegularExpressionNode` | `content` (S), `flags` (I)               | `/pat/`.                       |
| `InterpolatedRegularExpressionNode` | `parts` (A), `flags` (I)     | `/#{x}/`.                      |
| `ArrayNode`           | `elements` (A)                             | `[1, 2, 3]`.                   |
| `HashNode`            | `elements` (A) -- each an AssocNode        | `{a: 1}`.                      |
| `RangeNode`           | `left` (R), `right` (R), `flags` (I)       | `flags` bit 1 = exclusive end. |
| `LambdaNode`          | (see above)                                | `-> x { x*2 }`.                |
| `TrueNode` / `FalseNode` / `NilNode` / `SelfNode` | (no fields)    | Singleton literals.            |

### References & assignment

| Node                              | Fields used                                |
|-----------------------------------|--------------------------------------------|
| `LocalVariableReadNode`           | `name` (S)                                 |
| `LocalVariableWriteNode`          | `name` (S), `value` (R)                    |
| `LocalVariableTargetNode`         | `name` (S) -- LHS of `MultiWriteNode`      |
| `LocalVariableOperatorWriteNode`  | `name` (S), `value` (R), `operator` (S)    |
| `LocalVariableOrWriteNode` / `LocalVariableAndWriteNode` | `name` (S), `value` (R) |
| `InstanceVariableReadNode`        | `name` (S)                                 |
| `InstanceVariableWriteNode`       | `name` (S), `value` (R)                    |
| `InstanceVariableOperatorWriteNode` | (same triple, plus `operator` S)         |
| `ClassVariableReadNode` / `ClassVariableWriteNode` | `name` (S), `value` (R) |
| `GlobalVariableReadNode` / `GlobalVariableWriteNode` | `name` (S), `value` (R) |
| `ConstantReadNode`                | `name` (S)                                 |
| `ConstantPathNode`                | `parent` (R), `name` (S)                   |
| `ConstantWriteNode`               | `name` (S), `value` (R)                    |
| `MultiWriteNode`                  | `targets` (A), `expression` (R), `rest` (R) |
| `MultiTargetNode`                 | `lefts` (A), `rest` (R), `rights` (A)      |

### Index access

| Node                              | Fields used                                |
|-----------------------------------|--------------------------------------------|
| `IndexOrWriteNode` / `IndexAndWriteNode` / `IndexOperatorWriteNode` | `receiver` (R), `arguments` (R), `value` (R), plus `operator` for op-write |
| `CallOperatorWriteNode` / `CallAndWriteNode` / `CallOrWriteNode` | `receiver` (R), `read_name` (S), `write_name` (S), `value` (R), `operator` (S) -- `obj.x op= v` shape |
| (`a[i]` reads itself parse as a `CallNode` with `name == "[]"`. The `*OrWrite` / `*AndWrite` / `*OperatorWrite` nodes only appear for the assignment shape.) |

### Pattern matching

| Node                              | Fields used                                | Notes                                 |
|-----------------------------------|--------------------------------------------|---------------------------------------|
| `ArrayPatternNode`                | `requireds` (A), `rest` (R), `posts` (A)   | `case x; in [a, b]`                   |
| `HashPatternNode`                 | `elements` (A), `rest` (R)                 | `case x; in {a: 1}`                   |
| `FindPatternNode`                 | `left` (R), `requireds` (A), `right` (R)   | `case x; in [*, a, *]`                |
| `PinnedVariableNode`              | `variable` (R)                             | `^var` in a pattern.                  |
| `ImplicitNode`                    | `value` (R)                                | Sugar elision (`a:` short-hand).      |

### Forwarding / synthetic

| Node                              | Fields used                                |
|-----------------------------------|--------------------------------------------|
| `ForwardingArgumentsNode`         | (no fields)                                |
| `ForwardingParameterNode`         | (no fields)                                |

The parser also injects synthetic `CallNode`s for built-in idioms that
Prism parses as separate constructs: `catch` / `throw`, `defined?`, and
the `*-write` shorthand operators. Those are documented at their
emit sites in `spinel_parse.c`.

## Field semantics by category

### `R <id> <field> <child>`

A single reference. `-1` means "no child" -- consumers must check before
indexing arrays by this id. A handful of field names appear repeatedly
across node types with a consistent meaning:

- `receiver` -- the recv of a CallNode; `-1` for bare calls.
- `arguments` -- usually points at an ArgumentsNode, sometimes directly
  at the single expression (BreakNode / ReturnNode).
- `block` -- points at a BlockNode (literal block) or BlockArgumentNode
  (`&block` forward) attached to a call; `-1` otherwise.
- `value` / `expression` -- the RHS of an assignment / write.
- `statements` -- the body of a control-flow construct.
- `parameters` -- a ParametersNode attached to a method / lambda / block.

### `A <id> <field> <id,id,…>`

A list of references, comma-separated, empty body allowed. The empty
string after `<field>` means "zero elements". Used for collections
whose size isn't fixed at parse time: statement bodies, argument lists,
WhenNode condition lists, hash element lists, etc.

The order is the source order. Consumers walk left-to-right via
`get_args(args_id)` (or `parse_id_list` for raw lists), so reordering
within an `A` record changes program semantics.

### `S <id> <field> <string>`

The string is escape_str-encoded (see "Encoding rules" above). Common
fields:

- `name` -- the method name on CallNode, the variable name on
  `LocalVariableReadNode`, etc.
- `content` -- the literal text of `StringNode` / `SymbolNode` /
  `RegularExpressionNode`.
- `operator` -- `+=`, `-=`, `*=`, etc. on the op-write nodes.
- `pattern` -- regex source for `RegularExpressionNode`.

### `I <id> <field> <int>`

Decimal integer, possibly signed. Used for `IntegerNode.value` (the
parsed literal), `flags` bitfields, and Prism's `NumberedParametersNode`
arity.

### `F <id> <field> <float>`

Float literal in `printf("%.17g")` form, always with a decimal point.

## How consumers read the AST

Both `spinel_analyze.rb` and `spinel_codegen.rb` build the same set of
parallel arrays from the text AST:

| Array                | Meaning                                                |
|----------------------|--------------------------------------------------------|
| `@nd_type[id]`       | Node type name (the second field of the `N` record).   |
| `@nd_name[id]`       | `S name` value, if any.                                |
| `@nd_value[id]`      | `I value` value, if any.                               |
| `@nd_receiver[id]`   | `R receiver <child>` value (or -1).                    |
| `@nd_arguments[id]`  | `R arguments <child>` value.                           |
| `@nd_block[id]`      | `R block <child>` value.                               |
| `@nd_body[id]`       | `R body <child>` for non-array shapes, or first id of an `A body` list. |
| `@nd_stmts[id]`      | `A statements` body (for StatementsNode-shaped).       |
| `@nd_predicate[id]`  | `R predicate <child>` value.                           |
| ... (~50 more, all named `@nd_<field>`) |                                     |

Each `@nd_<field>` is a flat array indexed by node id. Initialization
sets the default (`-1` for refs, `""` for strings, `0` for ints) and
the AST loader overwrites the matching slot for every record it sees.
A given node's id is `@nd_count`-sized -- 99 % of slots are unused for
any one node, but the per-array cache locality at `infer_type` /
`compile_expr` time more than pays for the sparsity.

The loader treats unknown fields as errors-by-omission: it skips them
silently. This means an unknown record on the parser side stays
unreferenced on the consumer side -- usually fine, occasionally a
silent bug. New parser fields should land alongside the matching
loader arm in both `spinel_analyze.rb` and `spinel_codegen.rb`.

## Limitations

- **Integer overflow**: `IntegerNode.value` is encoded as a signed
  64-bit decimal. Literals outside `[INT64_MIN, INT64_MAX]` are
  truncated by `pm_int_value()`; the AOT compiler then auto-promotes
  to bigint at use sites it can recognise (loop multiplication,
  fibonacci-style addition). See [README.md](../README.md) §Bigint.
- **Floating point**: `%.17g` round-trips IEEE-754 doubles but the
  surface text isn't human-readable for boundary cases. Codegen uses
  the literal text directly in C output, so the encoded form drives
  what the C compiler sees.
- **Encoding**: bytes ≥ 0x80 pass through verbatim, so the parser
  assumes UTF-8. ASCII-7 sources are always safe; mixed-encoding
  sources are not supported (matches the README's "no encoding"
  limitation).
- **Source locations**: not emitted. Errors reported by `spinel_parse`
  include source positions (via libprism's diagnostic list), but the
  serialized AST drops them -- downstream errors point at AST node
  ids, not source lines. This is a deliberate trade-off for smaller
  AST files; adding a `LOC` record per node would roughly double the
  serialized size of `spinel_codegen.rb`'s ~123 K nodes.

## See also

- [docs/ANALYZE-IR.md](ANALYZE-IR.md) -- the `.ir` format
  `spinel_analyze` produces (consumed by `spinel_codegen`).
- [docs/CLASS-OBJECT.md](CLASS-OBJECT.md) -- the sp_Class value-type
  design and the per-program tables Spinel emits for class
  introspection.
- [docs/FFI.md](FFI.md) -- direct C-call declarations.
- `spinel_parse.c` -- the C frontend that produces this format.
- `spinel_analyze.rb` -- type inference; consumes `.ast`, produces `.ir`.
- `spinel_codegen.rb` -- C emission; consumes `.ast` + `.ir`.
