require File.expand_path("../fixtures/tier1", __FILE__)

describe "tier1 JIT" do
  before do
    @o = JITSpecs::Tier1Specs.new
  end

  it "can return a string literal" do
    @o.__send_tier1__(:strlit).should == "hello"
  end

  it "can build a string" do
    @o.__send_tier1__(:str_build, 1).should == "-1-"
  end
end
