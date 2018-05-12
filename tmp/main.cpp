  #include <map>
  #include <memory>
  #include <string>
  #include <vector>
#include <iostream>
#include "test.h"

int main () {
  std::vector<std::unique_ptr<Foo>> i;
  i.push_back(std::move(std::unique_ptr<Foo>()));

  new Test(i);
}
