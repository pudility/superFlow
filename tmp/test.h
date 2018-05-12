  #include <map>
  #include <memory>
  #include <string>
  #include <vector>

class Foo { };

class Test {
  std::vector<std::unique_ptr<Foo>> bar;
  public:
    Test(std::vector<std::unique_ptr<Foo>> bar): bar(std::move(bar)) { }
};

