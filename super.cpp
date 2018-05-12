#include "parser/parser.h"
#include "ast/ast.h"
#include "lexer/lexer.h"
#include "llvm/ADT/STLExtras.h"
#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <map>
#include <memory>
#include <string>
#include <vector>
#include <iostream>

int main() {
  auto *p = new Parser();
  p->BinaryOpporatorRank['<'] = 10;
  p->BinaryOpporatorRank['+'] = 20;
  p->BinaryOpporatorRank['-'] = 20;
  p->BinaryOpporatorRank['*'] = 40;

  std::cout << "# ";
  p->getNextToken();

  while (true) {
    std::cout << "# ";

    switch(p->currentToken) { // TODO: all parsing should be caught and moved on from
      case Token::token_eof:
        return -1;
      case Token::token_func:
        std::cout << "Function" << std::endl;
        p->ParseDefinition();
        break;
      default:
        std::cout << "Top Level" << std::endl;
        p->ParseTopLevel();
        break;
    }
  }
  
  delete p;
  return 0;
}
