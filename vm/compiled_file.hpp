#ifndef RBX_COMPILED_FILE_HPP
#define RBX_COMPILED_FILE_HPP

#include <string>

#include "prelude.hpp"

namespace rubinius {

  class CompiledMethod;
  class Object;
  class Symbol;
  class VM;

  class CompiledFile {
  public:
    std::string magic;
    uint64_t version;
    std::string sum;
    Symbol* path;

  private:
    std::istream* stream;

  public:
    CompiledFile(std::string magic, uint64_t version, std::string sum,
                 Symbol* path, std::istream* stream)
      : magic(magic)
      , version(version)
      , sum(sum)
      , path(path)
      , stream(stream)
    {}

    static CompiledFile* load(std::istream& stream, Symbol* path);
    static CompiledMethod* load_method(STATE, std::istream& stream, Symbol* path);
    Object* body(STATE);
    bool execute(STATE);
  };

}

#endif
