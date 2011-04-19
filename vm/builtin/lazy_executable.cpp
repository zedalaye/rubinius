#include "builtin/lazy_executable.hpp"
#include "builtin/class.hpp"
#include "builtin/lookuptable.hpp"
#include "builtin/symbol.hpp"
#include "builtin/fixnum.hpp"

#include "builtin/thunk.hpp"

#include "dispatch.hpp"
#include "call_frame.hpp"
#include "arguments.hpp"

#include "object_utils.hpp"

namespace rubinius {
  void LazyExecutable::init(STATE) {
    GO(lmethod).set(state->new_class("LazyExecutable", G(executable), G(rubinius)));
    G(lmethod)->set_object_type(state, LazyExecutableType);
    G(lmethod)->name(state, state->symbol("Rubinius::LazyExecutable"));
    G(lmethod)->set_const(state, "Index", LookupTable::create(state));
  }

  LazyExecutable* LazyExecutable::create(STATE, Symbol* path, Symbol* name) {
    LazyExecutable* le = state->new_object<LazyExecutable>(G(lmethod));
    le->path(state, path);
    le->name(state, name);

    le->execute = lazy_executor;
    return le;
  }

  Object* LazyExecutable::lazy_executor(STATE, CallFrame* call_frame, Executable* exec,
                                        Module* mod, Arguments& args)
  {
    // Should we raise an exception or load?
    return Qnil;
  }

  Executable* LazyExecutable::load(STATE) {
    return Thunk::create(state, G(object), state->symbol("booya"));
  }
}
