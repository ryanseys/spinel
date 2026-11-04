# A method taking *rest and **opts on a receiver of more than one class:
# each call still hands the method a new Array and a new Hash, whichever
# class answers, and sites with other argument shapes still dispatch alike.

class Pen
  def initialize = @kept = []
  def tag(*parts, **opts)
    parts << :pen
    @kept << opts
    "pen #{parts.inspect} #{opts.inspect} #{@kept.size}"
  end
  def kept_same? = @kept.size > 1 && @kept[0].equal?(@kept[1])
end

class Ink
  def tag(*parts, **opts) = "ink #{parts.size} #{opts.keys.inspect}"
end

class Nib
  def tag(*parts, **opts)
    opts[:seen] = true
    "nib #{parts.first.inspect} #{opts.inspect}"
  end
end

class Cap
  def other = :cap
end

def pick(n)
  case n % 4
  when 0 then Pen.new
  when 1 then Ink.new
  when 2 then Nib.new
  else Cap.new
  end
end

h = { size: 2 }
pen = Pen.new
4.times do |i|
  x = pick(i)
  begin
    puts x.tag(i, "a")
    puts x.tag(**h)
    puts x.tag(:s)
    puts x.tag(i, "b", k: 1)
    puts x.tag(i, "c", k: 1)
  rescue NoMethodError => e
    puts "no #{e.name}"
  end
end
p h
y = [pen, Ink.new][ARGV.size]
puts y.tag(1, k: 2)
puts y.tag(1, k: 2)
p pen.kept_same?
