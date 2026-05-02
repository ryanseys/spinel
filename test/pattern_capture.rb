# CapturePatternNode -- `pattern => name` binds the matched value
# to a local on success. Inner pattern can be a type check, literal,
# or another sub-pattern. Spinel emits:
#   (inner_check && (lv_name = bind_expr, 1))
# so the binding only fires when the inner pattern matches.
#
# When pred_type is poly and the inner is Integer/String/Float, the
# bind_expr unboxes from the sp_RbVal so the bound local stores the
# primitive value matching the type collect_pattern_targets gave it.

# Type capture inside a method that takes a poly param. Pattern
# matching's natural home is dispatching on poly values.
def classify(x)
  case x
  in Integer => n
    "int=" + n.to_s
  in String => s
    "str=" + s
  in Float => f
    "float=" + f.to_s
  end
end
puts classify(7)            #=> int=7
puts classify("foo")        #=> str=foo
puts classify(3.14)         #=> float=3.14

# Capture from a poly array: each element is sp_RbVal, captured as
# its unboxed primitive when the type matches.
mixed = [1, "two", 3]
mixed.each do |item|
  case item
  in Integer => n
    puts "i:" + n.to_s
  in String => s
    puts "s:" + s
  end
end
#=> i:1
#=> s:two
#=> i:3
