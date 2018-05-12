#pragma once

#include <string>

class Lexer {
	public:
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
  token_number, // There is only one type of number (double presision float), so this works
};
		
