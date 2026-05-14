# CRuby 4.0.4: Integer(nil) raises TypeError ("can't convert nil into
# Integer"), NOT ArgumentError. Distinguishable so rescues that catch
# only ArgumentError don't swallow it.

# Literal nil → TypeError
begin
  Integer(nil)
rescue TypeError => e
  puts "TypeError: #{e}"
rescue ArgumentError => e
  puts "WRONG: ArgumentError: #{e}"
end

# Rescuing ArgumentError does NOT catch this
begin
  Integer(nil)
rescue ArgumentError
  puts "WRONG: caught as ArgumentError"
rescue TypeError
  puts "OK: TypeError propagated past ArgumentError rescue"
end
