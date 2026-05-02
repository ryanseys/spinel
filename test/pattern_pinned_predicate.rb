# Pattern matching: PinnedVariableNode (^v), PinnedExpressionNode (^(expr)),
# MatchPredicateNode (value in pattern -- expression form).
#
# These are the simplest of the deferred pattern nodes. They reuse
# Spinel's existing compile_in_pattern dispatch (which already handles
# literal and type patterns) by adding equality-against-pinned arms.
# MatchPredicate is the expression form of `case/in`: it returns
# boolean directly without surrounding case/end.
#
# Tests use `if` predicates rather than printing the boolean directly
# because Spinel renders bool as 1/0 (C int) while CRuby renders as
# "true"/"false". Both branches of `if` agree, so the test is portable.

# MatchPredicate with literal patterns -- the most common form.
puts "lit_match" if 5 in 5             #=> lit_match
puts "lit_str" if "hi" in "hi"         #=> lit_str
puts "wrong" if 5 in 6                 # nothing

# MatchPredicate with type patterns -- ConstantReadNode dispatch.
def kind(x)
  if x in Integer
    "int"
  elsif x in String
    "str"
  else
    "other"
  end
end
puts kind(42)                          #=> int
puts kind("hello")                     #=> str

# MatchPredicate with PinnedVariable -- compare against a pinned local.
target = 7
puts "pin_match" if 7 in ^target       #=> pin_match
puts "pin_no" if 8 in ^target          # nothing

# MatchPredicate with alternation patterns.
def small_or_big(n)
  if n in 0 | 100
    "extreme"
  else
    "middle"
  end
end
puts small_or_big(0)                   #=> extreme
puts small_or_big(50)                  #=> middle
puts small_or_big(100)                 #=> extreme

# Use case/in with a pinned variable.
expected = "hello"
case "hello"
in ^expected
  puts "case_match"                    #=> case_match
end

# PinnedExpression: ^(literal expression).
case 10
in ^(5 + 5)
  puts "ten"                           #=> ten
end
