# Hash#flatten(depth): to_a.flatten(depth) semantics -- depth >= 2 expands
# array values, 0 keeps the pairs, negative flattens completely (the
# argless interleave keeps its fast arm).
