# Stream-fused lazy chains: `<array|range>.lazy.<op>*.<terminal>` evaluated in a
# single pass with no intermediate arrays. Receivers are routed through a method
# parameter where useful to exercise the runtime path.

# --- array sources, each op ---
def arr5(a)
  [
    a.lazy.map { |x| x * 10 }.first(2),
    a.lazy.map { |x| x * 10 }.to_a,
    a.lazy.select { |x| x.odd? }.to_a,
    a.lazy.reject { |x| x.even? }.to_a,
    a.lazy.take(3).to_a,
    a.lazy.drop(2).to_a,
    a.lazy.filter_map { |x| x * 2 if x.even? }.to_a,
  ]
end
arr5([1, 2, 3, 4, 5]).each { |r| p r }

def arr6(a)
  [
    a.lazy.take_while { |x| x < 4 }.to_a,
    a.lazy.drop_while { |x| x < 3 }.to_a,
    a.lazy.select { |x| x.odd? }.map { |x| x * x }.take(2).to_a,
    a.lazy.map { |x| x / 10 }.reject { |x| x == 2 }.to_a,
  ]
end
arr6([10, 20, 30, 40, 50, 60]).each { |r| p r }

# --- first with and without an argument ---
p [1, 2, 3, 4, 5].lazy.select { |x| x > 2 }.first
p [1, 2, 3, 4, 5].lazy.select { |x| x > 9 }.first   # nil: no match
p [1, 2, 3].lazy.map { |x| x * 2 }.first(10)         # fewer than asked

# --- range-variable sources (walked via sp_Range; bounded chains only) ---
def rsel(rng)
  rng.lazy.select { |x| x.even? }.first(3)
end
p rsel(2..20)
def rmap(rng)
  rng.lazy.map { |x| x * 2 }.take(3).to_a
end
p rmap(1..10)

# --- a map that changes element type ---
p ["a", "bb", "ccc"].lazy.map { |s| s.length }.to_a
p [1, 2, 3].lazy.map { |x| x.to_s }.to_a

# --- range sources, including endless (bounded by first/take) ---
p (1..10).lazy.map { |x| x + 1 }.select { |x| x.even? }.first(3)
p (1..Float::INFINITY).lazy.select { |x| x % 3 == 0 }.map { |x| x * x }.first(4)
p (1..Float::INFINITY).lazy.map { |x| x * 2 }.take(5).to_a
p (1..Float::INFINITY).lazy.select { |x| x.odd? }.first(5)
p (1..100).lazy.select { |x| x % 7 == 0 }.first(3)
p (1..Float::INFINITY).lazy.first(3)
p (5..Float::INFINITY).lazy.take_while { |x| x < 9 }.to_a
