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
  token_export = -3, //exportations

  // Primary commands
  token_id = -4,
  token_number = -5, // There is only one type of number (double presision float), so this works
};
		
