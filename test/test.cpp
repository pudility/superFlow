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

std::string run(const char* file) {
	std::string output = 
    exec((std::string("../run.sh $SUPERFLOW_DIR/test/scripts/") + std::string(file) + std::string(" 2")).c_str());
  std::string line;
  std::string lastLine;

  std::stringstream lineStream(output);
  while (lineStream.good()) {
    getline(lineStream, lastLine, '\n');
    if (lastLine != "")
      line = lastLine;
  }

  return line;
}

TEST(Basic, Computation) {
  std::string line = run("basic/comp");

  EXPECT_EQ(line, "2700.000000");
}


TEST(Basic, ArrayCreationElementAccess) {
  std::string line = run("basic/array");

  EXPECT_EQ(line, "1.000000");
}

TEST(Basic, ArrayElementSet) {
  std::string line = run("basic/arraySet");

  EXPECT_EQ(line, "2.000000");
}

TEST(Loops, BasicFor) {
  std::string line = run("loops/for");

  EXPECT_EQ(line, "10.000000");
}

TEST(Loops, ForStep) {
  std::string line = run("loops/forStep");

  EXPECT_EQ(line, "460.000000");
}


TEST(Loops, ForMultiple) {
  std::string line = run("loops/forMulti");

  EXPECT_EQ(line, "1.000000");
}

TEST(Loops, ForArraySetGet) {
  std::string line = run("loops/forArray");

  EXPECT_EQ(line, "9.000000");
}

int main (int argc, char* argv[]) {
  ::testing::InitGoogleTest(&argc, argv);

  RUN_ALL_TESTS();
}
