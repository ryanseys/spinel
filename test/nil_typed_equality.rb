# Two operands whose STATIC type is nil (calls the analyzer proves return
# nil) compare like literal nils: nil == nil is true, != is false, and both
# expressions still evaluate for their effects (the counter increments).
$calls = 0
def probe(x)
  $calls += 1
  nil
end

p probe(1) == probe(2)
p probe(3) != probe(4)
p $calls
