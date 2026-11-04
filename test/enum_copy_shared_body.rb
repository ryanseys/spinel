# Copies of a builtin Enumerable method made for sites whose types match
# share one body; each site still answers with its own block.

class Shelf
  include Enumerable

  def initialize(*xs)
    @xs = xs
  end

  def each(&blk)
    @xs.each(&blk)
    self
  end
end

def stock(n)
  n.even? ? Shelf.new(1, 2, 3) : [4, nil, 6]
end

a = stock(0)
b = stock(1)
c = stock(2)
lim = 2
p a.all? { |x| x && x > 0 }
p b.all? { |x| x && x > lim }
p c.all? { |x| x && x < lim }
p a.any? { |x| x.nil? }
p b.any? { |x| x.nil? }
p c.any? { |x| x == lim }
p a.none? { |x| x == 9 }
p b.none? { |x| x == 6 }
p a.filter_map { |x| x && x * 2 }
p b.filter_map { |x| x && x + lim }
p c.filter_map { |x| x && x - lim }
p a.count { |x| x && x.odd? }
p b.count { |x| x.nil? }

# a site whose block captures a variable it changes
hits = 0
p(b.all? { |x| hits += 1; x })
p hits
p(a.any? { |x| hits += 10; x == 3 })
p hits

# sites with other receiver types keep their own copies
p [1, 2, 3].all? { |x| x > 0 }
p({ k: 1 }.any? { |k, v| v == 1 })
p (1..4).filter_map { |x| x * 3 if x.even? }
