# CRuby 4.0.4: Integer(f) truncates toward zero. NaN / +/-Infinity raise
# FloatDomainError; finite-but-too-large-for-int64 raises RangeError.

# Truncation
puts Integer(1.7)
puts Integer(-1.7)
puts Integer(0.0)
puts Integer(-0.0)
puts Integer(1.0)
puts Integer(1.999999)

# FloatDomainError cases
begin
  Integer(Float::NAN)
rescue FloatDomainError => e
  puts "FDE: #{e}"
end

begin
  Integer(Float::INFINITY)
rescue FloatDomainError => e
  puts "FDE: #{e}"
end

begin
  Integer(-Float::INFINITY)
rescue FloatDomainError => e
  puts "FDE: #{e}"
end

# Finite but outside int64 range → RangeError (CRuby uses the same
# class for "float too large to fit in mrb_int", distinct from
# FloatDomainError reserved for NaN/Inf).
begin
  Integer(1.0e30)
rescue RangeError => e
  puts "RE: too big"
end

begin
  Integer(-1.0e30)
rescue RangeError => e
  puts "RE: too small"
end
