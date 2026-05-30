# Float#ceil / #floor / #round / #truncate return type

In CRuby these four methods choose their return class from the **runtime
value** of the `ndigits` argument:

| call            | CRuby returns      |
|-----------------|--------------------|
| `1.9.round`     | `2`    (Integer)   |
| `1.9.round(0)`  | `2`    (Integer)   |
| `1.9.round(-1)` | `0`    (Integer)   |
| `1.234.round(2)`| `1.23` (Float)     |

i.e. Integer when `ndigits <= 0`, Float when `ndigits > 0`.

## Why spinel can't follow that exactly

Spinel is an ahead-of-time compiler with static result types. A rule
keyed on the *value* of `ndigits` is unrepresentable in the general
case:

- `x.round(n)` with a variable `n` would need an Integer-or-Float
  return type chosen at runtime. Forcing it into a boxed/poly value to
  carry both would discard static typing for every expression the
  result flows into, which runs against spinel's static-typing design.
- `x.round(*args)` / `x.round(*[])` can't be classified at all: whether
  any argument is actually present is only known at runtime.

## The rule spinel uses

The return type is decided by argument **presence**, not value:

- **no argument** -> `Integer`  (`x.round`, `x.ceil`, ...)
- **any argument present** -> `Float`  (`x.round(0)`, `x.round(-1)`,
  `x.round(2)`, `x.round(n)`, `x.round(*args)`, ...)

This is fully static, never boxes, and always produces the numerically
correct value (`15.5.round(-1)` is `20.0`, which `== 20`). The single
divergence from CRuby is the *type label* on the ndigits form when
`ndigits <= 0`: spinel returns a Float (`10.0`) where CRuby returns an
Integer (`10`). The values are numerically equal; only `#class` and the
default string form (`"10.0"` vs `"10"`) differ.

Code that needs an Integer from a negative/zero ndigits can convert
explicitly: `x.round(-1).to_i`.

The implementation lives in `infer_method_name_type` (spinel_analyze.rb)
and `compile_float_method_expr` (spinel_codegen.rb); both branch only on
`@nd_arguments[nid] >= 0`.
