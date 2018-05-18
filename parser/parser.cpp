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

std::unique_ptr<PrototypeAST> Parser::LogErrorPlain(const char *str) { // TODO: Prototype not Plain
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

std::unique_ptr<AST> Parser::ParseArray() {
  getNextToken(); // Move past `[`
  std::vector<std::unique_ptr<AST>> numbers;

  while (currentToken != ']')
    numbers.push_back(ParseNumber());
  
  getNextToken(); // Move over `]`

  return llvm::make_unique<ArrayAST>(std::move(numbers));
}

std::unique_ptr<AST> Parser::ParseIdentifier() {
  const std::string idName = mLexer->identifier;
  
  getNextToken(); // Move past the identifier
  
  std::vector<std::unique_ptr<AST>> arguments; // We need even an empty vector either way

  if (currentToken != '(') { // We are refrencing the var not function
    if (std::find(namedFunctions.begin(), namedFunctions.end(), idName) != namedFunctions.end()) 
			return llvm::make_unique<CallAST>(idName, std::move(arguments)); // TODO: 
      /* this is a hack, but we will just return a function that returns the value instead of an *actual* llvm variable */
      /* if we know that the name id is a function, return a function call with no args (hacky variable) */

    else return llvm::make_unique<VariableAST>(idName); // otherwise return a real variable
  }
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

      // TODO: make sure this works
      // if (currentToken != ',') // we need separated commands or end //TODO: we want to change this maybe
        // return LogError("Expected either another argument separated by comma or a closing parenthesis");
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
    case '[':
      return ParseArray();
    case '(':
      return ParseParens();
    case Token::token_for:
      return ParseFor();
    case Token::token_print:
      return ParsePrint();
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
	namedFunctions.push_back(funcName); // make sure we know about it when we are deciding whats a function and whats a variable
  getNextToken();

  std::vector<std::string> argNames;

  if (currentToken == '(') { // return LogErrorPlain("Expected `(` in prototype");
    while (getNextToken() == Token::token_id) argNames.push_back(mLexer->identifier);
    if (currentToken != ')') return LogErrorPlain("Expected to end with `)` (Prototype)");

    getNextToken(); // Move over the closing `)`
  }

  return llvm::make_unique<PrototypeAST>(funcName, std::move(argNames), VarType::type_double); //TODO: functions that can return any type


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
    auto proto = llvm::make_unique<PrototypeAST>("__anon_expr" /* TODO: this should add a coutner so that there can be mutiple */, std::vector<std::string>(), VarType::type_double);
    return llvm::make_unique<FuncAST>(std::move(proto), std::move(expr));
  }
  return nullptr;
}

std::unique_ptr<PrototypeAST> Parser::ParseExtern() {
  getNextToken(); // move past `extern`
  return ParsePrototype();
}

std::unique_ptr<FuncAST> Parser::ParseVariable(VarType type) {
  getNextToken(); // Move over `var`

  const std::string idName = mLexer->identifier;
	namedFunctions.push_back(idName); // make sure we know about it when we are deciding whats a function and whats a variable
  
  getNextToken();
  
  std::vector<std::string> arguments;
  auto proto = llvm::make_unique<PrototypeAST>(idName, std::move(arguments), type);
  
  if (auto expr = ParseExpression()) 
    return llvm::make_unique<FuncAST>(std::move(proto), std::move(expr));

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

//   if (currentToken != Token::token_in)
//     return LogError("expected 'in' after for");
//   getNextToken(); // Move past `in`

  auto body = ParseExpression();
  if (!body) return nullptr;

  return llvm::make_unique<ForAST> (idName, std::move(start), std::move(end), std::move(start), std::move(body));
}

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
