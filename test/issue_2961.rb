class Arr
  include Comparable
  def <=>(o); [1]; end
end
p (Arr.new < Arr.new  rescue $!.class)
p (Arr.new > Arr.new  rescue $!.class)
p (Arr.new == Arr.new rescue $!.class)
class Num; include Comparable; def <=>(o); "x"; end; end
p (Num.new < Num.new rescue $!.class)
class Ok; include Comparable; attr_reader :v; def initialize(v);@v=v;end; def <=>(o); v <=> o.v; end; end
p [Ok.new(3), Ok.new(1), Ok.new(2)].sort.map(&:v)
