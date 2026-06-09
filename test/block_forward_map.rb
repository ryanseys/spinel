# A forwarded Proc/lambda block to Array#map over int elements collects
# each result into a new int array (the callable returns int). Rides the
# unified sp_Proc call ABI; complements block_forward_each.
dbl = proc { |x| x * 2 }
puts [1, 2, 3].map(&dbl).inspect

inc = ->(n) { n + 100 }
puts [5, 6].map(&inc).inspect

class Nums
  def initialize
    @ns = [10, 20]
  end

  def doubled(&blk)
    @ns.map(&blk)
  end
end

puts Nums.new.doubled { |x| x * 2 }.inspect
