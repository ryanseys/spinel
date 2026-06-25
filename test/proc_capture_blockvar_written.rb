# A proc can capture an enclosing inlined-block's loop variable that is WRITTEN
# (in the block body or in the proc). Each loop iteration gets a fresh heap cell
# shared by the block body and the proc, so a captured write is observed by both
# sides and each iteration's closure keeps its own value -- matching CRuby's
# fresh-per-iteration block binding.

# each: write in the block body, observed by the proc, fresh per iteration.
a = []
[1, 2, 3].each { |x| x *= 10; a << proc { x } }
ra = a.map { |pr| pr.call }
p ra

# each: write INSIDE the proc, observed by the block body afterwards.
r = []
[1, 2].each { |x| pr = proc { x += 5 }; pr.call; r << x }
p r

# each: immediate call of a proc over the written var.
total = 0
[10, 20, 30].each { |y| y += 1; total += proc { y }.call }
p total

# times: written index captured fresh each iteration.
b = []
3.times { |i| i *= 10; b << proc { i } }
rb = b.map { |pr| pr.call }
p rb

# upto: written counter captured.
u = []
1.upto(3) { |n| n *= 2; u << proc { n } }
ru = u.map { |pr| pr.call }
p ru

# each_with_index: both element and index written + captured.
acc = []
[5, 6, 7].each_with_index { |v, i| v += i; acc << proc { v } }
racc = acc.map { |pr| pr.call }
p racc

# hash each over a concrete int-valued hash: written value captured.
h = { 1 => 10, 2 => 20 }
hs = []
h.each { |k, val| val += k; hs << proc { val } }
rhs = hs.map { |pr| pr.call }
p rhs

# string-typed written block var (exercises the pointer cell launder); the proc
# is called immediately so the result is observed in the same iteration.
sres = []
["a", "b"].each { |s| s = s + "!"; sres << proc { s }.call }
p sres

# nested inlined blocks: the inner proc captures the inner written block var.
nested = []
[1, 2].each { |outer| [10, 20].each { |inner| inner += outer; nested << proc { inner } } }
rn = nested.map { |pr| pr.call }
p rn

# map: written element captured by an escaping proc, fresh per iteration.
m = [1, 2, 3].map { |x| x *= 10; proc { x } }
p m.map { |pr| pr.call }

# select: the param is reassigned + captured -- select must still collect the
# ORIGINAL element, while the proc observes the written value.
sacc = []
sres = [1, 2, 3, 4].select { |x| sacc << proc { x }; keep = x.even?; x = 99; keep }
p sres
p sacc.map { |pr| pr.call }

# reject: same, collecting the original elements the block rejects-as-falsy.
racc = []
rres = [1, 2, 3, 4].reject { |y| racc << proc { y }; drop = y.even?; y += 100; drop }
p rres
p racc.map { |pr| pr.call }

# flat_map: written element captured; the block returns an array to splat.
facc = []
fres = [1, 2, 3].flat_map { |x| facc << proc { x }; x *= 10; [x, x + 1] }
p fres
p facc.map { |pr| pr.call }

# filter_map: written element captured; falsy results dropped.
fmacc = []
fmres = [1, 2, 3, 4].filter_map { |x| fmacc << proc { x }; keep = x.even?; x += 100; keep ? x : nil }
p fmres
p fmacc.map { |pr| pr.call }

# range each: written counter captured fresh each iteration.
ra = []
(1..3).each { |n| n *= 5; ra << proc { n } }
p ra.map { |pr| pr.call }

# hash each_value / each_key: written value/key captured (concrete int hash).
hv = []
{ 1 => 10, 2 => 20 }.each_value { |v| v += 1; hv << proc { v } }
p hv.map { |pr| pr.call }
hk = []
{ 5 => 0, 6 => 0 }.each_key { |kk| kk *= 2; hk << proc { kk } }
p hk.map { |pr| pr.call }

# zip: both written pair members captured.
zacc = []
[1, 2, 3].zip([10, 20, 30]) { |a, bb| a += bb; zacc << proc { a } }
p zacc.map { |pr| pr.call }

# string upto: written string param captured; proc called immediately so the
# pointer-cell value is observed in the same iteration.
us = []
"a".upto("c") { |s| s = s + "!"; us << proc { s }.call }
p us
