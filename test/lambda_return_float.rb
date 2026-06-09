# An arrow lambda's explicit `return <expr>` yields that expression as
# the lambda's value (lambda return is local), and a float-returning
# arrow must carry the float back through `.call`, not 0.0.

# explicit return as the lambda's tail expression
l = -> { return 99 }
puts l.call + 1

# float return via a variable
f = -> { 3.5 }
puts f.call
