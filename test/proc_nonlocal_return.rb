# A `return` inside a non-lambda Proc is a non-local return: it returns
# from the method that lexically created the Proc, not just from the Proc
# body (Ruby's block-return semantics). A lambda's `return` stays local.

def direct
  proc { return 99 }.call
  1
end

puts direct

# Conditional non-local return, with the Proc reading an enclosing local.
def classify(n)
  pr = proc { return 100 if n > 10; 5 }
  pr.call
end

puts classify(20)
puts classify(3)

# Early return out of an `each` block — the canonical real-world shape.
def first_big(a)
  a.each { |x| return x if x > 2 }
  -1
end

puts first_big([1, 2, 5, 3])
puts first_big([1, 1])

# A lambda's return is local: it yields the lambda's value, the method
# continues, and the method's own value is returned.
def with_lambda
  f = lambda { return 7 }
  v = f.call
  v + 1
end

puts with_lambda
