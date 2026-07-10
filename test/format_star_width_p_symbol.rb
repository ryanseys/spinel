# format/sprintf/% directive coverage: %* (width/precision from an argument),
# %p (inspect), and %s with a Symbol argument.
p format("%*d", 5, 42)
p format("%-*d", 5, 42)
p format("%.*f", 2, 3.14159)
p("%p" % "ab")
p("%p" % :hi)
p("%s" % :hi)
p("%10s" % :hi)
def f(fmt, w, v); format(fmt, w, v); end
p f("%*d", 4, 7)
