def dbl(x)
  x * 2
end

def show(x)
  puts x * 10
end

# &method(:name) forwarded to map / each
p [1, 2, 3].map(&method(:dbl))
[1, 2, 3].each(&method(:show))

# method(:name).to_proc, called directly and via a variable
p method(:dbl).to_proc.call(5)
pr = method(:dbl).to_proc
p pr.call(7)

# &m where m holds a Method
m = method(:dbl)
p [4, 5].map(&m)

# instance method bound to a receiver, forwarded
class Mult
  def initialize(f)
    @f = f
  end

  def by(x)
    x * @f
  end
end

mm = Mult.new(3)
p [1, 2, 3].map(&mm.method(:by))
