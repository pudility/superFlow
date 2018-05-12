#include "llvm/ADT/STLExtras.h"
#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <map>
#include <memory>
#include <string>
#include <vector>
#include "lexer.h"

// static std::string identifier;
// static double value;

int Lexer::getToken () {
  static int lastChar = ' ';

  // This is for skipping white space
  while (isspace(lastChar))
    lastChar = getchar();

  if (isalpha(lastChar)) {// this means its a letter or number
    identifier = lastChar;
    while (isalnum((lastChar = getchar())))
      identifier += lastChar;

    if (identifier == "func")
      return token_func;
    if (identifier == "export")
      return token_export;
    return token_id;
  }

  if (isdigit(lastChar) || lastChar == '.') { // Means we should parse it as a number, eg 4+4, the `.` is for decimals
    std::string sNumber;
    do {
      sNumber += lastChar;
      lastChar = getchar();
    } while (isdigit(lastChar) || lastChar == '.');

    value = strtod(sNumber.c_str(), 0 ); // convert our stirng to double
    return token_number;
  }

  if (lastChar == '#') { //This is a comment
    // just go to the end of the line, becuase its a comment
    do {
      lastChar = getchar();
    } while (lastChar != EOF && lastChar != '\n' && lastChar != '\r');

    if (lastChar != EOF)
      return getToken();
  }

  if (lastChar == EOF) //this is the end of the file
    return token_eof;

  // for anuthing else, just give the char ascii value
  int thisChar = lastChar;
  lastChar = getchar();
  return thisChar;
}
