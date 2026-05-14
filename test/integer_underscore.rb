# CRuby 4.0.4 Integer() accepts underscore separators between digits.
# Rejects leading, trailing, doubled, or sign-/prefix-adjacent underscores.

# Well-formed underscores
puts Integer("1_000_000")
puts Integer("-1_000")
puts Integer("+1_000")
puts Integer(" 1_2_3 ")

# Negative cases — caught via rescue
puts (Integer("_42")   rescue -1)
puts (Integer("42_")   rescue -1)
puts (Integer("4__2")  rescue -1)
puts (Integer("1_")    rescue -1)
puts (Integer("_1")    rescue -1)
puts (Integer("-_42")  rescue -1)
puts (Integer("+_42")  rescue -1)
