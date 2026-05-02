# HashPatternNode -- destructuring hash patterns in case/in.
#
# `{k:, m:}` requires the hash to have keys :k and :m, then binds
# their values. Spinel emits has_key checks chained with && plus a
# binding chain via comma operators. Each LocalVariableTargetNode
# (the shorthand value) becomes `lv_<name> = sp_*Hash_get(tmp, sp_sym_<name>)`.

# Symbol-keyed hash with shorthand bindings.
case {name: "Alice", age: 30}
in {name:, age:}
  puts name        #=> Alice
  puts age.to_s    #=> 30
end

# Subset match: pattern {a:} succeeds for {a: 1, b: 2}.
case {a: 1, b: 2, c: 3}
in {a:}
  puts a.to_s      #=> 1
end

# Both matched -- the binding fires.
case {host: "localhost", port: 8080}
in {host:, port:}
  puts host + ":" + port.to_s   #=> localhost:8080
end

# No-match falls through.
case {x: 1}
in {y:}
  puts "wrong"
else
  puts "fell through"           #=> fell through
end
