module Rubinius
  class LazyExecutable
    attr_accessor :path
    attr_accessor :name

    def self.new(path, name, index)
      Ruby.primitive :lazy_executable_create
      raise PrimitiveFailure, "Rubinius::LazyExecutable.new failed"
    end

    def inspect
      "<Rubinius::LazyExecutable #{@path.inspect}, #{@name.inspect}>"
    end
  end
end
