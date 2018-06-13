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
#include "llvm/IR/DataLayout.h"

using namespace llvm;

static Type *ArrayTypeForType(Type *type) {
  return ArrayType::get(type, tArraySize);
}

static Type *ArrayPointerForType(Type *type) {
  return PointerType::getUnqual(ArrayType::get(type, tArraySize));
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

static std::map<Type *, bool> mallocExterns;

static ArrayRef<Value *> PrefixZero (Value *index) {
  std::vector<Value *> out;
  out.push_back(ConstantInt::get(mContext, APInt(32, 0)));
  out.push_back(index);
  return ArrayRef<Value *>(out);
}

static ArrayRef<Value *> ZeroZero () {
  std::vector<Value *> out;
  out.push_back(ConstantInt::get(mContext, APInt(32, 0)));
  out.push_back(ConstantInt::get(mContext, APInt(32, 0)));
  return ArrayRef<Value *>(out);
}

static Value *StoreAllElements(Value *newValue, Value *oldValue) {
	//TODO: if its an array of doubles none of the below needs to be run and we can just copy it

  Function *func = mBuilder.GetInsertBlock()->getParent();

  AllocaInst *alloca = entryCreateBlockAlloca(func, "store_all_elements_counter");

  mBuilder.CreateStore(ConstantFP::get(mContext, APFloat(0.0)), alloca);

  BasicBlock *loopBlock = BasicBlock::Create(mContext, "loop", func);

  mBuilder.CreateBr(loopBlock);
  mBuilder.SetInsertPoint(loopBlock);

  Value *currentVar = mBuilder.CreateLoad(alloca, "store_all_elements_counter");
  
  Value *workingElement = mBuilder.CreateGEP(newValue, PrefixZero(DoubleToInt(currentVar)));
  Value *oldElem = mBuilder.CreateGEP(oldValue, PrefixZero(DoubleToInt(currentVar)));
  
  Value *workingElementLoad = mBuilder.CreateLoad(workingElement, "load_working_el");
  if (auto *arrayTOfWorkingElLoad =  dyn_cast<ArrayType>(workingElementLoad->getType()))
		if (dyn_cast<ArrayType>(arrayTOfWorkingElLoad->getElementType())) {
			StoreAllElements(workingElement, oldElem);
		}
		else 
			mBuilder.CreateStore(workingElementLoad, oldElem);
	else 
		mBuilder.CreateStore(workingElementLoad, oldElem);

  Value *stepVal = ConstantFP::get(mContext, APFloat(1.0));

  Value *nextVar = mBuilder.CreateFAdd(currentVar, stepVal, "nested_loop_var");
  mBuilder.CreateStore(nextVar, alloca);

  DataLayout *DL = new DataLayout (M);

  Value *comparisonCondition = mBuilder.CreateFCmpULT(
		nextVar, 
		ConstantFP::get(mContext, APFloat(
      // (double)cast<ArrayType>(newValue->getType())->getNumElements()
      (double)tArraySize
    )), 
		"cmptmp"
	);
  Value *endCondition = mBuilder.CreateUIToFP(comparisonCondition, Type::getDoubleTy(mContext), "booltmp");

  endCondition = mBuilder.CreateFCmpONE(endCondition, ConstantFP::get(mContext, APFloat(0.0)), "loopcond");

  BasicBlock *afterBlock = BasicBlock::Create(mContext, "afterloop", func);

  mBuilder.CreateCondBr(endCondition, loopBlock, afterBlock);

  mBuilder.SetInsertPoint(afterBlock);

  return Constant::getNullValue(dType); // For loops always return 0.0
}

Value *NumberAST::codeGen() {
  return ConstantFP::get(mContext, APFloat(val));
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

  // for some dumb reason we need to get every element in the array and store it
  Value *calledValue = val->codeGen(); //TODO: this is temporary and will not work for most things

  return StoreAllElements(calledValue, alloca); // mBuilder.CreateStore(calledValue, alloca);
}

Value *VarAST::codeGen() {
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

  return alloca;
}

Value *ArrayAST::codeGen() {
  Value *emptyVector = UndefValue::get(ArrayTypeForType(numbers[0]->codeGen()->getType()));
  return emptyVector; //TODO: this does not init arrays but it works so we will need to figure out what we want to do here.

  DataLayout *DL = new DataLayout (M);
  // We pass malloc a single arg but it needs to be a vector
  std::vector<Value *> varAsArg = { 
    ConstantInt::get(iType, DL->getTypeSizeInBits(emptyVector->getType()))
  }; 
  VCallAST *calledMalloc = new VCallAST("malloc", varAsArg);
  Value *calledMallocVal = calledMalloc->codeGen();
  
  Instruction* calledMallocValInst = new BitCastInst(calledMallocVal, PointerType::getUnqual(emptyVector->getType()));
  mBuilder.Insert(calledMallocValInst);
  calledMallocVal = calledMallocValInst;


  std::vector<Value *> numberValues;
  Value *fullVector = mBuilder.CreateGEP(calledMallocVal, ZeroZero());
  mBuilder.CreateStore(numbers[0]->codeGen(), fullVector);
  
  int i = 0;
  for(auto const& n: numbers) {
    if (i == 0) goto end; // We already did this one when we set fullVector
    fullVector = mBuilder.CreateGEP(calledMallocVal, PrefixZero(DoubleToInt(n->codeGen())));
    mBuilder.CreateStore(n->codeGen(), fullVector);
end:;
    i++;
  }
  
  return mBuilder.CreateLoad(calledMallocVal, "initialized_array");
}

Value *ArrayElementAST::codeGen() {
  AllocaInst *alloca = namedValues[name];
  
  // We have to use an instruction so we can pass variables as index
  Value *newArray = mBuilder.CreateGEP(alloca, PrefixZero(DoubleToInt(indexs[0]->codeGen())));
  for (unsigned i = 1; i < indexs.size(); i++) {
    newArray = mBuilder.CreateGEP(newArray, PrefixZero(DoubleToInt(indexs[i]->codeGen()))); 
  }

  return mBuilder.CreateLoad(newArray, "__"); // TODO: do we want to use `__` here?
}

Value *ArrayElementSetAST::codeGen() {
  std::vector<Value *> emptyArgs; // We need to pass it this so we make an empty one
  AllocaInst *alloca = namedValues[name];
  
  std::vector<Value *> extracts;
  if (indexs.size() > 0) {
    Value *newArray = mBuilder.CreateGEP(alloca, PrefixZero(DoubleToInt(indexs[0]->codeGen())));
    extracts.push_back(newArray);

    for (unsigned i = 1; i < indexs.size(); i++) {
      newArray = 
        mBuilder.CreateGEP(
          extracts[extracts.size() - 1], PrefixZero(DoubleToInt(indexs[i]->codeGen()))
        ); 
      extracts.push_back(newArray);
    }

    std::reverse(extracts.begin(), extracts.end()); //TODO: there is no reason we need to do this
    std::reverse(indexs.begin(), indexs.end()); //TODO: or this
  }
  
  mBuilder.CreateStore(newVal->codeGen(), extracts[0]);
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

  Value *calledVal = mBuilder.CreateCall(fCallee, argsV, "calltmp");

  Value *tmpLoad = calledVal;
  int depth = 0;
  while (tmpLoad->getType()->isPointerTy()) {
    tmpLoad = mBuilder.CreateLoad(tmpLoad, "tmp_load");
    depth++;
  }

  if (depth > 0) {
    Type *arrayTypeToCastTo = ArrayTypeForType(tmpLoad->getType());
    for (int i = 1; i < depth; i++)
      arrayTypeToCastTo = ArrayTypeForType(arrayTypeToCastTo);

    Instruction* castValue = new BitCastInst(calledVal, PointerType::getUnqual(arrayTypeToCastTo));
    mBuilder.Insert(castValue);

    calledVal = castValue;
  }

  return calledVal;
}

Value *VCallAST::codeGen() {
  Function *fCallee = mModule->getFunction(callee);
  if (!fCallee) return Parser::LogErrorV((std::string("Unknown Function: ") + callee).c_str());

  if (fCallee->arg_size() != arguments.size()) 
    return Parser::LogErrorV((std::string("Incorrect # arguments passed to funtion: ") + callee).c_str());

  // we do not want convert form a pointer here becuase the only way this class will be created is for malloc
  return mBuilder.CreateCall(fCallee, arguments, "calltmp");
}

Function *PrototypeAST::codeGen() {
  std::vector<Type*> doubles;
  for (auto &arg: arguments)
    doubles.push_back(arg.second);

  int depth = 0;
  std::vector<Type *> nestedArrayTypes; //TODO: this does not need to be of type `Type` could just be an int
  while (ArrayType *arrayForType = dyn_cast<ArrayType>(type)) {
    depth++;

    nestedArrayTypes.push_back(PointerType::getUnqual(type));
    type = arrayForType->getElementType();
    if (!dyn_cast<ArrayType>(type)) 
      for (int i = 0; i < depth; i++)
        type = PointerType::getUnqual(type);
  }

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
    DataLayout *DL = new DataLayout (M);
    // We pass malloc a single arg but it needs to be a vector
    std::vector<Value *> varAsArg = { 
      ConstantInt::get(iType, DL->getTypeSizeInBits(returnValue->getType()))
    }; 
    VCallAST *calledMalloc = new VCallAST("malloc", varAsArg);
    Value *mallocOfReturn = calledMalloc->codeGen();

    std::vector<Value *> loadedGEPS;

    loadedGEPS.push_back(mallocOfReturn);
    while (dyn_cast<ArrayType>(mallocOfReturn->getType())) {
      mallocOfReturn = mBuilder.CreateGEP(mallocOfReturn, ZeroZero()); //TODO: this NEEDS to happen for EVERY element
      loadedGEPS.push_back(mallocOfReturn);
    }

    unsigned i = 0;
    for (auto *g: loadedGEPS) {
      if (i == 0) goto end;
      mallocOfReturn = mBuilder.CreateLoad(mallocOfReturn, "retval_pointee");
      mallocOfReturn = mBuilder.CreateStore(mallocOfReturn, g);
  end:;
      i++; 
    }

    mBuilder.CreateRet(mallocOfReturn); //we want a pointer of the return value, not the return value

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

  if (Value *returnValue = body[body.size() - 1]->codeGen()) {
    DataLayout *DL = new DataLayout (M);
    // We pass malloc a single arg but it needs to be a vector
    std::vector<Value *> varAsArg = { 
      ConstantInt::get(iType, DL->getTypeSizeInBits(returnValue->getType()))
    }; 

    int depth = 0;
    Type *type = returnValue->getType();
    std::vector<Type *> nestedArrayTypes; //TODO: this does not need to be of type `Type` could just be an int
    while (ArrayType *arrayForType = dyn_cast<ArrayType>(type)) {
      nestedArrayTypes.push_back(PointerType::getUnqual(type));
      type = arrayForType->getElementType();
      depth++;
    }

    if (VariableAST *retValAsVar = dynamic_cast<VariableAST *>(body[body.size() - 1].get())) // check to see if we can just return the var
      if (AllocaInst *retValAlloca = namedValues[retValAsVar->name]) // if so try to get the var
        returnValue = retValAlloca;
      else {} // otherwise we need to goto below
    else { //TODO: fill me in (store value)
    }

    if (depth > 0) {
      for (int i = 0; i < depth; i++)
        type = PointerType::getUnqual(type);

      Instruction* calledMallocValInst = new BitCastInst(returnValue, type);
      mBuilder.Insert(calledMallocValInst);

      returnValue = calledMallocValInst;
    }

    mBuilder.CreateRet(returnValue); //we want a pointer of the return value, not the return value

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
