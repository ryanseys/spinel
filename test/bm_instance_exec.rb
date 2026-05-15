# Test instance_exec { |args| block } direct-path lifting.
# Sibling of bm_instance_eval.rb -- exercises the receiver-resolution
# cases that the analyze pass handles (top-level local, ivar inside
# class method, method param, method-local copy, interleaved across
# different classes / instances). Each section's output is compared
# against CRuby by `make test`.
#
# v1 baseline ships with @iexec_return_types locked to "void", so
# none of these sections relies on the call's return value -- they
# either mutate the receiver's state (and read it back via a regular
# method afterwards) or perform side effects (puts). Once the
# expression-position follow-up wires up infer_iexec_body_return_types,
# call-site usage in expression position (`x = recv.instance_exec(...) { ... }`)
# becomes meaningful and a sibling test will cover it.

class Counter
  attr_accessor :n

  def initialize
    @n = 0
  end

  def bump
    @n = @n + 1
  end
end

# ---- 1. Top-level basic form, one arg ----
c = Counter.new
c.instance_exec(5) { |x| @n = @n + x }
c.instance_exec(7) { |x| @n = @n + x }
puts c.n    # 12

# ---- 2. Two args at the call site forward into two block params ----
c2 = Counter.new
c2.instance_exec(3, 4) { |a, b| @n = @n + a * b }
c2.instance_exec(2, 5) { |a, b| @n = @n + a * b }
puts c2.n   # 22

# ---- 3. Methods called inside the block dispatch through self ----
class Routes
  attr_accessor :entries

  def initialize
    @entries = "init".split(",")
    @entries.pop
  end

  def get(path)
    @entries.push("GET " + path)
  end

  def post(path, kind)
    @entries.push("POST " + path + " " + kind)
  end
end

app = Routes.new
app.instance_exec("/x") { |p| get(p) }
app.instance_exec("/y", "json") { |p, k| post(p, k) }
i = 0
while i < app.entries.length
  puts app.entries[i]
  i = i + 1
end

# ---- 4. instance_exec inside a top-level while loop ----
c3 = Counter.new
i = 0
while i < 3
  c3.instance_exec(i) { |k| @n = @n + k + 1 }
  i = i + 1
end
puts c3.n   # 6  (1 + 2 + 3)

# ---- 5. Different objects interleaved ----
a = Counter.new
b = Routes.new
a.instance_exec(1) { |x| @n = @n + x }
b.instance_exec("/p") { |p| get(p) }
a.instance_exec(2) { |x| @n = @n + x }
b.instance_exec("/q", "html") { |p, k| post(p, k) }
puts a.n              # 3
puts b.entries.length # 2
puts b.entries[0]     # GET /p
puts b.entries[1]     # POST /q html

# ---- 6. Receiver from ivar inside a class method ----
class Boot
  attr_accessor :app

  def initialize
    @app = Routes.new
  end

  def install
    @app.instance_exec("/from-ivar") { |p| get(p) }
    @app.instance_exec("/with-kind", "json") { |p, k| post(p, k) }
  end
end

boot = Boot.new
boot.install
puts boot.app.entries.length # 2
puts boot.app.entries[0]     # GET /from-ivar
puts boot.app.entries[1]     # POST /with-kind json

# ---- 7. Receiver from method param ----
class Wire
  def wire_param(r, prefix)
    r.instance_exec(prefix) { |p| get(p) }
  end
end

w = Wire.new
shared = Routes.new
w.wire_param(shared, "/from-param")
puts shared.entries.length # 1
puts shared.entries[0]     # GET /from-param

# ---- 8. Method-local copy of a class instance ----
class WireLocal
  def setup_routes
    routes = Routes.new
    routes.instance_exec("/local-a") { |p| get(p) }
    routes.instance_exec("/local-b", "html") { |p, k| post(p, k) }
    routes
  end
end

wlocal = WireLocal.new
ret = wlocal.setup_routes
puts ret.entries.length # 2
puts ret.entries[0]     # GET /local-a
puts ret.entries[1]     # POST /local-b html

puts "done"
