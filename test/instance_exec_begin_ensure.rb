# A begin/rescue/ensure used as the value of an instance_exec block must
# yield the begin (or rescue) body's last expression, not 0. The ensure
# clause still runs for its side effects, including on the exception
# path. Box has a subclass so Spinel keeps it heap-allocated.
class Box
  def initialize(v)
    @v = v
  end
end

class BoxPlus < Box
end

b = BoxPlus.new(10)

# ensure-only: value is the begin body; ensure runs afterward.
r1 = b.instance_exec do
  begin
    @v
  ensure
    puts "ensured"
  end
end
puts r1

# begin/rescue, no exception: the begin body's value.
r2 = b.instance_exec do
  begin
    @v + 5
  rescue
    -1
  end
end
puts r2

# begin/rescue with an exception: the rescue body's value.
r3 = b.instance_exec do
  begin
    raise "boom"
  rescue
    99
  end
end
puts r3
