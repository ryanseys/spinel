# Pattern matching depth -- ArrayPatternNode + PinnedVariableNode +
# PinnedExpressionNode. Literal-only requireds for arrays; pin
# patterns compare by `==` against the existing local/expression.
# HashPattern, FindPattern, and CapturePattern are parsed but
# error out at compile time as "not yet supported"; this PR's
# scope is the predicate-only subset.

# --- Array literals: int_array scrutinee
case [1, 2, 3]
in [1, 2, 3]
  puts "int exact"
in [1, 2, 4]
  puts "int off"
end

# --- Array length mismatch falls through
case [1, 2]
in [1, 2, 3]
  puts "len3"
in [1, 2]
  puts "len2"
end

# --- Empty array
case []
in []
  puts "empty"
end

# --- String elements
case ["a", "b"]
in ["a", "b"]
  puts "str match"
in ["a", "c"]
  puts "str off"
end

# --- Alternation still works (regression check)
def describe(v)
  case v
  in Integer | Float
    "number"
  in String
    "string"
  in nil
    "nil"
  in true | false
    "bool"
  end
end
puts describe(42)
puts describe(3.14)
puts describe("hi")
puts describe(nil)
puts describe(true)

# --- PinnedVariableNode: match against existing local
x = 42
case 42
in ^x
  puts "pin match int"
in 0
  puts "zero"
end

# Pin against string
s = "hello"
case "hello"
in ^s
  puts "pin match str"
end

# Pin no match -> fall to else
y = 10
case 42
in ^y
  puts "pin y"
else
  puts "no pin y"
end

# --- PinnedExpressionNode: `^(expr)` -- pin against arbitrary expr
val = 7
case 21
in ^(val * 3)
  puts "pin expr 21"
else
  puts "miss"
end
