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

static std::string handleFunc(Parser * &p) {
  std::string IR;
  raw_string_ostream OS(IR);

  if (auto fnAST = p->ParseDefinition()) {
		if (auto *fnIR = fnAST->codeGen())
			fnIR->print(OS);
  } else
    std::cerr << "Error - failed to parse definition" << std::endl;

  return IR;
}

static std::string loadTopLevel(Parser * &p) {
  std::string IR;
  raw_string_ostream OS(IR);

  if (auto fnAST = p->LoadAnnonFuncs()) {
    if (auto *fnIR = fnAST->codeGen()) //TODO: clean up printing
			fnIR->print(OS);
  } else
    std::cerr << "Error - failed to parse top level" << std::endl;

  return IR;
}

static void handleTopLevel(Parser * &p) {
  p->ParseTopLevel();
}

static std::string handleExtern(Parser * &p) {
  std::string IR;
  raw_string_ostream OS(IR);

  if (auto fnAST = p->ParseExtern()) {
    if (auto *fnIR = fnAST->codeGen())
      fnIR->print(OS);
  } else
    std::cerr << "Error - failed to parse extern" << std::endl;

  return IR;
}

static std::string externMalloc() {
  std::string IR;
  raw_string_ostream OS(IR);

  std::vector<std::pair<std::string, llvm::Type *>> argNames;
  argNames.push_back(std::make_pair("x", dType));

  auto mallocProto = llvm::make_unique<PrototypeAST>("malloc", std::move(argNames), aType);
  if (auto *etrnIR = mallocProto->codeGen())
    etrnIR->print(OS);

  return IR;
}

static std::string mainLoop(Parser * &p) {
  std::string IR;

  while (true) {
    switch(p->currentToken) { // TODO: all parsing should be caught and moved on from
      case ';': // ignore top-level semicolons.
        p->getNextToken();
        break;
      case '}': // TODO: this is a hack that we should not have to do
        p->getNextToken();
        break;
      case Token::token_eof:
        return IR;
      case Token::token_func:
        IR += handleFunc(p);
        break;
      case Token::token_extern:
        IR += handleExtern(p);
        break;
      default:
        handleTopLevel(p);
        break;
    }
  }
}

static std::string run(char* argv[]) {

  auto *p = new Parser(argv[1]);
  p->BinaryOpporatorRank['<'] = 10;
  p->BinaryOpporatorRank['+'] = 20;
  p->BinaryOpporatorRank['-'] = 20;
  p->BinaryOpporatorRank['*'] = 40;
  p->BinaryOpporatorRank['/'] = 50;

  p->getNextToken();

  std::string IR = externMalloc();

  IR += mainLoop(p);

  IR += loadTopLevel(p);

  IR += "define i32 @main() { \n";

  for (int i = 0; i < p->annonCount; i++)
    IR += std::string("  call double @__anon_expr") + std::to_string(i) + " ()\n";
  IR += std::string("  ret i32 0 \n} \n");

  delete p;

  return IR;
}
