# A proc/lambda `.call` returns its body's value type, not the int the
# proc-fn ABI slot defaults to. This holds for a direct literal receiver,
# a proc-variable receiver, and when the call is a method's return value
# (implicit tail or explicit `return`).

# Direct literal receiver.
p proc { :sym }.call
p proc { "str" }.call
p proc { 4.25 }.call
p(-> { :arrow }.call)

# Proc-variable receiver.
sym = proc { :hi }
p sym.call
str = proc { "hey" }
p str.call
flt = proc { 1.5 }
p flt.call

# Method whose implicit-tail value is a proc call.
def tail_sym
  pr = proc { :done }
  pr.call
end
p tail_sym

def tail_str
  pr = proc { "tail" }
  pr.call
end
p tail_str

# Method with an explicit `return <proc>.call`.
def ret_flt
  pr = proc { 2.5 }
  return pr.call
end
p ret_flt

# Conditional explicit return through a proc call.
def pick(n)
  pr = proc { "big" }
  return pr.call if n > 0
  "small"
end
p pick(5)
p pick(-1)

# Assigning a proc call to a temp, then using the temp by its value type.
def via_temp
  pr = proc { "kept" }
  x = pr.call
  x.upcase
end
p via_temp

def via_temp_float
  pr = proc { 2.0 }
  x = pr.call
  x * 3
end
p via_temp_float

# Control: int return still works unchanged.
def tail_int
  pr = proc { 42 }
  pr.call
end
p tail_int
