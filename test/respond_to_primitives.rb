# respond_to? on built-in (primitive) receivers, decided at compile time from
# the type's Ruby method set. Per-type helpers keep each receiver's concrete
# type (a single shared helper would widen every receiver to a poly value,
# whose runtime class can't be resolved statically).
def st(x); x; end
def si(x); x; end
def sf(x); x; end
def sa(x); x; end
def sh(x); x; end
def sy(x); x; end

# String: a real method, an absent one, an operator, and a String-form argument
p st("hi").respond_to?(:upcase)        # true
p st("hi").respond_to?(:no_such)       # false
p st("hi").respond_to?(:+)             # true
p st("hi").respond_to?("downcase")     # true  (String argument also accepted)

# Integer: own method + a Comparable mixin method
p si(5).respond_to?(:times)            # true
p si(5).respond_to?(:between?)         # true
p si(5).respond_to?(:upcase)           # false

# Float
p sf(1.5).respond_to?(:ceil)           # true

# Array: own method + an Enumerable mixin method, and an absent one
p sa([1, 2]).respond_to?(:each)        # true
p sa([1, 2]).respond_to?(:map)         # true
p sa([1, 2]).respond_to?(:upcase)      # false

# Hash
p sh({a: 1}).respond_to?(:key?)        # true
p sh({a: 1}).respond_to?(:fetch)       # true

# Symbol
p sy(:foo).respond_to?(:upcase)        # true
p sy(:foo).respond_to?(:each)          # false

# universal Object/Kernel methods resolve for any receiver
p st("x").respond_to?(:class)          # true
p si(5).respond_to?(:frozen?)          # true

# nil, true, range as literal receivers (already concrete-typed)
p nil.respond_to?(:to_a)               # true
p true.respond_to?(:&)                 # true
p (1..3).respond_to?(:each)            # true
