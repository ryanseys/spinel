# A frozen empty-literal constant (`EMPTY = [].freeze`) must initialize with
# the constructor of the constant's DECLARED type: usage below types EMPTY as
# a poly array, and emitting the IntArray default into that slot reinterprets
# the struct layout (size read through the wrong header). Same for the hash
# counterpart.
class Box
  EMPTY = [].freeze
  EMPTY_HASH = {}.freeze

  def kids
    @kids || EMPTY
  end

  def kids!
    @kids ||= []
  end

  def meta
    @meta || EMPTY_HASH
  end
end

empty_box = Box.new
p empty_box.kids.size
p empty_box.meta.size

full = Box.new
full.kids! << "a"
full.kids! << :b
p full.kids.size
