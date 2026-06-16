# A method taking a block (&blk) can forward it to single-element-arg array
# iterators beyond #each / #map. The forwarded `&blk` resolves to the caller's
# inlined literal block, so the full typed-iterator codegen runs for each.

# select / reject (filter)
def keep(arr, &blk)
  arr.select(&blk)
end
p keep([1, 2, 3, 4, 5]) { |x| x > 2 }

def drop(arr, &blk)
  arr.reject(&blk)
end
p drop([1, 2, 3, 4, 5]) { |x| x.even? }

# sort_by / min_by (key)
def ordered(arr, &blk)
  arr.sort_by(&blk)
end
p ordered([3, 1, 2]) { |x| 0 - x }

def smallest(arr, &blk)
  arr.min_by(&blk)
end
p smallest([3, 1, 2]) { |x| 0 - x }

# partition -> [matching, non-matching]
def split(arr, &blk)
  arr.partition(&blk)
end
p split([1, 2, 3, 4, 5]) { |x| x > 2 }
