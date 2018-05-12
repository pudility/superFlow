#pragma once

#include "llvm/ADT/STLExtras.h"
#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <map>
#include <memory>
#include <string>
#include <vector>

class AST {
  public:
    virtual ~AST() { }
};

class NumberAST: public AST {
  double val;

  public:
  NumberAST(double val): val(val) { }
};

class VariableAST: public AST {
  std::string name;
  
  public:
  VariableAST(const std::string &name): name(name) { }
};

class BinaryAST: public AST {
  char option;
  std::unique_ptr<AST> LHS, RHS;

  public:
  BinaryAST(char option, std::unique_ptr<AST> LHS, std::unique_ptr<AST> RHS): option(option), LHS(std::move(LHS)), RHS(std::move(RHS)) { } 
};

class CallAST: public AST {
  std::string callee;
  std::vector<std::unique_ptr<AST>> arguments;
  
  public:
  CallAST(const std::string &callee, std::vector<std::unique_ptr<AST>> arguments): 
    callee(callee), arguments(std::move(arguments)) { }
};

class PrototypeAST {
  std::string name;
  std::vector<std::string> arguments;

  public:
  PrototypeAST(const std::string &name, std::vector<std::string> arguments): name(name), arguments(std::move(arguments)) { }

  const std::string &getName() const { return name; }
};

class FuncAST {
  std::unique_ptr<PrototypeAST> prototype;
  std::unique_ptr<AST> body;

  public:
  FuncAST(std::unique_ptr<PrototypeAST> prototype, std::unique_ptr<AST> body): prototype(std::move(prototype)), body(std::move(body)) { }
};


