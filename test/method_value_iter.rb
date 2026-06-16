# Forwarding a top-level Method (&method(:m)) to an array iterator: each / map /
# collect / select / reject. The method is called per element through its own
# typed C signature -- no mrb_int laundering -- so int->string returns work.
# Int elements.

def triple(x) = x * 3
def to_label(n) = n.to_s
def positive(x) = x > 0
def even(x) = x.even?
def show(x)
  puts "got #{x}"
end

p [1, 2, 3].map(&method(:triple))
p [1, 2, 3].collect(&method(:to_label))      # int element -> string result
p [-1, 2, -3, 4].select(&method(:positive))
p [1, 2, 3, 4].reject(&method(:even))
[10, 20].each(&method(:show))                # side effect per element
