# A direct instance_exec receiver can be an array element: the element
# type resolves to the object class, so the block's self rebinds to that
# instance. Box has a subclass so Spinel keeps the elements heap. (This
# locks the obj-typed receiver fallback in recv_class_idx_for_rebind.)
class Box
  def initialize(v)
    @v = v
  end
end

class BoxPlus < Box
end

arr = [BoxPlus.new(3), BoxPlus.new(4)]
puts arr[0].instance_exec { @v + 100 }
puts arr[1].instance_exec { @v * 10 }
