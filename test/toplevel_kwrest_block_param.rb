class Sapp
  def initialize(title:, body:)
    @title = title
    @body = body
  end

  def render
    puts @title
    @body.call
  end
end

# Top-level method whose signature combines a `**kwrest` slot followed by
# `&block`. The body passes the block local as a keyword argument to a
# `.new` site, which routes through scan_new_calls → widen_ptypes_from_args.
# A prior bug omitted the kwrest type from the toplevel method's param-types
# row, leaving param_names.length one ahead of param_types.length. The
# resulting nil scope-type for `block` then crashed base_type / unify_call_types.
def sow(title = nil, **_chrome, &block)
  Sapp.new(title: title || "default", body: block)
end

sow("hello") { puts "ran" }.render
