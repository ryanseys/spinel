# Integer#digits with a base < 2 raises ArgumentError (invalid radix), not a
# silent base-10 fallback.
def f(x, b); x.digits(b); end
begin; p f(123, 1); rescue ArgumentError => e; puts e.message; end
begin; p f(123, 0); rescue ArgumentError => e; puts e.message; end
p f(123, 16)
