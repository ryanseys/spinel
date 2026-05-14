# CRuby 4.0.4 Integer() auto-detects base prefix (0x / 0b / 0o / 0d / leading-0).

# Hex
puts Integer("0x10")
puts Integer("0X10")
puts Integer("-0xff")
puts Integer("0xff_ff")

# Binary
puts Integer("0b101")
puts Integer("0B1111")
puts Integer("-0b10")

# Octal
puts Integer("0o17")
puts Integer("0O17")
puts Integer("017")        # legacy octal

# Explicit decimal
puts Integer("0d99")

# Invalid digits per base
puts (Integer("0xZZ")  rescue -1)
puts (Integer("0b102") rescue -1)  # 2 not binary
puts (Integer("08")    rescue -1)  # 8 not octal
puts (Integer("0b")    rescue -1)  # bare prefix

# Underscore adjacent to prefix is invalid
puts (Integer("0x_1f") rescue -1)
# Underscore between hex digits is fine
puts Integer("0x1_f")
