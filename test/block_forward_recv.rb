# Receivered call-site &block forwarding.
#
# Pins the codegen path that forwards a literal block at a typed-receiver
# call site (`recv.m { ... }`) and at a current-class self-call dispatch
# (`m { ... }` from inside another method of the same class) into the
# callee's `&block` parameter. Without this, the call site emits
# `sp_Cls_m(rc, 0)` and the binary segfaults inside `sp_proc_call(NULL, ...)`.
#
# Sites: compile_object_method_expr (Site B) and the @current_class_idx
# self-call branch (Site A) — both mirror the no-recv has_block_param
# template at compile_no_recv_call_expr.

# 1. Basic: `recv.m { ... }` with arity-0 block.call
class App
  def run(&block)
    block.call
  end
end

App.new.run { puts "1-basic" }

# 2. block.call with one int arg, return value used
class Doubler
  def apply(x, &block)
    block.call(x)
  end
end

puts Doubler.new.apply(21) { |n| n * 2 }

# 3. Mixed: regular arg + &block, multiple stmts in method body
class Logger
  def log(label, &block)
    puts label
    block.call
  end
end

Logger.new.log("3-mixed") { puts "  body" }

# 4. Block invoked multiple times from inside the method
class Repeater
  def thrice(&block)
    block.call
    block.call
    block.call
  end
end

Repeater.new.thrice { puts "4-thrice" }

# 5. Block uses interpolation of its own argument (no outer-local closure
#    needed). Spinel's call-site-block closure capture is a separate
#    subsystem — out of scope here; this case only pins that the block
#    body's local-arg interpolation survives the receivered forward.
class Wrapper
  def go(n, &block)
    block.call(n)
  end
end

Wrapper.new.go(7) { |i| puts "5-arg=#{i}" }

# 6. Self-call dispatch (Site A): one method invokes another method
#    of the same class with a literal block.
class Caller
  def kick
    inner { puts "6-self-call" }
  end

  def inner(&block)
    block.call
  end
end

Caller.new.kick

# 7. Multiple receivered calls in sequence — each forwards a distinct block.
class Bank
  def each_dollar(n, &block)
    i = 0
    while i < n
      block.call(i)
      i = i + 1
    end
  end
end

Bank.new.each_dollar(3) { |i| puts "7-dollar-#{i}" }

# 8. Block returns a value used by the method
class Picker
  def pick(&block)
    block.call(10) + block.call(20)
  end
end

puts Picker.new.pick { |n| n * 5 }

# 9. &proc_var forwarding: a method captures &block, then forwards it
#    via `inner(&block)` to another method on the same class. Exercises
#    the BlockArgumentNode lane (find_block_arg) at Site A.
class Forwarder
  def outer(&block)
    inner(&block)
  end

  def inner(&block)
    block.call
  end
end

Forwarder.new.outer { puts "9-proc-var" }

puts "done"
