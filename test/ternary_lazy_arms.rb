class D
  def initialize(t)
    @t = t
  end
  def t
    @t
  end
end
f = "/tmp/nope_#{Process.pid}"
d = File.exist?(f) ? D.new(File.read(f)) : D.new("")
puts d.t.length
g = File.exist?("/etc/hostname") ? D.new(File.read("/etc/hostname")) : D.new("")
puts g.t.length > 0
