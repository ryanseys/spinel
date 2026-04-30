# Hash#invert for str_int_hash <-> int_str_hash. str_str_hash#invert
# already shipped; this commit adds the swap pair so str-keyed-int and
# int-keyed-str hashes round-trip too.

# str_int_hash -> int_str_hash
h1 = {"a" => 1, "b" => 2, "c" => 3}
inv1 = h1.invert
puts inv1[1]
puts inv1[2]
puts inv1[3]

# int_str_hash -> str_int_hash
h2 = {1 => "x", 2 => "y", 3 => "z"}
inv2 = h2.invert
puts inv2["x"]
puts inv2["y"]
puts inv2["z"]

# Round-trip preserves shape
h3 = {"hello" => 100, "world" => 200}
rt = h3.invert.invert
puts rt["hello"]
puts rt["world"]

# Empty hash invert
e1 = {}
e1["k"] = 1
e1.delete("k")
puts e1.invert.length

# Single-element invert
puts({"only" => 42}.invert[42])
puts({99 => "single"}.invert["single"])
