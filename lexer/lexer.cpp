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
#include <fstream>
#include "lexer.h"

bool isoporator (char opp) {
  return opp == '+' || opp == '-' || opp == '*' || opp == '<' || opp == '/';
}

PreLex *Lexer::runPreLexer () {
  Lexer *mLexer = new Lexer(fileName);

  std::map<std::string, int> arrays;
  int currentToken = 0;
  while (currentToken != token_eof) {
    currentToken = mLexer->getToken(); 

//    if (currentToken == token_array) {
//      getToken(); // move past array
//      arrays[mLexer->identifier] = 0;
//    }
//
//    if (currentToken == token_id)
//      if (arrays.find(mLexer->identifier) != arrays.end())
//        arrays[mLexer->identifier]++;
  }

  return new PreLex(arrays);
}

int Lexer::getToken () {
  static int lastChar = ' ';

  // This is for skipping white space
  while ((isspace(lastChar) || lastChar == '_') && ifs.good())
    lastChar = ifs.get();

  if (isalpha(lastChar)) { // this means its a letter or number
    identifier = lastChar;
    while (ifs.good() && isalnum((lastChar = ifs.get())))
      identifier += lastChar;
    
    if (identifier == "func")
      return token_func;
    if (identifier == "extern")
      return token_extern;
    if (identifier == "var")
      return token_variable;
    if (identifier == "array")
      return token_array;
    if (identifier == "for")
      return token_for;
    if (identifier == "print")
      return token_print;
    return token_id;
  }

  if (isdigit(lastChar) || lastChar == '.') { // Means we should parse it as a number, eg 4+4, the `.` is for decimals
    std::string sNumber;
    do {
      if (!ifs.good()) return token_eof;
      if (isspace(lastChar)) break;

      sNumber += lastChar;
      lastChar = ifs.get();
    } while ((isdigit(lastChar) || lastChar == '.') && !isspace(lastChar));

    value = strtod(sNumber.c_str(), 0 ); // convert our stirng to double
    return token_number;
  }

  if (lastChar == '#') { //This is a comment
    // just go to the end of the line, becuase its a comment
    do {
      if (!ifs.good()) return token_eof;

      lastChar = ifs.get();
    } while (lastChar != EOF && lastChar != '\n' && lastChar != '\r');

    if (lastChar != EOF)
      return getToken();
  }

  if (lastChar == EOF) //this is the end of the file
    return token_eof;

  // for anuthing else, just give the char ascii value
  int thisChar = lastChar;
  lastChar = ifs.get();
  return thisChar;
}
