# RedoNode -- `redo` re-runs the current iteration of the
# enclosing loop without re-evaluating the loop guard or
# advancing the iterator. Spinel emits a labeled goto back
# to the iteration top.

# A counter prevents infinite redo: redo only on the first attempt
attempts = 0
3.times do |i|
  attempts += 1
  if i == 1 && attempts < 5
    redo
  end
  puts "i=#{i} attempt=#{attempts}"
end
puts "total=#{attempts}"
