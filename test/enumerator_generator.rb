# Enumerator.new { |y| ... } generator objects: external iteration (next/peek/
# rewind + StopIteration), eager materializers (to_a/first/take), infinite
# generators bounded by the consumer, captures, nested inline blocks, and
# generators returned from / passed through methods.

# --- external iteration: next / peek ---
e = Enumerator.new do |y|
  y << 10
  y << 20
  y << 30
end
p e.next
p e.peek
p e.peek
p e.next
p e.next

# --- StopIteration past the end ---
begin
  e.next
  p :no_stop
rescue StopIteration
  p :stopped
end

# --- rewind restarts the cursor ---
e.rewind
p e.next
p e.next

# --- to_a / entries each restart from the beginning ---
g = Enumerator.new { |y| y << 1; y << 2; y << 3 }
p g.to_a
p g.entries
p g.next        # the cursor is independent of the materializers
p g.to_a        # still the full sequence

# --- first / first(n) / take(n) ---
p g.first
p g.first(2)
p g.take(2)
p g.first(10)   # fewer available than requested

# --- empty generator ---
empty = Enumerator.new { |y| }
p empty.to_a
p empty.first
p empty.first(3)
begin
  empty.next
rescue StopIteration
  p :empty_stop
end

# --- infinite generator: naturals, bounded by the consumer ---
nats = Enumerator.new do |y|
  n = 1
  loop { y << n; n += 1 }
end
p nats.first(5)
p nats.take(3)
p nats.next
p nats.next

# --- infinite Fibonacci with a captured running pair ---
fib = Enumerator.new do |y|
  a, b = 0, 1
  loop { y << a; a, b = b, a + b }
end
p fib.first(10)

# --- capture an outer variable, used inside a nested inline block ---
factor = 5
scaled = Enumerator.new do |y|
  1.upto(4) { |i| y << i * factor }
end
p scaled.to_a

# --- nested each over a captured array (inline block param + capture) ---
words = ["a", "bb", "ccc"]
lengths = Enumerator.new do |y|
  words.each { |w| y << w.length }
end
p lengths.to_a

# --- yielding mixed element types ---
mixed = Enumerator.new do |y|
  y << 1
  y << "two"
  y << 3.5
  y << nil
  y << [1, 2]
end
p mixed.to_a

# --- multi-value yield builds arrays ---
pairs = Enumerator.new do |y|
  y.yield(1, "one")
  y.yield(2, "two")
end
p pairs.to_a

# --- each-block consumption ---
collected = []
nums = Enumerator.new { |y| y << 100; y << 200; y << 300 }
nums.each { |v| collected << v }
p collected

# --- each restarts, independent of an advanced next cursor ---
nums.next
seen = []
nums.each { |v| seen << v }
p seen

# --- generator driven by a while loop ---
countdown = Enumerator.new do |y|
  n = 3
  while n > 0
    y << n
    n -= 1
  end
end
p countdown.to_a

# --- conditional yields inside a nested inline block ---
evens = Enumerator.new do |y|
  1.upto(10) { |i| y << i if i.even? }
end
p evens.to_a

# --- a method returning a generator (non-literal receiver) ---
def make_counter(limit)
  Enumerator.new do |y|
    i = 0
    while i < limit
      y << i
      i += 1
    end
  end
end
c1 = make_counter(4)
p c1.to_a
p c1.first(2)
p c1.next
p c1.next

# --- a generator captured into a deeper closure body ---
base = 100
offsets = Enumerator.new do |y|
  [1, 2, 3].each do |d|
    y << base + d
  end
end
p offsets.to_a

# --- peek buffers a value; rewind discards it ---
pe = Enumerator.new { |y| y << 1; y << 2 }
p pe.peek
pe.rewind
p pe.next

# --- first(0) / take(0) are empty ---
z = Enumerator.new { |y| y << 1; y << 2 }
p z.first(0)
p z.take(0)

# --- trailing non-yield code: the body's return value is discarded ---
tr = Enumerator.new do |y|
  y << :a
  y << :b
  :ignored_return
end
p tr.to_a

# --- two-level nested inline blocks ---
grid = Enumerator.new do |y|
  1.upto(2) { |a| 1.upto(2) { |b| y << [a, b] } }
end
p grid.to_a

# --- explicit external-iteration drain via begin/rescue StopIteration ---
drain = Enumerator.new { |y| 5.times { |i| y << i * i } }
out = []
begin
  loop_guard = 0
  while loop_guard < 100
    out << drain.next
    loop_guard += 1
  end
rescue StopIteration
  # reached the end
end
p out
