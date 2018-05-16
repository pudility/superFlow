#include "llvm/ADT/STLExtras.h"
#include "llvm/IR/Value.h"
#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <map>
#include <memory>
#include <string>
#include <vector>
#include "ast.h"
#include "../lexer/lexer.h"
#include "../parser/parser.h"
#include "llvm/ADT/APFloat.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Verifier.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Instructions.h"

using namespace llvm;

Value *NumberAST::codeGen() {
  return ConstantFP::get(mContext, APFloat(val));
}

Value *VariableAST::codeGen() {
  Value *v = namedValues[name];
  if (!v) Parser::LogErrorV("Unknown Variable Name");

  return v; // Its okay to rutun v even if it is a nullptr - that will just bubble up the error
}

Value *ArrayAST::codeGen() {
  Type *dType = Type::getDoubleTy(mContext);
  Type *vectorType = VectorType::get(dType, 4);
  Value *emptyVector = UndefValue::get(vectorType);
  Constant *index0 = Constant::getIntegerValue(dType, llvm::APInt(32, 0));
  Value *numberValue = numbers[0] -> codeGen();
  numberValue->print(errs());
  Value *fullVector = InsertElementInst::Create(emptyVector, numberValue, index0);
  // namedValues[name] = fullVector;
  return fullVector;
}

Value *BinaryAST::codeGen() {
  Value *L = LHS->codeGen();
  Value *R = RHS->codeGen();

  if (!L || !R) return nullptr;

  switch(option) {// TODO: Oporator, not option :P
    case '+': // TODO: these should be enums
      return mBuilder.CreateFAdd(L, R, "addtmp"); // TODO: Thesse hsould also be enums
    case '-': 
      return mBuilder.CreateFSub(L, R, "subtmp");
    case '*':
      return mBuilder.CreateFMul(L, R, "multmp");
    case '<':
      L = mBuilder.CreateFCmpULT(L, R, "cmptmp");
      return mBuilder.CreateUIToFP(L, Type::getDoubleTy(mContext), "booltmp"); // Convertes the above comparison to a double (0.0 or 1.0)
    default:
      return Parser::LogErrorV("invalid binary oporator \n Oporator must be: (+, -, *, <)");
  }
}

Value *CallAST::codeGen() {
  Function *fCallee = mModule->getFunction(callee);
  if (!fCallee) return Parser::LogErrorV("Unknown function referenced");

  if (fCallee->arg_size() != arguments.size()) return Parser::LogErrorV("Incorrect # arguments passed");

  std::vector<Value *> argsV;
  for (unsigned i = 0, e = arguments.size(); i != e; ++i) {
    argsV.push_back(arguments[i]->codeGen());
    if (!argsV.back()) return nullptr;
  }

  return mBuilder.CreateCall(fCallee, argsV, "calltmp");
}

Function *PrototypeAST::codeGen() {
  std::vector<Type*> doubles(arguments.size(), Type::getDoubleTy(mContext));
  Type *dType = Type::getDoubleTy(mContext);
  FunctionType *FT = FunctionType::get(type == VarType::type_double ? 
      dType : VectorType::get(dType, 4)
      , doubles, false);
  Function *f = Function::Create(FT, Function::ExternalLinkage, name, M);

  unsigned index = 0;
  for (auto &arg: f->args())
    arg.setName(arguments[index++]);

  return f;
}

Function *FuncAST::codeGen() {
  Function *func = mModule->getFunction(prototype->getName()); // this checks if it already exists as part of llvm
  
  if (!func) func = prototype->codeGen();
  
  if (!func) return nullptr;

  if (!func->empty()) return (Function*) Parser::LogErrorV("Function cannot be redefined.");

  BasicBlock *block = BasicBlock::Create(mContext, "entry", func);
  mBuilder.SetInsertPoint(block);

  namedValues.clear();
  for (auto &arg: func->args()) 
    namedValues[arg.getName()] = &arg;

  if (Value *returnValue = body->codeGen()) {
    mBuilder.CreateRet(returnValue);

    llvm::verifyFunction(*func);

    return func;
  }

  func->removeFromParent(); // If there is an error get rid of the function
  return nullptr;
}
