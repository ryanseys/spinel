# Enumerator#filter_map over each_with_index: keep truthy block values.
arr = [10, 20, 30, 40]
puts arr.each_with_index.filter_map { |s, i| i if s > 15 }.inspect
puts arr.each_with_index.filter_map { |s, i| s + i if i.even? }.inspect
puts arr.each_with_index.filter_map { |s, i| "#{i}:#{s}" if s < 35 }.inspect
