#include "llvm/ADT/STLExtras.h"
  #include <algorithm>
  #include <cctype>
  #include <cstdio>
  #include <cstdlib>
  #include <map>
  #include <memory>
  #include <string>
  #include <vector>
#include "ast.h"

CallAST::CallAST(const std::string &_callee, std::vector<std::unique_ptr<AST>> _arguments) {
  callee = _callee;
  // std::memcpy(arguments, _arguments, sizeof(_arguments));
  arguments.assign(_arguments); // std::move(_arguments);
}
