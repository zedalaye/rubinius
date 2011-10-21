require File.expand_path("../fixtures/tier1", __FILE__)

describe "tier1 JIT" do
  before do
    @o = JITSpecs::Tier1Specs.new
  end

  it "checks the arity of called methods" do
    @o.__send_tier1__(:none).should be_nil
  end

  it "raises if passed more than declared" do
    lambda {
      @o.__send_tier1__(:none, true)
    }.should raise_error(ArgumentError)
  end

  it "can accept and argument and return it as a local" do
    @o.__send_tier1__(:one, "foo").should == "foo"
  end
end
