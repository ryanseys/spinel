# Negative test: `redo` reaches the codegen-level guardrail when it
# parses successfully but ends up in compile_stmt with no enclosing
# loop label on @redo_label_stack.
#
# Method body containing `yield`, called with a block whose body is a
# bare `redo`, parses fine (Prism allows redo in a block context) but
# inlines into a non-loop statement position — codegen must exit 1
# with the expected stderr message.
def f
  yield
end
f { redo }
