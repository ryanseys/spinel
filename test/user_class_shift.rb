# Tests user-class << and >> dispatch instead of C bit-shift fallback.
class Bag
  def initialize
    @items = []
  end
  def <<(x)
    @items << x
    self
  end
  def >>(n)
    @items.length + n
  end
  def size
    @items.length
  end
end

bag = Bag.new << 1 << 2 << 3
puts bag.size           # 3
puts bag >> 10          # 13
