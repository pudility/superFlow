#include "parser/parser.h"
#include "ast/ast.h"
#include "lexer/lexer.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/IR/Function.h"
#include "llvm/Support/raw_ostream.h"
#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <map>
#include <memory>
#include <string>
#include <vector>
#include <iostream>

static void handleFunc(Parser * &p) {
  if (p->ParseDefinition())
    std::cout << "Function" << std::endl;
  else
    std::cerr << "Error - failed to parse definition" << std::endl;
}

static void handleTopLevel(Parser * &p) {
  if (auto fnAST = p->ParseTopLevel()) {
    std::cout << "Top Level" << std::endl;
    if (auto *fnIR = fnAST->codeGen()) //TODO: clean up printing
			fnIR->print(llvm::errs());
  } else
    std::cerr << "Error - failed to parse top level" << std::endl;
}

int main() {
  auto *p = new Parser();
  p->BinaryOpporatorRank['<'] = 10;
  p->BinaryOpporatorRank['+'] = 20;
  p->BinaryOpporatorRank['-'] = 20;
  p->BinaryOpporatorRank['*'] = 40;

  p->getNextToken();

  while (true) {
    switch(p->currentToken) { // TODO: all parsing should be caught and moved on from
      case ';': // ignore top-level semicolons.
        p->getNextToken();
        break;
      case Token::token_eof:
        return -1;
      case Token::token_func:
        handleFunc(p);
        break;
      default:
        handleTopLevel(p);
        break;
    }
  }

  delete p;
  return 0;
}
