# Float#round/ceil/floor/truncate return type is presence-based in
# spinel: the no-arg form returns Integer, ANY ndigits arg (including
# `(0)`) returns Float. This intentionally diverges from CRuby, which
# returns Integer for ndigits <= 0 -- spinel can't key the result type
# on a runtime value statically. Values stay numerically correct.
# See docs/FLOAT-ROUNDING.md.
puts 3.5.round(0).inspect
puts 3.5.round.inspect
puts 3.5.round(1).inspect
puts 3.5.round(2).inspect

puts 3.7.ceil(0).inspect
puts 3.7.ceil.inspect
puts 3.7.ceil(1).inspect

puts 3.7.floor(0).inspect
puts 3.7.floor.inspect

puts 3.7.truncate(0).inspect
puts 3.7.truncate.inspect
