# A Method the analysis cannot name -- kept in an instance variable, a
# constant, a global or a Hash, or a local written to two methods -- called
# through its to_proc or handed over as a block: those calls type the
# target's parameters, whatever the arguments are.
class Show
  def initialize(flag) = @m = flag ? method(:one) : method(:two)
  def one(x) = "one:#{x.inspect}"
  def two(a, b) = "two:#{a.inspect},#{b.inspect}"
  def show1(v) = @m.to_proc.call(v)
  def show2(a, b) = @m.to_proc.call(a, b)
end
s = Show.new(true)
p s.show1(1.5)
p s.show1([1, 2])
p s.show1(nil)
p s.show1(Show)
p s.show1("str")
p Show.new(false).show2({k: 1}, :sym)

def len(s) = s.length
LEN = method(:len)
$len = method(:len)
TABLE = {len: method(:len)}
p LEN.to_proc.call("abcd")
p $len.to_proc.call("ab")
p TABLE[:len].to_proc.call("abc")

class Many
  def a(x) = "a:#{x}"
  def b(x) = "b:#{x}"

  def twice
    yield "p"
    yield "q"
  end

  def run(flag)
    m = method(:a)
    m = method(:b) if flag
    p twice(&m)
    one = method(:a)
    p twice(&one)
  end
end
Many.new.run(true)
Many.new.run(false)
