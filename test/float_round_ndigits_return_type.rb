# Float#ceil/floor/round/truncate return type is presence-based:
# no argument -> Integer, any ndigits argument -> Float (regardless of
# the ndigits value). Values stay numerically correct. See
# docs/FLOAT-ROUNDING.md. (Supersedes the CRuby value-keyed rule, which
# spinel can't represent statically.)

# no-arg -> Integer
puts 1.9.ceil.class
puts 1.9.floor.class
puts 1.5.round.class
puts 1.9.truncate.class

# any ndigits arg -> Float
puts 1.9.ceil(0).class
puts 1.9.ceil(-1).class
puts 1.111.ceil(1).class
puts 1.9.round(0).class
puts 1.9.round(-1).class
puts 1.234.round(2).class
puts 1.9.truncate(-1).class

# numerically-correct values
puts 1.9.ceil
puts 1.234.round(2)
puts 1.999.floor(1)
