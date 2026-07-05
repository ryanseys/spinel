require "json"

Rec = Struct.new(:id, :name)
puts JSON.generate(Rec.new(1, "alice"))
puts JSON.dump(Rec.new(2, "bob"))
puts JSON.generate(Rec.new)

Esc = Struct.new(:text)
puts JSON.generate(Esc.new("a\"b\nc\td"))
