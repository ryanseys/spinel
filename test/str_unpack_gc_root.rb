# String#unpack1 on a fresh, unrooted substring must root that input across
# unpack's internal allocations: without it, a GC triggered inside unpack
# frees the substring and a reused slot corrupts the read.
blob = ([0] * 200).pack('C*') + [0x11223344, 0x55667788].pack('V*') + ([0] * 200).pack('C*')
vals = []
20000.times do |i|
  vals << blob[200, 4].unpack1('V')
  vals << blob[204, 4].unpack1('V')
end
puts vals.uniq.sort.map { |v| v.to_s(16) }.inspect
puts vals.length
