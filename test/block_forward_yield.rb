# Name-independent block forwarding: a method can forward its &block to ANY
# user method that yields -- the forwarded block's params are typed from the
# callee's own `yield`, regardless of the callee's name (no builtin-iterator
# name list involved). The callee is inlined at the forward site.

# Forward to a method that yields strings; the block needs the concrete String
# type for #upcase (poly would not compile).
def gen(&blk)
  yield "hello"
  yield "world"
end
def relay(&blk)
  gen(&blk)
end
relay { |s| puts s.upcase }

# Two-argument yield forwarded through a relay.
def pairs(&blk)
  yield 1, "a"
  yield 2, "b"
end
def relay2(&blk)
  pairs(&blk)
end
relay2 { |n, s| puts "#{n}-#{s.upcase}" }

# Forward to a yielding instance method on an object receiver.
class Producer
  def produce
    yield "x"
    yield "yy"
  end
end
def drive(p, &blk)
  p.produce(&blk)
end
drive(Producer.new) { |s| puts s.length }

# A custom-named iterator (not a builtin name) that yields integers.
def each_step(&blk)
  yield 10
  yield 20
  yield 30
end
def forward_steps(&blk)
  each_step(&blk)
end
forward_steps { |n| puts n + 1 }
