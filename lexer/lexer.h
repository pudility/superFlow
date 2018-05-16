#pragma once

#include <string>
#include <fstream>
#include <iostream>

class Lexer {
  private:
  std::fstream ifs;

	public:
  Lexer(): ifs("tmpfile.spr", std::ifstream::in) { }
	int getToken ();
  
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
  token_in = -9,
};

enum class VarType {
  type_double,
  type_array
};

