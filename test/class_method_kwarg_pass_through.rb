# Class-method dispatch with a kwarg passed at the call site
# (`W.write(200, set_cookies: cookies)`) previously dropped the
# kwarg value to 0 because compile_constant_recv_expr's class-method
# branch used the generic compile_call_args, which walks the
# KeywordHashNode as an opaque arg and lowers it to a literal that
# resolves to 0 at the call site. The fix extracts kwargs by name
# from the call site and routes each pair to the matching param
# slot, falling back to the recorded default for missing slots.
# Same shape as compile_call_args_with_defaults' instance-method
# handler but reading from @cls_cmeth_* tables.

class W
  def self.write(status, set_cookies: {})
    n = 0
    set_cookies.each do |name, val|
      n = n + 1
    end
    n
  end
end

cookies = { flash_notice: "Hi", deferred: "Bye" }
puts W.write(200, set_cookies: cookies).to_s
puts W.write(404).to_s
