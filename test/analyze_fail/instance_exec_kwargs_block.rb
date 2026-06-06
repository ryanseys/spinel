class Box
  def initialize(v)
    @v = v
  end
end

class BoxPlus < Box
end

r = BoxPlus.new(0)
puts(r.instance_exec(k: 9) { |k:| k })
