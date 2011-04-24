#include "builtin/executable.hpp"

namespace rubinius {
  struct CallFrame;
  class Dispatch;
  class Arguments;
  class Fixnum;
  class Symbol;
  class StaticScope;

  class LazyExecutable : public Executable {
  public:
    const static object_type type = LazyExecutableType;

  private:
    Symbol* path_;        // slot
    Symbol* name_;        // slot
    StaticScope* scope_;  // slot

  public:
    attr_accessor(path, Symbol);
    attr_accessor(name, Symbol);
    attr_accessor(scope, StaticScope);

    static void init(STATE);
    static void LazyExecutable::add_index(STATE, Symbol* path, LookupTable* index);

    // Ruby.primitive :lazy_executable_create
    static LazyExecutable* create(STATE, Symbol* path, Symbol* name);

    static Object* lazy_executor(STATE, CallFrame* call_frame,
                                 Executable* exec, Module* mod, Arguments& args);

    Executable* load(STATE);

    class Info : public Executable::Info {
    public:
      BASIC_TYPEINFO(Executable::Info)
    };
  };
}
