#include <sstream>
#include <string>
#include <iostream>
#include <fstream>

int main (int argc, char** argv) {
  std::cout << "reading input: " << argv[1] << " to: " << argv[2] << std::endl;
  std::ifstream input(argv[1]);
  std::ofstream outputFile;

  outputFile.open(argv[2]);

	bool recording = false;
  int lineCount = 0;

  outputFile << "declare double @setLineCount(i32)" << std::endl;
  outputFile << "declare double @stepLineCount()" << std::endl;

  for (std::string line; std::getline(input, line); lineCount++) {
		outputFile << line << std::endl;

    std::size_t found = line.find("__anon_expr0");
  	if (found!=std::string::npos) {
			recording = true;
			outputFile << "\t%calltmp_set_init_line_count = call double @setLineCount(i32 " << lineCount << ")" 
        << std::endl << std::endl;
		}

    found = line.find("}");
  	if (found!=std::string::npos)
			recording = false;

		if (recording)
			outputFile << "\t%calltmp_step_line_count" << lineCount << " = call double @stepLineCount()" 
        << std::endl << std::endl;
  }
}
