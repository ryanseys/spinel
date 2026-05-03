# Issue #219: chained calls on a user-defined `[]` operator's
# result silently lost their coercion (`.to_i` / `.length`) because
# infer_type's `mname == "[]"` branch handled built-in collection
# receivers but fell through to "int" for obj receivers.
# `bag[:id].to_i` then routed through compile_int_method_expr's
# `to_i` (which is identity) and the underlying `const char *`
# leaked through.

class Bag
  def initialize(h); @h = h; end
  def [](k); @h[k.to_sym]; end
end

bag = Bag.new({id: "42"})

# .to_i: infers as int via String#to_i
puts bag[:id].to_i              # 42
puts bag[:id].to_i + 1          # 43

# .length: routes through String#length
puts bag[:id].length            # 2

# Chained into a method expecting mrb_int
class M
  def self.find(id); id + 1; end
end
puts M.find(bag[:id].to_i)      # 43

# Integer-valued bag — .to_s works the other direction (already
# worked before the fix; pinning regression).
class IntBag
  def initialize(h); @h = h; end
  def [](k); @h[k]; end
end
ib = IntBag.new({n: 42})
puts ib[:n].to_s                # "42"
