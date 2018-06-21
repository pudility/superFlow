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

static Type *ArrayTypeForType(Type *type) {
  return ArrayType::get(type, 4);
}

static AllocaInst *entryCreateBlockAlloca(Function *func, std::string name) {
  IRBuilder<> tmpBuilder(&func->getEntryBlock(), func->getEntryBlock().begin());
  return tmpBuilder.CreateAlloca(dType, nullptr, name);
}

//TODO: we should only use this alloca
static AllocaInst *entryCreateBlockAllocaType(Function *func, std::string name, Type* type) {
  IRBuilder<> tmpBuilder(&func->getEntryBlock(), func->getEntryBlock().begin());
  return tmpBuilder.CreateAlloca(type, nullptr, name);
}

// TODO: these methods should live elsewhere
static Value *DoubleToInt (Value *doubleVal) {
  return mBuilder.CreateFPToUI(doubleVal, mBuilder.getInt32Ty());
}

static int SizeForType(Type *type) {
  auto *DL = new DataLayout(M);
  return DL->getPointerTypeSize(type);
}

static ArrayRef<Value *> PrefixZero (Value *index) {
  std::vector<Value *> out;
  out.push_back(ConstantInt::get(mContext, APInt(32, 0)));
  out.push_back(index);
  return ArrayRef<Value *>(out);
}

static ArrayRef<Value *> GEP(int index) {
  auto *vIndex = ConstantInt::get(mContext, APInt(32, index));
  return PrefixZero(vIndex);
}

static ArrayRef<Value *> PGEP(Value *index) {
  std::vector<Value *>indexAsVec = { index };
  return ArrayRef<Value *>(indexAsVec);
}

static ArrayRef<Value *> PGEP(int index) {
  std::vector<Value *>indexAsVec = { ConstantInt::get(mContext, APInt(64, index)) };
  return ArrayRef<Value *>(indexAsVec);
}

Value *NumberAST::codeGen() {
  return ConstantFP::get(mContext, APFloat(val));
}

Value *IntAST::codeGen() {
  return ConstantInt::get(mContext, APInt(32, val));
}

Value *VariableAST::codeGen() {
  Value *v = namedValues[name];

  if (!v) Parser::LogErrorV((std::string("Unknown Variable Name: ") + name).c_str());

  return mBuilder.CreateLoad(v, name.c_str()); // Its okay to rutun v even if it is a nullptr - that will just bubble up the error
}

Value *VariableSetAST::codeGen() {
  AllocaInst *alloca = namedValues[name];
  if (!alloca) 
    return Parser::LogErrorV((std::string("Unknown Variable Name: ") + name).c_str()); 

  return mBuilder.CreateStore(val->codeGen(), alloca);
}

Value *VarAST::codeGen() {
  if (namedValues.find(var.first) != namedValues.end()) 
    return var.second.get()->codeGen();

  Function *func = mBuilder.GetInsertBlock()->getParent();
  
  std::string name = var.first;
  
  AST *init = var.second.get();

  Value *initVal;
  if (init) {
    initVal = init->codeGen();
    if (!initVal) return nullptr;
  } else {
    initVal = (type == VarType::type_double) ? Constant::getNullValue(dType) : Constant::getNullValue(aType);
  }

  AllocaInst *alloca = entryCreateBlockAllocaType(func, name, initVal->getType());
  namedValues[name] = alloca;

  return mBuilder.CreateStore(initVal, alloca);
}

// Arrays are in the following format: { arr_ptr, length, has_children } // has_children: 1 = true, 0 = false
Value *ArrayAST::codeGen() {
  auto arrayLength = numbers.size();
  auto arrayElementType = numbers[0]->codeGen()->getType();
  auto *emptyArray = UndefValue::get(ArrayType::get(arrayElementType, arrayLength));
  auto arraySize = SizeForType(emptyArray->getType());
  std::unique_ptr<AST> arraySizeAST = llvm::make_unique<IntAST>(arraySize * numbers.size());
  std::vector<std::unique_ptr<AST>>arraySizeAsVector;
  arraySizeAsVector.push_back(std::move(arraySizeAST)); // We cant initialize with this variable becuase... ¯\_(ツ)_/¯

  auto *vMalloc = llvm::make_unique<CallAST>("malloc", std::move(arraySizeAsVector))->codeGen();
  auto *castedMalloc = new BitCastInst(vMalloc, PointerType::getUnqual(arrayElementType));
  mBuilder.Insert(castedMalloc);

  // Get and store the depth of the array
  // this is done by looping through the 
  // value of the first element recusively 
  // until it is not castable to an array 
  // type anymore
  int depth = 1;
  auto *nAsArrayAST = dynamic_cast<ArrayAST *>(numbers[0].get());
  while (true) {
    if (nAsArrayAST)
      depth++;
    else 
      break;
    nAsArrayAST = dynamic_cast<ArrayAST *>(nAsArrayAST->numbers[0].get());
  }
  arrayDepths[name] = depth; // It is important we set this before anything ...
  // ... is returned because otherwise the wrong thing could be indexed in the wrong way.

  // store all the elements
  int i  = 0;
  for (auto &n: numbers) {
    auto *element = mBuilder.CreateGEP(castedMalloc, PGEP(i));
    mBuilder.CreateStore(n->codeGen(), element);
    i++;
  }

  //Create struct
  std::vector<Type *> structTypes = { castedMalloc->getType(), i32 };
  auto *structHolderT = StructType::get(mContext, structTypes);
  if (!mBuilder.GetInsertBlock()) return UndefValue::get(structHolderT); // If we are not in a function we only want the type
  auto *func = mBuilder.GetInsertBlock()->getParent();
  auto *allocStruct = entryCreateBlockAllocaType(func, name, structHolderT);

  //Insert elements
  auto *elOne = mBuilder.CreateGEP(allocStruct, GEP(0));
  auto *elTwo = mBuilder.CreateGEP(allocStruct, GEP(1));

  // Store array pointer and length
  mBuilder.CreateStore(castedMalloc, elOne);
  mBuilder.CreateStore(ConstantInt::get(mContext, APInt(32, arrayLength)), elTwo);

  namedValues[name] = allocStruct;
  return mBuilder.CreateLoad(allocStruct, "init_alloca_load");
}

Value *ArrayElementAST::codeGen() {
  auto *structAlloca = namedValues[name];
  auto depth = arrayDepths[name];

  // {{...}}
  auto *alloca = mBuilder.CreateGEP(structAlloca, GEP(0)); // we only want the first element because that is the array pointer
  Value *newArray = mBuilder.CreateLoad(alloca, "load_array_ptr");

  while (depth > 1) {
    // {...}
    newArray = mBuilder.CreateGEP(newArray, GEP(0));
    newArray = mBuilder.CreateLoad(newArray);
    depth--;
  }

  // double*
  newArray = mBuilder.CreateGEP(newArray, PGEP(0)); 

  if (returnPtr) return newArray;
  return mBuilder.CreateLoad(newArray, "final_element"); 
}

Value *ArrayElementSetAST::codeGen() {
  auto *element = llvm::make_unique<ArrayElementAST>(name, std::move(indexs), /*pointer=*/true)->codeGen();
  
  mBuilder.CreateStore(newVal->codeGen(), element);
  return nullValue; 
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
    case '/':
      return mBuilder.CreateFDiv(L, R, "divtmp");
    case '<':
      L = mBuilder.CreateFCmpULT(L, R, "cmptmp");
      return mBuilder.CreateUIToFP(L, Type::getDoubleTy(mContext), "booltmp"); // Convertes the above comparison to a double (0.0 or 1.0)
    default:
      return Parser::LogErrorV("invalid binary oporator \n Oporator must be: (+, -, *, <)");
  }
}

Value *CallAST::codeGen() {
  Function *fCallee = mModule->getFunction(callee);
  if (!fCallee) return Parser::LogErrorV((std::string("Unknown Function: ") + callee).c_str());

  if (fCallee->arg_size() != arguments.size()) 
    return Parser::LogErrorV((std::string("Incorrect # arguments passed to funtion: ") + callee).c_str());

  std::vector<Value *> argsV;
  for (unsigned i = 0, e = arguments.size(); i != e; ++i) {
    argsV.push_back(arguments[i]->codeGen());
    if (!argsV.back()) return nullptr;
  }

  return mBuilder.CreateCall(fCallee, argsV, "calltmp");
}

Function *PrototypeAST::codeGen() {
  std::vector<Type*> doubles;
  for (auto &arg: arguments)
    doubles.push_back(arg.second);
  
  FunctionType *FT = FunctionType::get(type, doubles, false);
  Function *f = Function::Create(FT, Function::ExternalLinkage, name, M);

  unsigned index = 0;
  for (auto &arg: f->args())
    arg.setName(arguments[index++].first);

  return f;
}

Function *FuncAST::codeGen() {
  Function *func = mModule->getFunction(prototype->getName()); // this checks if it already exists as part of llvm

  if (!func) func = prototype->codeGen();
  
  if (!func) return nullptr;

  if (!func->empty()) return (Function*) Parser::LogErrorV("Function cannot be redefined.");

  BasicBlock *block = BasicBlock::Create(mContext, "entry", func);
  mBuilder.SetInsertPoint(block);

  int i = 0;
  for (auto &arg: func->args()) {
    AllocaInst *alloca = entryCreateBlockAllocaType(func, arg.getName(), arg.getType());

    mBuilder.CreateStore(&arg, alloca);

    namedValues[arg.getName()] = alloca;

    i++;
  }

  if (Value *returnValue = body->codeGen()) {
    mBuilder.CreateRet(returnValue);

    llvm::verifyFunction(*func);

    return func;
  }

  func->removeFromParent(); // If there is an error get rid of the function
  return nullptr;
}

// Long Functions have multiple expressions in them
Function *LongFuncAST::codeGen() {
  Function *func = prototype->codeGen();
  
  if (!func) return nullptr;

  if (!func->empty()) return (Function*) Parser::LogErrorV("Function cannot be redefined.");

  BasicBlock *block = BasicBlock::Create(mContext, "entry", func);
  mBuilder.SetInsertPoint(block);

  for (auto &arg: func->args()) {
    AllocaInst *alloca = entryCreateBlockAllocaType(func, arg.getName(), arg.getType());

    mBuilder.CreateStore(&arg, alloca);

    namedValues[arg.getName()] = alloca;
  }

  Constant *exprConst;
  for (auto &expr: body) {
    Value *exprValue = expr->codeGen();
    exprConst = dyn_cast<Constant>(exprValue);
    mBuilder.Insert(exprConst);
  }

  if (auto retValue = body[body.size() - 1]->codeGen()) { // TODO: make a return for null value if this is null
    mBuilder.CreateRet(retValue); // TODO: This should not matter, but we might want to change this to exprValue

    llvm::verifyFunction(*func);

    return func;
  } else {
    mBuilder.CreateRet(Constant::getNullValue(dType));

    llvm::verifyFunction(*func);

    return func;
  }

  std::cerr << "Could not create function - null value" << std::endl;
  func->removeFromParent(); // If there is an error get rid of the function
  return nullptr;
}

Value *ForAST::codeGen() {
  Function *func = mBuilder.GetInsertBlock()->getParent();

  AllocaInst *alloca = entryCreateBlockAlloca(func, varName);

  Value *startV = start->codeGen();
  if (!startV) return nullptr;

  mBuilder.CreateStore(startV, alloca);

  BasicBlock *loopBlock = BasicBlock::Create(mContext, "loop", func);

  mBuilder.CreateBr(loopBlock);
  mBuilder.SetInsertPoint(loopBlock);

  AllocaInst *oldVal = namedValues[varName];
  namedValues[varName] = alloca;

  if (!body->codeGen())
    return nullptr;

  Value *stepVal = nullptr;
  if (step) {
    stepVal = step->codeGen();
    if (!stepVal) return nullptr;
  } else {
    stepVal = ConstantFP::get(mContext, APFloat(1.0)); // If nothing was specified then just add 1.0
  }

  Value *currentVar = mBuilder.CreateLoad(alloca, varName.c_str());
  Value *nextVar = mBuilder.CreateFAdd(currentVar, stepVal, "nextvar");
  mBuilder.CreateStore(nextVar, alloca);

  Value *endCondition = end->codeGen();
  if (!endCondition) return nullptr;

  endCondition = mBuilder.CreateFCmpONE(endCondition, ConstantFP::get(mContext, APFloat(0.0)), "loopcond");

  BasicBlock *afterBlock = BasicBlock::Create(mContext, "afterloop", func);

  mBuilder.CreateCondBr(endCondition, loopBlock, afterBlock);

  mBuilder.SetInsertPoint(afterBlock);

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
