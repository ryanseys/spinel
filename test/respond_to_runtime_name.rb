# respond_to? with a runtime-computed name resolves against the program's
# closed candidate set: builtin surfaces, user methods and attrs (visibility
# honored), and universal Kernel methods; an unknown name answers false.
def probe_str(m) = "hi".respond_to?(m)
p probe_str(:upcase)
p probe_str(:size)
p probe_str(:nonexistent_zzz)

def probe_arr(m) = [1, 2].respond_to?(m)
p probe_arr(:sum)
p probe_arr(:upcase)

class Widget
  attr_accessor :w
  def area = 4
  private
  def secret_calc = 9
end

def probe_widget(o, m) = o.respond_to?(m)
w = Widget.new
p probe_widget(w, :area)
p probe_widget(w, :w)
p probe_widget(w, :w=)
p probe_widget(w, :secret_calc)
p probe_widget(w, :missing_zzz)

def probe_all(o, m) = o.respond_to?(m, true)
p probe_all(w, :secret_calc)

names = [:upcase, :area, :bogus_qqq]
picked = names[0]
p "s".respond_to?(picked)

def probe_int(m) = 42.respond_to?(m)
p probe_int(:even?)
p probe_int(:upcase)

def probe_universal(o, m) = o.respond_to?(m)
p probe_universal(w, :class)
p probe_universal(w, :inspect)
