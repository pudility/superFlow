#include <iostream>
#include <cstdint>
#include <string>

#include "llvm/ExecutionEngine/JIT.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Module.h"
#include "llvm/PassManager.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Analysis/Verifier.h"

#include "../ast/ast.h"

using namespace llvm;

void initJIT () {
  // Exicute engine
  std::string sErr;
  engine = EngineBuilder(mModule).setErrorStr(&sErr).create();
  if (!engine) std::cerr << "Coud not create engine \n" sErr << std::endl;  
}
