# Exception class hierarchy matching: rescue should catch the
# raised class AND any of its ancestor classes (CRuby semantics).
# Spinel's runtime walks @cls_parents transitively via the codegen-
# emitted sp_exc_parent function.
#
# This test only exercises the hierarchy lookup itself. Exception
# objects (.message / .class) and propagation-on-no-match are
# intentionally NOT tested here — those are separate fixes that
# layer onto this one.

# Built-in chain: ArgumentError < StandardError < Exception
begin
  raise ArgumentError, "arg-1"
rescue StandardError => e
  puts "StandardError caught: #{e}"
end

begin
  raise ArgumentError, "arg-2"
rescue Exception => e
  puts "Exception caught: #{e}"
end

# Built-in chain: ZeroDivisionError < StandardError
begin
  raise ZeroDivisionError, "zd"
rescue StandardError => e
  puts "ZD as Std: #{e}"
end

# User-defined chain: MyError < StandardError, SubError < MyError.
class MyError < StandardError
end
class SubError < MyError
end

begin
  raise MyError, "user-1"
rescue StandardError => e
  puts "MyError as StandardError: #{e}"
end

# Transitive: SubError -> MyError -> StandardError -> Exception.
begin
  raise SubError, "user-2"
rescue Exception => e
  puts "SubError as Exception: #{e}"
end

# Direct match still works (no regression in the strcmp-equality path).
begin
  raise SubError, "user-3"
rescue SubError => e
  puts "SubError direct: #{e}"
end

# Multiple rescue clauses: parent class catches the descendant.
begin
  raise SubError, "user-4"
rescue TypeError => e
  puts "wrong: #{e}"
rescue MyError => e
  puts "MyError catches Sub: #{e}"
end

puts "done"
