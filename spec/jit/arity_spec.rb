describe "tier1 JIT" do

  before do
    @o = Object.new
    def @o.none
      nil
    end
  end

  it "checks the arity of called methods" do
    lambda {
      @o.__send_tier1__(:none)
    }.should_not raise_error(ArgumentError)
  end

  it "raises if passed more than declared" do
    lambda {
      @o.__send_tier1__(:none, true)
    }.should raise_error(ArgumentError)
  end
end
