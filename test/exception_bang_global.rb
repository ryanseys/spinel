# The $! global holds the exception being handled inside a rescue.
begin
  raise "boom"
rescue
  p $!.message
end
begin
  raise ArgumentError, "bad arg"
rescue
  p $!.message
end
begin
  raise "again"
rescue
  e = $!
  p e.message
end
