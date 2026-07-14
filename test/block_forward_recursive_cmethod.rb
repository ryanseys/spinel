# A literal block on an implicit-self call to a &block-forwarding recursive
# class method: the block lifts to a real proc (capturing enclosing locals)
# and the &block forward passes the caller's proc through.
class M
  def self.walk(node, &block)
    return block.call(node) if node.is_a?(String)
    node.each { |child| walk(child, &block) }
  end

  def self.go(root)
    out = []
    walk(root) { |n| out << n }
    out.join(",")
  end
end

puts M.go([["a"], "b"])
out2 = []
M.walk([["x", ["y"]], "z"]) { |n| out2 << n }
p out2
