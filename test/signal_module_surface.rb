# The Signal module surface: list/signame, trap validation and previous-handler
# return, handler firing via Process.kill, the EXIT pseudo-signal hook, the
# Signal constant itself, and SignalException/Interrupt objects.
h = Signal.list
p h["INT"]
p h["EXIT"]
p h["TERM"]
p Signal.list.class
p Signal.signame(2)
p Signal.signame(999)
p Signal.signame(0)
p Signal.signame(17)
p Signal.trap("USR1", "IGNORE")
p Signal.trap("USR1", "DEFAULT")
p((Signal.trap("NOPE", "IGNORE") rescue $!.class))
p((Signal.trap("KILL", "IGNORE") rescue $!.class))
p((Signal.trap(9999, "IGNORE") rescue $!.class))
old = Signal.trap("USR2") { }
p Signal.trap("USR2", "DEFAULT").class
p Signal
p Signal.class
p defined?(Signal)
e = SignalException.new("INT")
p e.message, e.signo, e.signm, e.class
p SignalException.new(:TERM).message
p SignalException.new(2).message
i = Interrupt.new
p i.message, i.signo
p Interrupt.new("stop").message
p((SignalException.new("NOPE") rescue $!.class))
begin; raise Interrupt; rescue SignalException => se; p se.class; end
p Interrupt.superclass
p Process.kill(0, Process.pid)
p((Process.kill(0, 99999999) rescue $!.class))
Signal.trap("USR1") { |n| puts "handler #{n}" }
Process.kill("USR1", Process.pid)
sleep 0.05
Signal.trap("EXIT") { puts "exit-handler-ran" }
puts "main"
