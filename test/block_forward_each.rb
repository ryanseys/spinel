# A method taking a block (&blk) can forward it to an int array's
# #each: each element is handed to the proc via the standard call ABI.
# Covers a direct forward and the common ivar-collection delegation
# shape (`def each(&blk); @items.each(&blk); end`). String/object
# element forwarding needs proc-param inference and is a follow-up.
def run_ints(&blk)
  [1, 2, 3].each(&blk)
end
run_ints { |x| puts x * 10 }

class Bag
  def initialize
    @items = [5, 6]
  end

  def each(&blk)
    @items.each(&blk)
  end
end

Bag.new.each { |n| puts n + 100 }
