# Array#sum with a String initial value on a string array is concatenation
# (init + e0 + e1 + ...), not a numeric sum. Spinel previously typed the
# accumulator as int and emitted an integer-to-pointer C compile error.
p ["a", "b", "c"].sum("")     # "abc"
p ["a", "b", "c"].sum("*")    # "*abc"

def f(a); a.sum("*"); end     # param-routed, defeats constant folding
p f(["a", "b"])               # "*ab"
p f([])                       # "*"  (empty array returns the init)

def g(a, init); a.sum(init); end
p g(["x", "y"], ">>")         # ">>xy"

# The result is a real String usable downstream.
p ["a", "b"].sum("_").upcase  # "_AB"
