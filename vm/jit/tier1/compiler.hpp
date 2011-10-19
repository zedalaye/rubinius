#ifndef VM_JIT_TIER1_COMPILER_HPP
#define VM_JIT_TIER1_COMPILER_HPP

namespace rubinius {
  class CompiledMethod;
  class VM;

namespace tier1 {
  class Compiler {
    bool debug_;
    CompiledMethod* cm_;
    void* function_;
    size_t size_;

  public:
    void* jitted_function() {
      return function_;
    }

    size_t jitted_size() {
      return size_;
    }

  public:
    void set_debug(bool val=true) {
      debug_ = val;
    }

    bool debug_p() {
      return debug_;
    }

  public:
    Compiler(CompiledMethod* cm);

    bool compile(VM*);
    size_t callframe_size();
    size_t variables_size();
    size_t stack_size();
    int required_args();
    int locals();
  };
}
}

#endif
