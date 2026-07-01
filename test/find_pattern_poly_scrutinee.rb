# A find pattern (`in [*head, x, y, *tail]`) against a poly scrutinee -- a value
# not statically known to be an array (a recursive parameter, or one unified with
# a scalar). It is coerced to a poly array at runtime; a non-array (or nil) never
# matches, so the else arm runs.
def f(dish)
  case dish
  in [*head, :a, :b, *tail]
    [head, :ab, tail]
  else
    dish
  end
end
p f([:x, :a, :b, :y])   # [[:x], :ab, [:y]]
p f([:a, :b])           # [[], :ab, []]
p f([:p, :q])           # [:p, :q]
p f(:scalar)            # :scalar (a non-array never matches)
