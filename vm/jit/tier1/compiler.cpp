#include "vendor/asmjit/AsmJit/Assembler.h"
#include "vendor/asmjit/AsmJit/Logger.h"
#include "jit/tier1/compiler.hpp"

#include "vm/vm.hpp"
#include "call_frame.hpp"

#include "builtin/compiledmethod.hpp"

#include "instructions_util.hpp"

#include "jit/tier1/offsets.hpp"

extern "C" void* rbx_arg_error();
extern "C" void* rbx_prologue_check2();
extern "C" void* rbx_flush_scope();
extern "C" void* rbx_check_frozen();
extern "C" void* rbx_meta_to_s();

namespace rubinius {
namespace tier1 {
  using namespace AsmJit;
    
  Compiler::Compiler(CompiledMethod* cm)
    : cm_(cm)
    , function_(0)
    , size_(0)
  {}

  size_t Compiler::callframe_size() {
    VMMethod* vmm = cm_->backend_method();
    return sizeof(CallFrame) + (vmm->stack_size * sizeof(Object*));
  }

  size_t Compiler::stack_size() {
    VMMethod* vmm = cm_->backend_method();
    return vmm->stack_size;
  }

  size_t Compiler::variables_size() {
    VMMethod* vmm = cm_->backend_method();
    return sizeof(StackVariables) + (vmm->number_of_locals * sizeof(Object*));
  }

  int Compiler::required_args() {
    VMMethod* vmm = cm_->backend_method();
    return vmm->required_args;
  }

  int Compiler::locals() {
    VMMethod* vmm = cm_->backend_method();
    return vmm->number_of_locals;
  }

  class VisitorX8664 : public VisitInstructions<VisitorX8664> {
    VM* state;
    Compiler& compiler_;
    AsmJit::Assembler _;
    AsmJit::Label exit_label;
    int stack_used;

    const AsmJit::GPReg& stack_ptr;
    const AsmJit::GPReg& arg1;
    const AsmJit::GPReg& arg2;
    const AsmJit::GPReg& arg3;
    const AsmJit::GPReg& arg4;
    const AsmJit::GPReg& arg5;
    const AsmJit::GPReg& return_reg;

    AsmJit::GPReg& scratch;
    AsmJit::GPReg& scratch32;
    AsmJit::GPReg& scratch2;

    AsmJit::FileLogger logger_;

    const static int cPointerSize = sizeof(Object*);

    int callframe_offset;
    int variables_offset;

  public:

    VisitorX8664(STATE, Compiler& compiler)
      : state(state)
      , compiler_(compiler)
      , exit_label(_.newLabel())
      , stack_used(0)
      , stack_ptr(AsmJit::rbx)
      , arg1(AsmJit::rdi)
      , arg2(AsmJit::rsi)
      , arg3(AsmJit::rdx)
      , arg4(AsmJit::rcx)
      , arg5(AsmJit::r8)
      , return_reg(AsmJit::rax)
      , scratch(const_cast<AsmJit::GPReg&>(AsmJit::r12))
      , scratch32(const_cast<AsmJit::GPReg&>(AsmJit::eax))
      , scratch2(const_cast<AsmJit::GPReg&>(AsmJit::r13))
      , logger_(stderr)
    {
      _.setLogger(&logger_);
    }

    void* make() {
      return _.make();
    }

    size_t code_size() {
      return _.getCodeSize();
    }

    AsmJit::Mem p(const GPReg& base, sysint_t disp = 0) {
      return AsmJit::qword_ptr(base, disp);
    }

    AsmJit::Mem p32(const GPReg& base, sysint_t disp = 0) {
      return AsmJit::dword_ptr(base, disp);
    }

    AsmJit::Mem fr(sysint_t disp) {
      return AsmJit::qword_ptr(rbp, disp);
    }

    static const int cVMOffset = -0x8;
    static const int cPreviousOffset = -0x10;
    static const int cExecOffset = -0x18;
    static const int cModOffset = -0x20;
    static const int cArgOffset = -0x28;

    AsmJit::Mem callframe(sysint_t disp, bool word=false) {
      if(word) {
        return AsmJit::dword_ptr(rbp, callframe_offset + disp);
      } else {
        return AsmJit::qword_ptr(rbp, callframe_offset + disp);
      }
    }

    AsmJit::Mem variables(sysint_t disp) {
      return AsmJit::qword_ptr(rbp, variables_offset + disp);
    }

    void check_arity() {
      int req = compiler_.required_args();
      _.movsxd(scratch, p32(arg5, offsets::Arguments::total));
      _.cmp(scratch, req);

      Label rest = _.newLabel();

      _.je(rest);

      _.mov(arg3, arg5);
      _.mov(arg4, scratch);
      _.call((void*)rbx_arg_error);

      _.mov(return_reg, 0);
      _.jmp(exit_label);
     
      _.bind(rest);
    }

    void initialize_frame() {
      _.comment("CallFrame::previous");
      _.mov(scratch, fr(cPreviousOffset));
      _.mov(callframe(offsets::CallFrame::previous), scratch);

      _.comment("CallFrame::arguments");
      _.mov(scratch, fr(cArgOffset));
      _.mov(callframe(offsets::CallFrame::arguments), scratch);

      _.comment("CallFrame::dispatch_data");
      _.mov(callframe(offsets::CallFrame::dispatch_data), 0);

      _.comment("CallFrame::cm");
      _.mov(scratch, fr(cExecOffset));
      _.mov(callframe(offsets::CallFrame::cm), scratch);

      _.comment("CallFrame::flags");
      _.mov(callframe(offsets::CallFrame::flags, true), 0);

      _.comment("CallFrame::ip");
      _.mov(callframe(offsets::CallFrame::ip, true), 0);

      _.comment("CallFrame::scope");
      _.lea(scratch, fr(variables_offset));
      _.mov(callframe(offsets::CallFrame::scope), scratch);

      _.comment("CallFrame::jit_data");
      _.mov(callframe(offsets::CallFrame::jit_data), 0);
    }

    void setup_scope() {
      _.mov(scratch2, fr(cArgOffset));

      _.comment("StackVariables::on_heap");
      _.mov(variables(offsets::StackVariables::on_heap), 0);

      _.comment("StackVariables::parent");
      _.mov(variables(offsets::StackVariables::parent), 0);

      _.comment("StackVariables::self");
      _.mov(scratch, p(scratch2, offsets::Arguments::recv));
      _.mov(variables(offsets::StackVariables::self), scratch);

      _.comment("StackVariables::block");
      _.mov(scratch, p(scratch2, offsets::Arguments::block));
      _.mov(variables(offsets::StackVariables::block), scratch);

      _.comment("StackVariables::module");
      _.mov(scratch, fr(cModOffset));
      _.mov(variables(offsets::StackVariables::module), scratch);

      _.comment("StackVariables::last_match");
      _.mov(variables(offsets::StackVariables::last_match), (sysint_t)Qnil);

      _.comment("StackVariables::locals");

      int size = compiler_.locals();
      for(int i = 0; i < size; i++) {
        _.mov(
              variables(offsets::StackVariables::locals + (i * cPointerSize)),
              (sysint_t)Qnil);
      }
    }

    void nil_stack() {
      int count = compiler_.stack_size();

      for(int i = 0; i < count; i++) {
        _.comment("CallFrame::stack => nil");
        _.mov(p(stack_ptr, count * cPointerSize), (sysint_t)Qnil);
      }
    }

    void import_args() {
      int req = compiler_.required_args();
      _.mov(scratch2, fr(cArgOffset));

      for(int i = 0; i < req; i++) {
        _.comment("argument => variables");
        _.mov(scratch, p(scratch2, offsets::Arguments::arguments + (i * cPointerSize)));
        _.mov(variables(offsets::StackVariables::locals + (i * cPointerSize)), scratch);
      }
    }

    void check_interrupts() {
      _.mov(arg1, fr(cVMOffset));
      _.lea(arg2, fr(callframe_offset));

      _.call((void*)rbx_prologue_check2);
      _.cmp(rax, 0);
      _.je(exit_label);
    }

    void prologue() {
      _.push(AsmJit::rbp);
      _.mov(AsmJit::rbp, AsmJit::rsp);

      stack_used = compiler_.callframe_size() + compiler_.variables_size();
      stack_used += (cPointerSize * 5);

      _.sub(AsmJit::rsp, stack_used);

      check_arity();

      _.mov(fr(cVMOffset), arg1);
      _.mov(fr(cPreviousOffset), arg2);
      _.mov(fr(cExecOffset), arg3);
      _.mov(fr(cModOffset), arg4);

      _.mov(fr(cArgOffset), arg5);

      callframe_offset = cArgOffset - compiler_.callframe_size();
      variables_offset = callframe_offset - compiler_.variables_size();

      int stack_top = callframe_offset + sizeof(CallFrame);

      initialize_frame();

      // Substract a pointer because the stack starts beyond the top.
      _.lea(stack_ptr, p(AsmJit::rbp, stack_top - sizeof(Object*)));

      nil_stack();
      setup_scope();
      import_args();
      check_interrupts();
    }

    void epilogue() {
      _.bind(exit_label);

      _.add(AsmJit::rsp, stack_used);
      _.pop(AsmJit::rbp);
      _.ret();
    }

    void visit(opcode op, opcode arg1, opcode arg2) {
      rubinius::bug("Missing opcode implementation");
    }

    void visit_pop() {
      _.add(stack_ptr, -cPointerSize);
    }

    void visit_pop_many(opcode count) {
      _.add(stack_ptr, -cPointerSize * count);
    }

    AsmJit::Mem stack_top_ptr() {
      return p(stack_ptr);
    }

    AsmJit::Mem stack_back_position(int offset) {
      return p(stack_ptr, -cPointerSize * offset);
    }

    void load_stack_top(AsmJit::GPReg& reg) {
      _.mov(reg, stack_top_ptr());
    }

    void visit_swap_stack() {
      AsmJit::Mem top = p(stack_ptr);
      AsmJit::Mem back1 = p(stack_ptr, -cPointerSize);

      _.mov(scratch, top);
      _.mov(scratch2, back1);
      _.mov(back1, scratch);
      _.mov(top, scratch2);
    }

    void visit_dup_top() {
      load_stack_top(scratch);
    }

    void visit_dup_many(opcode count) {
      _.lea(scratch2, stack_back_position(count));

      for(opcode i = 0; i < count; i++) {
        _.add(stack_ptr, cPointerSize);
        _.mov(stack_ptr, AsmJit::ptr(scratch2, i * cPointerSize));
      }
    }

    void visit_rotate(opcode count) {
      int diff = count / 2;

      for(int i = 0; i < diff; i++) {
        int offset = count - i - 1;
        AsmJit::Mem pos = stack_back_position(offset);
        AsmJit::Mem pos2 = stack_back_position(i);

        _.mov(scratch, pos);
        _.mov(scratch2, pos2);

        _.mov(pos2, scratch);
        _.mov(pos, scratch2);
      }
    }

    void visit_move_down(opcode positions) {
      load_stack_top(scratch);

      for(opcode i = 0; i < positions; i++) {
        int target = i;
        int current = target + 1;

        _.mov(scratch2, stack_back_position(current));
        _.mov(stack_back_position(target), scratch2);
      }

      _.mov(stack_back_position(positions), scratch);
    }

    void push_object(Object* obj) {
      _.add(stack_ptr, cPointerSize);
      _.mov(p(stack_ptr), (sysint_t)obj);
    }

    void push_reg(AsmJit::GPReg& reg) {
      _.add(stack_ptr, cPointerSize);
      _.mov(p(stack_ptr), reg);
    }

    void visit_push_nil() {
      push_object(Qnil);
    }

    void visit_push_true() {
      push_object(Qtrue);
    }

    void visit_push_false() {
      push_object(Qfalse);
    }

    void visit_push_undef() {
      Object** addr = state->shared.globals.undefined.object_address();
      _.mov_ptr(scratch, addr);
      push_reg(scratch);
    }

    void visit_push_int(opcode arg) {
      push_object(Fixnum::from(arg));
    }

    void visit_meta_push_0() {
      push_object(Fixnum::from(0));
    }

    void visit_meta_push_1() {
      push_object(Fixnum::from(1));
    }

    void visit_meta_push_2() {
      push_object(Fixnum::from(2));
    }

    void visit_meta_push_neg_1() {
      push_object(Fixnum::from(-1));
    }

    void visit_ret() {
      _.mov(arg1, fr(cVMOffset));
      _.lea(arg2, fr(variables_offset));
      _.call((void*)rbx_flush_scope);

      _.mov(return_reg, p(stack_ptr));
      _.jmp(exit_label);
    }

    void visit_set_local(opcode index) {
      load_stack_top(scratch);

      _.mov(variables(offsets::StackVariables::locals + (index * cPointerSize)),
            scratch);
    }

    void load_vm(const GPReg& reg) {
      _.mov(reg, fr(cVMOffset));
    }

    void load_callframe(const GPReg& reg) {
      _.lea(reg, fr(callframe_offset));
    }

    void visit_check_frozen() {
      load_vm(scratch);
      _.mov(arg1, scratch);

      load_callframe(scratch);
      _.mov(arg2, scratch);

      _.mov(arg3, stack_top_ptr());
      _.call((void*)rbx_check_frozen);
    }

    void visit_meta_to_s(opcode name) {
      InlineCache* cache = reinterpret_cast<InlineCache*>(name);

      load_vm(arg1);
      load_callframe(arg2);
      _.mov(arg3, (sysint_t)cache);
      _.mov(arg4, p(stack_ptr));
      _.call((void*)rbx_meta_to_s);
      _.cmp(rax, 0);
      _.je(exit_label);
      _.mov(p(stack_ptr), rax);
    }

  };

  bool Compiler::compile(STATE) {
    const char* reason;
    int ip;

    VMMethod* vmm = cm_->internalize(state, &reason, &ip);

    VisitorX8664 visit(state, *this);

    visit.prologue();

    visit.drive(vmm);

    visit.epilogue();

    function_ = visit.make();
    size_ = visit.code_size();

    return true;
  }

}
}
