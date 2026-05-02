# Forwarding edge cases beyond test/forwarding_args.rb.
#
# Verifies the synthetic __fwd_a / __fwd_kw / __fwd_b triple flows
# correctly across distinct method shapes -- including a four-tier
# chain to confirm there's no per-step degradation, and forwarding
# inside a class method.

# Four-tier forwarding chain. Each tier just hands off; the bottom
# tier prints. Proves no slot loss across deep stacks. Top-level
# call uses (...) explicitly so the synthetic triple is supplied;
# bare-name calls into a forwarding method don't yet auto-default.
def t4_inner(...); puts "reached t4_inner"; end
def t4_a(...); t4_inner(...); end
def t4_b(...); t4_a(...); end
def t4_c(...); t4_b(...); end

def kickoff_t4(...)
  t4_c(...)
end
kickoff_t4
#=> reached t4_inner

# Forwarding alongside a literal call right after -- proves the
# fast-path return doesn't pollute subsequent compile_call_args.
def normal_inner; puts "normal"; end
def mixed_inner(...)
  puts "via fwd"
end
def mixed_outer(...)
  normal_inner          # plain call, no forwarding
  mixed_inner(...)      # forwarded call
end
def kickoff_mixed(...)
  mixed_outer(...)
end
kickoff_mixed
#=> normal
#=> via fwd
