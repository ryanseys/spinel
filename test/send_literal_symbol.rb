# .send(:sym, args) statically rewrites to .sym(args) at parse time.
# Non-literal symbol args (variable, interpolation) leave the call
# alone -- those require runtime dispatch which Spinel doesn't model.

class Calc
  def add(a, b)
    a + b
  end

  def double(x)
    x * 2
  end

  def hello
    "hi"
  end
end

c = Calc.new
puts c.send(:add, 3, 4)        # 7
puts c.send(:double, 21)       # 42
puts c.send(:hello)            # hi

puts "done"
