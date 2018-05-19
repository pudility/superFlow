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
  Constant *indexT = Constant::getIntegerValue(dType, llvm::APInt(32, 0));
  
  std::vector<Value *> numberValues;
  Instruction *fullVector = InsertElementInst::Create(emptyVector, numbers[0]->codeGen(), indexT);
  mBuilder.Insert(fullVector);

  int i = 0;

  for(auto const& n: numbers) {
    if (i == 0) goto end; // We already did this one when we set fullVector
    indexT = Constant::getIntegerValue(dType, llvm::APInt(32, i));
    fullVector = InsertElementInst::Create(fullVector, n->codeGen(), indexT);
    mBuilder.Insert(fullVector);
end:;
    i++;
  }
  
  return fullVector;
}

Value *ArrayElementAST::codeGen() {
  std::vector<Value *> emptyArgs; // We need to pass it this so we make an empty one
  Value *refVector = mBuilder.CreateCall(mModule->getFunction(name), emptyArgs, "calltmp");
  Constant *indexT = Constant::getIntegerValue(dType, llvm::APInt(32, index));

  Instruction *newVector = ExtractElementInst::Create(refVector, indexT);

  mBuilder.Insert(newVector);
  return newVector;
}

Value *ArrayElementSetAST::codeGen() {
  std::vector<Value *> emptyArgs; // We need to pass it this so we make an empty one
  Value *refVector = mBuilder.CreateCall(mModule->getFunction(name), emptyArgs, "calltmp");
  Constant *indexT = Constant::getIntegerValue(dType, llvm::APInt(32, index));

  Instruction *newVector = InsertElementInst::Create(refVector, newVal->codeGen(), indexT);

  mBuilder.Insert(newVector);
  return Constant::getNullValue(dType);
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
  std::vector<Type*> doubles(arguments.size(), dType);
  
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

Value *ForAST::codeGen() {
  Value *startV = start->codeGen();
  if (!startV) return nullptr;

  Function *func = mBuilder.GetInsertBlock()->getParent();
  BasicBlock *preHeaderBlock = mBuilder.GetInsertBlock();
  BasicBlock *loopBlock = BasicBlock::Create(mContext, "loop", func);

  mBuilder.CreateBr(loopBlock);
  mBuilder.SetInsertPoint(loopBlock);

  PHINode *var = mBuilder.CreatePHI(dType, 2, varName);
  var->addIncoming(startV, preHeaderBlock);

  Value *oldVal = namedValues[varName];
  namedValues[varName] = var;

  if (!body->codeGen())
    return nullptr;

  Value *stepVal = nullptr;
  if (step) {
    stepVal = step->codeGen();
    if (!stepVal) return nullptr;
  } else {
    stepVal = ConstantFP::get(mContext, APFloat(1.0)); // If nothing was specified then just add 1.0
  }

  Value *nextVar = mBuilder.CreateFAdd(var, stepVal, "nextvar");
  Value *endCondition = end->codeGen();
  if (!endCondition) return nullptr;

  endCondition = mBuilder.CreateFCmpONE(endCondition, ConstantFP::get(mContext, APFloat(0.0)), "loopcond");

  BasicBlock *loopEndBlock = mBuilder.GetInsertBlock();
  BasicBlock *afterBlock = BasicBlock::Create(mContext, "afterloop", func);

  mBuilder.CreateCondBr(endCondition, loopBlock, afterBlock);

  mBuilder.SetInsertPoint(afterBlock);
  var->addIncoming(nextVar, loopEndBlock);

  if (oldVal)
    namedValues[varName] = oldVal;
  else
    namedValues.erase(varName);

  return Constant::getNullValue(dType); // For loops always return 0.0
}

Value *PrintAST::codeGen() {
	Constant *calleeFunc = mModule->getOrInsertFunction("printf", 
    FunctionType::get(dType, 
    	true /* this is var arg func type*/
  	)
	);

	std::vector<Value *> argsV;
	for (unsigned i = 0, e = arguments.size(); i != e; ++i)
		argsV.push_back(arguments[i]->codeGen());

	return mBuilder.CreateCall(calleeFunc, argsV, "printfCall");
}
