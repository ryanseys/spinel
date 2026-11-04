# frozen_string_literal: true

# Array and Hash literals whose elements are constants build from a static
# table; what they hold, in what order, and what is frozen or shared is
# CRuby's.

module Shapes
  WORDS = ["north", "east", "south", "west", "up", "down"].freeze
  SIZES = [1, 2, 3, -4, 5, 600_000, 7].freeze
  RATIOS = [0.5, 1.25, -2.0, 1.0e-3, 3.5e+10].freeze
  TAGS = [:one, :two, :three, :four].freeze
  MIXED = [1, "two", :three, 4.5, nil, true, false, "eight"].freeze
  STYLE = { color: "red", width: 3, ratio: 0.75, kind: :box, hidden: false, shown: true, note: nil }.freeze
  NAMES = { "a" => "alpha", "b" => "beta", "c" => "gamma", "d" => "delta" }.freeze
  TWO = { x: 1, y: 2 }.freeze
  NESTED = { name: "grid", rows: 4, cols: 4, cell: { w: 10, h: 12, unit: "px" }, list: ["p", "q", "r"], last: :end }.freeze
  AGAIN = { a: 1, b: 2, a: 3, c: 4 }
end

p Shapes::WORDS, Shapes::SIZES, Shapes::RATIOS, Shapes::TAGS, Shapes::MIXED
p Shapes::STYLE, Shapes::NAMES, Shapes::TWO, Shapes::NESTED, Shapes::AGAIN
p Shapes::AGAIN.keys
p Shapes::WORDS.frozen?, Shapes::STYLE.frozen?, Shapes::NESTED[:cell].frozen?, Shapes::NESTED[:list].frozen?
p Shapes::WORDS[0].frozen?, Shapes::STYLE[:color].frozen?, Shapes::MIXED[1].frozen?
p Shapes::WORDS[0].equal?("north"), Shapes::STYLE[:color].equal?("red"), Shapes::NAMES["a"].equal?("alpha")
p Shapes::STYLE.default, Shapes::STYLE[:missing], Shapes::STYLE.size, Shapes::NAMES.size
p Shapes::MIXED.map(&:class)

# a literal that runs again is a new object each time
log = []
3.times do |i|
  a = [10, 20, 30, 40]
  a << i
  h = { p: "x", q: "y", r: "z" }
  h[:s] = i
  log << a << h
end
p log

# constants beside elements that run code: each runs where it stands
def tick(log, v)
  log << v
  v
end
order = []
row = [1, 2, 3, tick(order, :mid), 4, 5, 6, tick(order, :end)]
pairs = { a: 1, b: 2, c: 3, d: tick(order, :d), e: 5, f: 6, g: 7 }
p row, pairs, order

# a String element of an unfrozen literal can be replaced, and the rest stay
words = ["keep", "this", "list", "intact"]
words[1] = words[1] + "!"
p words, words.map(&:frozen?)

# keyword arguments are a literal of their own
def kw(**opts) = opts
p kw(a: 1, b: "two", c: :three, d: 4.0, e: nil)
