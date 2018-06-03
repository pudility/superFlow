#include "gtest/gtest.h"
#include "../pstream/pstream.h"

#include <cstdio>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <array>

std::string exec(const char* cmd) {
  std::string command(cmd);
  command.append(" 2>&1");

  std::array<char, 128> buffer;
  std::string result;
  std::shared_ptr<FILE> pipe(popen(command.c_str(), "r"), pclose);
  if (!pipe) throw std::runtime_error("popen() failed!");
  while (!feof(pipe.get())) {
    if (fgets(buffer.data(), 128, pipe.get()) != nullptr)
      result += buffer.data();
  }
  return result;
}

TEST(Basic, Computation) {
	std::string output = exec("../run.sh $SUPERFLOW_DIR/test/scripts/basic/comp 2");
  std::string line;
  std::string lastLine;

  std::stringstream lineStream(output);
  while (lineStream.good()) {
    getline(lineStream, lastLine, '\n');
    if (lastLine != "")
      line = lastLine;
  }

  EXPECT_EQ(line, "2700.000000");
}

int main (int argc, char* argv[]) {
  ::testing::InitGoogleTest(&argc, argv);

  RUN_ALL_TESTS();
}
