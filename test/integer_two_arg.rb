# CRuby 4.0.4 Integer(s, base) with explicit base. Base 0 = auto-detect.
# Base must be 0 or in [2, 36]. Prefix in string must match explicit base
# (or auto-detect with base 0); mismatch raises ArgumentError.

# Bases without prefix
puts Integer("10", 2)
puts Integer("10", 8)
puts Integer("10", 10)
puts Integer("10", 16)
puts Integer("ff", 16)
puts Integer("z", 36)
puts Integer("ZZ", 36)

# With matching prefix
puts Integer("0x10", 16)
puts Integer("0b101", 2)
puts Integer("0o17", 8)
puts Integer("017", 8)

# Base 0 (auto)
puts Integer("0x10", 0)
puts Integer("0b101", 0)
puts Integer("017", 0)
puts Integer("10", 0)

# Mismatching prefix → ArgumentError
puts (Integer("0x10", 10)  rescue -1)
puts (Integer("0b101", 10) rescue -1)
puts (Integer("0o17", 10)  rescue -1)

# Invalid radix
puts (Integer("10", 1)  rescue -1)
puts (Integer("10", 37) rescue -1)
