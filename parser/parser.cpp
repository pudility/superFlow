#include "parser.h"
#include "../ast/ast.h"
#include "../lexer/lexer.h"
#include "llvm/ADT/STLExtras.h"
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
#include <utility>
#include "llvm/IR/NoFolder.h"

int Parser::getNextToken() {
  return currentToken = mLexer->getToken();
}

std::unique_ptr<AST> Parser::LogError (const char str[]) {
  std::cerr << "Error: " << str << std::endl;
  return nullptr;
}

std::unique_ptr<PrototypeAST> Parser::LogErrorPlain(const char str[]) { // TODO: Prototype not Plain
  LogError(str);
  return nullptr;
}

llvm::Value *Parser::LogErrorV(const char str[]) { // Right now this does the same thing as log error but it will be difforent later
  LogError(str);
  return nullptr;
}

std::unique_ptr<AST> Parser::ParseNumber() {
  auto result = llvm::make_unique<NumberAST>(mLexer->value);
  getNextToken(); // This moves to the next token
  return std::move(result);
}

std::unique_ptr<AST> Parser::ParseParens() {
  getNextToken(); // Move past open paren `(`
  auto expression = ParseExpression();
  
  if (!expression)
    return nullptr;

  if (currentToken != ')')
    return LogError("Expected closing paren `)`");

  getNextToken(); // Move past the end parent
  return expression;
}

std::unique_ptr<AST> Parser::ParseArray(std::string name) {
  getNextToken(); // Move past `[`
  std::vector<std::unique_ptr<AST>> numbers;

  while (currentToken != ']') 
    numbers.push_back(ParseExpression());
  
  getNextToken(); // Move over `]`

  return llvm::make_unique<ArrayAST>(std::move(numbers), name);
}

std::unique_ptr<AST> Parser::ParseIdentifier() {
  const std::string idName = mLexer->identifier;
  
  getNextToken(); // Move past the identifier

  if (currentToken != '(') { // We are refrencing the var not function
    if (currentToken == '[') { // We are getting an element of an array //TODO: move me to ast
      std::vector<std::unique_ptr<AST>> valIndexs;
      while (currentToken == '[') {
        getNextToken(); // Move past `[`
        valIndexs.push_back(ParseExpression());
        // getNextToken(); //  move past index      
        getNextToken(); // move past `]`
      }

      if (currentToken == '=') {
        getNextToken(); // Move past `=`
				return llvm::make_unique<ArrayElementSetAST>(idName, std::move(valIndexs), ParseExpression()); //TODO: parseexpression might be better for this and setvarast
      }
      
      return llvm::make_unique<ArrayElementAST>(idName, std::move(valIndexs));
    }
    
    if (currentToken == '=') {
      getNextToken(); // Move past `=`
      return llvm::make_unique<VariableSetAST>(idName, ParseExpression()); // return a set variable call
    }
    return llvm::make_unique<VariableAST>(idName); // otherwise return a variable
  }

  std::vector<std::unique_ptr<AST>> arguments; 

  // This means that we are calling a function
  getNextToken(); // Move past opening parenthesis
  if (currentToken != ')') { // the function has arguments
    while (true) {
      if (auto arg = ParsePrimary())
        arguments.push_back(std::move(arg));
      else
        return nullptr;

      if (currentToken == ')') // we reached the end of args
        break;

      // TODO: make sure this works
      // if (currentToken != ',') // we need separated commands or end //TODO: we want to change this maybe
        // return LogError("Expected either another argument separated by comma or a closing parenthesis");
      getNextToken();
    }
  }
  
  getNextToken(); //Move past the closing parenthesis
  
  return llvm::make_unique<CallAST>(idName, std::move(arguments));
}

std::unique_ptr<AST> Parser::ParsePrimary (std::string name) {
  switch(currentToken) {
    case Token::token_id:
      return ParseIdentifier();
    case Token::token_number:
      return ParseNumber();
    case '[':
      return ParseArray(name);
    case '(':
      return ParseParens();
    case Token::token_for:
      return ParseFor();
    case Token::token_variable:
      return ParseVariable(VarType::type_double);
    case Token::token_array:
      return ParseVariable(VarType::type_array);
    case Token::token_eof:
      return nullptr; //TODO: handle this better
    default: 
      std::cerr << "Error with Token: " << currentToken << " (" << (char) currentToken << ") " << std::endl;
      return LogError("unknown token recived - Parseing Error");
  } 
}

int Parser::getTokenRank() {
  //TODO: FIXME: if (isascii(currentToken)) return -1;
  int tokenRank = BinaryOpporatorRank[currentToken];
  if (tokenRank <= 0) return -1;
  return tokenRank;
}

std::unique_ptr<AST> Parser::ParseExpression(std::string name) {
  if (currentToken == '}' || currentToken == ';') return nullptr; // If we are closing a function we dont want to parse this

  auto LHS = ParsePrimary(name);
  if (!LHS) return nullptr;
     
  return ParseBinaryOporatorRHS(0, std::move(LHS));
}

std::unique_ptr<AST> Parser::ParseBinaryOporatorRHS(int exprRank, std::unique_ptr<AST> LHS) {
  while (true) {
    int tokenRank = getTokenRank();

    if (tokenRank < exprRank) return LHS;

    int binaryOpporator = currentToken;
    getNextToken(); // Now that we know what it is, move past it

    auto RHS = ParsePrimary();
    if (!RHS) return nullptr;

    int nextRank = getTokenRank();
    if (tokenRank < nextRank) {
      RHS = ParseBinaryOporatorRHS(tokenRank + 1, std::move(RHS));
      if (!RHS) return nullptr;
    }

    LHS = llvm::make_unique<BinaryAST> (binaryOpporator, std::move(LHS), std::move(RHS));
  }
}

std::unique_ptr<PrototypeAST> Parser::ParsePrototype() {
  llvm::Type *retType = ParsePrimary()->codeGen()->getType();
  if (currentToken != Token::token_id) return LogErrorPlain("Expected function name (Prototype)");
  
  std::string funcName = mLexer->identifier;
  getNextToken();

  std::vector<std::pair<std::string, llvm::Type *>> argNames;

  if (currentToken == '(') { // return LogErrorPlain("Expected `(` in prototype");
    getNextToken();
    while (currentToken != ')' /*== Token::token_id*/) {
      std::string argName = mLexer->identifier;
      getNextToken(); // Move past id

      auto argAST = ParsePrimary();
      if (auto *argAsArrayAST = dynamic_cast<ArrayAST *>(argAST.get())) {
        argAsArrayAST->name = argName;
        argNames.push_back(std::make_pair(argName, argAsArrayAST->codeGen()->getType()));
      } else
        argNames.push_back(std::make_pair(argName, argAST->codeGen()->getType()));
    }

    if (currentToken != ')') return LogErrorPlain("Expected to end with `)` (Prototype)");

    getNextToken(); // Move over the closing `)`
  }

  return llvm::make_unique<PrototypeAST>(funcName, std::move(argNames), retType); //TODO: functions that can return any type
}

std::unique_ptr<BaseFuncAST> Parser::ParseDefinition() {
  getNextToken(); // Move over `func`
  auto proto = ParsePrototype();
  
  if (!proto) return nullptr;
 
  if (currentToken == '{') { // Multiline func
    std::vector<std::unique_ptr<AST>> expresssions;

    getNextToken(); // Move past `{`

    while (currentToken != '}') {
      if (auto expr = ParseExpression())
        expresssions.push_back(std::move(expr));
      getNextToken();
    }
    
    return llvm::make_unique<LongFuncAST>(std::move(proto), std::move(expresssions));
  }
  
  if (auto expr = ParseExpression()) 
    return llvm::make_unique<FuncAST>(std::move(proto), std::move(expr));

  return nullptr;
}

// This is for parsign things that are not in functions eg `> 4+4`
void Parser::ParseTopLevel () {
  if (auto expr = ParseExpression()) 
    annonExprs.push_back(std::move(expr));
}

std::unique_ptr<LongFuncAST> Parser::LoadAnnonFuncs () {
  auto proto = llvm::make_unique<PrototypeAST>(
    "__anon_expr" + std::to_string(annonCount), 
    std::vector<std::pair<std::string, Type*>>()
  );
  annonCount++;

  return llvm::make_unique<LongFuncAST>(std::move(proto), std::move(annonExprs)); // we use long func so we can return null
}

std::unique_ptr<PrototypeAST> Parser::ParseExtern() {
  getNextToken(); // move past `extern`
  return ParsePrototype();
}

std::unique_ptr<AST> Parser::ParseVariable(VarType type) { //TODO: type does not need to exist
  getNextToken(); // Move over `var`

  const std::string idName = mLexer->identifier;
  
  getNextToken();
  
  if (auto expr = ParseExpression(idName)) 
    return llvm::make_unique<VarAST>(
      std::pair<std::string, std::unique_ptr<AST>>(idName, std::move(expr)),
      type
    );

  return nullptr;
}

//'for' identifier '=' expr ',' expr (',' expr)? 'in' expression
std::unique_ptr<AST> Parser::ParseFor() {
  getNextToken(); // Move past `for`
  
  if (currentToken != Token::token_id) 
    return LogError("expected identifier after for");

  std::string idName = mLexer->identifier;
  getNextToken(); // Move past identifier

  if (currentToken != '=')
    return LogError("expected '=' after for");
  getNextToken(); // move past `=`

  auto start = ParseExpression();
  if (!start) return nullptr;
  if (currentToken != ',') 
    return LogError("expected ',' after for start value");
  getNextToken(); // Move past `,`

  auto end = ParseExpression();
  if (!end) return nullptr;

  std::unique_ptr<AST> step;
  if (currentToken == ',') { // the step is optional
    getNextToken(); // Move over `,`
    step = ParseExpression();
    if (!step) return nullptr;
  }

  auto body = ParseExpression();
  if (!body) return nullptr;

  return llvm::make_unique<ForAST> (idName, std::move(start), std::move(end), std::move(step), std::move(body));
}

//TODO: im unsused
std::unique_ptr<AST> Parser::ParsePrint() {
  getNextToken(); // Move past `print`

  if (currentToken != '(') return LogError("Expected `(` to call print");

  std::vector<std::unique_ptr<AST>> arguments; 

  getNextToken(); // Move past opening parenthesis
  if (currentToken != ')') {
    while (true) {
      if (auto arg = ParseExpression())
        arguments.push_back(std::move(arg));
      else
        return nullptr;

      if (currentToken == ')') // we reached the end of args
        break;

      getNextToken();
    }
  } else return LogError("Print must have exactly one argument");
  
  getNextToken(); //Move past the closing parenthesis
  
  return llvm::make_unique<PrintAST>(std::move(arguments));
}
