#include "builtin/lazy_executable.hpp"
#include "builtin/class.hpp"
#include "builtin/lookuptable.hpp"
#include "builtin/symbol.hpp"
#include "builtin/fixnum.hpp"

#include "compiled_file.hpp"
#include "dispatch.hpp"
#include "call_frame.hpp"
#include "arguments.hpp"

#include "object_utils.hpp"

#include <iostream>
#include <fstream>

namespace rubinius {
  void LazyExecutable::init(STATE) {
    GO(lmethod).set(state->new_class("LazyExecutable", G(executable), G(rubinius)));
    G(lmethod)->set_object_type(state, LazyExecutableType);
    G(lmethod)->name(state, state->symbol("Rubinius::LazyExecutable"));
    G(lmethod)->set_const(state, "Index", LookupTable::create(state));
  }

  void LazyExecutable::add_index(STATE, Symbol* path, LookupTable* index, size_t base) {
    index->store(state, state->symbol("base"), Fixnum::from(base));
    LookupTable* tbl = as<LookupTable>(G(lmethod)->get_const(state, state->symbol("Index")));
    tbl->store(state, path, index);
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
    LookupTable* index = as<LookupTable>(
        G(lmethod)->get_const(state, state->symbol("Index")));
    native_int base = as<Fixnum>(index->fetch(state, state->symbol("base")))->to_native();
    native_int offset = as<Fixnum>(index->fetch(state, name_))->to_native();

    std::ifstream stream(path_->c_str(state));
    if(!stream) {
      std::string msg = "Unable to load CompiledMethod from ";
      msg += std::string(path_->c_str(state));
      throw std::runtime_error(msg);
    }
    stream.seekg(base + offset, std::ios::beg);

    return CompiledFile::load_method(state, stream, path_);
  }
}
