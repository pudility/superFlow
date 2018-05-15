#include "parser.h"
#include "../ast/ast.h"
#include "../lexer/lexer.h"
#include "llvm/ADT/STLExtras.h"
#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <map>
#include <memory>
#include <string>
#include <vector>
#include <iostream>

int Parser::getNextToken() {
  return currentToken = mLexer->getToken();
}

std::unique_ptr<AST> Parser::LogError (const char *str) {
  std::cerr << "Error: " << str << std::endl;
  return nullptr;
}

std::unique_ptr<PrototypeAST> Parser::LogErrorPlain(const char *str) {
  LogError(str);
  return nullptr;
}

llvm::Value *Parser::LogErrorV(const char *str) { // Right now this does the same thing as log error but it will be difforent later
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

std::unique_ptr<AST> Parser::ParseIdentifier() {
  const std::string idName = mLexer->identifier;
  
  getNextToken(); // Move past the identifier
  
  std::vector<std::unique_ptr<AST>> arguments; // We need even an empty vector either way

  if (currentToken != '(') // We are refrencing the var not function
    return llvm::make_unique<CallAST>(idName, std::move(arguments)); // TODO: this is a hack, but we will just return a function that returns the value instead of an *actual* llvm variable

  // This means that we are calling a function
  getNextToken(); // Move past opening parenthesis
  if (currentToken != ')') { // the function has arguments
    while (true) {
      if (auto arg = ParseExpression())
        arguments.push_back(std::move(arg));
      else
        return nullptr;

      if (currentToken == ')') // we reached the end of args
        break;

      if (currentToken != ',') // we need separated commands or end //TODO: we want to change this maybe
        return LogError("Expected either another argument separated by comma or a closing parenthesis");
      getNextToken();
    }
  }
  
  getNextToken(); //Move past the closing parenthesis
  
  return llvm::make_unique<CallAST>(idName, std::move(arguments));
}

std::unique_ptr<AST> Parser::ParsePrimary () {
  switch(currentToken) {
    case Token::token_id:
      return ParseIdentifier();
    case Token::token_number:
      return ParseNumber();
    case '(':
      return ParseParens();
    default: 
      return LogError("unknown token recived - Parseing Error");
  } 
}

int Parser::getTokenRank() {
  //TODO: FIXME: if (isascii(currentToken)) return -1;
  int tokenRank = BinaryOpporatorRank[currentToken];
  if (tokenRank <= 0) return -1;
  return tokenRank;
}

std::unique_ptr<AST> Parser::ParseExpression() {
  auto LHS = ParsePrimary();
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
  if (currentToken != Token::token_id) return LogErrorPlain("Expected function name (Prototype)");
  
  std::string funcName = mLexer->identifier;
  getNextToken();

  std::vector<std::string> argNames;

  if (currentToken == '(') { // return LogErrorPlain("Expected `(` in prototype");
    while (getNextToken() == Token::token_id) argNames.push_back(mLexer->identifier);
    if (currentToken != ')') return LogErrorPlain("Expected to end with `)` (Prototype)");

    getNextToken(); // Move over the closing `)`
  }

  return llvm::make_unique<PrototypeAST>(funcName, std::move(argNames));
}

std::unique_ptr<FuncAST> Parser::ParseDefinition() {
  getNextToken(); // Move over `func`
  auto proto = ParsePrototype();
  
  if (!proto) return nullptr;
  
  if (auto expr = ParseExpression()) 
    return llvm::make_unique<FuncAST>(std::move(proto), std::move(expr));

  return nullptr;
}

// This is for parsign things that are not in functions eg `> 4+4`
std::unique_ptr<FuncAST> Parser::ParseTopLevel () {
  if (auto expr = ParseExpression()) {
    auto proto = llvm::make_unique<PrototypeAST>("__anon_expr", std::vector<std::string>());
    return llvm::make_unique<FuncAST>(std::move(proto), std::move(expr));
  }
  return nullptr;
}

std::unique_ptr<PrototypeAST> Parser::ParseExtern() {
  getNextToken(); // move past `extern`
  return ParsePrototype();
}
