# p of an exception object inspects it as #<ClassName: message>.
begin
  raise "boom"
rescue => e
  x = e
  p x       # #<RuntimeError: boom>
  p e       # rescued binding directly
end

# a specific exception class with an explicit message
begin
  raise ArgumentError, "bad arg"
rescue => e
  p e       # #<ArgumentError: bad arg>
end

# an exception flowed through a method keeps its inspect form
def id(v); v; end
begin
  raise TypeError, "no good"
rescue => e
  p id(e)   # #<TypeError: no good>
end

# a default-message exception (message equals the class name)
begin
  raise RuntimeError
rescue => e
  p e       # #<RuntimeError: RuntimeError>
end
