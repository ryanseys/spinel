# Pattern matching: CapturePatternNode (`in <subpat> => name`).
#
# Matches a sub-pattern, then binds the scrutinee value to the
# target local with the narrowed type. The binding fires only
# after the sub-pattern predicate succeeds.

# --- Class capture from int scrutinee
case 42
in Integer => cap_int_n
  puts cap_int_n
end

# --- Class capture from string scrutinee
case "hello"
in String => cap_str_s
  puts cap_str_s
end

# --- Float class capture
case 3.14
in Float => cap_flt_f
  puts cap_flt_f
end

# --- Class capture from poly scrutinee narrows to int
def classify(v)
  case v
  in Integer => cap_n
    "int: " + cap_n.to_s
  in String => cap_s
    "str: " + cap_s
  end
end
puts classify(7)
puts classify("hi")

# --- Literal-value capture binds same scalar
case 0
in 0 => cap_lit_z
  puts cap_lit_z
end

# --- Capture inside alternation arm
def either(v)
  case v
  in Integer => cap_alt_n
    "got int: " + cap_alt_n.to_s
  in Float => cap_alt_f
    "got flt"
  end
end
puts either(99)
puts either(2.5)

# --- Symbol capture
case :foo
in :foo => cap_sym
  puts cap_sym
end
