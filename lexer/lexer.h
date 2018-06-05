#pragma once

#include <string>
#include <fstream>
#include <iostream>
#include <map>

class PreLex {
  public:
  PreLex(std::map<std::string, int> arrays): arrays(arrays) { }
  std::map<std::string, int> arrays; // this is an array of every array we make and the number of times we use it (so we know when to get rid of it)
};

class Lexer {
  private:
  std::fstream ifs;
  std::string fileName;

	public:
  Lexer(std::string fileName): ifs(fileName, std::ifstream::in), fileName(fileName) { }
	int getToken ();
  PreLex *runPreLexer ();
  
  std::string identifier;
  double value;
};

enum Token {
  token_eof = -1,

  //Commands
  token_func = -2, //Functions
  token_extern = -3, //exporternal functions

  // Primary commands
  token_id = -4,
  token_number = -5, // There is only one type of number (double presision float), so this works
  token_variable = -6,
  token_array = -7,
  token_for = -8,
  token_print = -9,
};

enum class VarType {
  type_double,
  type_array
};
