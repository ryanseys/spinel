# KeyError from Hash#fetch (and fetch_values) must include the missing key's
# inspect form, across typed and string-keyed poly hash representations.
def fe(h, k); h.fetch(k); end
begin; fe({a: 1}, :zzz); rescue KeyError => e; puts e.message; end
begin; {"x" => 1}.fetch("nope"); rescue KeyError => e; puts e.message; end
begin; {1 => "a"}.fetch(99); rescue KeyError => e; puts e.message; end
begin; {a: 1, b: 2}.fetch_values(:a, :zz); rescue KeyError => e; puts e.message; end
