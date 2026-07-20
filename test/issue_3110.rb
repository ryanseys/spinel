S = Struct.new(:verbose?, :name)
o = S.new(true, "x")
p o.verbose?
p o.members
p o.to_h
p o.inspect
p o["verbose?"]
o["verbose?"] = false
p o.verbose?
p o == S.new(false, "x")
p o.send(:"verbose?=", true)
p o.verbose?
D = Data.define(:ok?, :count)
d = D.new(ok?: true, count: 3)
p d.ok?
p d.members
p d.to_h
p d.inspect
p d.with(ok?: false).ok?
