# A class_eval reopen body that mixes in a non-definition statement
# (here a `puts`) is more than a static reopen -- the block must run at
# load time. Spinel only models `def` / `define_method` bodies, so it
# rejects this precisely rather than dropping the side effect.
class Box
  def initialize(v)
    @v = v
  end
end

Box.class_eval do
  def doubled
    @v * 2
  end

  puts "loaded"
end
