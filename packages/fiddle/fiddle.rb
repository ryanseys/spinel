# Spinel bundled `fiddle` -- the runtime half of the compile-time Fiddle subset.
#
# The Fiddle DSL (Importer `extern`, `Fiddle::Function.new`, TYPE_ constants,
# `Closure`) is recognized and lowered by the compiler (register_fiddle_decls /
# register_fiddle_locals). This file provides the one piece that needs a runtime
# representation: Fiddle::Pointer, a carried-C native class (sp_fiddle.c, linked
# only when `require "fiddle"` appears) carrying a raw pointer + byte size.
#
# Pointer.malloc, `+`/`-`, and Fiddle::NULL are emitted by the compiler (they
# mint a new Pointer with the class id); the instance methods below bind the
# rest.
module FiddlePointerPackage
  native_lib "fiddle"
  native_obj "packages/fiddle/sp_fiddle.o"

  native_struct "Fiddle::Pointer", "sp_FiddlePtr", "sp_FiddlePtr_gc_free"
  native_method :[],     [:int, :int],          :any,    "sp_FiddlePtr_slice"
  native_method :[]=,    [:int, :int, :string],  :int,    "sp_FiddlePtr_slice_set"
  native_method :null?,  [],                     :bool,   "sp_FiddlePtr_null_p"
  native_method :free,   [],                     :int,    "sp_FiddlePtr_free"
  native_method :to_s,   [],                     :string, "sp_FiddlePtr_to_s"
  native_method :to_str, [],                     :string, "sp_FiddlePtr_to_str"
end
