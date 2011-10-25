namespace rubinius {
namespace tier1 {
namespace offsets {
  namespace Arguments {
    int name = 0x0;
    int recv = 0x8;
    int block = 0x10;
    int total = 0x18;
    int arguments = 0x20;
    int tuple = 0x28;
  }

  namespace CallFrame {
    int previous = 0x0;
    int static_scope = 0x8;
    int dispatch_data = 0x10;
    int cm = 0x18;
    int flags = 0x20;
    int ip = 0x24;
    int jit_data = 0x28;
    int top_scope = 0x30;
    int scope = 0x38;
    int arguments = 0x40;
    int stack = 0x48;
  }

  namespace StackVariables {
    int on_heap = 0x0;
    int parent = 0x8;
    int self = 0x10;
    int block = 0x18;
    int module = 0x20;
    int last_match = 0x28;
    int locals = 0x30;
  }
}
}
}
