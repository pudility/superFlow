#pragma once

#include "llvm/ADT/STLExtras.h"
#include "llvm/IR/Value.h"
#include "../lexer/lexer.h"
#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <map>
#include <memory>
#include <string>
#include <vector>
#include "llvm/ADT/APFloat.h"
#include "llvm/ADT/APInt.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Verifier.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Instructions.h"

using namespace llvm;

static LLVMContext mContext;
static IRBuilder<> mBuilder(mContext);
static std::unique_ptr<Module> mModule = make_unique<Module>("Super", mContext);
static std::map<std::string, AllocaInst *> namedValues;
static std::map<std::string, int> arrayDepths;
static Module *M = mModule.get();
static Type *dType = Type::getDoubleTy(mContext);
static Type *i32 = IntegerType::get(mContext, 32);
static Type *pi8 = PointerType::getUnqual(IntegerType::get(mContext, 8));
static Type *aType = ArrayType::get(dType, 4); // TODO: implemnt x length
static Value *nullValue = Constant::getNullValue(dType);

class AST {
  public:
    virtual ~AST() { }
    virtual llvm::Value *codeGen() = 0;
};

class BaseFuncAST {
  public:
    virtual ~BaseFuncAST() { }
    virtual llvm::Function *codeGen() = 0;
};

class IntAST: public AST {
  int val;

  public:
  IntAST(int val): val(val) { }
  llvm::Value *codeGen() override;
};

class NumberAST: public AST {
  double val;

  public:
  NumberAST(double val): val(val) { }
  llvm::Value *codeGen() override;
};

class ArrayAST: public AST {
  std::vector<std::unique_ptr<AST>> numbers;
  std::string name;

  public:
  ArrayAST(std::vector<std::unique_ptr<AST>> numbers, std::string name): 
    numbers(std::move(numbers)), name(name) { }
  llvm::Value *codeGen() override;
};

class ArrayElementAST: public AST {
  bool returnPtr;
  std::string name;
  std::vector<std::unique_ptr<AST>> indexs;

  public:
  ArrayElementAST(std::string name, std::vector<std::unique_ptr<AST>> indexs, bool returnPtr = false): 
    name(name), returnPtr(returnPtr), indexs(std::move(indexs)) { }
  Value *codeGen() override;
};

class ArrayElementSetAST: public AST {
  std::string name;
  std::vector<std::unique_ptr<AST>> indexs;
  std::unique_ptr<AST> newVal;

  public:
  ArrayElementSetAST(std::string name, std::vector<std::unique_ptr<AST>> indexs, std::unique_ptr<AST> newVal): 
    name(name), indexs(std::move(indexs)), newVal(std::move(newVal)) { }
  Value *codeGen() override;
};

class VariableAST: public AST {
  std::string name;

  public:
  VariableAST(const std::string &name): name(name) { }
  llvm::Value *codeGen() override;
};

class VariableSetAST: public AST {
  std::string name;
  std::unique_ptr<AST> val;

  public:
  VariableSetAST(const std::string &name, std::unique_ptr<AST> val): 
    name(name), val(std::move(val)) { }
  llvm::Value *codeGen() override;
};

class VarAST: public AST {
  std::pair<std::string, std::unique_ptr<AST>> var;
  VarType type;

  public:
  VarAST(std::pair<std::string, std::unique_ptr<AST>> var, VarType type): 
    var(std::move(var)), type(type) { }
  Value *codeGen() override;
};

class BinaryAST: public AST {
  char option;
  std::unique_ptr<AST> LHS, RHS;

  public:
  BinaryAST(char option, std::unique_ptr<AST> LHS, std::unique_ptr<AST> RHS): 
    option(option), LHS(std::move(LHS)), RHS(std::move(RHS)) { } 
  llvm::Value *codeGen() override;
};

class CallAST: public AST {
  std::string callee;
  std::vector<std::unique_ptr<AST>> arguments;
  
  public:
  CallAST(const std::string &callee, std::vector<std::unique_ptr<AST>> arguments): 
    callee(callee), arguments(std::move(arguments)) { }
  llvm::Value *codeGen() override;
};

class PrototypeAST {
  std::string name;
  std::vector<std::pair<std::string, Type*>> arguments;

  public:
  PrototypeAST(const std::string &name, std::vector<std::pair<std::string, Type*>> arguments, Type *retType = dType): 
    name(name), arguments(std::move(arguments)) {
      type = std::move(retType);
    }
  Type *type;
  const std::string &getName() const { return name; }
  llvm::Function *codeGen();
};

class FuncAST: public BaseFuncAST {
  std::unique_ptr<PrototypeAST> prototype;
  std::unique_ptr<AST> body;

  public:
  FuncAST(std::unique_ptr<PrototypeAST> prototype, std::unique_ptr<AST> body): 
    prototype(std::move(prototype)), body(std::move(body)) { }
  llvm::Function *codeGen() override;
};

class LongFuncAST: public BaseFuncAST {
  std::unique_ptr<PrototypeAST> prototype;
  std::vector<std::unique_ptr<AST>> body;

  public:
  LongFuncAST(std::unique_ptr<PrototypeAST> prototype, std::vector<std::unique_ptr<AST>> body): 
    prototype(std::move(prototype)), body(std::move(body)) { }
  llvm::Function *codeGen() override;
};

class AnnonFuncAST { // TODO: remove me
  std::unique_ptr<PrototypeAST> prototype;
  std::vector<std::unique_ptr<AST>> body;

  public:
  AnnonFuncAST(std::unique_ptr<PrototypeAST> prototype, std::vector<std::unique_ptr<AST>> body): 
    prototype(std::move(prototype)), body(std::move(body)) { }
  llvm::Function *codeGen();
};

class ForAST: public AST {
  std::string varName;
  std::unique_ptr<AST> start, end, step, body;
  
  public:
  ForAST(const std::string &varName,
      std::unique_ptr<AST> start,
      std::unique_ptr<AST> end,
      std::unique_ptr<AST> step,
      std::unique_ptr<AST> body): 
    varName(varName), start(std::move(start)), end(std::move(end)), step(std::move(step)), body(std::move(body)) { }
  llvm::Value *codeGen() override;
};

class PrintAST: public AST {
  std::vector<std::unique_ptr<AST>> arguments;

  public:
  PrintAST(std::vector<std::unique_ptr<AST>> arguments): arguments(std::move(arguments)) { }
  llvm::Value *codeGen() override;
};
