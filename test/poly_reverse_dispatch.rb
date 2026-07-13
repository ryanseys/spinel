# Object#reverse on a POLY receiver must dispatch on the runtime type: an Array
# reverses its elements, a String reverses its characters. It previously always
# took the String path (sp_str_reverse of the value's inspect), so a poly Array
# reversed the garbage of its own inspect string.
def rev(x) = x.reverse
p rev([1, 2, 3])
p rev("hello")
p rev([1, "two", :three])
p rev([])

# reached as a boxed poly through a proc parameter (a def param the proc is
# called with) -- the original failing shape
def apply(p, x) = p.call(x)
p apply(->(a) { a.reverse }, [1, 2, 3])
p apply(->(s) { s.reverse }, "world")
