# Negative-test cases for the direct-path instance_exec lift. Each
# block below would compile to a hard analyze-time error
# ("Spinel: instance_exec: <reason>; <suggestion>" then exit(1))
# if uncommented. The .expected file is empty: the test framework
# runs the .rb without args and compares stdout; a clean compile is
# a pass.
#
# This file exists primarily as inline documentation of the
# boundaries -- the supported-shape positive tests (trampoline +
# direct) cover the happy paths, and any new rejection added by
# a follow-up adds a new commented section here so reviewers
# can see exactly what fails and why.
#
# To exercise a rejection manually:
#   uncomment one section, run `make build/test-results/instance_exec_unsupported.ok`,
#   observe the ERR status with the diagnostic on stderr, then re-comment.

class Builder
  def initialize
    @sum = 0
  end
  def add(n)
    @sum = @sum + n
  end
  def total
    @sum
  end
end

# --- Rejection 1: receiver type cannot be statically resolved
# Uncommenting this triggers:
#   "Spinel: instance_exec: receiver type cannot be statically resolved; ..."
# def make_thing(x)
#   x.instance_exec(5) { |n| add(n) }
# end
# make_thing(Builder.new)

# --- Rejection 2: arity mismatch
# Uncommenting this triggers:
#   "Spinel: instance_exec: 1 arg(s) do not match 2 block param(s); ..."
# b = Builder.new
# b.instance_exec(10) { |x, y| add(x + y) }

# --- Rejection 3: yield inside the block
# Uncommenting this triggers:
#   "Spinel: instance_exec: block uses 'yield' or 'block_given?', ..."
# b = Builder.new
# def outer
#   b = Builder.new
#   b.instance_exec(5) { |n| yield n }
# end
# outer { |x| puts x }

# --- Rejection 4: unsupported arg shape (method call as arg)
# Uncommenting this triggers:
#   "Spinel: instance_exec: arg #1 is a CallNode whose type cannot be ..."
# b = Builder.new
# def some_value
#   42
# end
# b.instance_exec(some_value) { |n| add(n) }

# Empty positive test so the file compiles cleanly. The supported
# shape is the smoke test; rejections are exercised by uncommenting
# the relevant section.
b = Builder.new
b.instance_exec(7) { |n| add(n) }
puts b.total   #=> 7
puts "done"
