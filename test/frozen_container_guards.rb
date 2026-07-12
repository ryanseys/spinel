# Frozen Array/Hash mutations raise FrozenError with CRuby's exact message,
# including the receiver inspect tail, across typed, poly, and empty-literal
# receivers, in expression and statement position.
def err
  yield
  "no error"
rescue => e
  "#{e.class}: #{e.message}"
end

# --- Array#clear across representations ---
def clr_int(a) = a.clear
def clr_flt(a) = a.clear
def clr_str(a) = a.clear
def clr_poly(a) = a.clear
puts err { clr_int([1, 2].freeze) }
puts err { clr_flt([1.5].freeze) }
puts err { clr_str(["a", "b"].freeze) }
puts err { clr_poly([1, "x", :y].freeze) }
frozen_poly = [1, "x"].freeze
puts err { frozen_poly.clear }
puts err { clr_int([1, 2]) }
p clr_int([3, 4])

# --- other array mutators carry the same message shape ---
puts err { [7].freeze.push(8) }
puts err { [7].freeze << 8 }
def apnd(a) = a << 8
puts err { apnd([1].freeze) }
puts err { ["q"].freeze.pop }
def spl(a) = (a[0..1] = [9]; a)
puts err { spl([1, 2, 3].freeze) }

# --- frozen String append family (chained receiver) ---
puts err { "abc".freeze << "x" }
puts err { "abc".freeze.concat("x", "y") }
puts err { "abc".freeze.prepend("x") }
puts err { "abc".freeze.insert(1, "x") }
s = "abc"
p s << "x"

# --- frozen Hash stores through a poly param ---
def store_sym(h) = h[:x] = 1
def store_str(h) = h["x"] = 1
def bump(h) = h[:x] = (h[:x] || 0) + 1
puts err { store_sym({}.freeze) }
puts err { store_sym({ a: 1 }.freeze) }
puts err { store_str({ "a" => 2 }.freeze) }
puts err { bump({ b: 3 }.freeze) }
puts err { store_sym({}) }

# --- Hash#clear across key/value representations ---
def hclr_sym(h) = h.clear
def hclr_ss(h) = h.clear
def hclr_ii(h) = h.clear
def hclr_si(h) = h.clear
puts err { hclr_sym({ a: 1, b: 2 }.freeze) }
puts err { hclr_ss({ "k" => "v" }.freeze) }
puts err { hclr_ii({ 1 => 2 }.freeze) }
puts err { hclr_si({ "n" => 5 }.freeze) }
unfrozen = { c: 9 }
hclr_sym(unfrozen)
p unfrozen.size

# --- freeze state itself ---
p({}.freeze.frozen?)
p [1].freeze.frozen?
p({ a: 1 }.freeze.frozen?)
