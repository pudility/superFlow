#include "parser/parser.h"
#include "ast/ast.h"
#include "lexer/lexer.h"
// #include "library/library.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/IR/Function.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/IR/Verifier.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Target/TargetMachine.h"
#include "llvm/Transforms/InstCombine/InstCombine.h"
#include "llvm/Transforms/Scalar.h"
#include "llvm/Transforms/Scalar/GVN.h"
#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <map>
#include <memory>
#include <string>
#include <vector>
#include <iostream>
#include <cassert>

using namespace llvm;

static void handleFunc(Parser * &p) {
  if (auto fnAST = p->ParseDefinition()) {
    // std::cout << "Function" << std::endl;
		if (auto *fnIR = fnAST->codeGen())
			fnIR->print(llvm::errs());
  } else
    std::cerr << "Error - failed to parse definition" << std::endl;
}

static void handleTopLevel(Parser * &p) {
  if (auto fnAST = p->ParseTopLevel()) {
    // std::cout << "Top Level" << std::endl;
    if (auto *fnIR = fnAST->codeGen()) { //TODO: clean up printing
      void *funcPtr = engine->getPointerToFunction(fnAST);
      dType (*fce_typed_ptr)() = (dType(*)()) funcPtr;

      /* Excute JITed function */
      std::cout << "Result: " << (*fce_typed_ptr)() << std::endl << std::endl;

			fnIR->print(llvm::errs());
    }
  } else
    std::cerr << "Error - failed to parse top level" << std::endl;
}

static void handleExtern(Parser * &p) {
  if (auto fnAST = p->ParseExtern()) {
    // std::cout << "External" << std::endl;
    if (auto *fnIR = fnAST->codeGen()) {
      fnIR->print(llvm::errs());
      std::cerr << std::endl;
      functionPrototypes[fnAST->getName()] = std::move(fnAST);
    }
  } else
    std::cerr << "Error - failed to parse extern" << std::endl;
}

static void handleVar(Parser * &p) {
  if (auto fnAST = p->ParseVariable(VarType::type_double)) {
    // std::cout << "Variable" << std::endl;
    if (auto *fnIR = fnAST->codeGen())
      fnIR->print(llvm::errs());
  } else
    std::cerr << "Error - failed to parse variable" << std::endl;
}

static void handleArrayVar(Parser * &p) {
  if (auto fnAST = p->ParseVariable(VarType::type_array)) {
    // std::cout << "Array Variable" << std::endl;
    if (auto *fnIR = fnAST->codeGen())
      fnIR->print(llvm::errs());
  } else
    std::cerr << "Error - failed to parse variable" << std::endl;
}

static int mainLoop(Parser * &p) {
  while (true) {
    switch(p->currentToken) { // TODO: all parsing should be caught and moved on from
      case ';': // ignore top-level semicolons.
        p->getNextToken();
        break;
      case Token::token_variable:
        handleVar(p);
        break;
      case Token::token_eof:
        return -1;
      case Token::token_func:
        handleFunc(p);
        break;
      case Token::token_extern:
        handleExtern(p);
        break;
      case Token::token_array:
        handleArrayVar(p);
        break;
      default:
        handleTopLevel(p);
        break;
    }
  }
}

static void initPassManager() { // TODO: this might want to live in its own file/class
  // mFPM = llvm::make_unique<legacy::FunctionPassManager>(M);

  // mFPM->add(createInstructionCombiningPass()); // Simple optimazations (bit-twiddling, "peephole")
  // mFPM->add(createReassociatePass());
  // mFPM->add(createGVNPass()); // Eliminate common sub - expressions
  // mFPM->add(createCFGSimplificationPass()); // delete unreachable blocks, unused vars, ect.
  // mFPM->doInitialization();
}

#ifdef _WIN32
#define DLLEXPORT __declspec(dllexport)
#else
#define DLLEXPORT
#endif

/// printd - printf that takes a double prints it as "%f\n", returning 0.
extern "C" DLLEXPORT double printd(double X) {
  fprintf(stderr, "%f\n", X);
  return 0;
}

int main() {
  initPassManager();

  auto *p = new Parser();
  p->BinaryOpporatorRank['<'] = 10;
  p->BinaryOpporatorRank['+'] = 20;
  p->BinaryOpporatorRank['-'] = 20;
  p->BinaryOpporatorRank['*'] = 40;

  p->getNextToken();

  mainLoop(p);
  
  const char *IRMain = 
    "define i32 @main() { \n"
      "call double @__anon_expr() \n"
      "ret i32 0 \n"
    "} \n";

  std::cout << IRMain << std::endl; // Make sure our program actually runs

  delete p;
  return 0;
}
