# A float-begin range carries its exact endpoints (sp_FRange) instead of
# truncating into the int-backed representation: min/max/first/last/cover?
# and clamp are exact through parameters; iteration raises like CRuby.
def mn(x) = x.min
def mx(x) = x.max
p mn(303.2..908.9)
p mx(303.2..908.9)
p mn(3.0..1.0)
def fst(x) = x.first
p fst(1.5..9.5)
p (1.5...9.5).last
p (1.5...9.5).exclude_end?

def cl(v, r) = v.clamp(r)
p cl(5.0, 1.0..3.0)
p cl(0.0, 1.0..3.0)
p cl(2.0, 1.0..3.0)
p 2.clamp(1.0..3.0)
p 5.clamp(1.0..3.0)

def cov(r, v) = r.cover?(v)
p cov(1.5..2.5, 2.0)
p cov(1.5..2.5, 3.0)
p (1.0...2.0) === 2.0
p (1.0..2.0).include?(2)

p (1.0..3.0)
p (1.0...3.0).to_s

def iter_err(r)
  r.to_a
rescue TypeError => e
  "#{e.class}: #{e.message}"
end
p iter_err(1.0..3.0)
begin
  (1.0...3.0).max
rescue TypeError => e
  puts "#{e.class}: #{e.message}"
end
begin
  2.0.clamp(1.0...3.0)
rescue ArgumentError => e
  puts "#{e.class}: #{e.message}"
end

p (1.0..3.0).step(0.5).to_a

def cw(x)
  case x
  when 1.5..3.5 then "in"
  else "out"
  end
end
p cw(2.0)
p cw(10.0)
p cw(1)
p cw(Time.at(0))

def inc(r, x) = r.include?(x)
p inc(1.5..3.5, "hello")
p inc(1.5..3.5, 2.0)

begin
  5.0.clamp(3.0..1.0)
rescue => e
  puts "#{e.class}: #{e.message}"
end
begin
  5.clamp(3.0..1.0)
rescue => e
  puts "#{e.class}: #{e.message}"
end

begin
  Random.rand(3.0..1.0)
rescue => e
  puts "#{e.class}: #{e.message}"
end
begin
  Random.new(1).rand(3.0..1.0)
rescue => e
  puts "#{e.class}: #{e.message}"
end
p rand(3.0..1.0)
p rand(1.0...1.0)
p Random.new(1).rand(1.0..1.0).class
