# ConstantPathOrWriteNode -- `M::FOO ||= val`.

module M
  CONFIGURED = "explicit"
end
M::CONFIGURED ||= "fallback"  # truthy -> doesn't fire
puts M::CONFIGURED            # explicit
