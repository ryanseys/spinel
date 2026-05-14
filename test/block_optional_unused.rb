class Greeter
  def greet(text = nil, &block)
    puts text || "anonymous"
    yield if block
  end
end
Greeter.new.greet("hi")    # no block — current Spinel: compile error.
