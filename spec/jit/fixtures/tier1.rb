module JITSpecs
  class Tier1Specs
    def none
      nil
    end

    def one(arg)
      arg
    end

    def strlit
      "hello"
    end

    def str_build(obj)
      "-#{obj}-"
    end
  end
end
